#!/usr/bin/env python3
"""Build the pinned objdiff metadata fix; see docs/objdiff_metadata.md."""
from __future__ import annotations

import argparse
import hashlib
import os
from pathlib import Path
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
PATCH = ROOT / "tools/patches/objdiff-metadata.patch"
UPSTREAM = "https://github.com/encounter/objdiff.git"
REVISION = "784a16740a8a67b835b752ab867e76347d8a9691"  # v3.8.0
DIFF_ARGS = [
    "diff", "--no-ext-diff", "--no-textconv", "--binary", "--full-index",
    "--no-renames", "--no-color", "--src-prefix=a/", "--dst-prefix=b/",
]


def cache_dir(build_dir: Path) -> Path:
    digest = hashlib.sha256(REVISION.encode() + PATCH.read_bytes()).hexdigest()[:16]
    return build_dir / "tools" / f"objdiff-metadata-{digest}"


def binary_path(build_dir: Path) -> Path:
    name = "objdiff-cli.exe" if os.name == "nt" else "objdiff-cli"
    return cache_dir(build_dir) / "target/release" / name


def run(*args: str | Path, cwd: Path | None = None) -> None:
    subprocess.run([str(arg) for arg in args], cwd=cwd, check=True)


def build(build_dir: Path, test: bool) -> Path:
    cache = cache_dir(build_dir).resolve()
    source = cache / "source"
    cache.mkdir(parents=True, exist_ok=True)
    if not source.exists():
        # Prepare a new checkout atomically; preserve any existing experiment.
        with tempfile.TemporaryDirectory(prefix="prepare-", dir=cache) as tmp:
            checkout = Path(tmp) / "source"
            run("git", "init", "--quiet", checkout)
            run("git", "config", "core.autocrlf", "false", cwd=checkout)
            run("git", "fetch", "--quiet", "--depth", "1", UPSTREAM, REVISION, cwd=checkout)
            run("git", "checkout", "--quiet", "--detach", "FETCH_HEAD", cwd=checkout)
            run("git", "apply", "--check", PATCH, cwd=checkout)
            run("git", "apply", PATCH, cwd=checkout)
            # Include added test fixtures in the reproducibility check. This
            # changes only the private checkout's index and creates no commit.
            run("git", "add", "--intent-to-add", "--", ".", cwd=checkout)
            checkout.rename(source)
    head = subprocess.check_output(["git", "rev-parse", "HEAD"], cwd=source, text=True).strip()
    diff = subprocess.check_output(["git", *DIFF_ARGS], cwd=source)
    untracked = subprocess.check_output(
        ["git", "ls-files", "--others", "--exclude-standard"], cwd=source,
    )
    staged = subprocess.check_output(["git", "diff", "--cached", "--name-only"], cwd=source)
    if head != REVISION or diff != PATCH.read_bytes() or untracked or staged:
        raise RuntimeError(f"Modified objdiff checkout: {source}; preserve it and use another --build-dir")
    cargo_args = [
        "--locked", "--release", "--manifest-path", source / "Cargo.toml",
        "--target-dir", cache / "target",
    ]
    run("cargo", "build", *cargo_args, "-p", "objdiff-cli")
    if test:
        run("cargo", "test", *cargo_args, "-p", "objdiff-core", "--features", "all")
    return binary_path(build_dir)


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--build-dir", type=Path, default=Path("build"))
    parser.add_argument("--test", action="store_true", help="also run all objdiff core tests")
    parser.add_argument("--print-path", action="store_true", help="print the binary path without building")
    args = parser.parse_args()
    os.chdir(ROOT)
    if args.print_path:
        print(binary_path(args.build_dir))
        return
    binary = build(args.build_dir, args.test)
    print(f"Built {binary}")
    print(f'Enable: python configure.py --matching --objdiff "{binary}"')


if __name__ == "__main__":
    main()
