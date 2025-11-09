#!/usr/bin/env python3

import signal
import sys
from pathlib import Path
import argparse
import subprocess
import os
import shlex
import time
try:
    from functools import cache
except ImportError:
    def cache(func):
        return func
from typing import Optional, Any, cast, Union, Tuple, List
import difflib
import shutil
import resource
import ctypes
import ctypes.util
import hashlib

repo_root = Path(os.path.realpath(__file__)).parent.parent.parent
problem_dir = os.getcwd()


def get_child_pid(pid: int) -> int:
    for i in range(10):
        p = subprocess.run(['ps', '--ppid', str(pid), '-o', 'pid='], capture_output=True)
        if p.stdout.strip():
            return int(p.stdout.strip())
        time.sleep(0.1)
    raise RuntimeError(f"No child process of {pid} found")


def check_exit_code(code: int, pat: str) -> bool:
    if pat == '!0':
        return code != 0
    return str(code) == pat


class Initializer:
    def __init__(self, cmd: Optional[str], input_file: Path, correct_file: Path, inf_file: Path, env,
                 run_path: Optional[Path], run_till_end: bool):
        self.cmd = shlex.split(cmd) if cmd else None
        if self.cmd:
            self.cmd[0] = str(Path(self.cmd[0]).absolute())
        self.input_file = input_file
        self.correct_file = correct_file
        self.inf_file = inf_file
        self.env = env
        self.run_path = run_path
        self.original_path: Optional[str] = None
        self.run_till_end = run_till_end
        self.p: Optional[subprocess.Popen[str]] = None

    def __enter__(self):
        if self.run_path is not None:
            if self.original_path is not None:
                raise RuntimeError('Original path exists')
            self.original_path = os.getcwd()
            os.chdir(str(self.run_path))
        if self.cmd:
            self.p = subprocess.Popen(self.cmd + ['start', str(self.input_file.absolute()),
                                                  str(self.correct_file.absolute()), str(self.inf_file.absolute())],
                                      shell=False, env=self.env)
            if not self.run_till_end:
                self.p.communicate()
                if self.p.returncode != 0:
                    raise RuntimeError(f"Failed to run initializer start {self.p.returncode}")
            else:
                time.sleep(0.1)
        return self

    def __exit__(self, exc_type, exc_val, exc_tb):
        if self.original_path is not None:
            os.chdir(str(self.original_path))
            self.original_path = None
        if self.cmd:
            assert self.p is not None
            if not self.run_till_end:
                self.p = subprocess.Popen(self.cmd + ['stop', str(self.input_file.absolute()),
                                                      str(self.correct_file.absolute()), str(self.inf_file.absolute())],
                                          shell=False, env=self.env)
            else:
                try:
                    self.p.send_signal(signal.SIGINT)
                except Exception:
                    pass
            self.p.communicate()
            if not self.run_till_end and self.p.returncode != 0:
                print(f"WARN: Failed to run initializer stop {self.p.returncode}")


def create_run_dir(original: Path, static_copy: bool, dst: Path = Path('run')) -> Path:
    try:
        shutil.rmtree(dst)
    except Exception:
        pass
    if original.exists():
        if not static_copy:
            dst.mkdir()
            for file in os.listdir(original):
                os.symlink((original / file).absolute(), dst / file)
        else:
            shutil.copytree(original, dst)
    else:
        dst.mkdir()
    return dst.absolute()


def relative_path(run_folder: Path, path: Path) -> str:
    path = path.absolute()
    parent_cnt = 0
    while not path.is_relative_to(run_folder):
        run_folder = run_folder.parent
        parent_cnt += 1
    return '../' * parent_cnt + str(path.relative_to(run_folder))


def fix_command_path(cmd: list[str], run_path: Path, extra_params: list[str]) -> list[str]:
    full_cmd = []
    any_params = False
    for param in cmd:
        if param.startswith('./'):
            full_cmd.append(relative_path(run_path, Path(param)))
        elif param == 'params':
            full_cmd += [param.replace('\\t', '\t').replace('\\n', '\n') for param in extra_params]
            any_params = True
        else:
            full_cmd.append(param)
    if not any_params:
        full_cmd += [param.replace('\\t', '\t').replace('\\n', '\n') for param in extra_params]
    return full_cmd


def cmp_checker(res: bytes, correct: bytes):
    diff = list(difflib.diff_bytes(difflib.unified_diff, correct.split(b'\n'), res.split(b'\n')))
    if diff:
        for line in diff:
            sys.stdout.write(line.decode(errors='replace'))
            if not line.endswith(b'\n'):
                print()
        raise RuntimeError(f"Output missmatched on test {test}. Check \"output\" file")


def sorted_lines_checker(res: bytes, correct: bytes):
    diff = list(difflib.diff_bytes(difflib.unified_diff, sorted(correct.strip().split(b'\n')), sorted(res.strip().split(b'\n'))))
    if diff:
        for line in diff:
            sys.stdout.write(line.decode(errors='replace'))
            if not line.endswith(b'\n'):
                print()
        raise RuntimeError(f"Output missmatched on test {test}. Check \"output\" file")


def parse_double(data: bytes) -> float:
    res = data.decode().strip()
    if res.startswith('0x'):
        return float.fromhex(res)
    return float(res)


def get_eps() -> float:
    return float(os.environ.get('EPS', 0))


def cmp_double_checker(res: bytes, correct: bytes):
    eps = get_eps()
    res_f = parse_double(res)
    ans_f = parse_double(correct)
    if abs(res_f - ans_f) > eps:
        raise RuntimeError(f'{res_f} != {ans_f} for EPS={eps}')


def cmp_double_seq_checker(res: bytes, correct: bytes):
    res_seq = res.split()
    correct_seq = correct.split()
    if len(res_seq) != len(correct_seq):
        raise RuntimeError(f'Sequence length mismatch: {len(res_seq)} vs {len(correct_seq)}')

    eps = get_eps()
    for i, (r, c) in enumerate(zip(res_seq, correct_seq)):
        res_f = parse_double(r)
        ans_f = parse_double(c)
        if abs(res_f - ans_f) > eps:
            raise RuntimeError(f'Element #{i + 1} mismatch: {res_f} != {ans_f}, EPS={eps}')


def ignore_spaces_checker(res: bytes, correct: bytes):
    res_s = res.decode().split()
    cmp_s = correct.decode().split()
    if res_s != cmp_s:
        context = ""
        if len(res_s) <= 100 and len(cmp_s) <= 100:
            context = f"Expected: {cmp_s}, actual: {res_s}"
        msg = f"Output missmatched on test {test}.{context}"\
            " Check \"run/output\" file for more details"
        raise RuntimeError(msg)


def ignore_checker(res: bytes, correct: bytes):
    pass


def res_checker(res: bytes, ans: Path, checker: str):
    with open(ans, 'rb') as expected:
        correct = expected.read()

    replacements = {
        b"${problem.problem_dir}": problem_dir.encode(),
    }
    for a, b in replacements.items():
        correct = correct.replace(a, b)

    checkers = {
        'cmp': cmp_checker,
        'sorted-lines': sorted_lines_checker,
        'cmp-double': cmp_double_checker,
        'cmp-double-seq': cmp_double_seq_checker,
        'ignore-spaces': ignore_spaces_checker,
        'ignore': ignore_checker
    }

    checker_fn = checkers.get(checker, None)

    if checker_fn is None:
        raise RuntimeError("Unknown checker " + checker)
    checker_fn(res, correct)


def run_solution(input_file: Path, correct_file: Path, inf_file: Path, cmd: str, params: str,
                 output_file: Optional[str], env_add: Optional[dict[str, str]],
                 interactor: Union[Path, str, None], initializer: Optional[str], user: Optional[str],
                 meta: dict[str, Any], is_pipeline: bool, dirent: Path, static_copy: bool, input_filename: str,
                 checker: Union[Path, str], may_fail_local: list[str], skip_tests: bool,
                 run_initializer_till_end: bool, ans_checksum: str) -> bytes:
    input_file = input_file.absolute()
    correct_file = correct_file.absolute()
    inf_file = inf_file.absolute()

    run_path = create_run_dir(dirent, static_copy)
    params = params.replace(input_filename, relative_path(run_path, input_file))
    if meta.get('enable_subst', False):
        params = params.replace('${problem.problem_dir}', problem_dir)
    cmd = cmd.replace(input_filename, relative_path(run_path, input_file))

    cmd = cmd.replace('test_name', 'tests/' + input_file.name.removesuffix('.dat'))
    full_cmd_origin = fix_command_path(shlex.split(cmd), run_path, shlex.split(params))
    if user:
        full_cmd = ['sudo', '-E', '-u', user] + full_cmd_origin
    else:
        full_cmd = full_cmd_origin
    print(shlex.join(full_cmd), flush=True)
    env = os.environ.copy()
    if meta.get('nuke_environ', False):
        env = {}
    if env_add:
        if meta.get('enable_subst', False):
            env_add = {
                key: val.replace('${problem.problem_dir}', problem_dir) for key, val in env_add.items()
            }
        env.update(env_add)
    before_children_user = os.times().children_user
    interactor = interactor and Path(interactor).absolute()
    if '/' in str(checker):
        checker = Path(checker).absolute()
    executable = full_cmd[0]
    popen_args: dict[str, Any] = {
        'stdin': subprocess.PIPE,
        'stdout': subprocess.PIPE,
        'start_new_session': True,  # Isolate process
        'env': env,
        'executable': executable,
    }
    full_cmd[0] = Path(executable).name
    if meta.get('drop_first_param', False):
        full_cmd = full_cmd[1:]
    if (proc_count := meta.get('max_process_count')) is not None and is_pipeline:
        m = int(cast(str, proc_count))

        def preexec_fn():
            print(resource.getrlimit(resource.RLIMIT_NPROC), file=sys.stderr)
            resource.setrlimit(resource.RLIMIT_NPROC, (m, m))
            print(resource.getrlimit(resource.RLIMIT_NPROC), file=sys.stderr)
        popen_args['preexec_fn'] = preexec_fn
    if meta.get('check_stderr', False):
        print('Use stderr instead of stdout')
        popen_args['stderr'] = popen_args.pop('stdout')
    with Initializer(initializer, input_file, correct_file, inf_file, env, run_path, run_till_end=run_initializer_till_end):
        if interactor:
            p = subprocess.Popen(full_cmd, **popen_args)
            pid = p.pid
            if user:
                try:
                    pid = get_child_pid(pid)
                except Exception:
                    print("Failed to start solution", p.returncode)
                    raise
            int_cmd = [relative_path(run_path, interactor), str(input_file),
                       'output', str(correct_file),
                       str(pid), str(inf_file) if inf_file.is_file() else '']
            print(shlex.join(int_cmd), flush=True)
            interactor_env = env.copy()
            interactor_env.update(meta.get('interactor_env', {}))
            i = subprocess.Popen(int_cmd, stdin=p.stdout.fileno(),
                                 stdout=p.stderr.fileno() if meta.get('check_stderr', False) else p.stdin.fileno(),
                                 shell=False, env=interactor_env)
            p.stdout.close()
            p.stdin.close()
            p.wait()
            i.wait()
            if i.returncode != 0:
                if str(test) not in may_fail_local:
                    if os.path.isfile('output'):
                        with open('output', 'rb') as f:
                            print(f.read().decode(errors='replace'))
                        print()
                    raise RuntimeError(f'Interactor failed with code {i.returncode} on test {input_file}')
            with open('output', 'rb') as f:
                res = f.read()
        else:
            with open(input_file) as fin:
                popen_args['stdin'] = fin
                p = subprocess.Popen(full_cmd, **popen_args)
                res, err = p.communicate()
                if meta.get('check_stderr', False):
                    res = err
        if checker != 'ignore' and not check_exit_code(p.returncode, meta.get('exit_code', '0')):
            if str(test) not in may_fail_local:
                print(res)
                raise RuntimeError(f'Solution failed with code {p.returncode} on test {input_file}, expected: {meta.get("exit_code", "0")}')
            print(f'Solution failed with code {p.returncode} on test {input_file}, expected: {meta.get("exit_code", "0")}')
            print('May fail local. Skipped')
        if not skip_tests:
            if isinstance(checker, Path):
                if output_file:
                    if res:
                        raise RuntimeError('Unsupported')
                else:
                    output_file = 'fake-output.txt'
                    with open(output_file, 'wb') as f:
                        f.write(res)
                checker_cmd = [str(checker), str(inf_file), str(input_file), output_file, str(correct_file), str(p.returncode)]
                print(shlex.join(checker_cmd))
                env = os.environ.copy()
                env["DIRENT"] = str(problem_dir / dirent)
                p = subprocess.run(checker_cmd, encoding='utf-8', input='', env=env)
                if p.returncode != 0:
                    if str(test) not in may_fail_local:
                        raise RuntimeError(f"Test {test} failed")
                    print(f"Test {test} skipped")
            else:
                if output_file:
                    if res:
                        raise RuntimeError(f'Unexpected output on test {input_file}')
                    with open(output_file, 'rb') as f:
                        res = f.read()
                try:
                    res_checker(res, correct_file, checker)
                except Exception:
                    if str(test) in may_fail_local:
                        print(f"Test {test} skipped")
                    else:
                        with open('output', 'wb') as f:
                            f.write(res)
                        raise
        else:
            if output_file:
                if res:
                    raise RuntimeError(f'Unexpected output on test {input_file}')
                with open(output_file, 'rb') as f:
                    res = f.read()
        if output_file:
            os.remove(output_file)

    current_ans_checksum = calculate_checksum(correct_file)
    if current_ans_checksum != ans_checksum:
        raise RuntimeError("Test output tampering detected")
    after_children_user = os.times().children_user
    real_time_limit = meta.get('time_limit', float(os.environ.get('EJUDGE_REAL_TIME_LIMIT_MS', 1.)))
    if after_children_user - before_children_user > real_time_limit:
        if is_pipeline:
            raise RuntimeError(f'Time limit exceed: {after_children_user - before_children_user} > {real_time_limit} secs')
        else:
            print('ERROR:', f'Time limit exceed: {after_children_user - before_children_user} > {real_time_limit} secs')

    children = find_children()
    if len(children) > 0:
        limit = 5
        raise RuntimeError(
            f"Found {len(children)} orphan processes. First {min(len(children), limit)} "
            f"of them are: {children[:limit]}"
        )

    return res


def parse_inf_file(f):
    res: dict[str, Any] = {
        'time_limit': float(os.environ.get('EJUDGE_REAL_TIME_LIMIT_MS', 1.)),
    }

    def parse_env(val: str, env: dict):
        if not val or val.isspace():
            return
        if val[0] == '"':
            eq = val.find('=')
            if not eq:
                raise RuntimeError("Unsupported env " + repr(val))
            if val[-1] != '"':
                raise RuntimeError("Unsupported env " + repr(val))
            env[val[1: eq]] = val[eq + 1: -1]
            return
        values = val.split()
        for val in values:
            eq = val.find('=')
            if not eq:
                raise RuntimeError("Unsupported env " + repr(val))
            env[val[:eq]] = val[eq + 1:]

    def parse_param(key, val):
        if key == 'params':
            if key in res:
                raise RuntimeError("Duplicated params")
            res[key] = val
        elif key == 'environ' or key == 'compiler_env':
            parse_env(val, res.setdefault('environ', {}))
        elif key == 'interactor_env':
            if key not in res:
                res[key] = {}
            parse_env(val, res[key])
        elif key == 'comment':
            pass
        elif key == 'time_limit_ms' or key == 'real_time_limit_ms':
            res['time_limit'] = int(val) / 1000
        elif key == 'exit_code':
            if key in res:
                raise RuntimeError("Duplicated params")
            res[key] = val
        elif key == 'max_process_count':
            res[key] = val
        elif key == 'max_vm_size' or key == 'max_rss_size':
            pass  # ignore
        else:
            raise RuntimeError(f"Unknown inf param {key} = {val}")

    flags = {'enable_subst', 'check_stderr', 'nuke_environ', 'drop_first_param'}

    for line in f.readlines():
        line = line.strip()
        if not line:
            continue
        if ' = ' in line:
            key, val = line.split(' = ', maxsplit=1)
            val = val.strip()
            parse_param(key, val)
        elif line.endswith(' ='):
            continue
        elif line in flags:
            res[line] = True
        else:
            raise RuntimeError(f"Unknown param '{line}'")
    return res


libc = ctypes.CDLL(ctypes.util.find_library("c"), use_errno=True)
PR_SET_CHILD_SUBREAPER = 36


def set_child_subreaper():
    if libc.prctl(PR_SET_CHILD_SUBREAPER, 1, 0, 0, 0) != 0:
        e = ctypes.get_errno()
        raise OSError(e, os.strerror(e))


def read_proc(pid: int) -> Tuple[int, List[str]]:
    with open(f"/proc/{pid}/stat", "rb") as f:
        s = f.read()
    r = s.rfind(b')')
    fields = s[r + 2:].split()
    ppid = int(fields[1])

    with open(f"/proc/{pid}/cmdline", "rb") as f:
        cmd = f.read()
    cmd = list(map(bytes.decode, cmd.split(b"\0")))
    return ppid, cmd


def find_children(parent=os.getpid()) -> List[Tuple[int, List[str]]]:
    children = []
    for name in os.listdir("/proc"):
        if not name.isdigit():
            continue
        pid = int(name)
        try:
            ppid, cmd = read_proc(pid)
        except Exception:
            continue
        if ppid == parent:
            children.append((pid, cmd))
    return children


def calculate_checksum(file: Path) -> str:
    with open(file, 'rb') as f:
        return hashlib.file_digest(f, "sha256").hexdigest()


parser = argparse.ArgumentParser()
parser.add_argument('--prepare-answers', action='store_true')
parser.add_argument('--output-file', required=False)
parser.add_argument('--source-file', default='solution.*')
parser.add_argument('--run-cmd', default='./solution')
parser.add_argument('--checker', default='cmp')
parser.add_argument('--interactor', required=False)
parser.add_argument('--initializer', required=False)
parser.add_argument('--initializer-run-till-end', action='store_true')
parser.add_argument('--may-fail-local', nargs='+', default=[])
parser.add_argument('--user', required=False)
parser.add_argument('--input-filename', default='input.txt')
parser.add_argument('--static-copy', action='store_true')
parser.add_argument('--retests-count', default=1, type=int)
parser.add_argument('--retries-count', default=1, type=int)
args = parser.parse_args()

is_pipeline = bool(os.environ.get('GITLAB_CI', None))

if is_pipeline:
    args.may_fail_local = []
else:
    args.user = None

set_child_subreaper()

for cnt in range(args.retests_count):
    print(f"Trying tests #{cnt}")
    tests_cnt = 0
    tests = sorted(Path('tests').glob('*.dat'))
    checksums = list(map(lambda p: calculate_checksum(p.with_suffix('.ans')), tests))
    for test, checksum in zip(tests, checksums):
        tests_cnt += 1
        inf = test.with_suffix('.inf')
        ans = test.with_suffix('.ans')
        dirent = test.with_suffix('.dir')
        static = Path('static')
        if static.is_dir():
            if dirent.is_dir():
                raise RuntimeError("Unsupported")
            dirent = static

        meta = {}
        if inf.is_file():
            with open(inf) as f:
                meta = parse_inf_file(f)
        if not ans.is_file() and not args.prepare_answers:
            raise RuntimeError("No answer for test " + test.name)

        def run() -> bytes:
            return run_solution(test, ans, inf, args.run_cmd, meta.get('params', ''), args.output_file, meta.get('environ'),
                                args.interactor, args.initializer, args.user, meta, is_pipeline, dirent, args.static_copy,
                                args.input_filename, args.checker, args.may_fail_local, skip_tests=args.prepare_answers,
                                run_initializer_till_end=args.initializer_run_till_end, ans_checksum=checksum)

        for i in range(args.retries_count):
            try:
                res = run()
                break
            except RuntimeError as e:
                will_retry = i + 1 < args.retries_count
                will_retry_str = " not" * (not will_retry)
                print(f"Attempt {i} failed with {e},{will_retry_str} going to retry")
                if not will_retry:
                    raise

        if args.prepare_answers:
            with open(ans, 'wb') as fout:
                fout.write(res)

    if tests_cnt == 0:
        raise RuntimeError("No tests were found")

print("All tests passed")
