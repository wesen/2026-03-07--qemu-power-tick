---
Title: Phase 2 Wayland Client Analysis and Implementation Guide
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
Summary: Detailed phase-2 guide for a Weston-based guest Wayland stack, a custom client, a QMP harness, and measurement-driven suspend/resume validation.
LastUpdated: 2026-03-07T17:05:00-05:00
WhatFor: Explain the full phase-2 architecture, constraints, implementation plan, and validation strategy.
WhenToUse: Read this before implementing or reviewing the phase-2 guest GUI stack, host harness, and suspend/resume workflow.
---

# Phase 2 Wayland Client Analysis and Implementation Guide

## Executive Summary

Phase 2 replaces the stage-1 serial/text renderer with a real guest GUI stack. The smallest viable system that satisfies the imported brief is:

- a guest compositor: Weston,
- a guest application: one custom Wayland client,
- a host orchestration layer: QMP automation for screenshots and input injection,
- the existing stage-1 state model: network-driven updates, idle-triggered suspend, reconnect logic, and measurement logging.

The key architectural shift is that rendering is no longer just local terminal output. The guest must now boot into a compositor, expose a Wayland socket, accept keyboard and pointer events, and produce a framebuffer QEMU can capture. That changes the guest runtime and packaging strategy more than any single source file change does.

## Problem Statement

The imported lab brief asks for a phase-2 system that can:

- boot into Weston,
- launch one minimal Wayland client,
- show visible state derived from the existing network/suspend logic,
- let the host capture screenshots at named checkpoints,
- let the host inject keyboard and pointer events,
- suspend and resume while preserving continuity,
- produce measurable latency and timing data.

This is a system-integration task. The challenge is not only writing a Wayland client. The implementation must also connect:

- kernel graphics and input devices,
- guest runtime packaging,
- Weston backend requirements,
- Wayland protocol objects and shared-memory buffers,
- QEMU display/input configuration,
- QMP control commands,
- suspend/resume timing and logging.

Scope boundaries:

- In scope: Weston, one custom Wayland client, screenshots, input injection, suspend/resume continuity, measurements, diary, guide, and later final report.
- Out of scope: Chromium, browser sandboxing, kiosk mode polish, multi-application desktop management.

## Current-State Architecture

The stage-1 work already provides a strong control-plane base:

- [`guest/sleepdemo.c`](/home/manuel/code/wesen/2026-03-07--qemu-power-tick/guest/sleepdemo.c): network connection state machine, idle timer, suspend metrics, reconnect handling, and structured serial logging.
- [`guest/init`](/home/manuel/code/wesen/2026-03-07--qemu-power-tick/guest/init): minimal PID 1 bootstrap and kernel command-line parsing.
- [`guest/build-initramfs.sh`](/home/manuel/code/wesen/2026-03-07--qemu-power-tick/guest/build-initramfs.sh): stage-1 rootfs assembly path.
- [`guest/run-qemu.sh`](/home/manuel/code/wesen/2026-03-07--qemu-power-tick/guest/run-qemu.sh): current QEMU launcher and serial-monitor setup.
- [`host/drip_server.py`](/home/manuel/code/wesen/2026-03-07--qemu-power-tick/host/drip_server.py): reproducible host-side byte stream with active and pause phases.
- [`scripts/measure_run.py`](/home/manuel/code/wesen/2026-03-07--qemu-power-tick/scripts/measure_run.py): parser for stage-1 metric lines.

That means phase 2 should reuse the state machine and measurements where possible instead of rewriting behavior from scratch.

## Proposed Solution

### High-Level Structure

```text
Host
 ├── drip_server.py
 │    emits periodic network payloads
 ├── qmp_harness.py
 │    drives QMP:
 │    - screendump
 │    - send-key
 │    - input-send-event
 │    - system_wakeup
 │    - query-status / quit
 └── qemu-system-x86_64
      ├── virtio-net
      ├── virtio-gpu
      ├── keyboard + pointer devices
      ├── serial log
      └── QMP socket

Guest
 ├── /init
 │    mounts kernel filesystems
 │    prepares runtime dirs
 │    launches weston
 │    waits for WAYLAND_DISPLAY
 │    launches wl_sleepdemo
 ├── weston
 │    compositor and input dispatcher
 └── wl_sleepdemo
      ├── network state machine
      ├── suspend controller
      ├── wayland event loop integration
      ├── shm-buffer rendering
      ├── keyboard/pointer event tracking
      └── structured measurement logging
```

### Why Weston In Phase 2

Weston is not only for the later Chromium kiosk stage. It is the compositor that makes phase 2 a real Wayland system at all. It provides:

- the Wayland server your custom client connects to,
- surface management,
- keyboard and pointer event dispatch,
- final framebuffer composition for QEMU screenshot capture,
- the same base graphical platform Chromium will use in stage 3.

Without Weston or another compositor, there is no actual Wayland client environment to validate.

### Guest Packaging Strategy

Stage 1’s busybox-plus-one-binary initramfs is too small for phase 2 by default. Weston requires:

- its binary,
- backend and renderer modules,
- shared libraries,
- protocol/runtime data,
- writable runtime directories,
- a compatible graphics device path.

The preferred strategy is:

1. keep the fast initramfs boot flow,
2. assemble a richer rootfs inside that initramfs,
3. copy busybox for bootstrap and debugging,
4. stage Weston binaries, libraries, and runtime modules into the rootfs,
5. build and install one custom Wayland client,
6. keep `/init` minimal and move the real application logic into the client.

This preserves rapid iteration while still making room for the phase-2 stack.

### Guest Runtime Flow

```text
kernel
  -> /init
     -> mount /proc /sys /dev /run /tmp
     -> bring up loopback and guest network
     -> prepare XDG_RUNTIME_DIR
     -> start weston
     -> wait for wayland socket
     -> exec wl_sleepdemo
        -> connect to host drip server
        -> render status surface
        -> track inactivity
        -> enter suspend
        -> resume
        -> redraw immediately
        -> reconnect if necessary
```

### Wayland Client Responsibilities

The new client should be a purpose-built C program, derived structurally from `guest/sleepdemo.c` but with a Wayland frontend. It should:

- connect with `wl_display_connect`,
- bind `wl_compositor`, `wl_shm`, `wl_seat`, and `xdg_wm_base`,
- create one toplevel surface,
- allocate a shared-memory pixel buffer,
- render a simple dashboard with status text and event indicators,
- respond to keyboard and pointer events,
- integrate the stage-1 network and suspend logic into the same main loop,
- continue emitting `@@LOG` and `@@METRIC` lines.

Suggested visible fields:

- connection state,
- packet count,
- last packet time,
- suspend cycle count,
- last suspend time,
- last resume time,
- last key event,
- last pointer event,
- last redraw reason.

### Host Harness Responsibilities

Phase 2 should use QMP rather than an ad hoc text monitor helper for the main automation path. The harness should:

- connect to the QMP socket and enable capabilities,
- capture screenshots with `screendump`,
- inject keys with `send-key`,
- inject pointer events with `input-send-event`,
- trigger wake attempts with `system_wakeup` where applicable,
- store command traces and artifacts in the results directory.

## Design Decisions

### Decision 1: Keep The Client Small And Explicit

Using GTK, Qt, SDL, or a browser shell would hide too much of the actual protocol and redraw behavior. A small `wl_shm` client is easier to understand, instrument, and debug.

### Decision 2: Prefer CPU Rendering First

The lab does not need accelerated rendering. CPU-backed shared-memory buffers are adequate and keep the pipeline understandable:

- client paints memory,
- client attaches the buffer,
- Weston composites,
- QEMU exposes the framebuffer,
- host captures screenshots.

### Decision 3: Preserve The Stage-1 Log Format Family

Phase-1 parsing already expects `@@LOG` and `@@METRIC`. Phase 2 should extend that format rather than replace it so measurement tooling stays small and comparable across phases.

### Decision 4: Treat A Thinner Bootstrap As A Bonus Path

The requested bonus path is to make the application own more of the system directly. That should be pursued without blocking the first working Weston build.

The practical target is:

- `/init` stays only for unavoidable PID-1 and mount duties,
- Weston remains the compositor,
- the custom client owns the application logic and as much setup as is reasonable.

## API References

### Wayland Client APIs

- `wl_display_connect`
- `wl_display_get_fd`
- `wl_display_dispatch`
- `wl_display_roundtrip`
- `wl_registry_bind`
- `wl_compositor_create_surface`
- `wl_shm_create_pool`
- `wl_shm_pool_create_buffer`
- `wl_surface_attach`
- `wl_surface_damage_buffer`
- `wl_surface_commit`
- `wl_seat_get_keyboard`
- `wl_seat_get_pointer`
- `xdg_wm_base_get_xdg_surface`
- `xdg_surface_get_toplevel`

### Linux And Timing APIs

- `epoll_create1`, `epoll_ctl`, `epoll_wait`
- `timerfd_create`, `timerfd_settime`
- `clock_gettime` with `CLOCK_MONOTONIC` and `CLOCK_BOOTTIME`
- `signalfd`
- `socket`, `connect`, `recv`

### Power-Control Interfaces

- `/sys/power/state`
- `/sys/power/pm_test`
- `/sys/class/rtc/rtc0/wakealarm`

### QMP Commands

- `qmp_capabilities`
- `screendump`
- `send-key`
- `input-send-event`
- `system_wakeup`
- `query-status`
- `quit`

## Pseudocode

### `/init`

```text
mount_kernel_fs()
prepare_runtime_dirs()
bring_up_network()
launch_weston()
wait_for_wayland_socket()
export WAYLAND_DISPLAY and XDG_RUNTIME_DIR
exec wl_sleepdemo with stage-1 style flags
```

### `wl_sleepdemo`

```text
parse_args()
setup_state()
connect_wayland()
create_surface_and_buffers()
connect_network_socket()
register_fds_in_epoll():
  - network socket
  - redraw timer
  - idle timer
  - signal fd
  - wayland display fd

while running:
  if reconnect_due:
    attempt_connect()

  for event in epoll_wait():
    handle_network()
    handle_idle_timer()
    handle_redraw_timer()
    handle_signals()
    dispatch_wayland()

  if state_changed:
    redraw(reason)

  if suspend_due:
    record_suspend_start()
    enter_suspend()
    record_resume()
    redraw("resume")
```

### QMP Harness

```text
connect_qmp()
negotiate_capabilities()

checkpoint("boot")
screendump("01-boot.ppm")

checkpoint("first-packet")
screendump("02-first-packet.ppm")

send_key(...)
checkpoint("post-key")
screendump("03-post-key.ppm")

input_send_event(pointer move/click)
checkpoint("post-pointer")
screendump("04-post-pointer.ppm")
```

## File Plan

Expected code and script additions:

- [`guest/wl_sleepdemo.c`](/home/manuel/code/wesen/2026-03-07--qemu-power-tick/guest/wl_sleepdemo.c)
- [`guest/build-phase2-rootfs.sh`](/home/manuel/code/wesen/2026-03-07--qemu-power-tick/guest/build-phase2-rootfs.sh)
- [`guest/init-phase2`](/home/manuel/code/wesen/2026-03-07--qemu-power-tick/guest/init-phase2) or an updated [`guest/init`](/home/manuel/code/wesen/2026-03-07--qemu-power-tick/guest/init)
- [`guest/run-qemu-phase2.sh`](/home/manuel/code/wesen/2026-03-07--qemu-power-tick/guest/run-qemu-phase2.sh)
- [`host/qmp_harness.py`](/home/manuel/code/wesen/2026-03-07--qemu-power-tick/host/qmp_harness.py)
- ticket-local copies in [`scripts`](/home/manuel/code/wesen/2026-03-07--qemu-power-tick/ttmp/2026/03/07/QEMU-02-LAB-TWO--qemu-lab-two-wayland-client-analysis-and-implementation/scripts)

## Testing And Validation Strategy

### Boot Validation

- Weston starts successfully.
- The Wayland socket appears.
- The client connects and paints a first frame.
- `screendump` produces a non-empty image.

### Network Validation

- The host drip server accepts a connection.
- Packet counters advance.
- The client visibly redraws after packet receipt.
- Reconnect behavior remains visible after disconnection or resume.

### Input Validation

- A host-injected key event is observed and logged by the client.
- A host-injected pointer move/click is observed and logged by the client.
- Screenshots taken after these events visibly differ from earlier ones.

### Suspend / Resume Validation

- Idle timeout triggers suspend logic.
- The guest resumes through the selected wake path.
- The client redraws immediately after resume.
- The client continues operating and reconnects if the network flow was interrupted.

### Measurement Outputs

Phase-2 metrics should include at minimum:

- suspend sleep interval,
- resume-to-redraw latency,
- resume-to-reconnect latency,
- first-packet-to-redraw timing if measurable,
- input-event-to-visible-update timing if measurable from logs/checkpoints.

## Alternatives Considered

### Keep Text Mode And Fake A Graphics Phase

Rejected because it would not validate a Wayland client, Weston, or host-side graphical capture and input.

### Use A Toolkit Or Browser Shell Immediately

Rejected because it would hide the key protocol path and make debugging harder before the platform is proven.

### Skip Weston Until Chromium

Rejected because the imported phase-2 brief explicitly makes Weston part of the platform layer before Chromium.

## Risks

- Weston may require more runtime files or modules than expected in the initial rootfs assembly.
- `screendump` behavior depends on the selected QEMU graphics path and may need iteration.
- Real wake from true guest suspend may remain less reliable than `pm_test` modes, as seen in phase 1.
- Pointer injection can be sensitive to guest device choice and compositor expectations.

## Open Questions

- Which Weston backend is most reliable in this guest: DRM with virtio-gpu, fbdev, or another fallback?
- Does fully headless QEMU still expose a stable screendump path for this stack, or is a hidden graphical backend needed?
- Is a true `freeze` wake path achievable here, or should measurements focus mainly on the validated `pm_test` modes first?

## Implementation Plan

1. Finalize ticket docs, tasks, and diary so the plan is explicit.
2. Validate tool and package availability for Weston, Wayland headers, and QEMU graphics/QMP support.
3. Build the phase-2 guest image assembly path for Weston plus the custom client.
4. Implement the minimal Wayland client with `wl_shm`, `xdg-shell`, input listeners, and the stage-1 state model.
5. Update the QEMU launcher for a graphical device and QMP socket.
6. Implement the host QMP harness and checkpoint workflow.
7. Run checkpoint-driven validation, capture screenshots/logs, and parse measurements.
8. Update tasks, diary, changelog, and later the final phase-2 report.

## Bonus Point Section

The requested bonus path is to make the demo do as much as possible itself rather than depending on busybox/bootstrap helpers. That should be pursued as a controlled extension:

- move network bring-up into the client with direct syscalls or netlink usage,
- reduce shell-based readiness polling where possible,
- move shutdown decisions into the client,
- keep `/init` as thin as practical while still behaving safely as PID 1.

That bonus path should not block the first correct Weston-based implementation.

## References

- Imported brief: [../sources/local/01-lab2.md](../sources/local/01-lab2.md)
- Diary: [../reference/01-diary.md](../reference/01-diary.md)
