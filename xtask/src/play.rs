use crate::{
    command::NativeCommandRunner,
    task::{BuildProfile, TaskContext},
};
use anyhow::{Context, Result, bail};
use clap::Parser;
use std::process;
use tracing::info;

#[derive(Debug, Parser)]
pub struct PlayArgs {}

pub fn do_play(_args: PlayArgs) -> Result<()> {
    let (ctx, _) = TaskContext::discover_at(".")?;
    let target = match ctx.test_config.playground.as_ref() {
        Some(p) => p,
        None => bail!("Task {} doesn't have a playground", ctx.task_path.display()),
    };

    let exe = ctx.build_target(target, BuildProfile::Release, &NativeCommandRunner)?;
    let mut cmd = process::Command::new(&exe);

    info!("Running {cmd:?}");

    cmd.status()
        .with_context(|| format!("Failed to run {}", exe.display()))?
        .exit_ok()?;
    Ok(())
}
