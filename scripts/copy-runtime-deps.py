#!/usr/bin/env python3
import argparse
import os
import pathlib
import shutil
import subprocess
import sys


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Copy ELF runtime dependencies into a rootfs")
    parser.add_argument("--rootfs", required=True)
    parser.add_argument(
        "--binary",
        action="append",
        default=[],
        help="ELF file to inspect with ldd; may be passed multiple times",
    )
    parser.add_argument(
        "--ld-library-path",
        default="",
        help="LD_LIBRARY_PATH value to use while resolving dependencies",
    )
    return parser.parse_args()


def parse_ldd_output(text: str) -> tuple[list[str], list[str]]:
    deps: list[str] = []
    missing: list[str] = []

    for raw_line in text.splitlines():
        line = raw_line.strip()
        if not line:
            continue
        if "=>" in line:
            left, right = line.split("=>", 1)
            right = right.strip()
            if right.startswith("not found"):
                missing.append(left.strip())
                continue
            candidate = right.split()[0]
            if candidate.startswith("/"):
                deps.append(candidate)
            continue
        if line.startswith("/"):
            deps.append(line.split()[0])

    return deps, missing


def copy_path(rootfs: pathlib.Path, source: pathlib.Path) -> None:
    if not source.is_absolute():
        raise ValueError(f"expected absolute source path, got {source}")

    dest = rootfs / source.relative_to("/")
    dest.parent.mkdir(parents=True, exist_ok=True)

    if source.is_symlink():
        target = os.readlink(source)
        if dest.exists() or dest.is_symlink():
            dest.unlink()
        dest.symlink_to(target)
        resolved = source.resolve(strict=True)
        copy_path(rootfs, resolved)
        return

    if dest.exists():
        return

    shutil.copy2(source, dest)


def main() -> int:
    args = parse_args()
    rootfs = pathlib.Path(args.rootfs).resolve()
    env = os.environ.copy()
    if args.ld_library_path:
        env["LD_LIBRARY_PATH"] = args.ld_library_path

    all_deps: set[str] = set()

    for binary in args.binary:
        completed = subprocess.run(
            ["ldd", binary],
            check=False,
            capture_output=True,
            text=True,
            env=env,
        )
        if completed.returncode != 0:
            sys.stderr.write(completed.stderr)
            sys.stderr.write(completed.stdout)
            return completed.returncode

        deps, missing = parse_ldd_output(completed.stdout)
        if missing:
            for item in missing:
                print(f"missing dependency for {binary}: {item}", file=sys.stderr)
            return 1

        all_deps.update(deps)

    for dep in sorted(all_deps):
        copy_path(rootfs, pathlib.Path(dep))

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
