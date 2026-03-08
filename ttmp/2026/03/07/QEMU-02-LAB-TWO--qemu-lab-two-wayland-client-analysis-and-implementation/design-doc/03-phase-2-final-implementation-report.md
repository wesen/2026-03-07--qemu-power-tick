---
Title: Phase 2 Final Implementation Report
Ticket: QEMU-02-LAB-TWO
Status: active
Topics:
    - qemu
    - virtualization
    - lab
    - systems
DocType: design-doc
Intent: long-term
Owners: []
RelatedFiles: []
ExternalSources:
    - local:01-lab2.md
Summary: Submission-style final report for the phase-2 Weston plus Wayland-client lab, covering architecture, implementation, debugging process, measurements, artifacts, limitations, and lessons learned.
LastUpdated: 2026-03-07T21:00:00-05:00
WhatFor: Provide the detailed phase-2 report deliverable that explains the built system, how it was implemented, what was measured, what failed, and what was learned.
WhenToUse: Read this when reviewing the finished phase-2 implementation or preparing the phase-2 submission bundle.
---

# Phase 2 Final Implementation Report

## Executive Summary

Phase 2 was completed as a Weston-based guest Wayland stack running inside QEMU with a custom client named `wl_sleepdemo`. The system boots from a hand-built initramfs, starts `systemd-udevd`, `seatd`, Weston, and then launches the client as PID 1. The host side drives the VM through QMP for screenshots and input injection, and through a drip server for network-state testing. Suspend/resume behavior from stage 1 was ported into the phase-2 client and verified with `pm_test=devices`.

The implementation is real and reproducible, not a paper design. The guest renders its own surface, receives host keyboard and pointer events, reconnects to the host network source, survives suspend/resume, logs timing metrics, and now has reusable automation for both interactive and suspend-oriented screenshot checkpoints.

## Goals And Scope

The imported brief for phase 2 required the guest GUI stack, not Chromium yet. That meant the key deliverables were:

- a real Wayland compositor in the guest,
- one minimal custom Wayland client,
- host screenshot capture,
- host input injection,
- suspend/resume continuity,
- measurement output and reportable artifacts.

Chromium remains out of scope for this phase. Weston is already required here because it is the compositor platform that makes the Wayland client, rendering path, and input path meaningful.

## System Architecture

At a high level, the system looks like this:

```text
Host
  |
  |-- QEMU ---------------------------------------------.
  |                                                     |
  |   QMP socket <---- host/qmp_harness.py              |
  |   user-net   <---- host/drip_server.py              |
  |                                                     |
  '---- Guest ------------------------------------------'
          |
          |-- /init (guest/init-phase2)
          |     mounts proc/sys/dev
          |     configures eth0
          |     loads drm/hid/usb modules
          |     starts systemd-udevd
          |     starts seatd
          |     starts weston
          |     execs /bin/wl_sleepdemo
          |
          |-- Weston (DRM backend, pixman renderer)
          |     owns /dev/dri/card0
          |     exposes wl_display + wl_seat + xdg-shell
          |
          '-- wl_sleepdemo
                Wayland surface + shm buffer
                network reconnect loop
                input listeners
                suspend state machine
                metric logging
```

## Main Components

### Guest bootstrap

The guest bootstrap is implemented by [init-phase2](/home/manuel/code/wesen/2026-03-07--qemu-power-tick/guest/init-phase2). Its responsibilities are deliberately narrow:

- mount the minimum kernel filesystems,
- bring up a simple fixed-address guest network,
- load the modules needed for DRM, USB, keyboard, and tablet devices,
- start `systemd-udevd` so Weston/libinput see real devices,
- start `seatd` so Weston can open seats and devices cleanly,
- start Weston with the DRM backend,
- parse phase-2 scenario control from `/proc/cmdline`,
- `exec` the client.

This bootstrap started out too policy-heavy, but the cleanup pass moved suspend/runtime scenario control out of `/init` and into kernel-cmdline arguments supplied by the host launcher.

### Guest compositor

Weston is packaged into the initramfs and launched with the DRM backend. The relevant platform build and configuration pieces are:

- [build-phase2-rootfs.sh](/home/manuel/code/wesen/2026-03-07--qemu-power-tick/guest/build-phase2-rootfs.sh)
- [weston.ini](/home/manuel/code/wesen/2026-03-07--qemu-power-tick/guest/weston.ini)
- [run-qemu-phase2.sh](/home/manuel/code/wesen/2026-03-07--qemu-power-tick/guest/run-qemu-phase2.sh)

The important device assumption is `virtio-vga` plus `virtio-gpu` in the guest, with Weston rendering through Pixman. That path is slower than a more optimized GPU stack, but it is stable and easy to reason about for lab automation.

### Custom Wayland client

The client started as a single large file and was later modularized. The main pieces are now:

- [wl_sleepdemo.c](/home/manuel/code/wesen/2026-03-07--qemu-power-tick/guest/wl_sleepdemo.c): orchestration and main loop
- [wl_app_core.c](/home/manuel/code/wesen/2026-03-07--qemu-power-tick/guest/wl_app_core.c): common helpers, logging, metric helpers
- [wl_wayland.c](/home/manuel/code/wesen/2026-03-07--qemu-power-tick/guest/wl_wayland.c): registry, seat, keyboard, pointer, xdg-shell wiring
- [wl_render.c](/home/manuel/code/wesen/2026-03-07--qemu-power-tick/guest/wl_render.c): shm buffer allocation and dashboard drawing
- [wl_net.c](/home/manuel/code/wesen/2026-03-07--qemu-power-tick/guest/wl_net.c): reconnect loop and packet handling
- [wl_suspend.c](/home/manuel/code/wesen/2026-03-07--qemu-power-tick/guest/wl_suspend.c): idle timer, RTC wakealarm, suspend entry, metric collection, runtime-limit shutdown

The client is intentionally simple:

- it renders one dashboard surface,
- it updates state on network/input/suspend events,
- it writes `@@LOG` and `@@METRIC` lines to the serial console,
- it does not depend on a large application stack.

### Host automation

The host-side control plane consists of:

- [qmp_harness.py](/home/manuel/code/wesen/2026-03-07--qemu-power-tick/host/qmp_harness.py): query status, query mice, screendump, send key, pointer motion, pointer click
- [drip_server.py](/home/manuel/code/wesen/2026-03-07--qemu-power-tick/host/drip_server.py): synthetic network source
- [resume_drip_server.py](/home/manuel/code/wesen/2026-03-07--qemu-power-tick/host/resume_drip_server.py): reconnect timing scenario keyed off the guest resume marker
- [capture_phase2_checkpoints.py](/home/manuel/code/wesen/2026-03-07--qemu-power-tick/host/capture_phase2_checkpoints.py): reproducible screenshot harness for interactive and suspend modes

## Build And Packaging Strategy

The biggest engineering challenge in early phase 2 was not the Wayland client code. It was building a guest image that was small, bootable, and still faithful enough to run Weston correctly.

The chosen strategy was:

1. keep the stage-1 initramfs workflow,
2. assemble a larger phase-2 rootfs by hand,
3. copy Weston, `seatd`, `udev`, XKB data, runtime libraries, and the custom client into it,
4. compress that rootfs into an initramfs,
5. boot QEMU directly with `-kernel` and `-initrd`.

That gave fast iteration and kept the lab self-contained. The cost was packaging complexity: a lot of early work went into fixing dynamic loader paths, merged-usr assumptions, XKB data, and module availability.

## Input Bring-Up

Input bring-up was one of the hardest parts of the phase. The core problem was that Weston/libinput need a complete enough userspace and device model to expose a real `wl_seat`, while QEMU input injection is easy to get slightly wrong from the host side.

The eventual working path required:

- USB keyboard and USB tablet devices in QEMU,
- `xhci` support in the guest,
- `hid`, `usbhid`, `usbkbd`, `usbmouse`, and related modules,
- `systemd-udevd` to create and tag the input devices,
- libinput-backed Weston seat creation,
- fallback keyboard/pointer binding in the client followed by rebound on real seat capability changes.

The dedicated bring-up notes live in [02-input-bring-up-playbook.md](/home/manuel/code/wesen/2026-03-07--qemu-power-tick/ttmp/2026/03/07/QEMU-02-LAB-TWO--qemu-lab-two-wayland-client-analysis-and-implementation/reference/02-input-bring-up-playbook.md).

## Suspend/Resume Implementation

Stage-1 suspend logic was ported into the modularized phase-2 client rather than pasted into the old monolith. The resulting suspend flow is:

```text
idle timer fires
  -> wl_suspend.enter_suspend_to_idle()
  -> optional /sys/power/pm_test write
  -> program /sys/class/rtc/rtc0/wakealarm
  -> write "freeze" to /sys/power/state
  -> resume
  -> compute boot-time and monotonic deltas
  -> emit @@METRIC lines
  -> redraw client
  -> restart reconnect attempts
```

The metric model reused the stage-1 conceptual split:

- `sleep_duration`: boottime delta across suspend
- `suspend_resume_gap`: monotonic delta across suspend call boundary
- `resume_to_redraw`: time from resume to first redraw
- `resume_to_reconnect`: time from resume to successful reconnect

## Measurements

### Main measured suspend/reconnect run

The cleanest reconnect-timing run remains `results-phase2-suspend6`, where a host watcher started the drip server only after the guest logged `state=RESUMED`.

Measured values:

- `sleep_duration = 5755 ms`
- `suspend_resume_gap = 5755 ms`
- `resume_to_redraw = 5 ms`
- `resume_to_reconnect = 1243 ms`

Interpretation:

- the suspend interval is stable and close to the 5-second wake target plus the expected overhead,
- redraw recovery is very fast,
- reconnect latency is dominated largely by host-orchestration and the client reconnect cadence, not by raw TCP cost.

### Cleanup validation run

After the postmortem cleanup pass, `results-phase2-cleanup-validate2` confirmed that:

- scenario control is now host-driven through kernel-cmdline parameters,
- the client still suspends and resumes correctly,
- the runtime-limit path powers off cleanly instead of panicking PID 1.

Measured values from that run:

- `sleep_duration = 5757 ms`
- `suspend_resume_gap = 5757 ms`
- `resume_to_redraw = 2 ms`

### Suspend screenshot capture run

The suspend artifact harness run `results-phase2-suspend-checkpoints1` produced:

- `05-pre-suspend.ppm`
- `06-post-resume.ppm`

and confirmed:

- `sleep_duration = 5774 ms`
- `suspend_resume_gap = 5774 ms`
- `resume_to_redraw = 10 ms`

These numbers are close to the earlier runs and support the conclusion that the phase-2 suspend path is reproducible.

## Artifact Sets

### Interactive checkpoint artifacts

The reproducible interactive screenshot set is in `results-phase2-checkpoints3/`:

- `00-boot.ppm`
- `01-first-frame.ppm`
- `02-network.ppm`
- `03-keyboard.ppm`
- `04-pointer.ppm`

The corresponding serial log proves that each interaction happened:

- `connected`
- `KEY=30 STATE=1`
- `BUTTON 272 STATE 1`

### Suspend checkpoint artifacts

The reproducible suspend screenshot set is in `results-phase2-suspend-checkpoints1/`:

- `00-boot.ppm`
- `01-first-frame.ppm`
- `05-pre-suspend.ppm`
- `06-post-resume.ppm`

This set complements the interactive screenshots by showing continuity across suspend/resume.

## What Went Well

- The diary discipline paid off. It made debugging transitions much easier and prevented the work from collapsing into half-remembered shell history.
- The decision to modularize `wl_sleepdemo.c` before adding more phase-2 behavior was correct. It reduced future change cost and made the suspend port cleaner.
- The input bring-up work was solved properly, not papered over. The end-to-end path now reaches the actual Wayland client.
- The host harnesses gradually moved the workflow from ad hoc manual experiments to reproducible scripts.

## What Was Hard

### Packaging Weston into the guest

This was more fragile than expected. The main early failures were caused by:

- dynamic loader and library-path mismatches,
- merged-usr assumptions,
- missing XKB data,
- missing DRM modules,
- missing udev/input plumbing.

These failures often surfaced as misleading symptoms such as “binary not found” even when the file existed.

### Input path debugging

Input debugging crossed multiple layers:

- QEMU device model,
- guest kernel device enumeration,
- udev tagging,
- Weston/libinput seat creation,
- Wayland seat capability delivery,
- application listener registration,
- host QMP command shape.

It took several rounds of narrowing to determine whether a failure lived in QEMU injection, kernel evdev delivery, Weston seat exposure, or client binding.

### Measurement validity

Reconnect timing was not initially a code bug. It was a scenario-design bug. Always-on host traffic prevented suspend. Delayed host startup produced meaningless reconnect numbers. The correct fix was a better host harness, not a more complicated guest state machine.

## What Went Wrong And How It Was Fixed

### Periodic redraw bias

The first client kept a periodic redraw timer. That made the client easier to observe during bring-up but biased idle behavior. The cleanup pass removed that unconditional redraw tick so later measurements better reflect real event-driven behavior.

### Policy trapped in `/init`

The initial phase-2 init script hardcoded one suspend scenario. That made experimentation awkward and conflated bootstrapping with test policy. The fix was to move scenario parameters to the kernel command line, parsed by `/init` and controlled by the host launcher.

### PID 1 exit panic

The postmortem correctly suspected the runtime-limit path was risky, and validation proved it: exiting `wl_sleepdemo` as PID 1 caused:

`Kernel panic - not syncing: Attempted to kill init!`

The fix was implemented in [wl_suspend.c](/home/manuel/code/wesen/2026-03-07--qemu-power-tick/guest/wl_suspend.c): if the client is PID 1 and hits the runtime limit, it powers off the guest instead of simply returning.

## Lessons Learned

- Bootstrapping a credible small guest image is its own engineering problem.
- “Looks modular enough for now” was false for the 882-line client file; splitting it earlier would have saved some pain.
- In these labs, the host harness is part of the system under test. Good measurements depend on good host orchestration.
- Logging the exact observable outcome you care about is more robust than waiting on inferred intermediate states.

## Remaining Limitations

- The strongest suspend baseline is still `pm_test=devices`, not a full realistic freeze/resume path with a perfect wake story.
- `resume_to_reconnect` remains partially harness-shaped because host server availability is intentionally staged around the resume event.
- `qmp_harness.py` and `capture_phase2_checkpoints.py` duplicate a small amount of QMP client code that could be shared later.
- The phase-3 Chromium work remains to be done on top of this platform.

## Recommended Next Steps

1. Treat the current phase-2 stack as the baseline for phase 3.
2. Reuse the modular guest structure and checkpoint harnesses instead of rewriting them.
3. If deeper energy-style analysis is needed, now that the forced redraw tick is gone, compare:
   - no-suspend idle,
   - no-suspend reconnect polling,
   - suspend with reconnect-after-resume.
4. For maintainability, consider extracting a small shared Python QMP helper.

## Conclusion

Phase 2 is complete enough to stand on its own. The guest Wayland stack is real, the custom client is real, the input and screenshot paths are real, suspend/resume is integrated, metrics are captured, and the workflow is now scriptable rather than dependent on manual timing and luck. The work was not smooth, but the struggles were mostly productive: each hard part forced the system into a more explicit, more reproducible shape.
