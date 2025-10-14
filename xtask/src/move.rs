use anyhow::{Context, Result, bail};
use std::{
    fs,
    path::{Path, PathBuf},
    rc::Rc,
};
use tracing::{debug, warn};
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
        for entry in WalkDir::new(&self.to.repo_root) {
            let entry =
                entry.with_context(|| format!("iterating over {}", self.to.repo_root.display()))?;

            if !entry.file_type().is_dir() || !TaskContext::is_task(entry.path())? {
                continue;
            }

            let ctx_to = TaskContext::new(entry.path(), Rc::clone(&self.to.repo_root))?;

            let from_path = self.from.repo_root.join(&ctx_to.task_path);
            if !TaskContext::is_task(&from_path)? {
                warn!(
                    "Path {} is not a task, likely your repository is outdated",
                    from_path.display()
                );
                continue;
            }

            let ctx_from = TaskContext::new(&from_path, Rc::clone(&self.from.repo_root))?;

            debug!("Found task {}", ctx_to.task_path.display());

            for path in ctx_to.find_editable_files()? {
                debug!("Removing target path {}", path.display());
                let meta = fs::symlink_metadata(&path)
                    .with_context(|| format!("failed to stat file {}", path.display()))?;

                if meta.is_symlink() || meta.is_file() {
                    fs::remove_file(&path)
                        .with_context(|| format!("failed to remove file {}", path.display()))?;
                } else {
                    fs::remove_dir(&path).with_context(|| {
                        format!("failed to remove directory {}", path.display())
                    })?;
                }
            }

            for path_from in ctx_from.find_editable_files()? {
                let file_path = ctx_from.strip_task_root_logged(&path_from)?;
                let path_to = ctx_to.path_of(file_path);
                debug!("Copying {}", file_path.display());
                if let Some(dir) = path_to.parent() {
                    fs::create_dir_all(dir)?;
                }
                let meta = fs::symlink_metadata(&path_from)
                    .with_context(|| format!("failed to stat {}", path_from.display()))?;
                if meta.is_symlink() {
                    let target = fs::read_link(&path_from).with_context(|| {
                        format!("failed to read link at {}", path_from.display())
                    })?;
                    std::os::unix::fs::symlink(&target, &path_to).with_context(|| {
                        format!(
                            "failed to create symlink {} {}",
                            target.display(),
                            path_to.display()
                        )
                    })?;
                } else if meta.is_file() {
                    fs::copy(&path_from, &path_to)?;
                } else if meta.is_dir() {
                    if path_to != ctx_to.full_path() {
                        warn!("Skipping {}", path_to.display());
                        fs::create_dir(&path_to).with_context(|| {
                            format!("failed to create directory {}", path_to.display())
                        })?;
                    }
                } else {
                    bail!(
                        "File {} with metadata {:?} have unknown type",
                        path_from.display(),
                        meta
                    );
                }
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
