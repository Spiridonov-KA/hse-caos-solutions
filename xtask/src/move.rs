use anyhow::{Context, Result};
use std::{
    fs,
    path::{Path, PathBuf},
    rc::Rc,
};
use tracing::{debug, trace};
use walkdir::WalkDir;

use clap::Parser;

use crate::task::TaskContext;

#[derive(Debug, Parser)]
pub struct MoveArgs {
    /// Path to repository with solutions
    #[arg(long)]
    pub from: PathBuf,

    /// Path to clean repository
    #[arg(long, default_value = ".")]
    pub to: PathBuf,
}

pub struct RepoContext {
    repo_root: Rc<Path>,
}

impl RepoContext {
    pub fn new(path: PathBuf) -> Self {
        RepoContext {
            repo_root: Rc::from(path),
        }
    }
}

pub struct MoveContext {
    from: RepoContext,
    to: RepoContext,
}

impl MoveContext {
    pub fn new(args: MoveArgs) -> Result<Self> {
        Ok(Self {
            from: RepoContext::new(args.from.canonicalize()?),
            to: RepoContext::new(args.to.canonicalize()?),
        })
    }

    pub fn run(&self) -> Result<()> {
        for entry in WalkDir::new(&self.to.repo_root).into_iter() {
            let entry =
                entry.with_context(|| format!("iterating over {}", self.to.repo_root.display()))?;

            if !entry.file_type().is_dir() || !TaskContext::is_task(entry.path())? {
                continue;
            }

            let ctx_to = TaskContext::new(entry.path(), Rc::clone(&self.to.repo_root))?;
            let ctx_from = TaskContext::new(
                &self.from.repo_root.join(&ctx_to.task_path),
                Rc::clone(&self.from.repo_root),
            )?;

            debug!("Found task {}", ctx_to.task_path.display());

            for file in ctx_to.find_editable_files()? {
                trace!("Removing target file {}", file.display());
                fs::remove_file(file)?;
            }

            for file_source in ctx_from.find_editable_files()? {
                let file_local = ctx_from.strip_task_root_logged(&file_source)?;
                let file_target = ctx_to.path_of(file_local);
                trace!("Copying {}", file_local.display());
                if let Some(dir) = file_target.parent() {
                    fs::create_dir_all(dir)?;
                }
                fs::copy(file_source, file_target)?;
            }
        }

        Ok(())
    }
}

pub fn do_move(args: MoveArgs) -> Result<()> {
    MoveContext::new(args)
        .context("creating move context")?
        .run()
}
