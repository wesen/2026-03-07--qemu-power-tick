---
Title: Direct DRM Ozone content_shell analysis and implementation guide
Ticket: QEMU-06-CONTENT-SHELL-DRM-OZONE
Status: active
Topics:
    - qemu
    - linux
    - drm
    - chromium
    - graphics
DocType: design-doc
Intent: long-term
Owners: []
RelatedFiles:
    - Path: guest/build-phase3-rootfs.sh
      Note: Current phase-3 rootfs builder used as the baseline to split out phase 4
    - Path: guest/chromium-wayland-launcher.sh
      Note: Existing Wayland launcher contrasted against the future DRM launcher
    - Path: host/bootstrap_chromium_checkout.sh
      Note: Reproducible bootstrap helper for the Chromium source/build prerequisite
    - Path: host/capture_phase4_smoke.py
      Note: First phase-4 QMP smoke capture helper for the no-Weston validation path
    - Path: guest/init-phase3
      Note: Current phase-3 runtime flow that phase 4 deliberately avoids reusing in place
    - Path: guest/kms_pattern.c
      Note: Direct KMS witness reused as the phase-4 low-level validation step
    - Path: host/qmp_harness.py
      Note: Existing QMP control interface reused for phase-4 capture and debugging
ExternalSources: []
Summary: ""
LastUpdated: 2026-03-08T12:13:43.035131716-04:00
WhatFor: ""
WhenToUse: ""
---


# Direct DRM Ozone content_shell Analysis and Implementation Guide

## Executive Summary

This ticket starts a new graphics path: Chromium `content_shell` talking directly to Linux DRM/KMS through Chromium's Ozone DRM backend, without Weston or any Wayland compositor in front of it. The motivation is not "Wayland is wrong." The motivation is that the current Weston/Wayland phase already proved Chromium-on-Wayland bring-up, and the next simpler question is whether the browser can own the DRM device directly in QEMU while keeping the rest of the lab small and inspectable.

This is a new phase, not a flag flip. The current repo has a phase-3 stack built around Weston, `seatd`, kiosk-shell, and a snap-packaged Chromium payload launched with `--ozone-platform=wayland --disable-gpu`. The new direct DRM/Ozone phase needs a different runtime contract:
- no Weston process
- no Wayland socket
- a Chromium binary that actually supports the DRM/Ozone path we want
- direct ownership of `/dev/dri/card0` and the display device
- a smaller init path that looks more like `direct KMS browser appliance` than `browser client inside compositor`

## Problem Statement

The repo currently proves several important things:
- QEMU boots a custom kernel/initramfs.
- DRM/KMS works well enough for Weston and direct KMS tests.
- Chromium can run as a Wayland client inside Weston.
- The remaining suspend/resume mismatch is now mostly in the host-visible QEMU capture path, not in guest-side rendering correctness.

However, the current working Chromium path is still deliberately layered and conservative:
- Weston owns DRM/KMS.
- Chromium is only a Wayland client.
- rendering is software-biased (`--renderer=pixman`, `--disable-gpu`).
- runtime packaging is tailored to a snap Chromium Wayland setup, not to direct DRM/GBM.

That makes it the wrong starting point for the next simplification step. If we want to answer "Can the browser itself own DRM/KMS directly in QEMU?" then the current phase-3 architecture contains too many layers that are irrelevant to that question.

## Current Baseline in This Repo

The most relevant existing files are:
- [guest/build-phase3-rootfs.sh](../../../../../../guest/build-phase3-rootfs.sh)
- [guest/init-phase3](../../../../../../guest/init-phase3)
- [guest/chromium-wayland-launcher.sh](../../../../../../guest/chromium-wayland-launcher.sh)
- [guest/kms_pattern.c](../../../../../../guest/kms_pattern.c)
- [guest/run-qemu-phase3.sh](../../../../../../guest/run-qemu-phase3.sh)
- [host/qmp_harness.py](../../../../../../host/qmp_harness.py)
- [host/capture_qmp_screendump_variants.py](../../../../../../host/capture_qmp_screendump_variants.py)
- [host/capture_qmp_state_snapshots.py](../../../../../../host/capture_qmp_state_snapshots.py)

Those files tell us the current phase-3 system is:

```text
host -> QEMU -> guest kernel/initramfs
                    -> udev + input modules
                    -> seatd
                    -> Weston DRM backend + kiosk shell
                    -> Chromium --ozone-platform=wayland --disable-gpu
```

The new ticket proposes this instead:

```text
host -> QEMU -> guest kernel/initramfs
                    -> udev + DRM/input modules
                    -> content_shell --ozone-platform=drm
```

## Imported Note Synthesis

The imported note in [drm-ozone.md](../sources/local/drm-ozone.md) points toward the right shape:
- direct Ozone DRM/GBM is a distinct Chromium platform choice, not the same thing as Wayland Ozone
- `content_shell` is the right first milestone before full Chrome
- the runtime needs real Chromium assets and graphics dependencies, not just a compositor environment
- `virtio-gpu-pci` with `-vga none` is the cleaner QEMU device choice for this experiment

The repo context agrees with that note:
- the phase-3 launcher currently hardcodes Wayland Ozone and disables GPU
- the phase-3 rootfs is built around snap Chromium and Weston runtime assets
- the bare KMS path already exists and can be reused as the "is DRM alive before the browser?" sanity check

## System Model

### Layer Map

```text
QEMU display device
  -> virtio-gpu(-pci) device emulation
    -> Linux virtio-gpu DRM driver
      -> DRM/KMS objects: connector, CRTC, plane, framebuffer
        -> GBM/EGL userspace interface
          -> Chromium Ozone DRM platform
            -> content_shell compositor/surface stack
              -> visible scanout
                -> QMP screendump / guest-side validation
```

### What Changes Relative to Phase 3

Removed from the critical path:
- Weston
- Wayland socket setup
- `seatd` as a compositor requirement
- kiosk shell
- `--ozone-platform=wayland`

Added or made stricter:
- direct Chromium DRM/Ozone launcher
- a custom rootfs layout for `content_shell`
- more exact GBM/EGL/DRI runtime packaging
- stronger reliance on direct DRM device access

### What Stays Reusable

Still reusable from the repo:
- QEMU serial/QMP harnesses
- initramfs build style
- module loading and udev bring-up patterns
- fbcon unbind logic
- direct KMS smoke testing via `kms_pattern`
- QMP capture and measurement infrastructure

## Proposed File Layout

The cleanest implementation is a new phase-4 family rather than editing phase 3 in place.

New proposed files:
- [guest/init-phase4-drm](../../../../../../guest/init-phase4-drm)
- [guest/build-phase4-rootfs.sh](../../../../../../guest/build-phase4-rootfs.sh)
- [guest/content-shell-drm-launcher.sh](../../../../../../guest/content-shell-drm-launcher.sh)
- [guest/run-qemu-phase4.sh](../../../../../../guest/run-qemu-phase4.sh)
- [guest/phase4-smoke.html](../../../../../../guest/phase4-smoke.html)

Optional later files:
- `guest/chrome-drm-launcher.sh`
- `guest/init-phase4-chrome`
- `host/capture_phase4_checkpoints.py`

## Runtime Flow

### Boot Sequence

```text
PID 1
  mount proc/sys/dev/run/tmp/debugfs
  load virtio-gpu + HID/xHCI modules
  start udev and settle devices
  confirm /dev/dri/card0 and /dev/input/event*
  optionally unbind fbcon
  set minimal Chromium env vars
  exec content-shell DRM launcher
```

### Pseudocode

```sh
mount_all_core_fs()
load_modules([
  virtio_dma_buf,
  virtio_gpu,
  hid,
  usbhid,
  usbkbd,
  usbmouse,
  xhci_pci
])

start_udev()
settle_udev()

assert_exists /dev/dri/card0
assert_exists /dev/input/event0 or event*

if fbcon_should_unbind:
  unbind_framebuffer_vtconsole()

prepare_runtime_dirs()
copy_or_select_local_test_page()

exec /usr/bin/content-shell-drm-launcher file:///root/phase4-smoke.html
```

The current implementation now also supports an earlier control mode:

```text
phase4_mode=kms-pattern
```

That mode bypasses Chromium and proves that the new phase-4 initramfs, module set, `virtio-gpu-pci` choice, and no-Weston QMP capture path are alive before the browser build lands.

## Design Decisions

### Decision: Start with `content_shell`, not full Chrome

Reason:
- it is the smallest Chromium-family browser artifact that still exercises Ozone/DRM
- it reduces user-data/profile noise
- it is the right first binary to validate before spending time on a fuller Chrome path

### Decision: Keep `virtio-gpu-pci` + `-vga none` as the default phase-4 device

Reason:
- it is the cleaner KMS-oriented QEMU device
- earlier ticket evidence already showed `virtio-vga` carries legacy VGA-facing behavior
- the new phase should start with the simpler graphics model, not the more legacy one

### Decision: Reuse the direct KMS helper before adding the browser

Reason:
- [guest/kms_pattern.c](../../../../../../guest/kms_pattern.c) already proves whether DRM/KMS is alive independently of browsers
- if phase-4 QEMU boot assumptions are wrong, `kms_pattern` will fail faster and more readably than Chromium

### Decision: Use file-backed local content first

Reason:
- the first milestone should not depend on network, DNS, certificates, or HTTP timing
- `file:///root/phase4-smoke.html` gives a deterministic visible witness

### Decision: Treat phase 4 as a new runtime branch, not a mutation of phase 3

Reason:
- phase 3 is already a valid, working Wayland stack
- mixing DRM and Wayland launch assumptions in the same init script will slow both down

## Rootfs Requirements

### Already Solved in Phase 3

- BusyBox boot environment
- initramfs assembly
- kernel module extraction from `/lib/modules/...`
- `udevadm` and udev rules packaging
- basic fontconfig and fonts
- serial logging

### New or Different for Phase 4

- a local Chromium source checkout and build toolchain (`depot_tools`, `gclient`, `gn`, `autoninja`)
- `content_shell` binary payload
- Chromium runtime assets:
  - `icudtl.dat`
  - `resources.pak`
  - `v8_context_snapshot.bin`
  - locales
- graphics runtime pieces needed by DRM/GBM/EGL path

## Current Bootstrap Status

The first real implementation slice established the Chromium bootstrap path:
- `/home/manuel/depot_tools` now exists and cloned cleanly from the official `chromium/tools/depot_tools` remote
- [host/bootstrap_chromium_checkout.sh](../../../../../../host/bootstrap_chromium_checkout.sh) provides the repeatable entry point for later sync reruns
- the first `fetch --nohooks chromium` run already created `/home/manuel/chromium/.gclient`

That means the ticket has moved past "missing local tooling" and into "long-running Chromium checkout/build execution." This distinction matters because it cleanly separates local setup bugs from remote sync latency and later build failures.

## Current Validation Status

The first local runtime validation for phase 4 is already complete even without Chromium:
- [guest/build-phase4-rootfs.sh](../../../../../../guest/build-phase4-rootfs.sh) builds a kms-only initramfs when `CHROMIUM_PAYLOAD_DIR` is unset
- [guest/init-phase4-drm](../../../../../../guest/init-phase4-drm) can boot in `phase4_mode=kms-pattern`
- [guest/run-qemu-phase4.sh](../../../../../../guest/run-qemu-phase4.sh) boots QEMU with `-vga none` and `virtio-gpu-pci`
- [host/capture_phase4_smoke.py](../../../../../../host/capture_phase4_smoke.py) captures a QMP screendump for the first visible frame

Evidence from `results-phase4-kms1`:
- guest serial log shows:
  - `@@KMSPATTERN device=/dev/dri/card0 connector_id=36 crtc_id=35 fb_id=42 width=1280 height=800 pattern=pre`
- the captured screenshot exists at:
  - `results-phase4-kms1/00-smoke.png`
- the screenshot size is:
  - `1280x800`

This matters because it proves the new phase-4 branch is already a valid no-Weston KMS/QMP environment. The remaining big dependency is the Chromium payload itself.
- launcher env vars for Ozone DRM
- no Weston packages in the minimal success path

## Testing Strategy

### Stage A: Phase-4 Boot Without Browser

Use reused direct KMS witness:
- boot the phase-4 QEMU/device configuration
- run `kms_pattern`
- verify QMP sees the expected frame

Goal:
- prove that the chosen QEMU device and init layout are correct before Chromium enters the picture

### Stage B: No-Suspend `content_shell`

Use:
- `file:///root/phase4-smoke.html`
- `virtio-gpu-pci`
- `-vga none`

Success means:
- browser starts
- visible frame appears in QMP
- no Weston anywhere in the process tree

### Stage C: Input

Only after Stage B:
- inject keyboard
- inject pointer
- prove direct browser ownership still works

### Stage D: Suspend

Only after Stage C:
- `pm_test=freezer`
- then `pm_test=devices`

This keeps the branch narrow. Do not start with suspend.

## Measurement and Artifact Plan

For each major milestone, preserve:
- QMP screenshot(s)
- guest serial log
- runtime stderr/stdout log for the browser
- quick diary step

Recommended result directories:
- `results-phase4-smoke1`
- `results-phase4-content-shell1`
- `results-phase4-input1`
- `results-phase4-suspend1`

## Risks

### Highest Risk: Binary Availability

The repo does not currently contain a known direct-DRM `content_shell` payload. On this machine, `gn` is not installed, `gclient` is not installed, and there is no existing Chromium checkout. That means the first failure may be checkout/build availability rather than runtime behavior.

### Highest Runtime Risk: Missing GBM/EGL/DRI Pieces

The phase-3 note already pointed out missing GBM-related runtime pieces. Phase 4 will be stricter here because direct DRM Chromium depends on them more directly.

### Medium Risk: Assumptions About Seat/Input Ownership

Because phase 3 used Weston + seatd, we need to verify which parts remain necessary when Chromium owns DRM/input directly as root in the guest.

### Medium Risk: Chromium Flag Drift

Direct DRM/Ozone flags are more specialized than Wayland flags. We should expect some experimentation here and capture failures verbatim in the diary.

## Alternatives Considered

### Keep Extending Phase 3

Rejected because:
- phase 3 is a compositor-based Wayland branch
- the new task is to remove that compositor layer

### Start With Full Chrome Instead of `content_shell`

Rejected because:
- it adds profile and browser-shell complexity before the basic DRM path is proven

### Start With Suspend Immediately

Rejected because:
- that would combine too many unknowns
- the first milestone must be simple no-suspend rendering

## Implementation Plan

1. Clone `depot_tools` and a Chromium source checkout.
2. Identify the smallest viable direct-DRM Chromium target set, starting with `content_shell`.
3. Create phase-4 file skeletons and keep them minimal.
4. Reuse phase-3 rootfs structure, but remove Weston from the success path.
5. Reuse `kms_pattern` as the first validation target under phase-4 QEMU/device assumptions.
6. Add a small local HTML page and a DRM launcher for `content_shell`.
7. Build or package the smallest viable direct-DRM Chromium payload.
8. Get the first stable no-suspend frame.
9. Add input only after that.
10. Add suspend only after that.

## Open Questions

- Is a locally available `content_shell` binary already present on this machine, or do we need a custom Chromium build step?
- Which Chromium checkout/build recipe will be the fastest stable path on this machine now that no build toolchain is preinstalled?
- Which exact GBM/EGL/DRI runtime pieces are missing from the current tree for direct DRM bring-up?
- Does Chromium direct DRM require any seat-management helper in this root-runner environment, or can it open devices directly as PID 1 child running as root?
- Do we need a separate phase-4 KMS-only init before the browser init, or is the existing `kms_pattern` helper enough as the low-level witness?
- Should phase 4 target only `content_shell`, or plan for a later full-Chrome DRM path from the start?

## Intern Playbook

When resuming this ticket, work in this order:
- read [drm-ozone.md](../sources/local/drm-ozone.md)
- read [01-diary.md](../reference/01-diary.md)
- inspect [guest/build-phase3-rootfs.sh](../../../../../../guest/build-phase3-rootfs.sh)
- inspect [guest/init-phase3](../../../../../../guest/init-phase3)
- inspect [guest/chromium-wayland-launcher.sh](../../../../../../guest/chromium-wayland-launcher.sh)
- implement phase-4 files as a new branch, not an edit-in-place
- always validate direct KMS assumptions before blaming Chromium

## Reference Diagram

```text
Current phase 3:
  QEMU -> Linux DRM -> Weston DRM backend -> Chromium Wayland client -> QMP capture

New phase 4:
  QEMU -> Linux DRM -> Chromium content_shell Ozone DRM -> QMP capture
```

## API and Interface References

- Linux DRM device nodes: `/dev/dri/card0`, `/dev/dri/renderD*`
- Linux power test knob: `/sys/power/pm_test`
- Linux suspend entry: `/sys/power/state`
- Linux framebuffer console binding: `/sys/class/vtconsole/vtcon*/bind`
- QMP harness: [host/qmp_harness.py](../../../../../../host/qmp_harness.py)
- Existing QMP capture helpers:
  - [host/capture_qmp_screendump_variants.py](../../../../../../host/capture_qmp_screendump_variants.py)
  - [host/capture_qmp_state_snapshots.py](../../../../../../host/capture_qmp_state_snapshots.py)
- Existing direct KMS helper: [guest/kms_pattern.c](../../../../../../guest/kms_pattern.c)

## Code Review Instructions

- Start with the imported source note in [drm-ozone.md](../sources/local/drm-ozone.md).
- Then compare the current Wayland assumptions in:
  - [guest/build-phase3-rootfs.sh](../../../../../../guest/build-phase3-rootfs.sh)
  - [guest/init-phase3](../../../../../../guest/init-phase3)
  - [guest/chromium-wayland-launcher.sh](../../../../../../guest/chromium-wayland-launcher.sh)
- Review proposed new files only after understanding why phase 3 is intentionally not reused in-place.
# Direct DRM Ozone content_shell analysis and implementation guide

## Executive Summary

<!-- Provide a high-level overview of the design proposal -->

## Problem Statement

<!-- Describe the problem this design addresses -->

## Proposed Solution

<!-- Describe the proposed solution in detail -->

## Design Decisions

<!-- Document key design decisions and rationale -->

## Alternatives Considered

<!-- List alternative approaches that were considered and why they were rejected -->

## Implementation Plan

<!-- Outline the steps to implement this design -->

## Open Questions

<!-- List any unresolved questions or concerns -->

## References

<!-- Link to related documents, RFCs, or external resources -->
