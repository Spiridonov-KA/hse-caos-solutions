use anyhow::{Context, Result, anyhow};
use std::{path::Path, process};

use crate::command::{CommandBuilder, CommandRunner};

pub trait PathExt {
    fn to_str_logged(&self) -> Result<&str>;

    fn strip_prefix_logged<P: AsRef<Path>>(&self, prefix: P) -> Result<&Path>;
}

impl PathExt for Path {
    fn to_str_logged(&self) -> Result<&str> {
        self.to_str()
            .ok_or_else(|| anyhow!("couldn't convert {} to UTF-8 string", self.display()))
    }

    fn strip_prefix_logged<P: AsRef<Path>>(&self, prefix: P) -> Result<&Path> {
        self.strip_prefix(&prefix).with_context(|| {
            format!(
                "{} is not a prefix of {}",
                self.display(),
                prefix.as_ref().display(),
            )
        })
    }
}

pub trait CommandRunnerExt {
    fn status_logged(&self, cmd: &CommandBuilder) -> Result<process::ExitStatus>;
}

impl<T: CommandRunner + ?Sized> CommandRunnerExt for T {
    fn status_logged(&self, cmd: &CommandBuilder) -> Result<process::ExitStatus> {
        self.status(cmd).with_context(|| format!("running {cmd:?}"))
    }
}
