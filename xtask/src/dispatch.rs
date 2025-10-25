use crate::{
    compose::{ComposeArgs, do_compose},
    r#move::{MoveArgs, do_move},
    play::{PlayArgs, do_play},
    test::{TestArgs, do_test},
};
use anyhow::Result;
use clap::Parser;

#[derive(Debug, Parser)]
pub enum Command {
    Compose(ComposeArgs),
    Test(TestArgs),
    Move(MoveArgs),
    Play(PlayArgs),
}

pub fn execute(cmd: Command) -> Result<()> {
    match cmd {
        Command::Compose(args) => do_compose(args),
        Command::Test(args) => do_test(args),
        Command::Move(args) => do_move(args),
        Command::Play(args) => do_play(args),
    }
}
