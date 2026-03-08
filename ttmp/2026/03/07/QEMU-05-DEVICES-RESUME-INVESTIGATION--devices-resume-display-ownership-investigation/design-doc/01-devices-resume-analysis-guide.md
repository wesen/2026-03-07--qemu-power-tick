---
Title: Devices Resume Analysis Guide
Ticket: QEMU-05-DEVICES-RESUME-INVESTIGATION
Status: active
Topics:
    - qemu
    - linux
    - power-management
    - wayland
    - chromium
DocType: design-doc
Intent: long-term
Owners: []
RelatedFiles:
    - Path: guest/display_probe.sh
      Note: Guest-side display ownership probe sampled by the investigation
    - Path: guest/init-phase2
      Note: Phase-2 probe and fbcon-unbind wiring used in the shared-control runs
    - Path: guest/init-phase3
      Note: Phase-3 probe and fbcon-unbind wiring used in the stage-3 controls
    - Path: host/capture_phase3_suspend_checkpoints.py
      Note: Host QMP screenshot harness used for pre/post suspend captures
ExternalSources: []
Summary: ""
LastUpdated: 2026-03-07T22:12:20.785397822-05:00
WhatFor: ""
WhenToUse: ""
---


# Devices Resume Analysis Guide

## Executive Summary

This ticket is a narrow continuation of the stage-2 and stage-3 suspend/resume work. The problem is not “Chromium suspend is broken” in a vague sense. The current evidence says something more specific:

- Stage 2 and stage 3 both show a shared post-resume screenshot fallback after `pm_test=devices`.
- The fallback looks like a firmware or early-console plane in QMP screenshots.
- At least in stage 2, guest-side display ownership indicators stay stable while that fallback happens.
- Chromium and even `weston-simple-shm` can additionally trigger `virtio_gpu_dequeue_ctrl_func ... response 0x1203` errors, which appear to be a separate but related resource/scanout problem.

The goal of this guide is to let a new intern understand the whole stack quickly, reproduce the existing evidence, and continue with smaller experiments rather than starting over.

## Problem Statement

We need to determine which layer is responsible for the visible post-resume fallback after `pm_test=devices`:

- framebuffer console ownership (`vtconsole` / fbcon),
- DRM scanout or plane restoration,
- Weston output recovery,
- client buffer reuse,
- or QEMU `screendump` capturing a fallback plane that is not the one Weston intends to present.

The mistake to avoid is collapsing all of those into one “graphics resume bug.” They are different failure domains and need different experiments.

## System Overview

### Stage 1

Stage 1 is the simplest power-management harness. A minimal guest boots, runs a custom test app, enters `pm_test` suspend, wakes on RTC, reconnects to a host socket, redraws, logs metrics, and powers off.

Relevant files:
- [guest/sleepdemo.c](../../../../../../guest/sleepdemo.c)
- [guest/init](../../../../../../guest/init)
- [guest/run-qemu.sh](../../../../../../guest/run-qemu.sh)

What stage 1 proves:
- basic suspend orchestration works,
- timing metrics work,
- the core problem is not “QEMU cannot suspend at all.”

### Stage 2

Stage 2 adds a real Wayland compositor and a custom Wayland client:

```text
QEMU
  -> virtio-vga / virtio-gpu + input devices
    -> Linux DRM + fbdev emulation + vtconsole
      -> Weston DRM backend
        -> wl_sleepdemo
```

Relevant files:
- [guest/init-phase2](../../../../../../guest/init-phase2)
- [guest/wl_sleepdemo.c](../../../../../../guest/wl_sleepdemo.c)
- [guest/wl_wayland.c](../../../../../../guest/wl_wayland.c)
- [guest/wl_render.c](../../../../../../guest/wl_render.c)
- [guest/wl_suspend.c](../../../../../../guest/wl_suspend.c)
- [guest/run-qemu-phase2.sh](../../../../../../guest/run-qemu-phase2.sh)

What stage 2 proves:
- Weston can boot on DRM in the minimal guest,
- pointer and keyboard input can be injected from the host,
- suspend/resume can be measured with a real Wayland stack.

### Stage 3

Stage 3 keeps the same lower layers and swaps in Chromium:

```text
QEMU
  -> virtio-vga / virtio-gpu + input devices
    -> Linux DRM + fbdev emulation + vtconsole
      -> Weston DRM backend
        -> Chromium Wayland client
```

Relevant files:
- [guest/init-phase3](../../../../../../guest/init-phase3)
- [guest/chromium-wayland-launcher.sh](../../../../../../guest/chromium-wayland-launcher.sh)
- [guest/suspendctl.c](../../../../../../guest/suspendctl.c)
- [guest/build-phase3-rootfs.sh](../../../../../../guest/build-phase3-rootfs.sh)
- [guest/run-qemu-phase3.sh](../../../../../../guest/run-qemu-phase3.sh)

What stage 3 changes:
- a more complex buffer lifecycle,
- more client-side GPU/GBM/EGL interactions,
- more opportunities for resume-time resource invalidation.

## Why QMP Screenshots Matter

The host capture helpers use QMP and HMP facilities to obtain screenshots and inject input. For this ticket, the critical operation is the QMP/HMP screenshot path used by [host/capture_phase3_suspend_checkpoints.py](../../../../../../host/capture_phase3_suspend_checkpoints.py).

Conceptually:

```text
host helper
  -> QMP socket
    -> QEMU screendump
      -> current visible guest output plane
```

This does **not** guarantee that the screenshot corresponds to the highest-level app surface. It only shows what QEMU believes is the current visible output. That distinction matters here, because the guest may still report a stable DRM/fb state while QEMU captures what looks like a fallback text plane.

## Current Hypotheses

### Hypothesis 1: fbcon ownership fully explains the fallback

Prediction:
- if framebuffer console ownership is the main problem,
- unbinding the framebuffer `vtconsole` should preserve the graphical pre/post path.

Status:
- weakened by the corrected phase-2 unbind experiment.

### Hypothesis 2: guest ownership indicators stay stable, but visible scanout changes underneath

Prediction:
- `fb0`, `/proc/fb`, `vtconsole bind`, and DRM connector state remain stable,
- but QMP screenshots still switch planes after resume.

Status:
- strongly supported by `results-phase2-probe1`.

### Hypothesis 3: stage 3 adds an extra resource invalidation bug on top of the shared fallback

Prediction:
- stage 2 and stage 3 share the screenshot fallback,
- but stage 3 may additionally show `virtio_gpu_dequeue_ctrl_func ... response 0x1203`.

Status:
- partially supported.
- phase-3 `weston-simple-shm` also showed `0x1203` in `results-phase3-probe-shm1`, which weakens the earlier “Chromium-only” framing.

## Probe Design

The probe was designed to avoid modifying the visible runtime path.

Relevant file:
- [guest/display_probe.sh](../../../../../../guest/display_probe.sh)

Pseudo-code:

```text
loop every second:
  sample /proc/uptime
  sample /proc/fb
  sample /sys/class/graphics/fb0/name
  sample each /sys/class/vtconsole/vtcon*/{name,bind}
  sample each /sys/class/drm/card*-*/{status,enabled,dpms}
  append one @@DISPLAY line to /var/log/display_probe.log

on shutdown:
  tail display_probe.log to serial
```

Why this matters:
- it avoids writing to `/dev/console` continuously,
- it preserves an audit trail in the guest,
- it lets us compare guest ownership state to host-visible screenshots.

## Experiment Matrix

### Control A: Stage 2 probe baseline

Command:
```bash
bash guest/run-qemu-phase2.sh \
  --kernel build/vmlinuz \
  --initramfs build/initramfs-phase2.img \
  --results-dir results-phase2-probe1 \
  --append-extra 'display_probe=1 phase2_idle_seconds=5 phase2_max_suspend_cycles=1 phase2_wake_seconds=5 phase2_pm_test=devices phase2_runtime_seconds=20'
```

Observed:
- `00-pre.png`: `1280x800`, graphical plane
- `01-post.png`: `720x400`, firmware-looking text plane
- probe state stable before and after resume:
  - `fb0=virtio_gpudrmfb`
  - `vtcon0 bind=1`
  - `vtcon1 bind=0`
  - `Virtual-1 status=connected enabled=enabled dpms=On`

Interpretation:
- this is strong evidence against a simplistic “fbcon rebound” explanation.

### Control B: Stage 3 `weston-simple-shm`

Command:
```bash
bash guest/run-qemu-phase3.sh \
  --kernel build/vmlinuz \
  --initramfs build/initramfs-phase3.img \
  --results-dir results-phase3-probe-shm1 \
  --append-extra 'display_probe=1 phase3_client=weston-simple-shm phase3_runtime_seconds=35 phase3_suspend_delay_seconds=10 phase3_wake_seconds=5 phase3_pm_test=devices'
```

Observed:
- same pre/post screenshot dimension pattern
- `virtio_gpu_dequeue_ctrl_func ... response 0x1203 (command 0x105)`
- `virtio_gpu_dequeue_ctrl_func ... response 0x1203 (command 0x104)`

Interpretation:
- the extra DRM error may not be Chromium-specific after all.

### Control C: Stage 3 Chromium

Command:
```bash
bash guest/run-qemu-phase3.sh \
  --kernel build/vmlinuz \
  --initramfs build/initramfs-phase3.img \
  --results-dir results-phase3-probe-chromium1 \
  --append-extra 'display_probe=1 phase3_runtime_seconds=40 phase3_url=data:text/html;base64,... phase3_suspend_delay_seconds=10 phase3_wake_seconds=5 phase3_pm_test=devices'
```

Observed:
- same screenshot dimension pattern
- current probe run did not visibly dump `@@DISPLAY`
- truncated inspection did not show `0x1203` lines in the same obvious way

Interpretation:
- stage 3 still reproduces the shared visual fallback,
- but this particular run is weaker for fine-grained diagnosis than the phase-2 baseline.

### Intervention D: Stage 2 corrected `display_unbind_fbcon=1`

Command:
```bash
bash guest/run-qemu-phase2.sh \
  --kernel build/vmlinuz \
  --initramfs build/initramfs-phase2.img \
  --results-dir results-phase2-probe-unbind2 \
  --append-extra 'display_probe=1 display_unbind_fbcon=1 phase2_idle_seconds=5 phase2_max_suspend_cycles=1 phase2_wake_seconds=5 phase2_pm_test=devices phase2_runtime_seconds=20'
```

Observed:
- unbound the real framebuffer console:
  - `vtcon0 bind=0`
  - `vtcon1 bind=1`
- `00-pre.png` changed dramatically and showed visible graphical content
- `01-post.png` still fell back to the `720x400` firmware-looking plane

Interpretation:
- fbcon binding affects what QMP sees before suspend,
- but unbinding fbcon alone does not fix the post-resume fallback.

## What the Intern Should Conclude Right Now

Do **not** conclude:
- “Chromium is the root cause.”
- “fbcon is definitely rebinding after resume.”
- “Weston failed to resume” based only on screenshots.

Do conclude:
- there is a shared lower-layer post-resume visibility problem,
- fbcon ownership influences the visible pre-suspend plane,
- the stage-2 probe evidence points below simple `vtconsole` ownership,
- the extra `virtio_gpu` resource errors are real but are not yet isolated cleanly enough to blame on Chromium alone.

## Design Decisions

### Decision: split the investigation into a new ticket

Reason:
- the prior tickets were too broad for a new contributor to trust or continue cleanly.

### Decision: use small controls instead of restarting from scratch

Reason:
- stage 1 still reproduces cleanly,
- stage 2 and stage 3 already give useful separation,
- rewinding would destroy evidence without increasing clarity.

### Decision: prefer file-backed probes over console-tail debugging

Reason:
- log spam to `/dev/console` can perturb the graphics path being debugged.

## Alternatives Considered

### Restart stage 2 from scratch

Rejected because:
- the current evidence is already structured enough to continue,
- the problem is narrowed better by controls than by a reset.

### Keep running full Chromium tests only

Rejected because:
- Chromium adds too many moving parts,
- a shared lower-layer issue can be hidden by higher-layer noise.

### Rely only on screenshots

Rejected because:
- screenshots alone cannot distinguish ownership state from captured output state.

## Implementation Plan

### Immediate next tests

1. Re-run stage-3 `display_unbind_fbcon=1` with capture attached concurrently, not post-hoc.
2. Fix or explain the missing phase-3 `@@DISPLAY` dump.
3. Add one direct phase-3 probe-validation run whose purpose is only to prove probe output reaches serial.

### Likely follow-up experiments

1. Compare `pm_test=freezer` vs `pm_test=devices` under the same probe instrumentation.
2. Add a tiny post-resume DRM/fb state dumper that runs immediately after resume and before the main client redraw.
3. If the shared fallback remains, investigate QEMU scanout ownership or `screendump` plane selection instead of staying at the app level.

Pseudo-code for the next disciplined loop:

```text
for each hypothesis in [fbcon_ownership, drm_scanout_restore, client_resource_invalidation]:
  choose the smallest test that isolates that hypothesis
  run one control and one intervention
  capture:
    - pre screenshot
    - post screenshot
    - serial log
    - probe dump
  update ticket diary immediately
  only then choose the next hypothesis
```

## Open Questions

- Why does the stage-3 probe branch not visibly dump `@@DISPLAY` in the current serial logs?
- Are the `virtio_gpu_dequeue_ctrl_func ... 0x1203` lines in `results-phase3-probe-shm1` caused by the same lower-layer issue as the screenshot fallback, or are they a second bug?
- Is QMP `screendump` observing a different visible plane from the one Weston thinks it owns after resume?
- Is there a resume-time fbcon or DRM event occurring in a narrower window than the one-second probe interval can capture?

## References

- Ticket diary: [../reference/01-investigation-diary.md](../reference/01-investigation-diary.md)
- Ticket index: [../index.md](../index.md)
- Stage-2 ticket: [../../QEMU-02-LAB-TWO--qemu-lab-two-wayland-client-analysis-and-implementation/index.md](../../QEMU-02-LAB-TWO--qemu-lab-two-wayland-client-analysis-and-implementation/index.md)
- Stage-3 ticket: [../../QEMU-04-LAB-THREE--qemu-lab-three-chromium-kiosk-analysis-and-implementation/index.md](../../QEMU-04-LAB-THREE--qemu-lab-three-chromium-kiosk-analysis-and-implementation/index.md)

### API and subsystem references

- Linux `pm_test`: `/sys/power/pm_test`
- Linux suspend entry: `/sys/power/state`
- Linux RTC wakealarm: `/sys/class/rtc/rtc0/wakealarm`
- Linux framebuffer info: `/proc/fb`
- Linux vtconsole state: `/sys/class/vtconsole/vtcon*/`
- Linux DRM connector state: `/sys/class/drm/card*-*/`
- QMP screenshot/input harness: [host/capture_phase3_suspend_checkpoints.py](../../../../../../host/capture_phase3_suspend_checkpoints.py)
