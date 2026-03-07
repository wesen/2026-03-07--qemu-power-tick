---
Title: Diary
Ticket: QEMU-03-PHASE2-CLEANUP
Status: active
Topics:
    - qemu
    - virtualization
    - lab
    - systems
    - refactor
DocType: reference
Intent: long-term
Owners: []
RelatedFiles:
    - Path: guest/wl_app_core.c
      Note: Shared core helper extraction recorded in Step 2
    - Path: guest/wl_net.c
      Note: Network extraction validated in Step 2
    - Path: guest/wl_render.c
      Note: Rendering extraction validated in Step 2
    - Path: guest/wl_sleepdemo.c
      Note: Main orchestration file discussed in the diary
    - Path: guest/wl_wayland.c
      Note: Wayland extraction validated in Step 2
ExternalSources: []
Summary: Chronological diary for the cleanup ticket that modularizes the phase-2 Wayland client without adding new features.
LastUpdated: 2026-03-07T17:56:10.747601603-05:00
WhatFor: Maintain a detailed implementation record for the phase-2 modularization cleanup, including extraction steps, regressions, commands, and commit boundaries.
WhenToUse: Update this whenever a meaningful cleanup, validation, or commit step completes in the refactor ticket.
---


# Diary

## Goal

Capture the modularization cleanup chronologically so the extraction logic, tradeoffs, and validation evidence stay reviewable and reusable.

## Context

The phase-2 guest is working, but `guest/wl_sleepdemo.c` has become the new bottleneck. This ticket exists to split it into smaller modules before more feature work is layered on top.

## Quick Reference

- Ticket: `QEMU-03-PHASE2-CLEANUP`
- Related feature ticket: `QEMU-02-LAB-TWO`
- Trigger for this cleanup: `guest/wl_sleepdemo.c` reached 882 lines and 53 `static` functions
- Cleanup rule: preserve current behavior first, then resume feature work later

## Usage Examples

Read the latest completed step first, then review the linked source files and validation commands for that step.

## Related

- [../design/01-phase-2-modularization-guide.md](../design/01-phase-2-modularization-guide.md)
- [../tasks.md](../tasks.md)

## Step 1: Cleanup Ticket Initialization And Module Boundary Plan

I created a dedicated cleanup ticket instead of continuing to grow the phase-2 feature ticket. That keeps the refactor accountable to a single goal: split the current monolith into smaller pieces without changing the working behavior that was just reached in phase 2.

I also wrote down the initial module map before editing code. That matters because the existing file is large enough that ad hoc extraction would be easy to do badly. The cleanup needs explicit seams, a defined extraction order, and a clear list of non-goals.

### Prompt Context

**User prompt (verbatim):** "alright, do a cleanup ticket, where you only split up the coe and make the system modular, so that we can advance faster later on."

**Assistant interpretation:** Create a separate ticket for a behavior-preserving refactor pass, document the modularization plan, and then carry out the extraction work there.

**Inferred user intent:** Reduce structural friction now so later phase-2 and stage-3 work can move faster and with lower risk.

**Commit (code):** N/A

### What I did
- Created the ticket `QEMU-03-PHASE2-CLEANUP`.
- Added:
  - [index.md](/home/manuel/code/wesen/2026-03-07--qemu-power-tick/ttmp/2026/03/07/QEMU-03-PHASE2-CLEANUP--phase-2-wayland-client-modularization-cleanup/index.md)
  - [tasks.md](/home/manuel/code/wesen/2026-03-07--qemu-power-tick/ttmp/2026/03/07/QEMU-03-PHASE2-CLEANUP--phase-2-wayland-client-modularization-cleanup/tasks.md)
  - [design/01-phase-2-modularization-guide.md](/home/manuel/code/wesen/2026-03-07--qemu-power-tick/ttmp/2026/03/07/QEMU-03-PHASE2-CLEANUP--phase-2-wayland-client-modularization-cleanup/design/01-phase-2-modularization-guide.md)
  - this diary
- Measured the current pressure point:
  - `guest/wl_sleepdemo.c` = 882 lines
  - 53 `static` functions
- Chose the first target module split:
  - core app state and helpers
  - rendering
  - networking
  - Wayland listeners/bootstrap

### Why
- The current file is still understandable, but it is already at the point where every new feature wants to touch too many unrelated areas.
- A separate ticket prevents the refactor from muddling the phase-2 delivery record.

### What worked
- The cleanup scope is now isolated and documented.
- The extraction boundaries are concrete enough to start implementing instead of debating architecture abstractly.

### What didn't work
- N/A

### What I learned
- The best time to split code is not at the first sign of discomfort. It is when the next feature would otherwise turn a workable monolith into a drag on every later change. The phase-2 client has reached that point.

### What was tricky to build
- The tricky part here was deciding how ambitious to be. It would be easy to turn this into a broad architecture rewrite, but that would slow the lab down. I kept the scope narrow: extract obvious modules, leave behavior intact, and stop once the next iteration of feature work becomes easier.

### What warrants a second pair of eyes
- Whether the chosen module boundaries are the right minimum set for the next suspend and metrics work, or whether one more shared helper layer will likely be needed soon after.

### What should be done in the future
- Perform the extraction in small slices with rebuild and boot validation after each meaningful step.

### Code review instructions
- Start with [design/01-phase-2-modularization-guide.md](/home/manuel/code/wesen/2026-03-07--qemu-power-tick/ttmp/2026/03/07/QEMU-03-PHASE2-CLEANUP--phase-2-wayland-client-modularization-cleanup/design/01-phase-2-modularization-guide.md).
- Then compare the current monolith in [guest/wl_sleepdemo.c](/home/manuel/code/wesen/2026-03-07--qemu-power-tick/guest/wl_sleepdemo.c) against the planned target split.

### Technical details
- Current measured size:
  - `wc -l guest/wl_sleepdemo.c` -> `882`
  - `rg -n "^static " guest/wl_sleepdemo.c | wc -l` -> `53`

## Step 2: Extracted The Working Client Into Real Modules And Revalidated Runtime Behavior

I carried out the extraction as a behavior-preserving refactor instead of a redesign. The resulting shape is what the cleanup ticket set out to produce: `wl_sleepdemo.c` is now the thin orchestration file, and the code that used to be tangled together is split across explicit modules for core state, rendering, networking, and Wayland setup.

I then validated the refactor against the same runtime capabilities that mattered before the cleanup. The modularized build compiled, booted in QEMU, reached Weston, rendered a first frame, reconnected to the host drip server, and still received both pointer and keyboard input from the QMP harness.

### Prompt Context

**User prompt (verbatim):** (see Step 1)

**Assistant interpretation:** Perform the actual modularization work in the cleanup ticket and prove that the refactor did not regress the working phase-2 behavior.

**Inferred user intent:** Make later work faster by improving structure now, but do not lose the already-working phase-2 stack.

**Commit (code):** `91076d0` — `Modularize phase 2 Wayland client`

### What I did
- Added shared app/core definitions in:
  - [guest/wl_app_core.h](/home/manuel/code/wesen/2026-03-07--qemu-power-tick/guest/wl_app_core.h)
  - [guest/wl_app_core.c](/home/manuel/code/wesen/2026-03-07--qemu-power-tick/guest/wl_app_core.c)
- Extracted rendering into:
  - [guest/wl_render.h](/home/manuel/code/wesen/2026-03-07--qemu-power-tick/guest/wl_render.h)
  - [guest/wl_render.c](/home/manuel/code/wesen/2026-03-07--qemu-power-tick/guest/wl_render.c)
- Extracted networking into:
  - [guest/wl_net.h](/home/manuel/code/wesen/2026-03-07--qemu-power-tick/guest/wl_net.h)
  - [guest/wl_net.c](/home/manuel/code/wesen/2026-03-07--qemu-power-tick/guest/wl_net.c)
- Extracted Wayland listeners and bootstrap into:
  - [guest/wl_wayland.h](/home/manuel/code/wesen/2026-03-07--qemu-power-tick/guest/wl_wayland.h)
  - [guest/wl_wayland.c](/home/manuel/code/wesen/2026-03-07--qemu-power-tick/guest/wl_wayland.c)
- Reduced [guest/wl_sleepdemo.c](/home/manuel/code/wesen/2026-03-07--qemu-power-tick/guest/wl_sleepdemo.c) to:
  - argument parsing,
  - epoll/timer/signal setup,
  - main-loop orchestration.
- Updated [guest/build-wl-sleepdemo.sh](/home/manuel/code/wesen/2026-03-07--qemu-power-tick/guest/build-wl-sleepdemo.sh) to compile the modularized translation units.
- Mirrored the new sources and headers into the cleanup ticket’s `scripts/` directory.
- Rebuilt the client and initramfs, then booted:
  - `guest/build-wl-sleepdemo.sh build build/wl_sleepdemo`
  - `guest/build-phase2-rootfs.sh build build/initramfs-phase2-cleanup.cpio.gz build/wl_sleepdemo`
  - `guest/run-qemu-phase2.sh --kernel build/vmlinuz --initramfs build/initramfs-phase2-cleanup.cpio.gz --results-dir results-phase2-cleanup1`
- Validated:
  - screenshot via `screendump`
  - reconnect via `host/drip_server.py`
  - pointer via QMP pointer move/button
  - keyboard via QMP `send-key`

### Why
- The previous file layout would have made suspend and measurement work more expensive than necessary because almost every change would cross rendering, input, networking, and setup code in one file.
- Extracting those concerns now gives the next phase a cleaner starting point without changing the lab behavior.

### What worked
- The modularized client compiled cleanly on the first pass.
- The rebuilt guest booted successfully under Weston.
- Screenshot capture still worked.
- The guest reconnected to the drip server and logged `connected`.
- The client still logged:
  - `@@LOG kind=input BUTTON 272 STATE 1`
  - `@@LOG kind=input BUTTON 272 STATE 0`
  - `@@LOG kind=input KEY=30 STATE=1`
  - `@@LOG kind=input KEY=30 STATE=0`

### What didn't work
- N/A

### What I learned
- The current phase-2 code already had good extraction seams; the monolith was big, but it was not architecturally hopeless.
- Moving render first, then network, then Wayland was the right order because it minimized churn while the client stayed buildable.

### What was tricky to build
- The main sharp edge was shared state ownership. Every extracted module needed access to the same `struct app`, but I wanted to avoid recreating the original hidden coupling in a different form. The workable compromise was a single shared app/core header for the state model plus narrow module headers for behavior. That keeps the interfaces explicit without inventing unnecessary abstraction layers in the middle of a small C codebase.

### What warrants a second pair of eyes
- Whether the next cleanup slice should introduce a smaller event/timer abstraction before suspend work is added, or whether the current main-loop orchestration is already the right stopping point.

### What should be done in the future
- Run `docmgr doctor` and commit the cleanup ticket docs.
- Resume phase-2 suspend/measurement work from this cleaner code base instead of continuing to grow the old monolith.

### Code review instructions
- Start with [guest/wl_app_core.h](/home/manuel/code/wesen/2026-03-07--qemu-power-tick/guest/wl_app_core.h) to see the shared state boundary.
- Then review:
  - [guest/wl_render.c](/home/manuel/code/wesen/2026-03-07--qemu-power-tick/guest/wl_render.c)
  - [guest/wl_net.c](/home/manuel/code/wesen/2026-03-07--qemu-power-tick/guest/wl_net.c)
  - [guest/wl_wayland.c](/home/manuel/code/wesen/2026-03-07--qemu-power-tick/guest/wl_wayland.c)
  - [guest/wl_sleepdemo.c](/home/manuel/code/wesen/2026-03-07--qemu-power-tick/guest/wl_sleepdemo.c)
- Validate with:
  - `guest/build-wl-sleepdemo.sh build build/wl_sleepdemo`
  - `guest/build-phase2-rootfs.sh build build/initramfs-phase2-cleanup.cpio.gz build/wl_sleepdemo`
  - `guest/run-qemu-phase2.sh --kernel build/vmlinuz --initramfs build/initramfs-phase2-cleanup.cpio.gz --results-dir results-phase2-cleanup1`
  - `python3 host/qmp_harness.py --socket results-phase2-cleanup1/qmp.sock screendump --file results-phase2-cleanup1/01-cleanup.ppm`
  - `python3 host/drip_server.py --host 0.0.0.0 --port 5555 --interval 0.5 --active-seconds 15 --pause-seconds 1`
  - `python3 host/qmp_harness.py --socket results-phase2-cleanup1/qmp.sock pointer-move-normalized --x 0.5 --y 0.5`
  - `python3 host/qmp_harness.py --socket results-phase2-cleanup1/qmp.sock pointer-button --button left --down`
  - `python3 host/qmp_harness.py --socket results-phase2-cleanup1/qmp.sock pointer-button --button left`
  - `python3 host/qmp_harness.py --socket results-phase2-cleanup1/qmp.sock send-key --key a`

### Technical details
- Post-refactor proof markers from `results-phase2-cleanup1/guest-serial.log`:
  - `@@LOG kind=state connected`
  - `@@LOG kind=state seat-capabilities=3`
  - `@@LOG kind=state keyboard-rebound`
  - `@@LOG kind=state pointer-rebound`
  - `@@LOG kind=input BUTTON 272 STATE 1`
  - `@@LOG kind=input BUTTON 272 STATE 0`
  - `@@LOG kind=input KEY=30 STATE=1`
  - `@@LOG kind=input KEY=30 STATE=0`
