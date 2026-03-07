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

## Step 3: First Custom Wayland Client Integration

With the platform layer stable, I replaced the packaged smoke client path with a first custom `wl_sleepdemo` client. This version owns its own Wayland surface, renders a status dashboard with a tiny built-in bitmap font, runs a network reconnect loop, and reports state transitions through the same `@@LOG` family as stage 1.

The key outcome of this step is that the guest is no longer only “Weston can run.” It is now “our own Wayland client runs inside Weston, survives in the live guest, reconnects to the host drip server when it appears, and can be captured from the host with QMP screenshots.”

### Prompt Context

**User prompt (verbatim):** (see Step 1)

**Assistant interpretation:** Continue implementing the phase-2 tasks rather than stopping after the compositor smoke test.

**Inferred user intent:** Move from platform bring-up into the actual application client that the later report can describe as our own work.

**Commit (code):** N/A

### What I did
- Added [`guest/build-wl-sleepdemo.sh`](/home/manuel/code/wesen/2026-03-07--qemu-power-tick/guest/build-wl-sleepdemo.sh) to generate `xdg-shell` bindings and compile the custom client.
- Added [`guest/wl_sleepdemo.c`](/home/manuel/code/wesen/2026-03-07--qemu-power-tick/guest/wl_sleepdemo.c) with:
  - registry/global binding,
  - `xdg-shell` toplevel setup,
  - `wl_shm` buffer allocation,
  - custom pixel rendering,
  - keyboard and pointer listeners,
  - a small reconnecting network loop,
  - timer-driven redraws,
  - `@@LOG` state logging.
- Rebuilt the phase-2 initramfs with `/bin/wl_sleepdemo` embedded so `init-phase2` launches it instead of `weston-simple-shm`.
- Booted the custom-client guest and captured a screenshot artifact.
- Started the host drip server after the guest was already running and verified the client eventually logged `connected`.

### Why
- The assignment is about a custom phase-2 Wayland client, not only compositor bring-up.
- It was important to validate that the client could own the surface and network loop before layering in suspend behavior.

### What worked
- `wl_sleepdemo` compiled successfully from generated `xdg-shell` protocol sources.
- The custom client booted under Weston and stayed alive.
- QMP screenshot capture still worked with the custom client in place.
- The client reconnected to the host drip server dynamically and logged `@@LOG kind=state connected` after the server came online.

### What didn't work
- The first custom-client run spammed `@@LOG kind=state socket-hup` because the drip server was not started yet; this was expected in hindsight, but it obscured the fact that the reconnect loop itself was fine.
- Keyboard and pointer injection have not been validated end-to-end yet. I sent QMP `send-key` and `input-send-event` commands, but the guest log did not yet show matching input events.

### What I learned
- The reconnect loop works well enough that the host server can be attached after boot and the client will recover on its own.
- The input-validation problem is likely in Wayland seat/focus handling or QMP device semantics, not in the basic client rendering path.

### What was tricky to build
- The hardest part of the client was avoiding a heavy dependency stack while still making the surface readable. I chose a tiny built-in 5x7 bitmap font and raw `wl_shm` drawing so the client could render text-like status without depending on Cairo text APIs, fontconfig setup, or additional guest font packages.
- Another subtle point was Wayland seat setup. Listener registration order matters; I had to revisit the seat handshake and add an extra display roundtrip so seat capability events had a chance to arrive for the custom client.

### What warrants a second pair of eyes
- The client’s Wayland event-loop integration, especially seat/focus handling and whether it should use a stricter prepare-read/dispatch pattern.
- The host pointer-event format in `host/qmp_harness.py`, because QMP pointer injection is still unverified.

### What should be done in the future
- Finish keyboard and pointer validation with screenshots and logs.
- Integrate the suspend/resume state model from stage 1 into `wl_sleepdemo`.
- Add measurement logging for redraw and reconnect timing in the custom client rather than only connection-state logs.

### Code review instructions
- Start with [`guest/wl_sleepdemo.c`](/home/manuel/code/wesen/2026-03-07--qemu-power-tick/guest/wl_sleepdemo.c) and [`guest/build-wl-sleepdemo.sh`](/home/manuel/code/wesen/2026-03-07--qemu-power-tick/guest/build-wl-sleepdemo.sh).
- Then inspect the interaction with [`guest/init-phase2`](/home/manuel/code/wesen/2026-03-07--qemu-power-tick/guest/init-phase2) and [`host/qmp_harness.py`](/home/manuel/code/wesen/2026-03-07--qemu-power-tick/host/qmp_harness.py).
- Validate with:
  `guest/build-wl-sleepdemo.sh build build/wl_sleepdemo`
  `guest/build-phase2-rootfs.sh build build/initramfs-phase2-client.cpio.gz build/wl_sleepdemo`
  `guest/run-qemu-phase2.sh --kernel build/vmlinuz --initramfs build/initramfs-phase2-client.cpio.gz --results-dir results-phase2-client1`
  `python3 host/drip_server.py --host 0.0.0.0 --port 5555 --interval 0.5 --active-seconds 30 --pause-seconds 5`
  `host/qmp_harness.py --socket results-phase2-client1/qmp.sock screendump --file results-phase2-client1/01-client.ppm`

### Technical details
- Custom-client boot log marker:
  `[init-phase2] launching client=/bin/wl_sleepdemo`
- Reconnect proof marker:
  `@@LOG kind=state connected`
- Artifacts captured with the custom client:
  `results-phase2-client1/01-client.ppm`
  `results-phase2-client1/03-after-network.ppm`
