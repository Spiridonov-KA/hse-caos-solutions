use anyhow::Result;
use clap::Parser;
use tracing_subscriber::EnvFilter;
use xtask::dispatch::{Command, execute};

#[derive(Debug, Parser)]
#[command(author, version, about, long_about = None)]
struct Args {
    #[command(subcommand)]
    cmd: Command,
}

fn main() -> Result<()> {
    let env_filter = EnvFilter::try_from_default_env().unwrap_or_else(|_| EnvFilter::new("info"));
    tracing_subscriber::fmt()
        .with_env_filter(env_filter)
        .without_time()
        .with_target(false)
        .init();

    let args = Args::try_parse()?;
    execute(args.cmd)
}
