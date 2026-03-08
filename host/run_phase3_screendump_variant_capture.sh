#!/usr/bin/env bash
set -euo pipefail

KERNEL=build/vmlinuz
INITRAMFS=build/initramfs-phase3.img
RESULTS_DIR=
APPEND_EXTRA=
PRE_DELAY_SECONDS=4.0
POST_RESUME_DELAY_SECONDS=1.0
DISPLAY_DEVICE=virtio-vga
DISPLAY_DEVICE_ID=
DISABLE_DEFAULT_VGA=0
HEAD=0
CAPTURE_PRE_VARIANTS=0

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
    --display-device)
      DISPLAY_DEVICE=$2
      shift 2
      ;;
    --display-device-id)
      DISPLAY_DEVICE_ID=$2
      shift 2
      ;;
    --disable-default-vga)
      DISABLE_DEFAULT_VGA=1
      shift
      ;;
    --head)
      HEAD=$2
      shift 2
      ;;
    --capture-pre-variants)
      CAPTURE_PRE_VARIANTS=1
      shift
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

if [[ -n "$DISPLAY_DEVICE_ID" && "$DISPLAY_DEVICE" != *",id="* ]]; then
  DISPLAY_DEVICE="${DISPLAY_DEVICE},id=${DISPLAY_DEVICE_ID}"
fi

RUN_QEMU_ARGS=(
  --kernel "$KERNEL"
  --initramfs "$INITRAMFS"
  --results-dir "$RESULTS_DIR"
  --display-device "$DISPLAY_DEVICE"
  --append-extra "$APPEND_EXTRA"
)
if [[ "$DISABLE_DEFAULT_VGA" == "1" ]]; then
  RUN_QEMU_ARGS+=(--disable-default-vga)
fi

bash guest/run-qemu-phase3.sh "${RUN_QEMU_ARGS[@]}" &
QEMU_PID=$!

cleanup() {
  if kill -0 "$QEMU_PID" 2>/dev/null; then
    kill "$QEMU_PID" 2>/dev/null || true
    wait "$QEMU_PID" 2>/dev/null || true
  fi
}
trap cleanup EXIT

CAPTURE_ARGS=(
  --socket "$RESULTS_DIR/qmp.sock"
  --serial-log "$RESULTS_DIR/guest-serial.log"
  --results-dir "$RESULTS_DIR"
  --head "$HEAD"
  --pre-delay-seconds "$PRE_DELAY_SECONDS"
  --post-resume-delay-seconds "$POST_RESUME_DELAY_SECONDS"
)
if [[ -n "$DISPLAY_DEVICE_ID" ]]; then
  CAPTURE_ARGS+=(--device-id "$DISPLAY_DEVICE_ID")
fi
if [[ "$CAPTURE_PRE_VARIANTS" == "1" ]]; then
  CAPTURE_ARGS+=(--capture-pre-variants)
fi

python3 host/capture_qmp_screendump_variants.py "${CAPTURE_ARGS[@]}"

wait "$QEMU_PID"
trap - EXIT
