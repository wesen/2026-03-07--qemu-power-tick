#!/usr/bin/env bash
set -euo pipefail

KERNEL=
INITRAMFS=
RESULTS_DIR=${RESULTS_DIR:-results-phase3}
SERIAL_LOG=${SERIAL_LOG:-$RESULTS_DIR/guest-serial.log}
QMP_SOCKET=${QMP_SOCKET:-$RESULTS_DIR/qmp.sock}
MEMORY_MB=${MEMORY_MB:-2048}
APPEND_EXTRA=${APPEND_EXTRA:-"phase3_runtime_seconds=25 phase3_url=about:blank"}

while [[ $# -gt 0 ]]; do
  case "$1" in
    --kernel) KERNEL=$2; shift 2 ;;
    --initramfs) INITRAMFS=$2; shift 2 ;;
    --results-dir)
      RESULTS_DIR=$2
      SERIAL_LOG=$RESULTS_DIR/guest-serial.log
      QMP_SOCKET=$RESULTS_DIR/qmp.sock
      shift 2
      ;;
    --append-extra)
      APPEND_EXTRA=$2
      shift 2
      ;;
    *)
      echo "unknown arg: $1" >&2
      exit 1
      ;;
  esac
done

if [[ -z "$KERNEL" || -z "$INITRAMFS" ]]; then
  echo "missing --kernel or --initramfs" >&2
  exit 1
fi

mkdir -p "$RESULTS_DIR"
rm -f "$SERIAL_LOG" "$QMP_SOCKET"

APPEND="console=ttyS0 panic=-1 no_console_suspend rdinit=/init"
if [[ -n "$APPEND_EXTRA" ]]; then
  APPEND="$APPEND $APPEND_EXTRA"
fi

exec qemu-system-x86_64 \
  -machine q35,accel=kvm:tcg \
  -cpu max \
  -m "$MEMORY_MB" \
  -kernel "$KERNEL" \
  -initrd "$INITRAMFS" \
  -append "$APPEND" \
  -no-reboot \
  -device virtio-vga \
  -device qemu-xhci \
  -device usb-kbd \
  -device usb-tablet \
  -nic user,model=virtio-net-pci \
  -display none \
  -serial "file:$SERIAL_LOG" \
  -qmp "unix:$QMP_SOCKET,server,wait=off"
