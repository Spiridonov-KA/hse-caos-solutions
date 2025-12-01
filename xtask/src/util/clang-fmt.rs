use anyhow::{Context, Result, bail};
use regex::Regex;
use serde_json::Value as Json;
use std::{
    io,
    path::{Path, PathBuf},
    process,
    rc::Rc,
};
use tracing::warn;

use crate::{
    command::{CommandBuilder, CommandRunner},
    util::ext::CommandRunnerExt,
};

pub struct ClangFmtRunner {
    repo_root: Rc<Path>, // Not necessary, but anyways
    use_old: bool,
    fmt_runner: PathBuf,
}

impl ClangFmtRunner {
    pub fn new(repo_root: Rc<Path>, fmt_runner: PathBuf) -> Result<Self> {
        const MINIMAL_SUPPORTED: u8 = 11;
        const DESIRED: u8 = 15;

        let version = Self::major_clang_format_version()?;
        let mut use_old = false;
        if version < MINIMAL_SUPPORTED {
            bail!(
                "Your version of clang-format ({}) is less than minimal supported ({}). Consider upgrading it",
                version,
                MINIMAL_SUPPORTED
            );
        }
        if version < DESIRED {
            warn!(
                "Your version of clang-format ({}) is less than desired ({}). Going to use convervative config. Consider upgrading clang-format",
                version, DESIRED
            );
            use_old = true;
        }

        Ok(Self {
            repo_root,
            use_old,
            fmt_runner,
        })
    }

    fn major_clang_format_version() -> Result<u8> {
        let out = process::Command::new("clang-format")
            .arg("--version")
            .output()
            .map_err(anyhow::Error::from)
            .and_then(|o| o.exit_ok().map_err(From::from))
            .context("running clang-format")?;

        let out = String::try_from(out.stdout).context("parsing clang-format output")?;

        let re = Regex::new(r"\d+")?;
        let major = re
            .find(&out)
            .context("no version in clang-format output")?
            .as_str()
            .parse()?;
        Ok(major)
    }

    pub fn check<'a, I, F, R: CommandRunner + ?Sized>(&self, runner: &R, files: I) -> Result<()>
    where
        I: IntoIterator<Item = &'a F>,
        F: AsRef<Path> + 'a,
    {
        let mut cmd = CommandBuilder::new("python3").arg(&self.fmt_runner);

        cmd = if self.use_old {
            let old_config_path = self.repo_root.join(".clang-format-11");
            let f = std::fs::File::open(&old_config_path)
                .with_context(|| format!("failed to open {}", old_config_path.display()))?;
            let f = io::BufReader::new(f);
            let cfg: Json = serde_yml::from_reader(f)
                .with_context(|| format!("failed to parse {}", old_config_path.display()))?;
            let cfg = serde_json::to_string(&cfg)?;

            cmd.arg("--style").arg(cfg)
        } else {
            cmd.arg(format!(
                "--style=file:{}/.clang-format",
                self.repo_root.display()
            ))
        };

        cmd = cmd
            .args(files.into_iter().map(|f| f.as_ref()))
            .inherit_env("PATH")
            .inherit_env("HOME")
            .with_rw_mount(&*self.repo_root)
            .with_tmpfs_mount("/dev/shm");
        runner
            .status_logged(&cmd)?
            .exit_ok()
            .context("failed to check formatting, check the logs above for a possible fix")?;

        Ok(())
    }
}
