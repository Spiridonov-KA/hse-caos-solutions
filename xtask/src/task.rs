use std::{
    collections::{BTreeSet, HashMap},
    fs, io,
    path::{Path, PathBuf},
    process,
    rc::Rc,
};

use anyhow::{Context, Result, anyhow};
use gix::Repository;
use globset::{Glob, GlobSetBuilder};
use serde::{Deserialize, Serialize};
use strum_macros::EnumIter;
use tracing::{debug, warn};
use walkdir::WalkDir;

use crate::{
    command::{CommandBuilder, CommandLimits, CommandRunner},
    util::{CommandRunnerExt, PathExt},
};

const TESTING_CONFIG_FILENAME: &str = "testing.yaml";

#[derive(Debug, Serialize, Deserialize, Clone, Copy, EnumIter, PartialEq, Eq)]
#[serde(rename_all = "lowercase", deny_unknown_fields)]
pub enum BuildProfile {
    Release,
    Debug,
    ASan,
    TSan,
}

impl BuildProfile {
    pub fn representations(self) -> (&'static str, &'static str, &'static str) {
        use BuildProfile::*;
        match self {
            Release => ("Release", "build_release", "rel"),
            Debug => ("Debug", "build_debug", "dbg"),
            ASan => ("Asan", "build_asan", "asan"),
            TSan => ("Tsan", "build_tsan", "tsan"),
        }
    }

    pub fn cmake_build_type(self) -> &'static str {
        self.representations().0
    }

    pub fn build_dir(self) -> &'static str {
        self.representations().1
    }

    pub fn short(self) -> &'static str {
        self.representations().2
    }
}

fn default_build_profiles() -> Vec<BuildProfile> {
    Vec::from_iter([BuildProfile::Release])
}

const fn default_true() -> bool {
    true
}

#[derive(Debug, Serialize, Deserialize)]
#[serde(deny_unknown_fields)]
pub struct RunCmd {
    #[serde(default = "default_build_profiles")]
    pub profiles: Vec<BuildProfile>,

    pub cmd: Vec<String>,

    #[serde(default = "default_true")]
    pub echo: bool,

    #[serde(default = "default_true")]
    pub isolate: bool,

    #[serde(default)]
    pub extra_env: HashMap<String, String>,

    #[serde(default)]
    pub limits: CommandLimits,
}

#[derive(Debug, Serialize, Deserialize)]
#[serde(deny_unknown_fields)]
pub(crate) struct ForbiddenPatternsGroup {
    #[serde(default)]
    pub(crate) substring: Vec<String>,

    #[serde(default)]
    pub(crate) regex: Vec<String>,

    #[serde(default)]
    pub(crate) token: Vec<String>,

    pub(crate) hint: Option<String>,
}

impl ForbiddenPatternsGroup {
    pub(crate) fn is_empty(&self) -> bool {
        [&self.substring, &self.regex, &self.token]
            .into_iter()
            .all(Vec::is_empty)
    }
}

#[derive(Debug, Serialize, Deserialize)]
#[serde(deny_unknown_fields)]
pub(crate) struct ForbiddenPatterns {
    #[serde(default)]
    pub(crate) groups: Vec<ForbiddenPatternsGroup>,

    #[serde(flatten)]
    pub(crate) patterns: Option<ForbiddenPatternsGroup>,
}

impl ForbiddenPatterns {
    pub(crate) fn groups(&self) -> impl Iterator<Item = &ForbiddenPatternsGroup> {
        self.patterns
            .iter()
            .chain(&self.groups)
            .filter(|g| !g.is_empty())
    }
}

#[derive(Debug, Serialize, Deserialize)]
#[serde(deny_unknown_fields)]
pub struct ReportScore {
    pub task: String,
}

#[derive(Debug, Serialize, Deserialize)]
#[serde(tag = "type", rename_all = "kebab-case", deny_unknown_fields)]
pub enum TestStep {
    ForbiddenPatterns(ForbiddenPatterns),
    RunCmd(RunCmd),
    ReportScore(ReportScore),
}

#[derive(Debug, Serialize, Deserialize)]
#[serde(deny_unknown_fields)]
pub struct TestConfig {
    pub tests: Vec<TestStep>,
    pub editable: Vec<String>,
    #[serde(default)]
    pub playground: Option<String>,
}

#[derive(Debug)]
pub(crate) struct TaskContext {
    /// Relative to project root
    pub task_path: PathBuf,
    pub test_config: TestConfig,
    pub repo_root: Rc<Path>,
}

impl TaskContext {
    pub const INHERIT_VARS: [&str; 5] = ["PATH", "USER", "HOME", "TERM", "CI"];
    pub const BUILD_TMPFS_SIZE: usize = 1 << 30; // 1 GiB

    pub(crate) fn new_with_config(
        task_abs_path: &Path,
        repo_root: Rc<Path>,
        config_path: &Path,
    ) -> Result<Self> {
        let r: Result<_> = try {
            let config_file = fs::File::open(config_path)
                .with_context(|| format!("failed to open {}", config_path.display()))?;
            let test_config = serde_yml::from_reader(config_file)
                .with_context(|| format!("failed to parse {}", config_path.display()))?;

            let task_path = task_abs_path.strip_prefix_logged(&repo_root)?.to_owned();
            Self {
                task_path,
                test_config,
                repo_root: Rc::clone(&repo_root),
            }
        };
        r.with_context(|| {
            format!(
                "creating task context for {} at {} with config {}",
                task_abs_path.display(),
                repo_root.display(),
                config_path.display()
            )
        })
    }

    pub(crate) fn new(task_abs_path: &Path, repo_root: Rc<Path>) -> Result<Self> {
        Self::new_with_config(task_abs_path, repo_root, &Self::config_path(task_abs_path))
    }

    pub fn discover_at<P: AsRef<Path>>(p: P) -> Result<(Self, Repository)> {
        let p = p.as_ref();
        let repo = gix::discover(p)?;

        let task_abs_path = fs::canonicalize(p)?;
        let repo_root = repo
            .workdir()
            .context("failed to get working directory")?
            .canonicalize()?;
        let repo_root = Rc::<Path>::from(repo_root);
        let context = TaskContext::new(&task_abs_path, Rc::clone(&repo_root))?;
        Ok((context, repo))
    }

    pub fn config_path(task_abs_path: &Path) -> PathBuf {
        task_abs_path.join(TESTING_CONFIG_FILENAME)
    }

    pub fn is_task(path: &Path) -> Result<bool> {
        let r: Result<_> = try {
            match fs::metadata(path.join(TESTING_CONFIG_FILENAME)) {
                Ok(meta) => meta.is_file(),
                Err(e) if e.kind() == io::ErrorKind::NotFound => false,
                Err(e) => Err(e)?,
            }
        };
        r.with_context(|| format!("checking task at {}", path.display()))
    }

    pub(crate) fn full_path(&self) -> PathBuf {
        self.repo_root.join(&self.task_path)
    }

    pub(crate) fn path_of<P: AsRef<Path>>(&self, p: P) -> PathBuf {
        self.full_path().join(p)
    }

    pub(crate) fn strip_task_root_logged<'a>(&self, path: &'a Path) -> Result<&'a Path> {
        path.strip_prefix_logged(&self.repo_root)?
            .strip_prefix_logged(&self.task_path)
    }

    pub(crate) fn find_editable_files(&self) -> Result<BTreeSet<PathBuf>> {
        let r: Result<_> = try {
            let full_task_path = self.full_path();
            let is_editable = {
                let mut builder = GlobSetBuilder::new();
                for path in &self.test_config.editable {
                    builder.add(
                        Glob::new(path)
                            .with_context(|| format!("creating glob pattern from \"{path}\""))?,
                    );
                }
                builder.build().context("creating globset")?
            };

            let mut editable = BTreeSet::new();

            for item in WalkDir::new(&full_task_path)
                .into_iter()
                .filter_entry(|entry| {
                    entry.file_type().is_dir()
                        || entry
                            .path()
                            .strip_prefix(&full_task_path)
                            .map(|p| is_editable.is_match(p))
                            .unwrap_or(false)
                })
            {
                let item =
                    item.with_context(|| format!("iterating over {}", full_task_path.display()))?;
                if item.file_type().is_dir() {
                    continue;
                }
                editable.insert(item.path().to_path_buf());
            }

            if editable.is_empty() {
                Err(anyhow!("no editable files found"))?;
            }

            editable
        };

        r.with_context(|| {
            format!(
                "searching editable files for task {}",
                self.task_path.display()
            )
        })
    }

    pub fn run_cmake<R: CommandRunner + ?Sized>(
        &self,
        build_dir: &Path,
        profile: BuildProfile,
        fresh: bool,
        runner: &R,
    ) -> Result<process::ExitStatus> {
        let mut cmd = CommandBuilder::new("cmake")
            .inherit_envs(Self::INHERIT_VARS)
            .inherit_envs(["CXX", "CC"]);
        if fresh {
            cmd = cmd.arg("--fresh");
        }
        cmd = cmd
            .arg(format!("-DCMAKE_BUILD_TYPE={}", profile.cmake_build_type()))
            .arg("-B")
            .arg(build_dir)
            .arg("-S")
            .arg(self.repo_root.as_ref())
            .with_rw_mount(&*self.repo_root)
            .with_default_tmpfs_size(Self::BUILD_TMPFS_SIZE)
            .with_cwd(&*self.repo_root);
        debug!("Running cmake: {cmd:?}");
        runner.status_logged(&cmd)
    }

    pub fn build_target<R: CommandRunner + ?Sized>(
        &self,
        target: &str,
        profile: BuildProfile,
        runner: &R,
    ) -> Result<PathBuf> {
        let build_dir = self.repo_root.join(profile.build_dir());
        if !build_dir.exists() {
            debug!(
                "Build directory {} doesn't exist, creating",
                build_dir.display()
            );
            fs::create_dir(&build_dir)
                .with_context(|| format!("failed to create {}", build_dir.display()))?;
            self.run_cmake(&build_dir, profile, false, runner)?;
        }

        let cpus_str = format!("{}", num_cpus::get());
        let build = || {
            let build_cmd = CommandBuilder::new("cmake")
                .inherit_envs(Self::INHERIT_VARS)
                .inherit_envs(["CXX", "CC"])
                .arg("--build")
                .arg(&build_dir)
                .arg("--target")
                .arg(target)
                .arg("-j")
                .arg(&cpus_str)
                .with_rw_mount(&*self.repo_root)
                .with_default_tmpfs_size(Self::BUILD_TMPFS_SIZE)
                .with_cwd(&*self.repo_root);
            debug!("Running build: {build_cmd:?}");
            runner.status_logged(&build_cmd)
        };

        let target_path = build_dir.join(target);

        let status = build()?;
        match status.exit_ok() {
            Ok(()) => return Ok(target_path),
            Err(e) => warn!("Failed to build target: {e}. Going to re-run cmake"),
        }

        self.run_cmake(&build_dir, profile, true, runner)?;

        build()?.exit_ok().context("running cmake")?;

        Ok(target_path)
    }
}
