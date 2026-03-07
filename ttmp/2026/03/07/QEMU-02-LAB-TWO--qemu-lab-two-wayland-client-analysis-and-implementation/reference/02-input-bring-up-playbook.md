---
Title: Input Bring-Up Playbook
Ticket: QEMU-02-LAB-TWO
Status: active
Topics:
    - qemu
    - virtualization
    - lab
    - systems
DocType: reference
Intent: long-term
Owners: []
RelatedFiles: []
ExternalSources: []
Summary: ""
LastUpdated: 2026-03-07T17:58:00-05:00
WhatFor: Help the next person bring up working host-driven mouse and keyboard input for the phase-2 Weston plus Wayland-client guest without repeating the full debugging loop.
WhenToUse: Use this when QEMU screenshots work but host-driven keyboard or mouse input does not reach the Wayland client.
---

# Input Bring-Up Playbook

## Goal

Provide the shortest reliable path from “Weston is visible” to “host QMP input reaches the custom Wayland client,” including the concrete failure modes that blocked this lab and the exact fixes that resolved them.

## Context

Phase 2 uses:

- a QEMU guest launched by [run-qemu-phase2.sh](/home/manuel/code/wesen/2026-03-07--qemu-power-tick/guest/run-qemu-phase2.sh)
- a Weston compositor started by [init-phase2](/home/manuel/code/wesen/2026-03-07--qemu-power-tick/guest/init-phase2)
- a custom Wayland client in [wl_sleepdemo.c](/home/manuel/code/wesen/2026-03-07--qemu-power-tick/guest/wl_sleepdemo.c)
- a host QMP helper in [qmp_harness.py](/home/manuel/code/wesen/2026-03-07--qemu-power-tick/host/qmp_harness.py)

The main trap is that “QMP accepted my input command” does not mean the Wayland client will receive it. Input can fail at several layers:

1. guest kernel modules missing
2. udev/libinput missing, so Weston exports no `wl_seat`
3. input devices present, but client bindings are wrong or stale
4. QMP events reach evdev but never reach the focused Wayland surface
5. USB keyboard/tablet never enumerate, so keyboard injection still misses the real guest device

This playbook is ordered to eliminate those layers quickly.

## Quick Reference

### Working file set

- Guest image assembly: [build-phase2-rootfs.sh](/home/manuel/code/wesen/2026-03-07--qemu-power-tick/guest/build-phase2-rootfs.sh)
- Guest bootstrap: [init-phase2](/home/manuel/code/wesen/2026-03-07--qemu-power-tick/guest/init-phase2)
- Client: [wl_sleepdemo.c](/home/manuel/code/wesen/2026-03-07--qemu-power-tick/guest/wl_sleepdemo.c)
- Evdev probe: [evdev_probe.c](/home/manuel/code/wesen/2026-03-07--qemu-power-tick/guest/evdev_probe.c)
- Host QMP harness: [qmp_harness.py](/home/manuel/code/wesen/2026-03-07--qemu-power-tick/host/qmp_harness.py)

### Minimum working module set

Graphics:
- `virtio_dma_buf.ko`
- `virtio-gpu.ko`

Basic input:
- `hid.ko`
- `hid-generic.ko`
- `usbhid.ko`
- `psmouse.ko`

USB keyboard/tablet:
- `usbkbd.ko`
- `usbmouse.ko`
- `xhci-pci-renesas.ko`
- `xhci-pci.ko`

### Userspace pieces that are required

- `systemd-udevd` via `udevadm`
- `/usr/lib/udev/rules.d`
- `/usr/lib/udev/hwdb.bin`
- `/usr/share/libinput`
- `seatd`
- Weston

Without the udev/libinput payload, Weston may log:

```text
warning: no input devices found, but none required as per configuration.
```

and the client will not see `wl_seat`.

### Signals that each layer is healthy

Kernel/input device nodes:

```text
ls -l /dev/input
event0
event1
event2
event3
```

Weston sees devices:

```text
event1  - AT Translated Set 2 keyboard: device is a keyboard
event2  - VirtualPS/2 VMware VMMouse: device is a pointer
```

Wayland seat exists:

```text
@@LOG kind=state global name=14 interface=wl_seat version=7
@@LOG kind=state seat-capabilities=3
```

Client bindings are live:

```text
@@LOG kind=state keyboard-rebound
@@LOG kind=state pointer-rebound
```

Pointer reaches client:

```text
@@LOG kind=input BUTTON 272 STATE 1
@@LOG kind=input BUTTON 272 STATE 0
```

Keyboard reaches client:

```text
@@LOG kind=input KEY=30 STATE=1
@@LOG kind=input KEY=30 STATE=0
```

### Fastest diagnosis order

1. Confirm QMP works at all:

```bash
host/qmp_harness.py --socket results-phase2-client12/qmp.sock query-status
host/qmp_harness.py --socket results-phase2-client12/qmp.sock screendump --file results-phase2-client12/check.ppm
```

2. Confirm Weston has input devices:

```bash
rg -n 'device is a keyboard|device is a pointer|warning: no input devices found' results-phase2-client12/guest-serial.log
```

3. Confirm `wl_seat` exists:

```bash
rg -n 'interface=wl_seat|seat-capabilities' results-phase2-client12/guest-serial.log
```

4. Confirm raw evdev receives pointer input:

```bash
host/qmp_harness.py --socket results-phase2-client12/qmp.sock pointer-move-normalized --x 0.5 --y 0.5
host/qmp_harness.py --socket results-phase2-client12/qmp.sock pointer-button --button left --down
host/qmp_harness.py --socket results-phase2-client12/qmp.sock pointer-button --button left
rg -n '@@EVDEV|BUTTON 272' results-phase2-client12/guest-serial.log
```

5. Confirm keyboard reaches the client:

```bash
host/qmp_harness.py --socket results-phase2-client12/qmp.sock send-key --key a
rg -n 'KEY=30|keyboard-rebound|seat-capabilities' results-phase2-client12/guest-serial.log
```

### Critical lessons

Do not assume these are equivalent:

- `input-send-event` returned `{ "return": {} }`
- `/dev/input/event*` saw the event
- Weston/libinput processed the event
- the Wayland client received the event

Treat those as four separate checkpoints.

### Common failure modes and fixes

No `wl_seat` in the client registry:
- Cause: missing udev/libinput userspace in the initramfs.
- Fix: copy `udevadm`, udev rules, `hwdb.bin`, and `/usr/share/libinput`; start `systemd-udevd`; run `udevadm trigger` before Weston.

Pointer reaches evdev but not the client:
- Cause: fallback keyboard/pointer objects were bound before the real seat capabilities arrived.
- Fix: in [wl_sleepdemo.c](/home/manuel/code/wesen/2026-03-07--qemu-power-tick/guest/wl_sleepdemo.c), rebind keyboard/pointer on the real `seat-capabilities` callback. The important log markers are `keyboard-rebound` and `pointer-rebound`.

USB keyboard/tablet never enumerate:
- Cause: `xhci-pci.ko` loaded without `xhci-pci-renesas.ko`.
- Fix: ship and load both modules, with `xhci-pci-renesas.ko` first.

Keyboard still missing while pointer works:
- Cause in this lab: the guest only had the AT keyboard until the USB host path was fixed.
- Fix: make sure the log contains `QEMU USB Keyboard` before trusting keyboard tests.

### Suggested bootstrap sequence

This is the sequence that worked:

```text
1. mount proc/sys/dev/run/tmp
2. bring up networking
3. load DRM/input/USB modules
4. start systemd-udevd
5. trigger input + drm uevents
6. wait for settle
7. start evdev probe
8. start seatd
9. start Weston
10. wait for Wayland socket
11. launch wl_sleepdemo
```

If you move Weston earlier than udev settling, you are likely to recreate the “no input devices found” failure.

## Usage Examples

### Example: full rebuild and boot

```bash
bash guest/build-evdev-probe.sh build build/evdev_probe
guest/build-wl-sleepdemo.sh build build/wl_sleepdemo
guest/build-phase2-rootfs.sh build build/initramfs-phase2-client.cpio.gz build/wl_sleepdemo build/evdev_probe
guest/run-qemu-phase2.sh --kernel build/vmlinuz --initramfs build/initramfs-phase2-client.cpio.gz --results-dir results-phase2-client12
```

### Example: prove pointer works end-to-end

```bash
host/qmp_harness.py --socket results-phase2-client12/qmp.sock pointer-move-normalized --x 0.5 --y 0.5
host/qmp_harness.py --socket results-phase2-client12/qmp.sock pointer-button --button left --down
host/qmp_harness.py --socket results-phase2-client12/qmp.sock pointer-button --button left
rg -n '@@EVDEV|BUTTON 272|pointer-rebound|seat-capabilities' results-phase2-client12/guest-serial.log
```

### Example: prove keyboard works end-to-end

```bash
host/qmp_harness.py --socket results-phase2-client12/qmp.sock send-key --key a
rg -n 'QEMU USB Keyboard|KEY=30|keyboard-rebound' results-phase2-client12/guest-serial.log
```

## Related

- [01-phase-2-wayland-client-analysis-and-implementation-guide.md](../design-doc/01-phase-2-wayland-client-analysis-and-implementation-guide.md)
- [01-diary.md](./01-diary.md)
- [index.md](../index.md)
