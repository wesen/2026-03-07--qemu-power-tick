#!/usr/bin/env python3
import argparse
import pathlib
import subprocess
import sys
import time

from qmp_harness import wait_for_socket


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Boot the phase-4 guest and capture one QMP screendump")
    parser.add_argument("--kernel", required=True)
    parser.add_argument("--initramfs", required=True)
    parser.add_argument("--results-dir", required=True)
    parser.add_argument("--append-extra", default="")
    parser.add_argument("--capture-delay", type=float, default=5.0)
    parser.add_argument("--timeout", type=float, default=60.0)
    return parser.parse_args()


def maybe_convert_to_png(ppm_path: pathlib.Path) -> None:
    png_path = ppm_path.with_suffix(".png")
    code = (
        "from PIL import Image\n"
        f"img = Image.open(r'{ppm_path}')\n"
        f"img.save(r'{png_path}')\n"
    )
    subprocess.run([sys.executable, "-c", code], check=False)


def main() -> int:
    args = parse_args()
    repo_root = pathlib.Path(__file__).resolve().parent.parent
    results_dir = pathlib.Path(args.results_dir).resolve()
    results_dir.mkdir(parents=True, exist_ok=True)
    qmp_socket = results_dir / "qmp.sock"

    run_cmd = [
        str(repo_root / "guest" / "run-qemu-phase4.sh"),
        "--kernel",
        str(pathlib.Path(args.kernel).resolve()),
        "--initramfs",
        str(pathlib.Path(args.initramfs).resolve()),
        "--results-dir",
        str(results_dir),
    ]
    if args.append_extra:
        run_cmd.extend(["--append-extra", args.append_extra])

    proc = subprocess.Popen(run_cmd, cwd=repo_root)
    try:
        wait_for_socket(qmp_socket, timeout=args.timeout)
        time.sleep(args.capture_delay)

        ppm_path = results_dir / "00-smoke.ppm"
        subprocess.run(
            [
                sys.executable,
                str(repo_root / "host" / "qmp_harness.py"),
                "--socket",
                str(qmp_socket),
                "screendump",
                "--file",
                str(ppm_path),
            ],
            check=True,
            cwd=repo_root,
        )
        maybe_convert_to_png(ppm_path)

        try:
            proc.wait(timeout=args.timeout)
        except subprocess.TimeoutExpired:
            proc.terminate()
            proc.wait(timeout=10)
        return 0
    finally:
        if proc.poll() is None:
            proc.terminate()
            try:
                proc.wait(timeout=10)
            except subprocess.TimeoutExpired:
                proc.kill()
                proc.wait(timeout=10)


if __name__ == "__main__":
    raise SystemExit(main())
