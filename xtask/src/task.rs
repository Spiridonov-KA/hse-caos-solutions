use std::{
    collections::BTreeSet,
    fs, io,
    path::{Path, PathBuf},
    rc::Rc,
};

use anyhow::{Context, Result, bail};
use globset::{Glob, GlobSetBuilder};
use serde::{Deserialize, Serialize};
use strum_macros::EnumIter;
use walkdir::WalkDir;

use crate::util::PathExt;

const TESTING_CONFIG_FILENAME: &str = "testing.yaml";

#[derive(Debug, Serialize, Deserialize, Clone, Copy, EnumIter, PartialEq, Eq)]
#[serde(rename_all = "lowercase")]
pub(crate) enum BuildProfile {
    Release,
    Debug,
    ASan,
    TSan,
}

fn default_build_profiles() -> Vec<BuildProfile> {
    Vec::from_iter([BuildProfile::Release])
}

const fn default_true() -> bool {
    true
}

#[derive(Debug, Serialize, Deserialize)]
pub(crate) struct RunCmd {
    #[serde(default = "default_build_profiles")]
    pub(crate) profiles: Vec<BuildProfile>,

    pub(crate) cmd: Vec<String>,

    #[serde(default = "default_true")]
    pub(crate) echo: bool,
}

#[derive(Debug, Serialize, Deserialize)]
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
pub struct ReportScore {
    pub task: String,
}

#[derive(Debug, Serialize, Deserialize)]
#[serde(tag = "type", rename_all = "kebab-case")]
pub(crate) enum TestStep {
    ForbiddenPatterns(ForbiddenPatterns),
    RunCmd(RunCmd),
    ReportScore(ReportScore),
}

#[derive(Debug, Serialize, Deserialize)]
pub(crate) struct TestConfig {
    pub(crate) tests: Vec<TestStep>,
    pub(crate) editable: Vec<String>,
}

#[derive(Debug)]
pub(crate) struct TaskContext {
    /// Relative to project root
    pub(crate) task_path: PathBuf,
    pub(crate) test_config: TestConfig,
    repo_root: Rc<Path>,
}

impl TaskContext {
    pub(crate) fn new(task_abs_path: &Path, repo_root: Rc<Path>) -> Result<Self> {
        let r: Result<_> = try {
            let config_path = task_abs_path.join(TESTING_CONFIG_FILENAME);
            let config_file = fs::File::open(&config_path)
                .with_context(|| format!("failed to open {}", config_path.display()))?;
            let test_config = serde_yml::from_reader(config_file)
                .with_context(|| format!("failed to parse {}", config_path.display()))?;

            let task_path = task_abs_path.strip_prefix_logged(&repo_root)?.to_owned();
            Self {
                task_path,
                test_config,
                repo_root,
            }
        };
        r.with_context(|| format!("creating task context for {}", task_abs_path.display()))
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
                bail!("no editable files found");
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
}
