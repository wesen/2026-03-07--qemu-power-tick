#!/usr/bin/env python3
import argparse
import json
import pathlib
import subprocess
import time
from typing import Any

from qmp_harness import QMPClient, wait_for_socket


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Capture default and explicit QMP screendump variants before and after resume"
    )
    parser.add_argument("--socket", required=True)
    parser.add_argument("--serial-log", required=True)
    parser.add_argument("--results-dir", required=True)
    parser.add_argument("--device-id")
    parser.add_argument("--head", type=int, default=0)
    parser.add_argument("--pre-delay-seconds", type=float, default=9.0)
    parser.add_argument("--post-resume-delay-seconds", type=float, default=1.0)
    parser.add_argument("--resume-timeout-seconds", type=float, default=60.0)
    parser.add_argument("--capture-pre-variants", action="store_true")
    return parser.parse_args()


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


def convert_if_present(ppm_path: pathlib.Path) -> None:
    if ppm_path.exists():
        subprocess.run(["convert", str(ppm_path), str(ppm_path.with_suffix(".png"))], check=True)


def image_info(path: pathlib.Path) -> dict[str, Any]:
    if not path.exists():
        return {"exists": False}
    result = subprocess.run(
        ["identify", "-format", "%w %h %[mean]", str(path)],
        check=True,
        capture_output=True,
        text=True,
    )
    width, height, mean = result.stdout.strip().split()
    return {
        "exists": True,
        "width": int(width),
        "height": int(height),
        "mean": float(mean),
    }


def screendump_variant(
    client: QMPClient,
    results_dir: pathlib.Path,
    stage: str,
    variant_name: str,
    arguments: dict[str, Any],
) -> dict[str, Any]:
    ppm_path = results_dir / f"{stage}-{variant_name}.ppm"
    png_path = ppm_path.with_suffix(".png")
    payload = {"filename": str(ppm_path), **arguments}
    response = client.execute("screendump", payload)

    record = {
        "stage": stage,
        "variant": variant_name,
        "arguments": arguments,
        "response": response,
        "ppm": str(ppm_path),
        "png": str(png_path),
    }

    if "return" in response:
        convert_if_present(ppm_path)
        record["image"] = image_info(png_path)
    return record


def build_variants(device_id: str | None, head: int) -> list[tuple[str, dict[str, Any]]]:
    variants: list[tuple[str, dict[str, Any]]] = [("default", {})]
    variants.append((f"head-{head}", {"head": head}))
    if device_id:
        variants.append((f"device-{device_id}", {"device": device_id}))
        variants.append((f"device-{device_id}-head-{head}", {"device": device_id, "head": head}))
    return variants


def main() -> int:
    args = parse_args()
    socket_path = pathlib.Path(args.socket)
    serial_log = pathlib.Path(args.serial_log)
    results_dir = pathlib.Path(args.results_dir)
    results_dir.mkdir(parents=True, exist_ok=True)

    wait_for_socket(socket_path, 15.0)
    time.sleep(args.pre_delay_seconds)

    client = QMPClient(str(socket_path))
    try:
        client.handshake()
        variants = build_variants(args.device_id, args.head)
        records: list[dict[str, Any]] = []

        if args.capture_pre_variants:
            for variant_name, variant_args in variants:
                records.append(screendump_variant(client, results_dir, "pre", variant_name, variant_args))
        else:
            records.append(screendump_variant(client, results_dir, "pre", "default", {}))

        wait_for_resume(serial_log, args.resume_timeout_seconds)
        time.sleep(args.post_resume_delay_seconds)
        for variant_name, variant_args in variants:
            records.append(screendump_variant(client, results_dir, "post", variant_name, variant_args))
    finally:
        client.close()

    summary = {
        "device_id": args.device_id,
        "head": args.head,
        "records": records,
    }
    summary_path = results_dir / "screendump-variants.json"
    summary_path.write_text(json.dumps(summary, indent=2) + "\n", encoding="utf-8")
    print(json.dumps(summary, indent=2))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
