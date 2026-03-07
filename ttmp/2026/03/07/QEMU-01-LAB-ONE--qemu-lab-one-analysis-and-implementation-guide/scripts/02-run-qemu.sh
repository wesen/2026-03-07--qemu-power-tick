#!/usr/bin/env bash
set -euo pipefail

KERNEL=
INITRAMFS=
RESULTS_DIR=${RESULTS_DIR:-results}
MONITOR_SOCKET=${MONITOR_SOCKET:-$RESULTS_DIR/qemu-monitor.sock}
SERIAL_LOG=${SERIAL_LOG:-$RESULTS_DIR/guest-serial.log}
MEMORY_MB=${MEMORY_MB:-512}
HOST_PORT=${HOST_PORT:-5555}
IDLE_SECONDS=${IDLE_SECONDS:-5}
RECONNECT_MS=${RECONNECT_MS:-1000}
PM_TEST=${PM_TEST:-none}
MAX_SUSPEND_CYCLES=${MAX_SUSPEND_CYCLES:-1}
NO_SUSPEND=${NO_SUSPEND:-0}
RUNTIME_SECONDS=${RUNTIME_SECONDS:-0}
WAKE_SECONDS=${WAKE_SECONDS:-5}

while [[ $# -gt 0 ]]; do
  case "$1" in
    --kernel) KERNEL=$2; shift 2 ;;
    --initramfs) INITRAMFS=$2; shift 2 ;;
    --results-dir) RESULTS_DIR=$2; MONITOR_SOCKET=$RESULTS_DIR/qemu-monitor.sock; SERIAL_LOG=$RESULTS_DIR/guest-serial.log; shift 2 ;;
    *) echo "unknown arg: $1" >&2; exit 1 ;;
  esac
done

if [[ -z "$KERNEL" || -z "$INITRAMFS" ]]; then
  echo "missing --kernel or --initramfs" >&2
  exit 1
fi

mkdir -p "$RESULTS_DIR"
rm -f "$MONITOR_SOCKET" "$SERIAL_LOG"

APPEND="console=ttyS0 panic=-1 no_console_suspend rdinit=/init sleepdemo.host=10.0.2.2 sleepdemo.port=$HOST_PORT sleepdemo.idle=$IDLE_SECONDS sleepdemo.reconnect_ms=$RECONNECT_MS sleepdemo.pm_test=$PM_TEST sleepdemo.max_suspend_cycles=$MAX_SUSPEND_CYCLES sleepdemo.runtime=$RUNTIME_SECONDS sleepdemo.wake=$WAKE_SECONDS"
if [[ "$NO_SUSPEND" == "1" ]]; then
  APPEND="$APPEND sleepdemo.no_suspend=1"
fi

exec qemu-system-x86_64 \
  -machine q35,accel=kvm:tcg \
  -cpu max \
  -m "$MEMORY_MB" \
  -kernel "$KERNEL" \
  -initrd "$INITRAMFS" \
  -append "$APPEND" \
  -nographic \
  -no-reboot \
  -nic user,model=virtio-net-pci \
  -monitor "unix:$MONITOR_SOCKET,server,nowait" \
  -serial "file:$SERIAL_LOG"
