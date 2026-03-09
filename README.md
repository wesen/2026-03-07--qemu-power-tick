# QEMU Sleep/Wake Lab

This repository started as the stage-1 single-process suspend/resume lab and later grew into:

- a Weston-based Wayland client stack,
- a Chromium-on-Weston kiosk path,
- a direct DRM/Ozone `content_shell` investigation,
- several follow-up investigations into suspend/resume display behavior.

The best project-level starting point is:

- [PROJECT-POSTMORTEM-AND-INTERN-GUIDE.md](./PROJECT-POSTMORTEM-AND-INTERN-GUIDE.md)

That document explains the repository layout, the architecture of each stage, what actually worked, what failed, and where a new contributor should start.

## Planned Layout

- `guest/sleepdemo.c`: guest event-loop application
- `host/drip_server.py`: host-side TCP byte source
- `guest/build-initramfs.sh`: builds the minimal initramfs guest image
- `guest/run-qemu.sh`: launches the VM with serial and monitor sockets
- `scripts/measure_run.py`: orchestrates an end-to-end measurement run

## Current Workflow

Build the guest binary and initramfs:

```sh
make build initramfs
```

Run a simple smoke test with networking but no suspend:

```sh
python3 host/drip_server.py --host 0.0.0.0 --port 5555 --interval 0.25 --active-seconds 30 --pause-seconds 2 --stop-after 12
RUNTIME_SECONDS=5 NO_SUSPEND=1 RESULTS_DIR=results guest/run-qemu.sh --kernel build/vmlinuz --initramfs build/initramfs.cpio.gz
```

Run an auto-resume suspend validation with `pm_test=freezer`:

```sh
python3 host/drip_server.py --host 0.0.0.0 --port 5555 --interval 0.25 --active-seconds 3 --pause-seconds 8 --disconnect-on-pause --stop-after 20
RUNTIME_SECONDS=15 IDLE_SECONDS=3 WAKE_SECONDS=4 PM_TEST=freezer RESULTS_DIR=results guest/run-qemu.sh --kernel build/vmlinuz --initramfs build/initramfs.cpio.gz
```

Run a reconnect-after-resume validation with `pm_test=devices`:

```sh
python3 host/drip_server.py --host 0.0.0.0 --port 5555 --interval 0.25 --active-seconds 3 --pause-seconds 8 --disconnect-on-pause --stop-after 22
RUNTIME_SECONDS=17 IDLE_SECONDS=3 RECONNECT_MS=7000 WAKE_SECONDS=4 PM_TEST=devices RESULTS_DIR=results guest/run-qemu.sh --kernel build/vmlinuz --initramfs build/initramfs.cpio.gz
```

Parse the resulting metrics:

```sh
python3 scripts/measure_run.py --serial-log results/guest-serial.log --json-out results/metrics.json
```

The implementation is being built incrementally. See the ticket diary for the exact sequence, findings, and caveats.
