use std::{
    borrow::Cow,
    collections::HashMap,
    ffi::{OsStr, OsString},
    fs, iter,
    path::{Path, PathBuf},
    rc::Rc,
    time::{Duration, Instant},
};

use crate::{
    command::{CommandBuilder, CommandLimits, CommandRunner, NSJailRunner, NativeCommandRunner},
    task::{
        BuildProfile, ForbiddenPatterns, ForbiddenPatternsGroup, ReportScore, RunCmd, TaskContext,
        TestStep,
    },
    util::{ClangFmtRunner, ManytaskClient, PathExt},
};
use annotate_snippets::{Annotation, AnnotationKind, Group, Level, Renderer, Snippet, renderer};
use anyhow::{Context, Result, bail};
use chrono::DateTime;
use clap::Parser;
use derivative::Derivative;
use gix::{self, Repository};
use itertools::Itertools;
use regex::Regex;
use strum::IntoEnumIterator;
use tracing::{debug, info, warn};

#[derive(Debug, Parser)]
pub struct TestArgs {
    /// Only build necessary binaries without running any tests
    #[arg(long)]
    build_only: bool,

    /// Submit score to the system
    #[arg(long)]
    report_score: bool,

    /// Run solution in a jail
    #[arg(long)]
    jail: bool,
}

impl ForbiddenPatternsGroup {
    fn build_regex(&self) -> Result<Regex> {
        let substrings = self
            .substring
            .iter()
            .map(|lit| format!("({})", regex::escape(lit)));

        let tokens = self.token.iter().map(|lit| {
            format!(
                "(?:^|[^[:alnum:]])({})(?:[^[:alnum:]]|$)",
                regex::escape(lit)
            )
        });

        let regexes = self.regex.iter().map(Clone::clone);

        let pattern = substrings.chain(tokens).chain(regexes).join("|");

        let pattern = format!("(?ms){pattern}");

        Regex::new(&pattern).with_context(|| format!("compiling regex pattern: {pattern}"))
    }
}

#[derive(Debug)]
struct RepoConfig {
    tools: HashMap<&'static str, &'static str>,
    repo_root: Rc<Path>,
    manytask_url: &'static str,
    tester_token: Option<String>,
    manytask_course_name: String,
    default_limits: CommandLimits,
    disable_fmt_check: bool,
}

impl RepoConfig {
    fn new(repo_root: Rc<Path>) -> Self {
        let course_name = std::env::var("MT_COURSE_NAME").unwrap_or("hse-caos-2025".to_string());
        let disable_fmt_check = std::env::var("DISABLE_FMT_CHECK")
            .is_ok_and(|s| s == "1" || s == "On" || s == "ON" || s == "YES");
        Self {
            tools: HashMap::from_iter([
                ("nej-runner", "common/tools/test.py"),
                ("clang-fmt-runner", "common/tools/run-clang-format.py"),
            ]),
            repo_root,
            manytask_url: "https://manytask.hse-caos.org",
            tester_token: std::env::var("TESTER_TOKEN").ok(),
            manytask_course_name: course_name,
            default_limits: CommandLimits {
                procs: Some(10),
                memory_mb: Some(256),
                wall_time_sec: Some(10),
                cpu_ms_per_sec: Some(3000),
            },
            disable_fmt_check,
        }
    }

    fn get_tool(&self, name: &str) -> Result<&'static str> {
        self.tools
            .get(name)
            .copied()
            .with_context(|| format!("tool {name} not found"))
    }

    fn get_tool_path(&self, name: &str) -> Result<PathBuf> {
        Ok(self.repo_root.join(self.get_tool(name)?))
    }
}

#[derive(Derivative)]
#[derivative(Debug)]
struct TestContext {
    #[allow(dead_code)]
    repo: Repository,
    repo_root: Rc<Path>,
    repo_config: RepoConfig,
    task_context: TaskContext,
    args: TestArgs,
    manytask_client: Option<ManytaskClient>,

    #[derivative(Debug = "ignore")]
    cmd_runner: Box<dyn CommandRunner>,
}

impl TestContext {
    fn new(args: TestArgs) -> Result<Self> {
        let (task_context, repo) = TaskContext::discover_at(".")?;
        let repo_config = RepoConfig::new(Rc::clone(&task_context.repo_root));

        let manytask_client = if args.report_score {
            let tester_token = repo_config
                .tester_token
                .as_ref()
                .context("TESTER_TOKEN should be set to report score")?;
            Some(ManytaskClient::new(repo_config.manytask_url, tester_token)?)
        } else {
            None
        };

        let use_jail = args.jail || args.report_score;
        let runner: Box<dyn CommandRunner> = if use_jail {
            Box::new(NSJailRunner::new()?)
        } else {
            Box::new(NativeCommandRunner)
        };

        Ok(TestContext {
            repo,
            repo_config,
            repo_root: Rc::clone(&task_context.repo_root),
            task_context,
            args,
            manytask_client,
            cmd_runner: runner,
        })
    }

    fn build_target(&self, target: &str, profile: BuildProfile) -> Result<PathBuf> {
        self.task_context
            .build_target(target, profile, &*self.cmd_runner)
    }

    fn run(self) -> Result<()> {
        self.check_format()?;
        for step in &self.task_context.test_config.tests {
            self.run_step(step)?;
        }
        Ok(())
    }

    #[allow(clippy::type_complexity)]
    fn cmd_arg_processors<'a>(
        &'a self,
    ) -> HashMap<
        Cow<'static, str>,
        Box<dyn Fn(&TestContext, BuildProfile, &str) -> Result<OsString> + 'a>,
    > {
        let mut processors = HashMap::<
            Cow<'static, str>,
            Box<dyn Fn(&TestContext, BuildProfile, &str) -> Result<OsString>>,
        >::new();

        let find_tool = {
            |_: &TestContext, _: BuildProfile, arg: &str| {
                Ok(self.repo_config.get_tool_path(arg)?.into())
            }
        };
        processors.insert(Cow::from("tool"), Box::new(find_tool));

        let local = |ctx: &TestContext, _: BuildProfile, arg: &str| -> Result<OsString> {
            Ok(ctx.task_context.path_of(arg).into())
        };
        processors.insert(Cow::from("local"), Box::new(local));

        let build = |ctx: &TestContext, profile: BuildProfile, target: &str| {
            ctx.build_target(target, profile).map(Into::into)
        };
        processors.insert(Cow::from("build"), Box::new(build));

        for profile in BuildProfile::iter() {
            let build_profile = move |ctx: &TestContext, _: BuildProfile, target: &str| {
                ctx.build_target(target, profile).map(Into::into)
            };
            processors.insert(
                Cow::from(format!("build({})", profile.short())),
                Box::new(build_profile),
            );
        }

        processors
    }

    fn run_cmd_profile(&self, cfg: &RunCmd, profile: BuildProfile) -> Result<()> {
        let process_cmd_arg = {
            let processors = self.cmd_arg_processors();

            let f: impl for<'a> Fn(&'a str) -> Result<Cow<'a, OsStr>> = move |arg: &str| {
                let pos = if let Some(pos) = arg.find(':') {
                    pos
                } else {
                    return Ok(Cow::from(OsStr::new(arg)));
                };

                let processor = &arg[..pos];
                let arg = &arg[pos + 1..];

                let processor = processors
                    .get(processor)
                    .with_context(|| format!("processor \"{}\" not found", processor))?;
                processor(self, profile, arg).map(Cow::from)
            };
            f
        };

        let args = cfg
            .cmd
            .iter()
            .map(|p| process_cmd_arg(p))
            .collect::<Result<Vec<_>, _>>()?;

        if self.args.build_only {
            return Ok(());
        }

        if cfg.echo {
            info!("Running {}", args.iter().map(|s| s.display()).join(" "));
        }

        let bin = args.first().context("no cmd was given")?;

        let limits = {
            let mut limits = self.repo_config.default_limits.update_with(cfg.limits);
            let memory_multiplier = match profile {
                // https://clang.llvm.org/docs/AddressSanitizer.html#limitations
                BuildProfile::ASan => 3,
                // https://releases.llvm.org/18.1.7/tools/clang/docs/ThreadSanitizer.html#introduction
                BuildProfile::TSan => 9,
                _ => 1,
            };
            if memory_multiplier != 1 {
                debug!(
                    "Increasing memory limit {memory_multiplier} times because build type is {profile:?}"
                );
                limits.memory_mb = limits.memory_mb.map(|m| m * memory_multiplier);
            }
            limits
        };

        let cmd = CommandBuilder::new(bin)
            .args(&args[1..])
            // TODO: Move to config
            .inherit_envs(TaskContext::INHERIT_VARS)
            .with_envs(cfg.extra_env.iter())
            .with_limits(limits)
            .with_ro_mount(&*self.repo_root)
            .with_rw_mount(self.task_context.full_path())
            .with_cwd(self.task_context.full_path());

        let start = Instant::now();
        let status = if cfg.isolate {
            self.cmd_runner.status(&cmd)
        } else {
            NativeCommandRunner.status(&cmd)
        }?;

        match status.exit_ok() {
            Ok(()) => {}
            Err(e) => {
                let elapsed = start.elapsed();
                if let Some(wtl) = limits.wall_time_sec
                    && elapsed >= Duration::from_secs(wtl as u64) - Duration::from_millis(100)
                {
                    Err(e).context(format!(
                        "execution time was {elapsed:?}, likely time limit exceeded"
                    ))?
                } else {
                    Err(e)?
                }
            }
        };

        Ok(())
    }

    fn run_cmd(&self, cfg: &RunCmd) -> Result<()> {
        for &profile in &cfg.profiles {
            self.run_cmd_profile(cfg, profile)?;
        }
        Ok(())
    }

    fn report_score(&self, cfg: &ReportScore) -> Result<()> {
        if self.args.build_only {
            debug!(
                "Build-only mode, score for task {} is not reported",
                cfg.task
            );
            return Ok(());
        }
        let client = if let Some(ref client) = self.manytask_client {
            client
        } else {
            info!(
                "Task {} is solved, submit it to gitlab to get points",
                cfg.task
            );
            return Ok(());
        };

        let user = if let Ok(username) = std::env::var("CI_PROJECT_NAME") {
            username
        } else {
            info!("No username so score wouldn't be reported");
            return Ok(());
        };

        const SUBMIT_TS_VAR: &str = "CI_PIPELINE_CREATED_AT";
        const SUBMIT_TS_OVERRIDE: &str = "SUBMIT_TS";
        let commit_time = std::env::var(SUBMIT_TS_OVERRIDE)
            .or_else(|_| std::env::var(SUBMIT_TS_VAR))
            .with_context(|| {
                format!("neither {SUBMIT_TS_OVERRIDE} nor {SUBMIT_TS_VAR} variable was specified")
            })?;
        let commit_ts = DateTime::parse_from_rfc3339(&commit_time)
            .with_context(|| format!("failed to parse {commit_time}"))?;

        info!("Reporting score for user {user}");

        client.report_score_with_retries(
            &self.repo_config.manytask_course_name,
            &user,
            &cfg.task,
            commit_ts,
        )?;

        Ok(())
    }

    fn run_step(&self, step: &TestStep) -> Result<()> {
        use TestStep::*;

        match step {
            ForbiddenPatterns(cfg) => self.check_forbidden_patterns(cfg),
            RunCmd(cfg) => self.run_cmd(cfg),
            ReportScore(cfg) => self.report_score(cfg),
        }
    }

    fn check_forbidden_patterns_file<'a>(
        &self,
        cfg: &'a ForbiddenPatterns,
        path: &'a Path,
        text: &'a str,
    ) -> Result<Vec<Snippet<'a, Annotation<'a>>>> {
        const ERROR_CTX: usize = 1;

        let line_ends = text.match_indices('\n').map(|(i, _m)| i);
        let line_starts = iter::once(0)
            .chain(line_ends)
            .chain(iter::once(text.len()))
            .collect::<Vec<_>>();

        let line_count = line_starts.len() - 1;

        let line_index_for_byte_offset = |offset: usize| -> usize {
            match line_starts.binary_search(&offset) {
                Ok(i) => i,
                Err(i) => i - 1, // Should never be 0
            }
        };

        let mut messages = Vec::new();

        for group in cfg.groups() {
            let re = group.build_regex()?;
            let hint = group.hint.as_deref().unwrap_or("Here");

            for caps in re.captures_iter(text) {
                let target = caps
                    .iter()
                    .flatten()
                    .last()
                    .expect("regex match has at least one group");

                let span_start = target.start();
                let span_end = target.end();

                let start_line = line_index_for_byte_offset(span_start);
                let end_line = line_index_for_byte_offset(span_end - 1);

                let first_line = start_line.saturating_sub(ERROR_CTX);
                let last_line = (end_line + ERROR_CTX + 1).min(line_count);

                let snippet_start = line_starts[first_line];
                let snippet_end = line_starts[last_line];
                let snippet_str = &text[snippet_start..snippet_end];

                let local_start = span_start - snippet_start;
                let local_end = span_end - snippet_start;
                let local_span = local_start..local_end;

                let local_path = self
                    .task_context
                    .strip_task_root_logged(path)?
                    .to_str_logged()?;

                let mut snip = Snippet::source(snippet_str)
                    .line_start(first_line)
                    .path(local_path)
                    .annotation(AnnotationKind::Primary.span(local_span.clone()));
                snip = snip.annotation(AnnotationKind::Context.span(local_span).label(hint));

                messages.push(snip);
            }
        }

        Ok(messages)
    }

    fn check_forbidden_patterns(&self, cfg: &ForbiddenPatterns) -> Result<()> {
        const MAX_MESSAGES: usize = 20;

        let editable_files = self.task_context.find_editable_files()?;
        let mut messages = vec![];
        let editable_files_content = editable_files
            .iter()
            .map(|path| -> Result<_> {
                Ok((
                    path,
                    fs::read_to_string(path)
                        .with_context(|| format!("failed to read {}", path.display()))?,
                ))
            })
            .collect::<Result<Vec<_>>>()?;

        for (path, content) in &editable_files_content {
            let m = self.check_forbidden_patterns_file(cfg, path, content)?;
            messages.extend(m);
        }

        if messages.is_empty() {
            return Ok(());
        }

        let messages_count = messages.len();
        let renderer = Renderer::styled().help(renderer::AnsiColor::BrightBlue.on_default().bold());

        let mut group = Group::with_title(Level::ERROR.primary_title("Found forbidden patterns"));

        for m in messages.into_iter().take(MAX_MESSAGES) {
            group = group.element(m);
        }

        anstream::println!("{}", renderer.render(&[group]));

        if MAX_MESSAGES < messages_count {
            warn!(
                "Showing only first {MAX_MESSAGES} out of {}",
                messages_count
            );
        }

        bail!("{} forbidden patters found in your files", messages_count);
    }

    fn check_format(&self) -> Result<()> {
        if self.args.build_only {
            info!("Running in build-only mode. Formatting wouldn't be checked");
            return Ok(());
        }
        if self.repo_config.disable_fmt_check {
            info!("Not checking formatting");
            return Ok(());
        }
        let solution_files = self.task_context.find_editable_files()?;
        if solution_files.is_empty() {
            warn!("No solution files were found. Skipping formatting step");
            return Ok(());
        }

        let fmt_runner = self.repo_config.get_tool_path("clang-fmt-runner")?;

        fn check_extension(p: &Path) -> bool {
            let e = if let Some(e) = p.extension() {
                e
            } else {
                return false;
            };
            ["c", "cpp", "h", "hpp"]
                .into_iter()
                .any(|ext| OsStr::new(ext) == e)
        }

        let files_iter = solution_files.iter().filter(|p| check_extension(p));

        if files_iter.clone().count() == 0 {
            return Ok(());
        }

        ClangFmtRunner::new(Rc::clone(&self.repo_root), fmt_runner)?
            .check(&*self.cmd_runner, files_iter)
    }
}

pub fn do_test(args: TestArgs) -> Result<()> {
    TestContext::new(args)?.run()
}
