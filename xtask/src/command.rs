use std::{
    collections::HashMap,
    env,
    ffi::{OsStr, OsString},
    io,
    path::PathBuf,
    process,
};

use serde::{Deserialize, Serialize};
use which::which;

#[derive(Debug, Default, Serialize, Deserialize, Clone, Copy)]
pub struct CommandLimits {
    pub procs: Option<usize>,
    pub memory_mb: Option<usize>,
    pub wall_time_sec: Option<usize>,
    pub cpu_ms_per_sec: Option<usize>,
}

impl CommandLimits {
    pub fn update_with(self, other: CommandLimits) -> Self {
        Self {
            procs: other.procs.or(self.procs),
            memory_mb: other.memory_mb.or(self.memory_mb),
            wall_time_sec: other.wall_time_sec.or(self.wall_time_sec),
            cpu_ms_per_sec: other.cpu_ms_per_sec.or(self.cpu_ms_per_sec),
        }
    }
}

pub struct CommandBuilder {
    pub cmd_name: OsString,
    pub args: Vec<OsString>,
    pub cwd: Option<PathBuf>,
    pub env: HashMap<OsString, OsString>,
    pub limits: CommandLimits,
    pub mounts_ro: Vec<PathBuf>,
    pub mounts_rw: Vec<PathBuf>,
}

#[allow(dead_code)]
impl CommandBuilder {
    pub fn new(cmd_name: impl Into<OsString>) -> Self {
        let cmd_name = cmd_name.into();

        const DEFAULT_RO_MOUNTS: [&str; 10] = [
            "/bin",
            "/usr/bin",
            "/usr/sbin",
            "/usr/libexec",
            "/sbin",
            "/lib",
            "/lib64",
            "/usr/lib",
            "/usr/lib64",
            "/etc",
        ];

        Self {
            cmd_name: cmd_name.clone(),
            args: vec![],
            cwd: None,
            env: HashMap::new(),
            limits: Default::default(),
            mounts_ro: DEFAULT_RO_MOUNTS.into_iter().map(PathBuf::from).collect(),
            mounts_rw: vec![],
        }
    }

    pub fn with_limits(mut self, limits: CommandLimits) -> Self {
        self.limits = limits;
        self
    }

    pub fn with_env(self, k: impl Into<OsString>, v: impl Into<OsString>) -> Self {
        self.with_envs([(k, v)])
    }

    pub fn inherit_envs<I: IntoIterator<Item = K>, K: Into<OsString>>(mut self, keys: I) -> Self {
        for k in keys {
            self = self.inherit_env(k);
        }
        self
    }

    pub fn inherit_env(mut self, k: impl Into<OsString>) -> Self {
        let key = k.into();
        if let Some(v) = env::var_os(&key) {
            self = self.with_env(key, v)
        }
        self
    }

    pub fn with_envs<I: IntoIterator<Item = (K, V)>, K: Into<OsString>, V: Into<OsString>>(
        mut self,
        vars: I,
    ) -> Self {
        self.env
            .extend(vars.into_iter().map(|(k, v)| (k.into(), v.into())));

        self
    }

    pub fn set_envs<I: IntoIterator<Item = (K, V)>, K: Into<OsString>, V: Into<OsString>>(
        mut self,
        vars: I,
    ) -> Self {
        self.env.clear();
        self.with_envs(vars)
    }

    pub fn without_env(self, k: impl AsRef<OsStr>) -> Self {
        self.without_envs([k])
    }

    pub fn without_envs<I: IntoIterator<Item = K>, K: AsRef<OsStr>>(mut self, keys: I) -> Self {
        keys.into_iter().for_each(|k| {
            self.env.remove(k.as_ref());
        });

        self
    }

    pub fn with_cwd(mut self, cwd: impl Into<PathBuf>) -> Self {
        self.cwd = Some(cwd.into());
        self
    }

    pub fn arg(self, arg: impl Into<OsString>) -> Self {
        self.args([arg])
    }

    pub fn args<I: IntoIterator<Item = K>, K: Into<OsString>>(mut self, args: I) -> Self {
        self.args.extend(args.into_iter().map(Into::into));

        self
    }

    pub fn set_args<I: IntoIterator<Item = K>, K: Into<OsString>>(mut self, args: I) -> Self {
        self.args.clear();
        self.args(args)
    }

    pub fn with_rw_mount(mut self, p: impl Into<PathBuf>) -> Self {
        self.mounts_rw.push(p.into());
        self
    }

    pub fn with_ro_mount(mut self, p: impl Into<PathBuf>) -> Self {
        self.mounts_ro.push(p.into());
        self
    }
}

pub trait CommandRunner {
    fn status(&self, cmd: &CommandBuilder) -> io::Result<process::ExitStatus>;
}

pub struct NativeCommandRunner;

impl CommandRunner for NativeCommandRunner {
    fn status(&self, cmd: &CommandBuilder) -> io::Result<process::ExitStatus> {
        let mut c = process::Command::new(&cmd.cmd_name);

        c.args(&cmd.args);
        if let Some(path) = cmd.cwd.as_ref() {
            c.current_dir(path);
        }

        c.env_clear();
        c.envs(&cmd.env);

        c.status()
    }
}

pub struct NSJailRunner(());

impl NSJailRunner {
    pub fn new() -> io::Result<Self> {
        process::Command::new("nsjail")
            .arg("--help")
            .stdout(process::Stdio::null())
            .stderr(process::Stdio::null())
            .status()?;
        Ok(Self(()))
    }
}

impl CommandRunner for NSJailRunner {
    fn status(&self, cmd: &CommandBuilder) -> io::Result<process::ExitStatus> {
        let mut c = process::Command::new("nsjail");
        c.args([
            "-Mo",
            "--use_cgroupv2",
            "--disable_clone_newnet",
            "--rlimit_as",
            "inf",
            "--rlimit_stack",
            "inf",
            // TODO: Move to config
            "--rlimit_fsize",
            "256",
            "--rlimit_nofile",
            "128",
            "--nice_level",
            "0",
            "-q",
        ]);

        for p in &cmd.mounts_ro {
            c.arg("-R").arg(p);
        }

        for p in &cmd.mounts_rw {
            c.arg("-B").arg(p);
        }

        if let Some(ref cwd) = cmd.cwd {
            c.arg("--cwd").arg(cwd);
        }

        let limits = cmd.limits;
        for (argname, value) in [
            ("--cgroup_mem_max", limits.memory_mb.map(|m| m << 20)),
            ("--time_limit", limits.wall_time_sec),
            ("--cgroup_pids_max", limits.procs),
            ("--cgroup_cpu_ms_per_sec", limits.cpu_ms_per_sec),
        ] {
            if let Some(limit) = value {
                c.args([argname, &limit.to_string()]);
            }
        }

        for (k, v) in &cmd.env {
            let mut arg = OsString::with_capacity(k.len() + 1 + v.len());
            arg.push(k);
            arg.push("=");
            arg.push(v);

            c.args([OsString::from("-E"), arg]);
        }

        c.arg("--");
        let cmd_name =
            which(&cmd.cmd_name).map_err(|e| io::Error::new(io::ErrorKind::NotFound, e))?;
        c.arg(cmd_name);
        c.args(&cmd.args);

        c.status()
    }
}
