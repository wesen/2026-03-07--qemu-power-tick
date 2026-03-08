#!/usr/bin/env bash
set -euo pipefail

KERNEL=build/vmlinuz
INITRAMFS=build/initramfs-phase3.img
RESULTS_DIR=
APPEND_EXTRA=
PRE_DELAY_SECONDS=4.0
POST_RESUME_DELAY_SECONDS=1.0
PRE_NAME=00-pre
POST_NAME=01-post

while [[ $# -gt 0 ]]; do
  case "$1" in
    --kernel)
      KERNEL=$2
      shift 2
      ;;
    --initramfs)
      INITRAMFS=$2
      shift 2
      ;;
    --results-dir)
      RESULTS_DIR=$2
      shift 2
      ;;
    --append-extra)
      APPEND_EXTRA=$2
      shift 2
      ;;
    --pre-delay-seconds)
      PRE_DELAY_SECONDS=$2
      shift 2
      ;;
    --post-resume-delay-seconds)
      POST_RESUME_DELAY_SECONDS=$2
      shift 2
      ;;
    --pre-name)
      PRE_NAME=$2
      shift 2
      ;;
    --post-name)
      POST_NAME=$2
      shift 2
      ;;
    *)
      echo "unknown argument: $1" >&2
      exit 1
      ;;
  esac
done

if [[ -z "$RESULTS_DIR" || -z "$APPEND_EXTRA" ]]; then
  echo "usage: $0 --results-dir DIR --append-extra '...'" >&2
  exit 1
fi

bash guest/run-qemu-phase3.sh \
  --kernel "$KERNEL" \
  --initramfs "$INITRAMFS" \
  --results-dir "$RESULTS_DIR" \
  --append-extra "$APPEND_EXTRA" &
QEMU_PID=$!

cleanup() {
  if kill -0 "$QEMU_PID" 2>/dev/null; then
    kill "$QEMU_PID" 2>/dev/null || true
    wait "$QEMU_PID" 2>/dev/null || true
  fi
}
trap cleanup EXIT

python3 host/capture_phase3_suspend_checkpoints.py \
  --socket "$RESULTS_DIR/qmp.sock" \
  --serial-log "$RESULTS_DIR/guest-serial.log" \
  --results-dir "$RESULTS_DIR" \
  --pre-delay-seconds "$PRE_DELAY_SECONDS" \
  --post-resume-delay-seconds "$POST_RESUME_DELAY_SECONDS" \
  --pre-name "$PRE_NAME" \
  --post-name "$POST_NAME"

wait "$QEMU_PID"
trap - EXIT
