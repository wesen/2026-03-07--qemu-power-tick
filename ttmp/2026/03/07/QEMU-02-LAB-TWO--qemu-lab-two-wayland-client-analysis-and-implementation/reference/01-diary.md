---
Title: Diary
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
ExternalSources:
    - local:lab2.md
Summary: Chronological implementation diary for phase 2, including decisions, commands, failures, validation notes, and lessons learned.
LastUpdated: 2026-03-07T17:05:00-05:00
WhatFor: Maintain a step-by-step implementation diary for phase 2, including failures, commands, lessons, and validation notes.
WhenToUse: Update this whenever a meaningful design, implementation, or validation step completes during the phase-2 work.
---

# Diary

## Goal

Capture the phase-2 implementation chronologically so another engineer can understand what changed, what failed, what was learned, and how to validate the resulting system.

## Context

Phase 2 extends the stage-1 QEMU sleep lab into a graphical guest environment. The work includes Weston, a custom Wayland client, a host-side QMP harness, measurements, and a later submission report.

## Quick Reference

- Ticket: `QEMU-02-LAB-TWO`
- Phase focus: Weston plus custom Wayland client, no Chromium yet
- Imported source: `sources/local/lab2.md`
- Prior implementation base: `guest/sleepdemo.c`, `guest/init`, `host/drip_server.py`

## Usage Examples

Review the latest completed step first, then use that step’s `Code review instructions` and technical details to reproduce or validate the current state.

## Related

- [../design-doc/01-phase-2-wayland-client-analysis-and-implementation-guide.md](../design-doc/01-phase-2-wayland-client-analysis-and-implementation-guide.md)
- [../tasks.md](../tasks.md)

## Step 1: Ticket Initialization And Phase-2 Design Baseline

I created the new phase-2 ticket, imported the provided lab brief, and read it closely enough to separate the actual phase-2 requirements from the later Chromium stage. That clarified a key point early: Weston belongs in phase 2 as the compositor layer, while Chromium is intentionally deferred.

I then translated that reading into an implementation baseline. The ticket now has a concrete architecture, a task list, a diary structure, and an explicit bonus path for thinning the bootstrap later without blocking the first working system.

### Prompt Context

**User prompt (verbatim):** "Now create a new ticket and import /tmp/lab2.md . DO the analysis for the phase 2: wayland client (no chromium yet) and make a detailed analysis and implementation guid eand create tasks and then work them off one by one, committing at appropriate intervals, keeping a detailed diary (you're going to write another report for this phase)."

**Assistant interpretation:** Create a new docmgr ticket for the second lab, analyze the phase-2 Wayland-client work only, produce a detailed guide and task plan, and then execute the implementation while maintaining a diary and commit history.

**Inferred user intent:** Turn the second lab brief into a documented, reproducible engineering effort instead of only a high-level analysis.

**Commit (code):** N/A

### What I did
- Created ticket `QEMU-02-LAB-TWO`.
- Imported `/tmp/lab2.md`.
- Read the brief and isolated the phase-2 scope from the stage-3 Chromium work.
- Inspected the stage-1 guest and host files for reusable logic.
- Checked local availability of Wayland development headers, `wayland-scanner`, Cairo, Pixman, xkbcommon, and QEMU.
- Wrote the first detailed guide, task plan, and diary baseline.

### Why
- The brief spans more than one stage, so the first task was narrowing the implementation target correctly.
- Phase 2 changes the guest runtime model significantly, which makes a documented architecture and task plan necessary before code changes.

### What worked
- The new ticket and import path were created cleanly.
- The local environment already has the client-side Wayland development toolchain needed to build a small custom client.
- The stage-1 state machine looks reusable enough to evolve rather than rewrite from zero.

### What didn't work
- `weston` is not already installed locally, so the guest runtime cannot be assembled purely from tools already present on the host.

### What I learned
- The hardest phase-2 decision is guest packaging and runtime assembly, not the existence of Wayland headers.
- A richer guest image is likely necessary even if the fast initramfs boot loop is preserved.

### What was tricky to build
- The main conceptual trap was assuming stage 2 might be "just a Wayland client" and Weston would only matter later. After reading the brief carefully, that interpretation did not hold: Weston is the phase-2 compositor platform, and Chromium comes after that. Locking that down early prevents building an intermediate system that does not satisfy the assignment.

### What warrants a second pair of eyes
- The eventual Weston backend choice.
- The exact guest packaging approach used to make Weston available inside the VM.

### What should be done in the future
- Build and boot the smallest Weston-capable guest image and validate the basic compositor path before implementing the full client logic.

### Code review instructions
- Start with the phase-2 design doc and confirm the scope matches the imported brief.
- Then inspect the stage-1 files in `guest/` and `host/` to verify the planned reuse points.

### Technical details
- Imported brief location: `ttmp/2026/03/07/QEMU-02-LAB-TWO--qemu-lab-two-wayland-client-analysis-and-implementation/sources/local/lab2.md`
- Existing reusable code: `guest/sleepdemo.c`, `guest/init`, `guest/build-initramfs.sh`, `guest/run-qemu.sh`, `host/drip_server.py`, `scripts/measure_run.py`

## Step 2: Weston Guest Bring-Up And First Host Screenshot

The second implementation step focused on making phase 2 real rather than hypothetical. I built a new guest assembly path that can package Weston, `seatd`, runtime libraries, XKB data, and a small first client into the initramfs, then iterated until the guest could boot into a working Wayland socket on `virtio-gpu`.

This step produced the first concrete phase-2 proof point: the host can connect to QMP, query VM status, and capture a framebuffer image from a running Weston guest. The client is still `weston-simple-shm` at this point, not the custom lab client, but the compositor, graphics device, and screenshot control plane are now validated.

### Prompt Context

**User prompt (verbatim):** (see Step 1)

**Assistant interpretation:** Implement the phase-2 platform incrementally, keep a detailed diary, and work through the tasks with coherent commits.

**Inferred user intent:** Build the actual Weston-based lab environment, not just documentation, and preserve the reasoning trail while doing it.

**Commit (code):** N/A

### What I did
- Added [`scripts/copy-runtime-deps.py`](/home/manuel/code/wesen/2026-03-07--qemu-power-tick/scripts/copy-runtime-deps.py) to copy ELF runtime dependencies into a hand-assembled rootfs.
- Added [`guest/build-phase2-rootfs.sh`](/home/manuel/code/wesen/2026-03-07--qemu-power-tick/guest/build-phase2-rootfs.sh), [`guest/init-phase2`](/home/manuel/code/wesen/2026-03-07--qemu-power-tick/guest/init-phase2), [`guest/run-qemu-phase2.sh`](/home/manuel/code/wesen/2026-03-07--qemu-power-tick/guest/run-qemu-phase2.sh), and [`guest/weston.ini`](/home/manuel/code/wesen/2026-03-07--qemu-power-tick/guest/weston.ini).
- Mirrored those scripts into the ticket-local `scripts/` directory as requested.
- Downloaded and extracted `weston`, `libweston-13-0`, `libseat1`, and `seatd` into the phase-2 rootfs.
- Identified that the kernel did not expose `/dev/dri` initially and then changed the guest init to load `virtio_dma_buf` and `virtio-gpu` modules explicitly.
- Fixed several rootfs/runtime issues: relative initramfs output path handling, missing dynamic loader target, broken merged-usr library layout, and missing XKB data.
- Added [`host/qmp_harness.py`](/home/manuel/code/wesen/2026-03-07--qemu-power-tick/host/qmp_harness.py) and used it to run `query-status` and `screendump`.

### Why
- Weston is the phase-2 platform layer, so the first practical milestone had to be “guest compositor actually starts and can be observed from the host.”
- Using `weston-simple-shm` first keeps the bring-up loop narrow; it proves the compositor and framebuffer path before the custom client is introduced.

### What worked
- The guest now loads `virtio-gpu` successfully, creates `/dev/dri/card0`, and exposes a framebuffer.
- Weston reaches a working Wayland socket.
- `weston-simple-shm` launches as the first smoke-test client.
- QMP `query-status` works.
- QMP `screendump` produces a valid Netpbm framebuffer image.

### What didn't work
- The first phase-2 build failed because `guest/build-phase2-rootfs.sh` wrote the output initramfs path relative to the rootfs `cpio` subprocess:
  `guest/build-phase2-rootfs.sh: line 74: build/initramfs-phase2.cpio.gz: No such file or directory`
- The first guest boot had no `/dev/dri` node at all.
- Weston initially failed with:
  `/usr/bin/weston: not found`
  This turned out to be a dynamic-loader/rootfs-layout problem, not the binary literally missing.
- After that, Weston failed on:
  `xkbcommon: ERROR: failed to add default include path /usr/share/X11/xkb`
  followed by:
  `failed to create XKB context`

### What I learned
- The hardest part of phase-2 bring-up is guest packaging fidelity: small runtime mismatches break the compositor long before application logic is relevant.
- The generic kernel image is usable for phase 2, but only after the right GPU modules are shipped and loaded inside the initramfs.
- A merged-usr host layout matters when copying libraries by hand; if the guest rootfs layout disagrees, symlink-based libraries fail in non-obvious ways.

### What was tricky to build
- The loader and library-path issues were subtle because the shell only reported `/usr/bin/weston: not found`, which looks like a missing file but was actually a broken dynamic runtime. The root cause was that the hand-built rootfs split `/lib` and `/usr/lib`, while the source system uses merged-usr semantics. I fixed that by rebuilding the rootfs from scratch each time, wiring `lib/x86_64-linux-gnu` to the `usr/lib` tree, and explicitly copying the loader target.
- The graphics bring-up was also easy to misdiagnose. The initial missing `/dev/dri` could have been mistaken for an unsupported kernel, but the real problem was that the right module path had not been loaded. `bochs` was the wrong driver for `virtio-vga`; switching to `virtio_dma_buf` plus `virtio-gpu` matched the actual device and brought the DRM path up cleanly.

### What warrants a second pair of eyes
- The QMP pointer-event shape in `host/qmp_harness.py`, because QEMU input event semantics are easy to get slightly wrong.
- Whether the current Weston launch arguments are the cleanest long-term settings for the custom client stage.

### What should be done in the future
- Replace `weston-simple-shm` with the custom `wl_sleepdemo` client.
- Extend the QMP harness from screenshot/status into full keyboard and pointer checkpoint automation.
- Start capturing named phase-2 artifacts and timing metrics with the custom client in place.

### Code review instructions
- Start with [`guest/build-phase2-rootfs.sh`](/home/manuel/code/wesen/2026-03-07--qemu-power-tick/guest/build-phase2-rootfs.sh) and [`guest/init-phase2`](/home/manuel/code/wesen/2026-03-07--qemu-power-tick/guest/init-phase2).
- Then inspect [`scripts/copy-runtime-deps.py`](/home/manuel/code/wesen/2026-03-07--qemu-power-tick/scripts/copy-runtime-deps.py) and [`host/qmp_harness.py`](/home/manuel/code/wesen/2026-03-07--qemu-power-tick/host/qmp_harness.py).
- Validate with:
  `guest/build-phase2-rootfs.sh build build/initramfs-phase2.cpio.gz`
  `guest/run-qemu-phase2.sh --kernel build/vmlinuz --initramfs build/initramfs-phase2.cpio.gz --results-dir results-phase2-smoke5`
  `host/qmp_harness.py --socket results-phase2-smoke5/qmp.sock query-status`
  `host/qmp_harness.py --socket results-phase2-smoke5/qmp.sock screendump --file results-phase2-smoke5/01-weston-simple-shm.ppm`

### Technical details
- First successful graphics bring-up log lines included:
  `Initialized virtio_gpu 0.1.0`
  `fb0: virtio_gpudrmfb frame buffer device`
  `[init-phase2] wayland ready`
- First successful screenshot artifact:
  `results-phase2-smoke5/01-weston-simple-shm.ppm`
