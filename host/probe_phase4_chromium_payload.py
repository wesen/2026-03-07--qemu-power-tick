#!/usr/bin/env python3
import argparse
import json
import pathlib
import subprocess
import sys


BASE_REQUIRED_PAYLOAD_FILES = [
    "content_shell",
    "chrome_sandbox",
    "icudtl.dat",
]

CONTENT_SHELL_OPTIONAL_ASSETS = [
    "content_shell.pak",
    "resources.pak",
    "snapshot_blob.bin",
    "v8_context_snapshot.bin",
    "chrome_100_percent.pak",
    "chrome_200_percent.pak",
]

OPTIONAL_PAYLOAD_FILES = [
    "chrome",
    "chrome_crashpad_handler",
    "nacl_helper",
    "content_shell.pak",
    "resources.pak",
    "snapshot_blob.bin",
    "v8_context_snapshot.bin",
    "chrome_100_percent.pak",
    "chrome_200_percent.pak",
    "libminigbm.so",
    "libEGL.so",
    "libGLESv2.so",
    "libvk_swiftshader.so",
    "libvulkan.so.1",
]

REQUIRED_RUNTIME_LIBS = [
    "/lib/x86_64-linux-gnu/libdrm.so.2",
    "/lib/x86_64-linux-gnu/libgbm.so.1",
    "/lib/x86_64-linux-gnu/libEGL.so.1",
    "/lib/x86_64-linux-gnu/libGLdispatch.so.0",
    "/usr/lib/x86_64-linux-gnu/libEGL_mesa.so.0",
    "/lib/x86_64-linux-gnu/libGLESv2.so.2",
    "/lib/x86_64-linux-gnu/libxkbcommon.so.0",
]

DRI_DIRS = [
    "/usr/lib/x86_64-linux-gnu/dri",
    "/lib/x86_64-linux-gnu/dri",
]

REQUIRED_GL_CONFIGS = [
    "/usr/share/glvnd/egl_vendor.d/50_mesa.json",
]


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Probe a phase-4 Chromium payload directory and required DRM runtime bits")
    parser.add_argument("--payload-dir", required=True)
    parser.add_argument("--output")
    parser.add_argument("--allow-missing-payload", action="store_true")
    return parser.parse_args()


def run_ldd(binary: pathlib.Path) -> dict[str, object]:
    completed = subprocess.run(
        ["ldd", str(binary)],
        check=False,
        capture_output=True,
        text=True,
    )
    missing = []
    for line in completed.stdout.splitlines():
        if "=> not found" in line:
            missing.append(line.strip())
    return {
        "returncode": completed.returncode,
        "missing": missing,
        "stdout": completed.stdout,
        "stderr": completed.stderr,
    }


def main() -> int:
    args = parse_args()
    payload_dir = pathlib.Path(args.payload_dir).resolve()
    payload_exists = payload_dir.exists()

    required_files = {name: (payload_dir / name).exists() for name in BASE_REQUIRED_PAYLOAD_FILES}
    content_shell_assets = {
        name: (payload_dir / name).exists() for name in CONTENT_SHELL_OPTIONAL_ASSETS
    }
    optional_files = {name: (payload_dir / name).exists() for name in OPTIONAL_PAYLOAD_FILES}
    locales_dir = payload_dir / "locales"
    locale_files = sorted(p.name for p in locales_dir.glob("*.pak")) if locales_dir.is_dir() else []

    runtime_libs = {path: pathlib.Path(path).exists() for path in REQUIRED_RUNTIME_LIBS}
    gl_configs = {path: pathlib.Path(path).exists() for path in REQUIRED_GL_CONFIGS}
    dri_dirs = {}
    for directory in DRI_DIRS:
        path = pathlib.Path(directory)
        if path.is_dir():
            dri_dirs[directory] = sorted(p.name for p in path.iterdir() if p.is_file())[:40]
        else:
            dri_dirs[directory] = None

    binaries_to_probe = []
    for name in ("content_shell", "chrome"):
        binary = payload_dir / name
        if binary.exists():
            binaries_to_probe.append(binary)

    ldd_results = {binary.name: run_ldd(binary) for binary in binaries_to_probe}

    summary = {
        "payload_dir": str(payload_dir),
        "payload_exists": payload_exists,
        "required_payload_files": required_files,
        "content_shell_assets": content_shell_assets,
        "optional_payload_files": optional_files,
        "locale_files": locale_files,
        "required_runtime_libs": runtime_libs,
        "required_gl_configs": gl_configs,
        "dri_dirs": dri_dirs,
        "ldd_results": ldd_results,
    }

    rendered = json.dumps(summary, indent=2) + "\n"
    if args.output:
        pathlib.Path(args.output).write_text(rendered, encoding="utf-8")
    sys.stdout.write(rendered)

    missing_required = [name for name, present in required_files.items() if not present]
    missing_runtime = [path for path, present in runtime_libs.items() if not present]
    missing_gl_configs = [path for path, present in gl_configs.items() if not present]
    has_shell_pack = content_shell_assets["content_shell.pak"] or content_shell_assets["resources.pak"]
    has_snapshot = (
        content_shell_assets["snapshot_blob.bin"]
        or content_shell_assets["v8_context_snapshot.bin"]
    )

    if not payload_exists and args.allow_missing_payload:
        return 0
    if missing_required or missing_runtime or missing_gl_configs or not has_shell_pack or not has_snapshot:
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
