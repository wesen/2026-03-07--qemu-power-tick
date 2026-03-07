# QEMU Sleep/Wake Lab

This repository implements the stage-1 single-process suspend/resume lab described in the ticket docs.

## Planned Layout

- `guest/sleepdemo.c`: guest event-loop application
- `host/drip_server.py`: host-side TCP byte source
- `guest/build-initramfs.sh`: builds the minimal initramfs guest image
- `guest/run-qemu.sh`: launches the VM with serial and monitor sockets
- `scripts/measure_run.py`: orchestrates an end-to-end measurement run

## Current Workflow

1. `make build`
2. `make initramfs`
3. `make run-qemu`
4. `make measure`

The implementation is being built incrementally. See the ticket diary for the exact sequence and validation record.

