#![deny(warnings)]
#![feature(exit_status_error)]
#![feature(try_blocks)]
#![feature(impl_trait_in_bindings)]

mod command;
mod compose;
pub mod dispatch;
mod r#move;
mod play;
mod task;
mod test;
mod util;
