#!/usr/bin/env python3
import argparse
import pathlib
import subprocess
import time


REPO_ROOT = pathlib.Path(__file__).resolve().parents[1]
QMP_HARNESS = REPO_ROOT / "host" / "qmp_harness.py"


def run_qmp(socket_path: pathlib.Path, *args: str) -> None:
    subprocess.run(
        ["python3", str(QMP_HARNESS), "--socket", str(socket_path), *args],
        check=True,
    )


def screendump(socket_path: pathlib.Path, stem: pathlib.Path) -> None:
    ppm_path = stem.with_suffix(".ppm")
    png_path = stem.with_suffix(".png")
    run_qmp(socket_path, "screendump", "--file", str(ppm_path))
    subprocess.run(["convert", str(ppm_path), str(png_path)], check=True)


def wait_for_resume(serial_log: pathlib.Path, timeout: float) -> None:
    deadline = time.time() + timeout
    offset = 0

    while time.time() < deadline:
        if serial_log.exists():
            with serial_log.open("r", errors="ignore") as handle:
                handle.seek(offset)
                chunk = handle.read()
                offset = handle.tell()
            if "state=RESUMED" in chunk:
                return
        time.sleep(0.1)

    raise TimeoutError(f"timed out waiting for resume marker in {serial_log}")


def metric_ae(before: pathlib.Path, after: pathlib.Path) -> str:
    result = subprocess.run(
        ["compare", "-metric", "AE", str(before), str(after), "null:"],
        capture_output=True,
        text=True,
        check=False,
    )
    return (result.stderr or result.stdout).strip()


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Capture stage-3 suspend/resume screenshots around a running VM")
    parser.add_argument("--socket", required=True)
    parser.add_argument("--serial-log", required=True)
    parser.add_argument("--results-dir", required=True)
    parser.add_argument("--pre-delay-seconds", type=float, default=9.0)
    parser.add_argument("--post-resume-delay-seconds", type=float, default=1.0)
    parser.add_argument("--resume-timeout-seconds", type=float, default=60.0)
    parser.add_argument("--pre-name", default="00-pre-suspend")
    parser.add_argument("--post-name", default="01-post-resume")
    return parser.parse_args()


def main() -> int:
    args = parse_args()
    socket_path = pathlib.Path(args.socket)
    serial_log = pathlib.Path(args.serial_log)
    results_dir = pathlib.Path(args.results_dir)
    results_dir.mkdir(parents=True, exist_ok=True)

    deadline = time.time() + 15.0
    while time.time() < deadline:
        if socket_path.exists():
            break
        time.sleep(0.1)
    else:
        raise SystemExit(f"timed out waiting for qmp socket: {socket_path}")

    time.sleep(args.pre_delay_seconds)
    pre = results_dir / args.pre_name
    post = results_dir / args.post_name
    screendump(socket_path, pre)
    wait_for_resume(serial_log, args.resume_timeout_seconds)
    time.sleep(args.post_resume_delay_seconds)
    screendump(socket_path, post)

    print(f"resume_ae={metric_ae(pre.with_suffix('.png'), post.with_suffix('.png'))}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
