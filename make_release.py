#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""Update AcroClock versions transactionally, then verify all firmware builds.

Usage:
    python3 make_release.py 0.1.1
    python3 make_release.py 0.1.1 --skip-build
    python3 make_release.py --version
    python3 make_release.py --help

The optional repository path defaults to this script's directory. Git-tracked
C/C++ source files and linker scripts (.ld, .cmd) under bsp/inc, bsp/src, inc
and src are updated, along with the project version in requirements.md.
Scheduler files under lib, generated files, README, dates and changelogs are
not changed.

Written files are read back and compared byte-for-byte with the prepared
updates before build verification. A mismatch triggers rollback.

Backups use the suffix .release.bak. Existing backups abort the operation;
after an interrupted process they can be restored manually before retrying.

Author: Anton Chernov
Date: 2026-11-22
"""

import argparse
import re
import shutil
import subprocess
import sys
from pathlib import Path
from typing import Dict, Mapping, Optional, Pattern, Sequence, Set


SCRIPT_VERSION = "1.0.0"
BACKUP_SUFFIX = ".release.bak"
PROJECT_VERSION_PATH = Path("requirements.md")
PROJECT_SOURCE_PATHS = ("bsp/inc", "bsp/src", "inc", "src")
SOURCE_SUFFIXES: Set[str] = {
    ".c", ".h", ".cc", ".cpp", ".cxx", ".hh", ".hpp", ".ld", ".cmd"
}
HEADER_VERSION: Pattern[bytes] = re.compile(
    rb"(?m)^([ \t]*\*[ \t]+@version[ \t]+)"
    rb"[0-9]+\.[0-9]+\.[0-9]+([ \t]*\r?$)"
)
PROJECT_VERSION: Pattern[bytes] = re.compile(
    rb"(?m)^(\*\*Version:\*\*[ \t]+)"
    rb"[0-9]+\.[0-9]+\.[0-9]+([^\r\n]*\r?)$"
)


def parse_version(value: str) -> str:
    if not re.fullmatch(r"(?:0|[1-9][0-9]*)\.(?:0|[1-9][0-9]*)"
                        r"\.(?:0|[1-9][0-9]*)", value):
        raise argparse.ArgumentTypeError("Version must be MAJOR.MINOR.PATCH.")
    return value


def replace_one(
    pattern: Pattern[bytes], value: bytes, content: bytes, label: str
) -> bytes:
    updated, count = pattern.subn(
        lambda match: match[1] + value + match[2], content
    )
    if count != 1:
        raise ValueError(f"Expected exactly one {label}; found {count}.")
    return updated


def prepare_updates(root: Path, version: str) -> Dict[Path, bytes]:
    tracked = subprocess.run(
        ["git", "-C", str(root), "ls-files", "-z", "--",
         *PROJECT_SOURCE_PATHS],
        check=True, stdout=subprocess.PIPE,
    ).stdout
    updates: Dict[Path, bytes] = {}
    for entry in tracked.split(b"\0"):
        if not entry:
            continue
        relative = Path(entry.decode("utf-8"))
        if relative.suffix not in SOURCE_SUFFIXES:
            continue
        path = root / relative
        if path.is_symlink():
            raise ValueError(f"Refusing to modify a symlink: {relative}")
        original = path.read_bytes()
        header_end = original.find(b"*/")
        if header_end < 0:
            raise ValueError(f"Missing file header in {relative}.")
        header = original[:header_end + 2]
        updated_header = replace_one(
            HEADER_VERSION, version.encode("ascii"), header,
            f"@version in {relative}",
        )
        updated = updated_header + original[header_end + 2:]
        updates[path] = updated

    version_path = root / PROJECT_VERSION_PATH
    if version_path.is_symlink():
        raise ValueError(f"Refusing to modify a symlink: {PROJECT_VERSION_PATH}")
    version_content = version_path.read_bytes()
    updates[version_path] = replace_one(
        PROJECT_VERSION,
        version.encode("ascii"),
        version_content,
        f"project version in {PROJECT_VERSION_PATH}",
    )
    return updates


def backup_path(path: Path) -> Path:
    return path.with_name(path.name + BACKUP_SUFFIX)


def restore_files(backups: Sequence[Path]) -> None:
    failed = []
    for path in reversed(backups):
        try:
            backup_path(path).replace(path)
        except OSError as error:
            failed.append(path)
            print(f"Cannot restore {path}: {error}", file=sys.stderr)
    if failed:
        raise RuntimeError("Restore failed; keep remaining .release.bak files.")


def update_and_build(
    root: Path, updates: Mapping[Path, bytes], skip_build: bool,
) -> None:
    for path in updates:
        backup = backup_path(path)
        if backup.exists() or backup.is_symlink():
            raise FileExistsError(f"Existing backup must be resolved: {backup}")

    backups = []
    try:
        for path in updates:
            backup = backup_path(path)
            destination = backup.open("xb")
            try:
                with destination, path.open("rb") as source:
                    shutil.copyfileobj(source, destination)
            except BaseException:
                backup.unlink()
                raise
            backups.append(path)
            shutil.copystat(path, backup)
        for path, content in updates.items():
            path.write_bytes(content)
        for path, content in updates.items():
            if path.read_bytes() != content:
                raise ValueError(f"Written content verification failed: {path}")
        print("Written content verified for all updated files.", flush=True)
        if skip_build:
            print("Build explicitly skipped (--skip-build).", flush=True)
        else:
            for toolchain in ("GCC", "AC6"):
                for build_type in ("Debug", "Release"):
                    context = (
                        f"acroclock.{build_type}+STM32F103C8"
                    )
                    command = [
                        "cbuild",
                        "acroclock.csolution.yml",
                        "--context",
                        context,
                        "--toolchain",
                        toolchain,
                    ]
                    print(f"Build command: {command}", flush=True)
                    subprocess.run(command, cwd=root, check=True)
    except BaseException:
        restore_files(backups)
        print("Original source files restored.", file=sys.stderr)
        raise

    for path in backups:
        backup_path(path).unlink()


def main(argv: Optional[Sequence[str]] = None) -> int:
    parser = argparse.ArgumentParser(
        description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter,
    )
    parser.add_argument(
        "-V", "--version", action="version",
        version=f"%(prog)s {SCRIPT_VERSION}",
        help="Show the release script version and exit.",
    )
    parser.add_argument(
        "version", type=parse_version,
        help="New AcroClock version: MAJOR.MINOR.PATCH.",
    )
    parser.add_argument(
        "repository", nargs="?", type=Path,
        default=Path(__file__).resolve().parent,
        help="Repository path (default: this script's directory).",
    )
    parser.add_argument(
        "--skip-build", action="store_true",
        help="Update versions without build verification.",
    )
    args = parser.parse_args(argv)
    root = args.repository.resolve()
    try:
        if not (root / "acroclock.csolution.yml").is_file():
            raise ValueError(f"Missing acroclock.csolution.yml in {root}")
        updates = prepare_updates(root, args.version)
        print(f"Updating {len(updates)} files to {args.version}.", flush=True)
        update_and_build(root, updates, args.skip_build)
    except KeyboardInterrupt:
        print("Release update interrupted.", file=sys.stderr)
        return 130
    except (OSError, ValueError, RuntimeError,
            subprocess.CalledProcessError) as error:
        print(f"Release update failed: {error}", file=sys.stderr)
        return 1
    print("Version update completed; backups removed.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
