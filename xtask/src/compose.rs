// Стащено из
// https://gitlab.com/sergey-v-galtsev/nikka-public/-/blob/3f452a8ee780500e93257fb7d8402676631fac7d/tools/compose/src/main.rs
use anyhow::{Context, Result, bail};
use clap::Parser;
use globset::{Glob, GlobSet, GlobSetBuilder};
use itertools::Itertools;
use serde::{Deserialize, Serialize};
use std::{
    collections::HashMap,
    fs,
    io::{self, BufRead},
    path::{Path, PathBuf},
};
use tracing::{debug, info};

use crate::util::PathExt;

#[derive(Debug, Parser)]
pub struct ComposeArgs {
    /// Path to the private repo.
    #[arg(short, long)]
    in_path: PathBuf,

    /// Path to the public repo.
    #[arg(short, long)]
    out_path: Option<PathBuf>,

    /// Skip given directories from pruning. Accepts glob patters as well.
    #[arg(long)]
    skip: Vec<String>,

    #[arg(long, action)]
    gitignore: bool,

    #[arg(long = "stat")]
    stat: bool,
}

enum TokenKind {
    Private,
    Public,
    BeginPrivate,
    EndPrivate,
}

#[derive(Clone, PartialEq, Eq)]
enum TokenProperty {
    Task(String),
    Stub(String),
    NoHint,
    Unimplemented,
}

struct Token {
    kind: TokenKind,
    properties: Vec<TokenProperty>,
}

fn parse_token(line: &str) -> Result<Option<Token>> {
    let comment = match line.find("//") {
        Some(pos) => &line[pos..],
        None => return Ok(None),
    };

    const COMPOSE_PREFIX: &str = "compose::";
    let cmd = match comment.find(COMPOSE_PREFIX) {
        Some(pos) => &comment[pos + COMPOSE_PREFIX.len()..],
        None => return Ok(None),
    };

    let command_token = {
        let len = cmd
            .find(|c: char| !c.is_ascii_alphanumeric() && !"_-".contains(c))
            .unwrap_or(cmd.len());
        &cmd[..len]
    };

    let kind = match command_token {
        "private" => TokenKind::Private,
        "public" => TokenKind::Public,
        "begin_private" => TokenKind::BeginPrivate,
        "end_private" => TokenKind::EndPrivate,
        cmd => bail!("unknown compose command: {cmd}"),
    };

    let properties_str = match cmd.find('(') {
        Some(pos) => {
            let trimmed = cmd.trim_end();
            if !trimmed.ends_with(')') {
                bail!("unclosed '('");
            }
            &cmd[pos + 1..trimmed.len() - 1]
        }
        None => "",
    };

    let mut properties = vec![];
    match kind {
        TokenKind::Public => properties.push(TokenProperty::Stub(properties_str.to_string())),
        _ => {
            for (i, prop) in properties_str.split(',').enumerate() {
                match prop {
                    "no_hint" => properties.push(TokenProperty::NoHint),
                    "unimpl" | "unimplemented" => properties.push(TokenProperty::Unimplemented),
                    s => {
                        if i == 0 {
                            properties.push(TokenProperty::Task(String::from(s)))
                        } else {
                            bail!("unknown property: {}", prop)
                        }
                    }
                }
            }
        }
    }

    Ok(Some(Token { kind, properties }))
}

fn find_token(lines: &[&str], start: usize) -> Result<Option<(usize, Token)>> {
    for (i, line) in lines[start..].iter().enumerate() {
        let mb_token = parse_token(line)
            .with_context(|| format!("failed to parse token on line {}", i + 1))?;
        if let Some(token) = mb_token {
            return Ok(Some((i + start, token)));
        }
    }
    Ok(None)
}

struct ProcessedSource {
    src: String,

    removed_lines: usize,
    removed_by_task: HashMap<String, usize>,
}

fn process_source(src: String) -> Result<ProcessedSource> {
    let mut dst = String::new();
    let mut removed_lines: usize = 0;
    let mut removed_by_task: HashMap<String, usize> = HashMap::new();

    let lines = src.lines().collect::<Vec<_>>();
    let mut next_pos = 0;
    while let Some((begin, token)) = find_token(&lines, next_pos)? {
        let end = match token.kind {
            TokenKind::EndPrivate => bail!("unpaired 'end_private' on line {}", begin + 1),
            TokenKind::Private | TokenKind::Public => begin + 1,
            TokenKind::BeginPrivate => {
                let mut pos = begin + 1;
                let mut mb_end: Option<usize> = None;
                while let Some((k, token)) = find_token(&lines, pos)? {
                    match token.kind {
                        TokenKind::BeginPrivate => {
                            bail!("nested 'begin_private' on line {}", k + 1)
                        }
                        TokenKind::Private | TokenKind::Public => pos = k + 1,
                        TokenKind::EndPrivate => {
                            mb_end = Some(k);
                            break;
                        }
                    }
                }
                match mb_end {
                    Some(end) => end + 1,
                    None => bail!("unclosed 'begin_private' on line {}", begin + 1),
                }
            }
        };

        for line in lines.iter().take(begin).skip(next_pos) {
            dst += line;
            dst += "\n";
        }

        removed_lines += end - begin;
        if !token.properties.is_empty()
            && let TokenProperty::Task(task) = &token.properties[0]
        {
            *removed_by_task.entry(task.clone()).or_insert(0) += end - begin;
        }

        let no_hint = token.properties.contains(&TokenProperty::NoHint);
        let unimpl = token.properties.contains(&TokenProperty::Unimplemented);
        let stub = token
            .properties
            .iter()
            .filter_map(|property| {
                if let TokenProperty::Stub(stub) = property {
                    Some(stub)
                } else {
                    None
                }
            })
            .next();
        if no_hint {
            if begin > 0
                && lines[begin - 1].trim().is_empty()
                && end < lines.len()
                && lines[end].trim().is_empty()
            {
                next_pos = end + 1;
            } else {
                next_pos = end;
            }
        } else {
            let mut insert_line = |line: &str| {
                for c in lines[begin].chars().take_while(|c| c.is_whitespace()) {
                    dst.push(c);
                }
                dst.push_str(line);
            };

            if let Some(stub) = stub {
                insert_line(stub);
                dst.push_str("  // TODO: remove before flight.\n");
            } else {
                insert_line("// TODO: your code here.\n");
                if unimpl {
                    insert_line("throw \"TODO\";\n");
                }
            }

            next_pos = end;
        }
    }

    for line in &lines[next_pos..] {
        dst += line;
        dst += "\n";
    }

    Ok(ProcessedSource {
        src: dst,
        removed_lines,
        removed_by_task,
    })
}

#[derive(Clone, Debug, Deserialize, Serialize)]
struct ExportList {
    export: Vec<PathBuf>,
}

impl ExportList {
    fn get_checker(&self) -> Result<impl for<'a> Fn(&'a Path) -> bool + use<>> {
        let mut builder = GlobSetBuilder::new();
        for path in &self.export {
            builder.add(Glob::new(path.to_str_logged()?)?);
        }

        let set = builder.build()?;

        let checker: impl for<'a> Fn(&'a Path) -> bool = move |p| set.is_match(p);
        Ok(Box::new(checker))
    }
}

struct Compose {
    args: ComposeArgs,

    removed_lines: usize,
    removed_by_task: HashMap<String, usize>,
    glob_set: GlobSet,
}

impl Compose {
    const PROCESS_EXTENSIONS: [&'static str; 6] = [".h", ".hpp", ".c", ".cpp", ".s", ".S"];

    fn new(args: ComposeArgs) -> Result<Self> {
        let mut glob_builder = GlobSetBuilder::new();

        let mut add_glob = |path: &str| -> Result<()> {
            let path = path.trim().trim_end_matches("/");
            if path.is_empty() || path.starts_with("#") {
                return Ok(());
            }
            glob_builder
                .add(Glob::new(path).with_context(|| format!("Failed to add pattern {path}"))?);
            Ok(())
        };

        if args.gitignore {
            let gitignore_path = PathBuf::from_iter([&args.in_path, Path::new(".gitignore")]);
            let f = fs::File::open(&gitignore_path)
                .with_context(|| format!("Failed to open file {}", gitignore_path.display()))?;

            for line in io::BufReader::new(f)
                .lines()
                .chain(std::iter::once(Ok(".git".to_string())))
            {
                let line = line.with_context(|| {
                    format!("Failed to read from file {}", gitignore_path.display())
                })?;
                let line = line.trim().trim_end_matches("/");
                if line.is_empty() || line.starts_with("#") {
                    continue;
                }
                add_glob(line)?;
            }
        }

        for path in &args.skip {
            add_glob(path)?;
        }

        Ok(Compose {
            args,
            removed_lines: 0,
            removed_by_task: HashMap::new(),
            glob_set: glob_builder.build().context("Failed to build glob set")?,
        })
    }

    fn process_file(&mut self, in_path: &Path, out_path: Option<&Path>) -> Result<()> {
        if in_path
            .to_str()
            .map(|s| Self::PROCESS_EXTENSIONS.iter().any(|ext| s.ends_with(ext)))
            .unwrap_or(false)
        {
            let content = fs::read_to_string(in_path)
                .with_context(|| format!("failed to read file {}", in_path.display()))?;

            let new_content = process_source(content)
                .with_context(|| format!("failed to process file {}", in_path.display()))?;

            self.removed_lines += new_content.removed_lines;

            for (k, v) in new_content.removed_by_task {
                *self.removed_by_task.entry(k).or_insert(0) += v;
            }

            if let Some(out_file) = out_path {
                fs::write(out_file, new_content.src)
                    .with_context(|| format!("failed to write file {}", out_file.display()))?;
            }
        } else if let Some(out_file) = out_path
            && in_path != out_file
        {
            fs::copy(in_path, out_file).with_context(|| {
                format!(
                    "failed to copy {} to {}",
                    in_path.display(),
                    out_file.display()
                )
            })?;
        }

        Ok(())
    }

    fn process_symlink(&self, in_path: &Path, out_path: Option<&Path>) -> Result<()> {
        let out_path = if let Some(p) = out_path {
            p
        } else {
            return Ok(());
        };

        if let Ok(to_meta) = fs::symlink_metadata(out_path) {
            if to_meta.is_dir() {
                fs::remove_dir_all(out_path)
            } else {
                fs::remove_file(out_path)
            }
            .with_context(|| format!("removing {}", out_path.display()))?
        }

        let target = fs::read_link(in_path)?;
        std::os::unix::fs::symlink(&target, out_path)?;

        Ok(())
    }

    #[allow(clippy::type_complexity)]
    fn get_export_checker(dir: &Path) -> Result<Box<dyn Fn(&Path) -> bool>> {
        let export_config = Path::new(".export.yaml");
        let path = PathBuf::from_iter([dir, export_config]);

        let f = fs::File::open(&path);
        let f = match f {
            Ok(f) => f,
            Err(e) => match e.kind() {
                io::ErrorKind::NotFound => return Ok(Box::new(|_| true)),
                _ => Err(e).context(format!("failed to open {}", path.display()))?,
            },
        };

        let config: ExportList = serde_yml::from_reader(f)?;
        Ok(Box::new(config.get_checker()?))
    }

    fn process_dir(&mut self, in_path: &Path, out_path: Option<&Path>) -> Result<()> {
        let dir = fs::read_dir(in_path)
            .with_context(|| format!("failed to read dir {}", in_path.display()))?;

        if let Some(out_dir) = out_path {
            fs::create_dir_all(out_dir)
                .with_context(|| format!("failed to create dir {}", out_dir.display()))?;
        }

        let checker = Self::get_export_checker(in_path)?;

        for mb_entry in dir {
            let name = mb_entry
                .with_context(|| format!("failed to read entry in dir {}", in_path.display()))?
                .file_name();

            if !checker(Path::new(&name)) {
                debug!(
                    "Skipping {} export because it's not in export list",
                    name.display()
                );
                continue;
            }

            let new_in_path = in_path.join(&name);
            let new_out_path = out_path.map(|p| p.join(&name));

            if self
                .glob_set
                .is_match(
                    new_in_path
                        .strip_prefix(&self.args.in_path)
                        .with_context(|| {
                            format!(
                                "{} is outside of {}",
                                new_in_path.display(),
                                self.args.in_path.display()
                            )
                        })?,
                )
            {
                continue;
            }

            if new_in_path.is_symlink() {
                self.process_symlink(&new_in_path, new_out_path.as_deref())?;
            } else if new_in_path.is_dir() {
                self.process_dir(&new_in_path, new_out_path.as_deref())?;
            } else if new_in_path.is_file() {
                self.process_file(&new_in_path, new_out_path.as_deref())?;
            } else {
                bail!("unknown thing at {}", new_in_path.display());
            }
        }

        Ok(())
    }

    fn process(&mut self) -> Result<()> {
        self.process_dir(
            self.args.in_path.clone().as_path(),
            self.args.out_path.clone().as_deref(),
        )?;

        if self.args.stat {
            for (k, v) in self.removed_by_task.iter().sorted_by_key(|x| x.0) {
                info!("{v}\t{k}");
            }

            info!("{}\ttotal", self.removed_lines);
        }

        Ok(())
    }
}

pub fn do_compose(args: ComposeArgs) -> Result<()> {
    let mut compose = Compose::new(args)?;

    compose.process().context("failed to process entries")?;

    Ok(())
}
