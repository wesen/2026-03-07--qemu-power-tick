#!/usr/bin/env bash
set -euo pipefail

KERNEL=
INITRAMFS=
RESULTS_DIR=${RESULTS_DIR:-results-phase2}
SERIAL_LOG=${SERIAL_LOG:-$RESULTS_DIR/guest-serial.log}
QMP_SOCKET=${QMP_SOCKET:-$RESULTS_DIR/qmp.sock}
MEMORY_MB=${MEMORY_MB:-1024}
APPEND_EXTRA=${APPEND_EXTRA:-"phase2_idle_seconds=5 phase2_max_suspend_cycles=1 phase2_wake_seconds=5 phase2_pm_test=devices"}
DISPLAY_DEVICE=${DISPLAY_DEVICE:-virtio-vga}
DISABLE_DEFAULT_VGA=${DISABLE_DEFAULT_VGA:-0}

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
    --display-device)
      DISPLAY_DEVICE=$2
      shift 2
      ;;
    --disable-default-vga)
      DISABLE_DEFAULT_VGA=1
      shift
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

QEMU_DISPLAY_ARGS=()
if [[ "$DISABLE_DEFAULT_VGA" == "1" ]]; then
  QEMU_DISPLAY_ARGS+=(-vga none)
fi
QEMU_DISPLAY_ARGS+=(-device "$DISPLAY_DEVICE")

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
  "${QEMU_DISPLAY_ARGS[@]}" \
  -device qemu-xhci \
  -device usb-kbd \
  -device usb-tablet \
  -nic user,model=virtio-net-pci \
  -display none \
  -serial "file:$SERIAL_LOG" \
  -qmp "unix:$QMP_SOCKET,server,wait=off"
