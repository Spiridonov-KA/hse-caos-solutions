#!/usr/bin/env python3
import os
import argparse
from dataclasses import dataclass
from enum import Enum
from pathlib import Path
from typing import Dict, List, Optional, Tuple


usermap = {"cher": "ejudge"}

uidmap = {"ejudge": 1000, "ejexec": 1002}
gidmap = {"ejudge": 1000, "ejexec": 1002}


class FileType(Enum):
    REGULAR = "-"
    FIFO = "p"
    DIR = "d"
    SYMLLINK = "l"


@dataclass
class File:
    inode: int
    type: FileType
    permissions: int
    user: str
    group: str
    size: int
    path: Path
    symlink_target: Optional[str] = None

    def __post_init__(self):
        assert self.user in ["ejudge", "ejexec"], self.user
        assert self.group in ["ejudge", "ejexec"], self.group


# Old typing
def parse_dump(dump: str) -> Tuple[List[Path], Dict[int, List[File]]]:
    dirs = dump.split("\n\n")

    dirs = dump.split('\n\n')
    dirnames = []
    for dir_ in dirs:
        dirname, total, *entries = dir_.split('\n')
        dirname = dirname[:-1]
        dirnames.append(Path(dirname))

    def parse_perms(s):
        perms = 0
        for i, c in enumerate(s):
            if c in "rwx":
                perms += 1 << (len(s) - i - 1)
            else:
                assert c == "-", c
        return perms

    files: Dict[int, List[File]] = {}

    for dir_ in dirs:
        dirname, total, *entries = dir_.split("\n")
        dirname = dirname[:-1]
        curdir = Path(dirname)
        for entry in entries:
            if not entry:
                continue
            inode, mode, nlinks, user, group, size, _m, _d, _t, *name = entry.split()
            if name[0] in [".", ".."]:
                continue
            assert len(name) in [1, 3]
            if len(name) == 3:
                assert name[1] == "->" and mode[0] == "l"

            inode = int(inode)
            files.setdefault(inode, []).append(
                File(
                    inode,
                    FileType(mode[0]),
                    parse_perms(mode[1:]),
                    usermap.get(user, user),
                    usermap.get(group, group),
                    int(size),
                    curdir / name[0],
                    name[2] if len(name) == 3 else None,
                )
            )

    return dirnames, files


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "dump_path",
        metavar="dump",
        type=Path,
    )
    parser.add_argument(
        "--preserve-ownership",
        action="store_true",
    )
    parser.add_argument(
        "--truncate-files",
        action="store_true",
    )
    return parser.parse_args()


def main():
    args = parse_args()

    with open(args.dump_path) as f:
        dirnames, files = parse_dump(f.read())

    if all(d.exists(follow_symlinks=False) for d in dirnames):
        print("Everything seems to be set up. Skipping this step")
        return

    for links in files.values():
        thefile = links[0]
        thefile.path.parent.mkdir(parents=True, exist_ok=True)
        if thefile.type is FileType.DIR:
            thefile.path.mkdir(exist_ok=True)
        elif thefile.type is FileType.FIFO:
            os.mkfifo(thefile.path)
        elif thefile.type is FileType.SYMLLINK:
            thefile.path.symlink_to(thefile.symlink_target)
        elif thefile.type is FileType.REGULAR:
            with thefile.path.open("a"):
                pass
            if args.truncate_files:
                os.truncate(thefile.path, thefile.size)
        else:
            assert False, thefile
        if thefile.type is not FileType.SYMLLINK:
            thefile.path.chmod(thefile.permissions)
        if args.preserve_ownership:
            os.chown(
                thefile.path,
                uidmap[thefile.user],
                gidmap[thefile.group],
                follow_symlinks=False,
            )
        for link in links[1:]:
            link.path.hardlink_to(thefile.path)


if __name__ == "__main__":
    main()
