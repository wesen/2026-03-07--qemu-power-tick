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
    - local:01-lab2.md
Summary: Chronological implementation diary for phase 2, including decisions, commands, failures, validation notes, and lessons learned.
LastUpdated: 2026-03-07T18:55:00-05:00
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
- Imported source: `sources/local/01-lab2.md`
- Prior implementation base: `guest/sleepdemo.c`, `guest/init`, `host/drip_server.py`
- External session reference: `019cc9de-fbce-7bc3-91dc-30a36f28f3ec`

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
- Imported brief location: `ttmp/2026/03/07/QEMU-02-LAB-TWO--qemu-lab-two-wayland-client-analysis-and-implementation/sources/local/01-lab2.md`
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

## Step 4: Input Path Investigation And HID Root Cause

The next blocker was clearly input rather than rendering. QMP key and pointer injection commands were accepted, but the guest log still showed no keyboard-focus, key, pointer, or seat-capability events. That pattern strongly suggested the failure was below the Wayland client listener layer, so I switched from adjusting the client loop to checking whether Weston actually had any input devices to export as a seat.

That change in perspective paid off quickly. The added `seat-capabilities`, `keyboard-bound`, and `pointer-bound` instrumentation in `wl_sleepdemo` produced no output at all on a fresh boot, while the client kept logging network transitions normally. The most likely root cause was that the initramfs had enough graphics support to bring up `virtio-gpu`, but not enough HID support for QEMU’s USB keyboard and tablet devices.

### Prompt Context

**User prompt (verbatim):** "ok, let's go, continue"

**Assistant interpretation:** Continue implementing the phase-2 system from the current scaffold rather than stopping at the first custom-client milestone.

**Inferred user intent:** Keep driving the phase-2 lab toward a fully working, measurable system while maintaining enough context to resume reliably after interruptions.

**Commit (code):** N/A

### What I did
- Rebuilt the instrumented custom client and a fresh phase-2 initramfs.
- Booted a new VM in `results-phase2-client2` and confirmed QMP `query-status` still worked.
- Inspected `results-phase2-client2/guest-serial.log` for the new seat instrumentation.
- Confirmed the log contained repeated network `connected` and `socket-hup` events but no seat or input events.
- Checked the running host processes and noticed an earlier `host/drip_server.py` instance was still active, which explained the immediate connect/hup churn but not the input silence.
- Audited the phase-2 rootfs assembly path and module-loading list.
- Checked the host kernel module tree for USB/HID drivers that match QEMU’s `usb-kbd` and `usb-tablet`.
- Patched [`guest/build-phase2-rootfs.sh`](/home/manuel/code/wesen/2026-03-07--qemu-power-tick/guest/build-phase2-rootfs.sh) and [`guest/init-phase2`](/home/manuel/code/wesen/2026-03-07--qemu-power-tick/guest/init-phase2) to ship and load `hid.ko`, `hid-generic.ko`, and `usbhid.ko`.
- Mirrored those bootstrap changes into the ticket-local copies under [`../scripts/`](../scripts/).
- Added `/dev/input` boot logging so the next run can show immediately whether the guest now sees input nodes.

### Why
- QMP acknowledging an injected input event does not prove the guest compositor can consume it.
- Without a `wl_seat` capability callback, debugging client-side input listeners or QMP command shape further would be premature.
- Shipping the HID stack is the smallest concrete change that matches the observed absence of seat capabilities.

### What worked
- The added client instrumentation narrowed the failure from “input does not work” to “no seat capability events are arriving.”
- The existing phase-2 rootfs build path made it straightforward to add a missing driver family without redesigning the image.
- The new boot logging should make the next validation step much faster to interpret.

### What didn't work
- The earlier suspicion that the main issue might be QMP command shape did not hold up once the client showed no seat-capability activity at all.
- The fresh `results-phase2-client2` boot still came up without any client-visible seat logs.

### What I learned
- The absence of `wl_seat` capability callbacks is a stronger diagnostic signal than the absence of key events alone.
- The phase-2 initramfs had already reached “graphics complete” before it reached “input complete”; those hardware paths need separate packaging checks.

### What was tricky to build
- The tricky part was avoiding a false diagnosis. Because QMP returned success for `send-key` and pointer events, it was easy to keep adjusting the host harness or the Wayland dispatch loop. The better signal came from the seat instrumentation: if Weston never advertises keyboard or pointer capabilities, then no amount of key-injection formatting will matter. That forced the investigation down into guest kernel module packaging and device-node availability instead of staying at the Wayland API level.

### What warrants a second pair of eyes
- Whether `usbhid` plus the generic HID modules are sufficient on this kernel, or whether another input dependency still needs to be carried explicitly.
- Whether the long-running background drip server should be folded into the scripted harness so reconnect noise does not complicate unrelated debugging.

### What should be done in the future
- Reboot with the updated initramfs and confirm `/dev/input` appears.
- Verify that the client logs `seat-capabilities`, `keyboard-bound`, and `pointer-bound`.
- Only then resume QMP keyboard and pointer validation.

### Code review instructions
- Start with [`guest/build-phase2-rootfs.sh`](/home/manuel/code/wesen/2026-03-07--qemu-power-tick/guest/build-phase2-rootfs.sh) and [`guest/init-phase2`](/home/manuel/code/wesen/2026-03-07--qemu-power-tick/guest/init-phase2) to review the newly added HID path.
- Then inspect the seat instrumentation in [`guest/wl_sleepdemo.c`](/home/manuel/code/wesen/2026-03-07--qemu-power-tick/guest/wl_sleepdemo.c).
- Validate with:
  `guest/build-phase2-rootfs.sh build build/initramfs-phase2-client.cpio.gz build/wl_sleepdemo`
  `guest/run-qemu-phase2.sh --kernel build/vmlinuz --initramfs build/initramfs-phase2-client.cpio.gz --results-dir results-phase2-client3`
  `tail -n 120 results-phase2-client3/guest-serial.log`

### Technical details
- Fresh-run symptom:
  `results-phase2-client2/guest-serial.log` showed repeated `@@LOG kind=state connected` and `@@LOG kind=state socket-hup`, but no `seat-capabilities`, `keyboard-bound`, or `pointer-bound`.
- Input-related module candidates identified on the host:
  `/lib/modules/6.8.0-101-generic/kernel/drivers/hid/hid.ko.zst`
  `/lib/modules/6.8.0-101-generic/kernel/drivers/hid/hid-generic.ko.zst`
  `/lib/modules/6.8.0-101-generic/kernel/drivers/hid/usbhid/usbhid.ko.zst`

## Step 5: Weston Registry Proof That `wl_seat` Is Missing

After adding the HID modules, the guest finally showed `/dev/input/event0`, `/dev/input/event1`, and `/dev/input/mice` during boot. That ruled out the earlier “kernel has no input devices at all” theory, but it still did not make the Wayland input path work. The next useful question was therefore not whether input nodes existed, but whether Weston was actually exporting a seat to clients.

I answered that by instrumenting Wayland global discovery directly in `wl_sleepdemo`. The result was decisive: the client sees `wl_compositor`, `wl_shm`, `xdg_wm_base`, `wl_output`, and several Weston protocol extensions, but no `wl_seat` global at all. So the problem is now narrowed to Weston’s own input stack, not the client listeners and not the QMP harness.

### Prompt Context

**User prompt (verbatim):** "don't forget to keep a frequent diary as you do these investigations, because you have frequent amnesia so need to keep a diary"

**Assistant interpretation:** Keep updating the ticket diary continuously while investigating, instead of waiting until after a fix lands.

**Inferred user intent:** Preserve enough chronological detail that the work can be resumed safely after interruptions and so the final report includes the real debugging process.

**Commit (code):** N/A

### What I did
- Added registry logging in [`guest/wl_sleepdemo.c`](/home/manuel/code/wesen/2026-03-07--qemu-power-tick/guest/wl_sleepdemo.c) so each advertised Wayland global is written to the serial log.
- Added an explicit `seat-missing` log when the client reaches setup without any `wl_seat`.
- Mirrored the same changes into the ticket-local script copy at [`../scripts/wl_sleepdemo.c`](../scripts/wl_sleepdemo.c).
- Rebuilt the client and initramfs.
- Booted a fresh run in `results-phase2-client4`.
- Inspected the guest serial log for the new registry-level evidence.

### Why
- The previous evidence only showed that seat callbacks never fired; it did not distinguish between “seat exists but capabilities are empty” and “seat global is absent.”
- Logging the registry directly is the cleanest way to remove that ambiguity.

### What worked
- The new logging proved that `wl_seat` is absent from the client-visible registry.
- The guest now clearly has kernel-level input device nodes, so the problem boundary has moved upward into Weston’s compositor input stack.

### What didn't work
- Adding HID support alone was not enough to make Weston export a seat.
- The fresh client run still only logged `seat-missing` followed by network reconnect churn.

### What I learned
- “`/dev/input` exists” and “Wayland clients receive `wl_seat`” are distinct milestones; the second still depends on Weston successfully building an input seat from its runtime environment.
- The likely next suspect is the compositor’s libinput/udev path rather than the client’s registry or listener logic.

### What was tricky to build
- The hard part was maintaining the right abstraction boundary while debugging. Once `/dev/input` appeared, it would have been easy to jump back into QMP input injection testing. But the registry log made that premature: if the server never publishes `wl_seat`, then input injection, focus, and client listener details are downstream effects, not root causes. The correct move was to instrument the registry and then pivot to Weston’s own logs.

### What warrants a second pair of eyes
- Whether Weston in this minimal initramfs requires a running `udevd` and populated device metadata before it will create a seat from those `/dev/input` nodes.
- Whether there is a simpler phase-2-compatible backend or device choice that preserves QMP screenshot/input goals while reducing the udev/libinput dependency surface.

### What should be done in the future
- Expose `weston.log` on the serial console so the compositor’s reason for omitting `wl_seat` is visible from the host.
- Confirm whether libinput is rejecting the devices due to missing udev properties or some launcher/session issue.
- If needed, add a minimal udev userspace to the phase-2 image and retry seat creation.

### Code review instructions
- Start with the new registry logging in [`guest/wl_sleepdemo.c`](/home/manuel/code/wesen/2026-03-07--qemu-power-tick/guest/wl_sleepdemo.c).
- Then inspect `results-phase2-client4/guest-serial.log` for the global list and the `seat-missing` marker.
- Use this command to reproduce the log view:
  `rg -n '@@LOG|seat-missing|interface=' results-phase2-client4/guest-serial.log`

### Technical details
- Key registry evidence from `results-phase2-client4/guest-serial.log`:
  `@@LOG kind=state global name=1 interface=wl_compositor version=5`
  `@@LOG kind=state global name=13 interface=wl_shm version=1`
  `@@LOG kind=state global name=16 interface=xdg_wm_base version=5`
  `@@LOG kind=state seat-missing`
- There was no `interface=wl_seat` line at all in the captured global list.

## Step 6: Udev Integration Restored Weston Seats

The Weston log made the next blocker explicit: even after the HID modules were present, Weston still printed `warning: no input devices found`. That pointed away from kernel device-node creation and toward userspace device discovery. In a minimal initramfs, the missing piece was a udev environment that could tag devices so libinput and Weston would accept them as real seats.

I added a minimal udev userspace to the phase-2 image and triggered input/drm events before starting Weston. That changed the compositor behavior immediately. Weston started `systemd-udevd`, picked up keyboard devices, then after adding `psmouse` it also picked up pointer devices. The client registry now includes `wl_seat`, which is the first time the full compositor input path has been visible to the custom client.

### Prompt Context

**User prompt (verbatim):** (see Step 5)

**Assistant interpretation:** Keep the diary current while continuing the phase-2 input investigation instead of batching the notes for later.

**Inferred user intent:** Preserve the actual debugging path and intermediate conclusions so the later report is accurate and the work remains resumable.

**Commit (code):** N/A

### What I did
- Patched [`guest/build-phase2-rootfs.sh`](/home/manuel/code/wesen/2026-03-07--qemu-power-tick/guest/build-phase2-rootfs.sh) to copy:
  - `/usr/bin/udevadm`
  - `/etc/udev/udev.conf`
  - `/usr/lib/udev/rules.d`
  - `/usr/lib/udev/hwdb.bin`
  - `/usr/share/libinput`
- Added `udevadm` runtime dependency copying to the phase-2 rootfs assembly path.
- Patched [`guest/init-phase2`](/home/manuel/code/wesen/2026-03-07--qemu-power-tick/guest/init-phase2) to:
  - create `/run/udev`,
  - start `systemd-udevd`,
  - trigger input and DRM add events,
  - wait for `udevadm settle`,
  - print Weston’s log to the serial console after the compositor comes up.
- Mirrored all of those changes into the ticket-local [`../scripts/`](../scripts/) copies.
- Added `psmouse.ko` to the initramfs and load list so the guest exposes pointer devices rather than only the keyboard.
- Added pragmatic keyboard and pointer fallback binding in [`guest/wl_sleepdemo.c`](/home/manuel/code/wesen/2026-03-07--qemu-power-tick/guest/wl_sleepdemo.c) so the client can proceed even though the seat-capability callback still appears absent.
- Validated the resulting behavior in `results-phase2-client6`, `results-phase2-client7`, and `results-phase2-client8`.

### Why
- Weston had enough state to run a compositor but not enough userspace to identify input devices correctly.
- A real seat requires more than `/dev/input/*`; it also requires udev-tagged devices so libinput recognizes them.
- Printing Weston’s own log into the serial stream made each iteration much easier to interpret from the host.

### What worked
- `systemd-udevd` starts successfully inside the guest.
- Weston now logs tagged input devices and `libinput: configuring device ...` lines.
- The client now sees `interface=wl_seat version=7` in the Wayland registry.
- After adding `psmouse`, Weston also recognizes pointer devices:
  `VirtualPS/2 VMware VMMouse: device is a pointer`
- The client now logs:
  - `seat-listener-added`
  - `keyboard-bound-fallback`
  - `pointer-bound-fallback`

### What didn't work
- Weston still does not appear to emit the `seat-capabilities` callback to this client, even though the seat is present and the input devices are active.
- QMP-injected keyboard and pointer events still are not reaching the client surface yet, despite the compositor and client now having real keyboard/pointer paths.

### What I learned
- In this minimal guest, udev is not optional for Weston/libinput if the goal is a real `wl_seat`.
- The ticket-local `scripts/` mirror is paying off: every time I changed the bootstrap or client, the ticket retained the exact implementation state alongside the diary.
- The remaining host-injection problem is now isolated much more tightly than before. It is no longer “bring up a seat,” but “make QMP-delivered events hit the correct focused surface.”

### What was tricky to build
- The tricky part was identifying the exact boundary between kernel input visibility and compositor seat creation. Once `/dev/input` existed, it would have been easy to assume the guest side was “done.” Weston’s own log proved otherwise: without udev rules, hwdb, and libinput data, it still considered the system to have no usable input devices. The fix therefore had to be a small userspace expansion, not another client-side change.

### What warrants a second pair of eyes
- Why Weston is not sending a visible `seat-capabilities` event to this client even though the seat exists and fallback bindings work.
- Whether the remaining QMP input problem is about focus, device targeting, or event coordinate semantics.

### What should be done in the future
- Verify the exact QMP input semantics against the current QEMU schema and docs.
- Update the host harness so pointer coordinates are expressed in the correct absolute range and so device selection, if needed, is explicit.
- Once host input lands, capture the required input screenshots and then move on to suspend/resume integration.

### Code review instructions
- Start with [`guest/build-phase2-rootfs.sh`](/home/manuel/code/wesen/2026-03-07--qemu-power-tick/guest/build-phase2-rootfs.sh) and [`guest/init-phase2`](/home/manuel/code/wesen/2026-03-07--qemu-power-tick/guest/init-phase2) for the udev/libinput bring-up.
- Then inspect the fallback binding logic in [`guest/wl_sleepdemo.c`](/home/manuel/code/wesen/2026-03-07--qemu-power-tick/guest/wl_sleepdemo.c).
- Review the corresponding ticket-local mirrors in [`../scripts/`](../scripts/), since those are being kept in sync on purpose.
- Validate with:
  `guest/build-phase2-rootfs.sh build build/initramfs-phase2-client.cpio.gz build/wl_sleepdemo`
  `guest/run-qemu-phase2.sh --kernel build/vmlinuz --initramfs build/initramfs-phase2-client.cpio.gz --results-dir results-phase2-client8`
  `tail -n 220 results-phase2-client8/guest-serial.log`

### Technical details
- `results-phase2-client6/guest-serial.log` first showed successful udev startup and a real seat:
  `Starting systemd-udevd version 255.4-1ubuntu8.8`
  `@@LOG kind=state global name=14 interface=wl_seat version=7`
- `results-phase2-client7/guest-serial.log` showed pointer discovery after `psmouse`:
  `event2  - VirtualPS/2 VMware VMMouse: device is a pointer`
- `results-phase2-client8/guest-serial.log` currently shows the client-side binding state:
  `@@LOG kind=state seat-listener-added`
  `@@LOG kind=state keyboard-bound-fallback`
  `@@LOG kind=state pointer-bound-fallback`

## Step 7: Backfilled Ticket Script Archive And Continued Input Debugging

Before pushing further on the host input path, I backfilled the ticket-local script archive so it reflects the full phase-2 working set rather than only the newest files. That matters for this ticket because phase 2 is explicitly building on stage-1 host/guest helpers and measurement tooling, and the ticket should remain continuation-friendly even if the top-level repo changes later.

After that housekeeping, I resumed the current input investigation from the `results-phase2-client8` state. The guest side is no longer the main uncertainty: Weston sees keyboard and pointer devices, the client sees `wl_seat`, and fallback bindings for both keyboard and pointer are active. The remaining issue is now concentrated in host-side QMP event delivery and focus semantics.

### Prompt Context

**User prompt (verbatim):** "ok, you should backfill the scripts, then continue your work, continuing with the frequent diary entries"

**Assistant interpretation:** Make sure the ticket-local `scripts/` directory contains the missing earlier helpers too, then continue implementing and keep updating the diary frequently.

**Inferred user intent:** Keep the ticket self-contained and continuation-safe while implementation proceeds.

**Commit (code):** N/A

### What I did
- Compared the top-level `guest/`, `host/`, and `scripts/` helper set against the ticket-local [`../scripts/`](../scripts/) directory.
- Copied these missing supporting files into the ticket-local archive:
  - [`host/drip_server.py`](/home/manuel/code/wesen/2026-03-07--qemu-power-tick/host/drip_server.py) -> `scripts/drip_server.py`
  - [`scripts/measure_run.py`](/home/manuel/code/wesen/2026-03-07--qemu-power-tick/scripts/measure_run.py) -> `scripts/measure_run.py`
  - [`guest/run-qemu.sh`](/home/manuel/code/wesen/2026-03-07--qemu-power-tick/guest/run-qemu.sh) -> `scripts/run-qemu-stage1.sh`
  - [`guest/init`](/home/manuel/code/wesen/2026-03-07--qemu-power-tick/guest/init) -> `scripts/init-stage1`
- Kept the newer phase-2 files mirrored as before.
- Continued the input investigation from `results-phase2-client8`.

### Why
- The ticket-local script archive should be sufficient for another engineer to understand both the inherited stage-1 tooling and the new phase-2 additions.
- The backfill prevents the ticket from depending too heavily on undocumented repo context outside the ticket workspace.

### What worked
- The ticket-local `scripts/` directory now contains both the phase-2-specific helpers and the reused stage-1 support files that shaped the current implementation.
- The current debugging state remains narrowed to QMP/focus behavior rather than missing guest runtime pieces.

### What didn't work
- N/A for the archival step itself.

### What I learned
- The archival requirement is worth treating as part of the engineering workflow, not as cleanup at the end; it reduces context loss while the implementation is still moving.

### What was tricky to build
- The only subtle point here was deciding what counts as “previous scripts too.” I treated that as “anything materially reused or likely needed to understand/reproduce phase 2,” not just files edited in this exact slice of work. That is why the backfill includes the stage-1 runner/bootstrap and the measurement/drip-server helpers.

### What warrants a second pair of eyes
- Whether any other reused helper should also be mirrored into the ticket-local archive before the final report, such as `scripts/guestfwd-to-drip.sh` or `scripts/hmp-command.sh` if they become relevant again.

### What should be done in the future
- Keep mirroring new or materially changed scripts immediately, instead of relying on end-of-step cleanup.
- Continue the host input and focus validation from `results-phase2-client8`.

### Code review instructions
- Inspect the current archive at [`../scripts/`](../scripts/) and confirm it now contains both phase-2 files and the reused stage-1 support helpers.
- Then continue from the active input-debugging artifacts in `results-phase2-client8/`.

### Technical details
- Newly backfilled ticket-local files:
  - `scripts/drip_server.py`
  - `scripts/measure_run.py`
  - `scripts/run-qemu-stage1.sh`
  - `scripts/init-stage1`

## Step 8: Host Injection Semantics Narrowed Further

With the guest seat path working, I turned back to the host-side injection channel and made the QMP harness less opaque. The main finding from this slice is that the remaining failure is not “no mouse devices exist” and not “the client has no pointer or keyboard objects.” QEMU does acknowledge the injected events, and it exposes multiple emulated mice that can be switched between, but none of the tested combinations are yet producing visible input callbacks in the client log.

That means the next debugging layer should be the guest kernel input stream itself. If QMP injection never shows up in `/dev/input/event*`, the problem is entirely on the QEMU injection side. If it does show up there but not in Weston/client callbacks, then the remaining issue is compositor focus or Wayland delivery.

### Prompt Context

**User prompt (verbatim):** (same as Step 7)

**Assistant interpretation:** Continue the implementation after backfilling scripts, and keep recording the intermediate findings rather than waiting for a final fix.

**Inferred user intent:** Preserve the ongoing reasoning trail while the remaining input issue is still being narrowed.

**Commit (code):** N/A

### What I did
- Extended [`host/qmp_harness.py`](/home/manuel/code/wesen/2026-03-07--qemu-power-tick/host/qmp_harness.py) and its ticket-local mirror to add:
  - `query-mice`
  - `pointer-move-normalized`
  - `pointer-move-rel`
- Verified with `query-mice` that QEMU currently exposes:
  - `QEMU HID Tablet`
  - `QEMU PS/2 Mouse`
  - `vmmouse`
- Used `human-monitor-command` with `mouse_set` to switch the current emulated mouse between indices `2`, `4`, and `6`.
- Retested pointer motion and clicks against both absolute and relative paths.
- Confirmed that the client still logs no `pointer-enter`, `pointer-motion`, `pointer-button`, `keyboard-enter`, or `@@LOG kind=input` lines after those injections.

### Why
- Once the guest side had a real seat and pointer devices, the remaining ambiguity shifted to QMP semantics.
- Adding explicit mouse inspection and relative/normalized movement paths avoids more blind trial-and-error with raw `input-send-event` payloads.

### What worked
- `mouse_set` works, and QEMU really does switch the current mouse device.
- The harness now matches the input model more closely and is useful as a reusable debugging tool instead of only a one-shot screenshot helper.
- The absence of client callbacks is now backed by stronger negative evidence across multiple mouse-device modes.

### What didn't work
- Neither absolute normalized pointer movement nor relative pointer movement produced visible client-side pointer events.
- `send-key` still does not produce visible keyboard-focus or key-event logs in the client.

### What I learned
- The remaining bug is tighter than before: it is not explained by missing seat creation, missing pointer devices, or a too-thin host harness.
- The next sensible breakpoint is the evdev layer inside the guest.

### What was tricky to build
- The subtle part here was not the harness code itself, but resisting the temptation to keep perturbing Wayland client code without new evidence. By extending the QMP harness just enough to inspect mouse state and vary injection modes, I was able to rule out another whole class of vague explanations before moving lower in the stack.

### What warrants a second pair of eyes
- Whether QEMU `input-send-event` on this setup requires a more specific display/input routing assumption than the current `display none + screendump + QMP` model exposes.
- Whether keyboard focus on Weston kiosk surfaces requires an explicit activation path beyond pointer motion/click.

### What should be done in the future
- Add a guest-side evdev probe so injected input can be observed before Weston/Wayland handling.
- If evdev stays quiet, continue investigating QEMU injection behavior.
- If evdev is active, shift the focus to Weston focus handling or client-side delivery.

### Code review instructions
- Start with the new host harness capabilities in [`host/qmp_harness.py`](/home/manuel/code/wesen/2026-03-07--qemu-power-tick/host/qmp_harness.py) and the mirrored copy in [`../scripts/qmp_harness.py`](../scripts/qmp_harness.py).
- Then inspect `results-phase2-client8/guest-serial.log` for the absence of client-visible input events despite successful QMP command returns.

### Technical details
- QEMU-reported mouse state after inspection:
  - `index=4 name="QEMU HID Tablet" absolute=true`
  - `index=2 name="QEMU PS/2 Mouse" absolute=false`
  - `index=6 name="vmmouse" absolute=true`
- `mouse_set` was accepted for indices `2`, `4`, and `6`, and `query-mice` reflected the changes.
- Despite that, `rg -n 'kind=input|pointer-enter|pointer-motion|pointer-button|keyboard-enter|KEY=' results-phase2-client8/guest-serial.log` still finds no client-visible input events.

## Step 9: Evdev Probe Proved QMP Pointer Events Reach The Guest Kernel

I added a raw evdev probe to break the remaining uncertainty cleanly. That paid off immediately: QMP pointer motion and button events are reaching `/dev/input/event2` inside the guest. So the remaining gap is no longer in QEMU-to-kernel delivery for pointer input. It is somewhere between the evdev/libinput layer and our Wayland client surface.

Keyboard is different. `send-key` and the generic `input-send-event` key form still do not show up at the guest evdev layer, so the keyboard path appears to diverge earlier than the pointer path. That means the remaining host-input problem is now split: pointer delivery reaches evdev but not the client, while keyboard injection is still not visible even at evdev.

### Prompt Context

**User prompt (verbatim):** "go ahead."

**Assistant interpretation:** Continue with the next debugging step rather than stopping to summarize.

**Inferred user intent:** Keep driving the phase-2 implementation forward until the remaining blockers are concretely narrowed or fixed.

**Commit (code):** N/A

### What I did
- Added [`guest/evdev_probe.c`](/home/manuel/code/wesen/2026-03-07--qemu-power-tick/guest/evdev_probe.c) and [`guest/build-evdev-probe.sh`](/home/manuel/code/wesen/2026-03-07--qemu-power-tick/guest/build-evdev-probe.sh).
- Mirrored both into the ticket-local [`../scripts/`](../scripts/) archive.
- Extended [`guest/build-phase2-rootfs.sh`](/home/manuel/code/wesen/2026-03-07--qemu-power-tick/guest/build-phase2-rootfs.sh) so it can optionally package an `evdev_probe` binary.
- Patched [`guest/init-phase2`](/home/manuel/code/wesen/2026-03-07--qemu-power-tick/guest/init-phase2) to start the probe in the background and stream its log to the serial console.
- Built a fresh guest image with both `wl_sleepdemo` and `evdev_probe`.
- Booted `results-phase2-client9`.
- Injected:
  - normalized pointer move,
  - pointer button press/release,
  - `send-key a`,
  - generic `input-send-event` key down/up for `a`.
- Compared the resulting `@@EVDEV` lines against the Wayland client logs.

### Why
- The prior evidence only showed that the Wayland client was not receiving visible input callbacks.
- The evdev probe creates an independent checkpoint below Weston and Wayland, which lets the remaining problem be localized rather than guessed at.

### What worked
- The probe starts correctly and opens the relevant devices:
  - `event3` / `event2` for `VirtualPS/2 VMware VMMouse`
  - `event1` for `AT Translated Set 2 keyboard`
  - `event0` for `Power Button`
- Pointer injection now has hard proof at the guest kernel layer:
  - absolute X and Y writes arrived on `event2`
  - button code `272` (`BTN_LEFT`) arrived on `event2`
- The client also eventually logged `seat-capabilities=3`, which confirms both keyboard and pointer capability bits are present on the seat.

### What didn't work
- Even with evdev proof for pointer input, the Wayland client still does not log `pointer-enter`, `pointer-motion`, `pointer-button`, or `@@LOG kind=input`.
- Neither `send-key` nor generic `input-send-event` key injections produced visible evdev events on the keyboard device.

### What I learned
- The pointer path reaches the guest kernel successfully. The remaining pointer bug is above evdev.
- The keyboard path is still different enough that it has to be debugged separately from the pointer path.
- The boundary is now much clearer than before:
  - pointer: `QMP -> evdev` works, `evdev -> Weston/client` still unclear
  - keyboard: `QMP -> evdev` not yet demonstrated

### What was tricky to build
- The key challenge here was choosing the right instrumentation layer. More Wayland logging alone would not have told me whether QMP events were getting lost before Weston. The evdev probe solved that, but it required threading another compiled helper through the initramfs build and bootstrap flow while keeping the ticket-local script archive synchronized.

### What warrants a second pair of eyes
- Why pointer events visible at evdev still are not surfacing in the Wayland client.
- Why QMP keyboard injection is not reaching the guest keyboard evdev node in the same way pointer injection reaches the mouse evdev node.

### What should be done in the future
- Continue debugging pointer delivery at the Weston/client boundary.
- Investigate whether QMP keyboard injection needs a different route or device model than the current `send-key` and generic key event forms.
- Once input is visible in the client, move on to suspend/resume integration and measurements for phase 2.

### Code review instructions
- Start with [`guest/evdev_probe.c`](/home/manuel/code/wesen/2026-03-07--qemu-power-tick/guest/evdev_probe.c), [`guest/build-phase2-rootfs.sh`](/home/manuel/code/wesen/2026-03-07--qemu-power-tick/guest/build-phase2-rootfs.sh), and [`guest/init-phase2`](/home/manuel/code/wesen/2026-03-07--qemu-power-tick/guest/init-phase2).
- Then inspect `results-phase2-client9/guest-serial.log` for `@@EVDEV` and `seat-capabilities=3`.

### Technical details
- Probe startup markers:
  - `[init-phase2] evdev_probe pid=115`
  - `@@EVDEV open path=/dev/input/event3 name=VirtualPS/2 VMware VMMouse`
  - `@@EVDEV open path=/dev/input/event2 name=VirtualPS/2 VMware VMMouse`
  - `@@EVDEV open path=/dev/input/event1 name=AT Translated Set 2 keyboard`
- Pointer injection proof from `results-phase2-client9/guest-serial.log`:
  - `@@EVDEV path=/dev/input/event2 type=3 code=0 value=32768`
  - `@@EVDEV path=/dev/input/event2 type=3 code=1 value=32768`
  - `@@EVDEV path=/dev/input/event2 type=1 code=272 value=1`
- Capability proof from the same run:
  - `@@LOG kind=state seat-capabilities=3`

## Step 10: Pointer Delivery Fixed, Keyboard Hypothesis Shifted To USB Enumeration

The evdev probe exposed the key asymmetry, but the next fix showed the actual client-side bug for pointer delivery. The fallback keyboard and pointer objects were being created before Weston reported the real seat capabilities, and once `seat-capabilities=3` arrived the normal binding path never replaced those fallback handles. After changing the client to rebind on the real capability callback, injected pointer clicks started reaching the Wayland client properly.

That changes the diagnosis materially. Pointer input is now working end-to-end through QMP, evdev, Weston, and the client. Keyboard is still missing, but at this point it no longer makes sense to treat it as the same bug. The guest currently enumerates only the built-in AT keyboard, while the QEMU launch also includes `usb-kbd`, so the next plausible explanation is that QMP key injection is effectively targeting a USB keyboard path the guest is not yet bringing up.

### Prompt Context

**User prompt (verbatim):** "mark down session 019cc9de-fbce-7bc3-91dc-30a36f28f3ec in the diary for future reference.\n\nThen continue."

**Assistant interpretation:** Record the provided external session reference in the diary and continue implementation/debugging work immediately.

**Inferred user intent:** Keep the work traceable and preserve continuity while pushing forward on the remaining input issues.

**Commit (code):** N/A

### What I did
- Added the external session reference to the diary quick-reference block.
- Patched [`guest/wl_sleepdemo.c`](/home/manuel/code/wesen/2026-03-07--qemu-power-tick/guest/wl_sleepdemo.c) so fallback keyboard/pointer objects are replaced when the real `seat-capabilities` callback arrives:
  - `keyboard-rebound`
  - `pointer-rebound`
- Mirrored those changes into [`../scripts/wl_sleepdemo.c`](../scripts/wl_sleepdemo.c).
- Rebuilt and booted `results-phase2-client10` with the evdev probe still enabled.
- Re-ran normalized pointer motion and click injection.
- Confirmed the client now logs pointer button events after seat rebinding.
- Checked the guest logs for keyboard input again and confirmed it is still absent.
- Audited the current guest boot log and found that only the AT keyboard is enumerated.
- Patched the phase-2 image and init path to add:
  - `xhci-pci.ko`
  - `usbkbd.ko`
  - `usbmouse.ko`
- Fixed a shell continuation bug in the `init-phase2` module list after adding those modules.

### Why
- The evdev evidence proved pointer events were reaching the guest kernel, which made the old fallback-binding behavior a credible client-side failure point.
- The remaining keyboard issue now has a better hardware hypothesis than before: the emulated USB keyboard path is not visible in the guest yet.

### What worked
- After the rebind fix, the client logs:
  - `seat-capabilities=3`
  - `keyboard-rebound`
  - `pointer-rebound`
  - `@@LOG kind=input BUTTON 272 STATE 1`
  - `@@LOG kind=input BUTTON 272 STATE 0`
- Pointer input therefore now works through the full stack.

### What didn't work
- Keyboard input is still not visible in the client.
- `send-key` and the generic key event form still did not show keyboard activity in the current guest logs.

### What I learned
- Pointer and keyboard were not blocked at the same layer.
- The fallback-seat-object behavior was a real client bug for pointer delivery.
- Once that was fixed, keyboard stood out as a separate enumeration/injection problem rather than a general “Wayland input is broken” problem.

### What was tricky to build
- The tricky part here was avoiding over-generalization from the earlier “no input visible” symptoms. Once pointer events were proven at evdev, it became tempting to keep patching the client broadly. The better move was narrower: fix the specific fallback-binding lifecycle first, then re-evaluate what still failed. That immediately separated the pointer and keyboard problems and kept the debugging path disciplined.

### What warrants a second pair of eyes
- Whether QEMU `send-key` in this environment is meant to target the USB keyboard device created by `usb-kbd`, the legacy PS/2 keyboard path, or both.
- Whether the USB keyboard path needs more than `xhci-pci`, `usbkbd`, and `usbmouse` to enumerate cleanly in this minimal guest.

### What should be done in the future
- Reboot with the new USB input modules included.
- Confirm whether a USB keyboard input node appears in the guest and in Weston’s log.
- Retry `send-key` only after that enumeration check.

### Code review instructions
- Start with the rebind logic in [`guest/wl_sleepdemo.c`](/home/manuel/code/wesen/2026-03-07--qemu-power-tick/guest/wl_sleepdemo.c).
- Then inspect `results-phase2-client10/guest-serial.log` for:
  - `seat-capabilities=3`
  - `keyboard-rebound`
  - `pointer-rebound`
  - `@@LOG kind=input BUTTON 272 STATE ...`
- Finally review the new USB-input module additions in [`guest/build-phase2-rootfs.sh`](/home/manuel/code/wesen/2026-03-07--qemu-power-tick/guest/build-phase2-rootfs.sh) and [`guest/init-phase2`](/home/manuel/code/wesen/2026-03-07--qemu-power-tick/guest/init-phase2).

### Technical details
- Pointer end-to-end proof from `results-phase2-client10/guest-serial.log`:
  - `@@LOG kind=state seat-capabilities=3`
  - `@@LOG kind=state keyboard-rebound`
  - `@@LOG kind=state pointer-rebound`
  - `@@LOG kind=input BUTTON 272 STATE 1`
  - `@@LOG kind=input BUTTON 272 STATE 0`
- Current keyboard enumeration clue from the same debugging slice:
  - the guest log still only showed `AT Translated Set 2 keyboard`, not a USB keyboard device

## Step 11: Full Input Path Working After USB Keyboard Enumeration

The last missing input piece turned out to be the guest’s USB host path, not another Wayland bug. Once the xHCI controller and its Renesas helper were included, the guest finally enumerated the QEMU USB keyboard and tablet. After that, QMP key injection started producing real key events in the Wayland client log, which closed the main phase-2 host-input loop.

At this point the situation is much cleaner than it was earlier in the day. Pointer input had already been proven end-to-end after the seat-rebind fix. This step added the missing keyboard side, which means the guest now has a working compositor, a custom client, host screenshots, host pointer input, and host keyboard input, all in the same phase-2 environment.

### Prompt Context

**User prompt (verbatim):** (see Step 10)

**Assistant interpretation:** Continue the debugging work until the remaining input issue is resolved or tightly narrowed.

**Inferred user intent:** Reach a working phase-2 interactive stack rather than stopping at partial input success.

**Commit (code):** N/A

### What I did
- Determined from `modinfo` that `xhci-pci.ko` depends on `xhci-pci-renesas`.
- Patched [`guest/build-phase2-rootfs.sh`](/home/manuel/code/wesen/2026-03-07--qemu-power-tick/guest/build-phase2-rootfs.sh) and [`guest/init-phase2`](/home/manuel/code/wesen/2026-03-07--qemu-power-tick/guest/init-phase2) to include and load `xhci-pci-renesas.ko` before `xhci-pci.ko`.
- Mirrored those changes into the ticket-local script copies.
- Rebuilt and booted `results-phase2-client12`.
- Verified that the guest now enumerates:
  - `QEMU USB Keyboard`
  - `QEMU USB Tablet`
- Re-ran:
  - `send-key --key a`
  - generic `input-send-event` key down/up for `a`
- Inspected the client log for actual key delivery.

### Why
- The previous attempt loaded `xhci-pci.ko` alone, but it failed with an unresolved Renesas helper symbol, so the USB keyboard never had a chance to enumerate.
- Without the emulated USB keyboard present in the guest, it was not reasonable to expect QMP key injection to match the guest-visible device model.

### What worked
- The xHCI path now comes up successfully.
- The guest enumerates:
  - `QEMU QEMU USB Keyboard`
  - `QEMU QEMU USB Tablet`
- The Wayland client now logs keyboard input:
  - `@@LOG kind=input KEY=30 STATE=1`
  - `@@LOG kind=input KEY=30 STATE=0`
- Combined with the earlier pointer fix, the host-to-client input path is now working for both keyboard and pointer.

### What didn't work
- The evdev probe still only watches the devices present at its startup time, so it does not automatically open hotplugged USB input devices yet. That did not block the keyboard confirmation because the client logs were enough, but it is still a limitation of the current probe.

### What I learned
- The keyboard problem was not a duplicate of the pointer problem. It depended on the guest actually enumerating the emulated USB keyboard path exposed by QEMU.
- The combination of layered instrumentation paid off:
  - evdev probe for raw guest input,
  - Weston log for compositor/device discovery,
  - `wl_sleepdemo` logs for actual client delivery.

### What was tricky to build
- The subtle part was separating dependency failures from behavioral failures. The first USB attempt looked promising because `usbkbd` and `usbmouse` registered, but that still was not enough without the xHCI host controller loading successfully. The unresolved-symbol error on `xhci-pci.ko` would have been easy to overlook if I had focused only on the later Weston and client logs.

### What warrants a second pair of eyes
- Whether the evdev probe should be upgraded to rescan or subscribe to hotplugged input devices so it can observe the USB keyboard/tablet directly.
- Whether the final phase-2 harness should explicitly wait for USB input enumeration before running input checkpoints.

### What should be done in the future
- Capture the required screenshot and log checkpoints now that keyboard and pointer input both work.
- Commit this input-complete milestone.
- Move on to suspend/resume integration and phase-2 timing measurements.

### Code review instructions
- Start with the USB host/module additions in [`guest/build-phase2-rootfs.sh`](/home/manuel/code/wesen/2026-03-07--qemu-power-tick/guest/build-phase2-rootfs.sh) and [`guest/init-phase2`](/home/manuel/code/wesen/2026-03-07--qemu-power-tick/guest/init-phase2).
- Then inspect `results-phase2-client12/guest-serial.log` for:
  - `Product: QEMU USB Keyboard`
  - `Product: QEMU USB Tablet`
  - `@@LOG kind=input KEY=30 STATE=1`
  - `@@LOG kind=input KEY=30 STATE=0`

### Technical details
- Successful USB input enumeration markers from `results-phase2-client12/guest-serial.log`:
  - `input: QEMU QEMU USB Keyboard as .../input/input5`
  - `input: QEMU QEMU USB Tablet as .../input/input6`
- Keyboard delivery proof from the same run:
  - `@@LOG kind=input KEY=30 STATE=1`
  - `@@LOG kind=input KEY=30 STATE=0`
  - repeated twice across the tested key injection forms

## Step 12: Wrote A Reusable Input Bring-Up Playbook

Once the input path was working, I turned the debugging trail into a focused playbook for the next person doing the lab. The point was to preserve the shortest successful route, not force someone to reconstruct it from the diary alone. This is especially useful here because the input stack failed at several different layers during bring-up, and the order of fixes mattered.

The playbook captures the required module set, the required udev/libinput userspace, the bootstrap order, the known failure modes, the log markers that prove each layer is healthy, and copy/paste validation commands. That should make the phase-2 handoff materially better than a purely narrative report.

### Prompt Context

**User prompt (verbatim):** "write a playbook on how to get the mouse and the keyboard to work, to help the person who does the lab next. store in ticket as well."

**Assistant interpretation:** Turn the resolved input-debugging path into a dedicated reference document in the ticket so another engineer can reproduce the working keyboard and mouse setup quickly.

**Inferred user intent:** Preserve the operational knowledge as a reusable guide, not just as implementation history.

**Commit (code):** N/A

### What I did
- Added the new reference doc [02-input-bring-up-playbook.md](/home/manuel/code/wesen/2026-03-07--qemu-power-tick/ttmp/2026/03/07/QEMU-02-LAB-TWO--qemu-lab-two-wayland-client-analysis-and-implementation/reference/02-input-bring-up-playbook.md).
- Wrote the playbook with:
  - required kernel modules,
  - required userspace payload,
  - ordered diagnosis steps,
  - concrete log markers,
  - known failure modes and fixes,
  - exact commands for rebuild, boot, and validation.
- Linked the playbook from the ticket [index.md](/home/manuel/code/wesen/2026-03-07--qemu-power-tick/ttmp/2026/03/07/QEMU-02-LAB-TWO--qemu-lab-two-wayland-client-analysis-and-implementation/index.md).
- Added the playbook creation to the ticket [changelog.md](/home/manuel/code/wesen/2026-03-07--qemu-power-tick/ttmp/2026/03/07/QEMU-02-LAB-TWO--qemu-lab-two-wayland-client-analysis-and-implementation/changelog.md).

### Why
- The diary is good for chronology, but it is not the fastest operational reference for someone trying to get keyboard and mouse working from scratch.
- The input bring-up path had enough dead ends that a compact, prescriptive guide is worth keeping separately.

### What worked
- The ticket now has a dedicated, discoverable input playbook rather than burying the procedure entirely in diary entries.

### What didn't work
- N/A

### What I learned
- Once a subsystem is finally working, the highest-value documentation is usually a task-focused playbook rather than another narrative recap.

### What was tricky to build
- The tricky part was deciding what to include. I kept the playbook focused on reproducible steps and failure signatures, not the full exploration history, since the diary already covers that.

### What warrants a second pair of eyes
- Whether the playbook should eventually grow one short section on suspend/resume input validation once phase-2 suspend is integrated.

### What should be done in the future
- Keep the playbook updated if the input bootstrap or harness changes.
- Reference it in the final phase-2 report as the operational appendix for input bring-up.

### Code review instructions
- Read [02-input-bring-up-playbook.md](/home/manuel/code/wesen/2026-03-07--qemu-power-tick/ttmp/2026/03/07/QEMU-02-LAB-TWO--qemu-lab-two-wayland-client-analysis-and-implementation/reference/02-input-bring-up-playbook.md).
- Confirm it matches the currently working input path reflected in `results-phase2-client12/guest-serial.log`.

### Technical details
- Ticket links updated:
  - [index.md](/home/manuel/code/wesen/2026-03-07--qemu-power-tick/ttmp/2026/03/07/QEMU-02-LAB-TWO--qemu-lab-two-wayland-client-analysis-and-implementation/index.md)
  - [changelog.md](/home/manuel/code/wesen/2026-03-07--qemu-power-tick/ttmp/2026/03/07/QEMU-02-LAB-TWO--qemu-lab-two-wayland-client-analysis-and-implementation/changelog.md)

## Step 13: Froze The Input-Complete Milestone Before Suspend Work

Before touching suspend and timing logic, I stopped to formalize the current phase-2 checkpoint. That matters here because the stack is finally interactive end-to-end: Weston boots, the custom client renders, host screenshots work, pointer input works, and keyboard input works. This is the first point where phase 2 has a clean rollback target if the suspend integration destabilizes the guest.

I also tightened the task list so it reflects reality instead of the earlier bring-up uncertainty. The QMP harness work is no longer speculative, and the screenshot task is now split so the suspend-dependent artifacts stay separate from the already-working input and rendering checkpoints.

### Prompt Context

**User prompt (verbatim):** "continue."

**Assistant interpretation:** Continue the phase-2 execution plan from the working input state, with proper milestone hygiene before starting the next risky subsystem.

**Inferred user intent:** Preserve progress in a disciplined way and then move directly into suspend/resume and measurements.

**Commit (code):** Pending at this step; this diary entry exists to describe the checkpoint that will be committed immediately after the ticket docs are updated.

### What I did
- Updated [tasks.md](/home/manuel/code/wesen/2026-03-07--qemu-power-tick/ttmp/2026/03/07/QEMU-02-LAB-TWO--qemu-lab-two-wayland-client-analysis-and-implementation/tasks.md) to:
  - mark host-side QMP screenshot and input automation complete,
  - split non-suspend checkpoint capture from the later suspend/resume checkpoint capture,
  - add an explicit milestone-commit task before suspend work.
- Reviewed the staged phase-2 diff to confirm the commit boundary is coherent and limited to the input-complete milestone plus ticket documentation.
- Chose suspend/resume integration in `wl_sleepdemo` as the next implementation block after this checkpoint is committed.

### Why
- Suspend integration is the next risky change and it touches the main phase-2 client loop, timing model, and validation harness.
- A clean input-complete milestone makes regression isolation much easier if the next iteration breaks rendering, focus, reconnects, or input delivery.

### What worked
- The ticket tasks now match the actual state of the implementation.
- The staged diff boundary is coherent enough for a dedicated milestone commit rather than mixing it into later suspend logic.

### What didn't work
- N/A

### What I learned
- The value of the diary is not only preserving failures. It also helps force sensible cut points for commits and report structure, especially in a debugging-heavy lab where otherwise everything blurs together.

### What was tricky to build
- The subtle part was deciding where the milestone boundary should be. It would have been easy to keep coding into suspend immediately, but that would have mixed two different problem spaces: input/platform bring-up and suspend/timing behavior. Splitting them now keeps the later report and measurements easier to trust.

### What warrants a second pair of eyes
- Whether the eventual suspend harness should live primarily in `wl_sleepdemo` or be driven more heavily from the host-side QMP harness.

### What should be done in the future
- Commit this milestone.
- Port the stage-1 suspend/metric model into `wl_sleepdemo`.
- Re-run the phase-2 checkpoint matrix with suspend-aware artifacts and measurements.

### Code review instructions
- Review [tasks.md](/home/manuel/code/wesen/2026-03-07--qemu-power-tick/ttmp/2026/03/07/QEMU-02-LAB-TWO--qemu-lab-two-wayland-client-analysis-and-implementation/tasks.md) first.
- Then inspect the staged phase-2 changes and confirm the current milestone cleanly represents “input-complete, suspend not yet integrated.”

### Technical details
- The current stable working inputs are evidenced by:
  - `results-phase2-client10/guest-serial.log` for pointer button delivery
  - `results-phase2-client12/guest-serial.log` for USB keyboard enumeration and key delivery

## Step 14: Ported Suspend And Metric Logging Into The Modularized Phase-2 Client

After the cleanup ticket split the monolith into smaller modules, I returned to phase 2 and added the first real suspend-aware client path on top of that new structure. This was intentionally done as another modular step rather than shoving the stage-1 suspend logic back into a single large file. The result is a new `wl_suspend` module plus hook points in the render, networking, input, and init paths.

The first outcome is real progress, not just structural work. The phase-2 guest now reaches `pm_test=devices` suspend, resumes, emits `@@METRIC` lines for sleep interval and redraw latency, and still accepts keyboard and pointer input after resume. The main thing still missing from this slice is a clean `resume_to_reconnect` measurement; the scenarios I ran either kept the socket alive across suspend or started the server too late to make the latency number meaningful.

### Prompt Context

**User prompt (verbatim):** "continue. keep diary and check tasks and commit at regular intervals"

**Assistant interpretation:** Resume the phase-2 implementation from the modularized codebase, keep the phase-2 ticket updated, and make progress in coherent, committed increments.

**Inferred user intent:** Move forward on the actual suspend/measurement requirements while keeping the work traceable and disciplined.

**Commit (code):** Pending at this step

### What I did
- Added the new suspend module:
  - [guest/wl_suspend.h](/home/manuel/code/wesen/2026-03-07--qemu-power-tick/guest/wl_suspend.h)
  - [guest/wl_suspend.c](/home/manuel/code/wesen/2026-03-07--qemu-power-tick/guest/wl_suspend.c)
- Expanded [guest/wl_app_core.h](/home/manuel/code/wesen/2026-03-07--qemu-power-tick/guest/wl_app_core.h) and [guest/wl_app_core.c](/home/manuel/code/wesen/2026-03-07--qemu-power-tick/guest/wl_app_core.c) with:
  - suspend-cycle metrics state,
  - monotonic and boottime helpers,
  - `@@METRIC` emission.
- Updated:
  - [guest/wl_render.c](/home/manuel/code/wesen/2026-03-07--qemu-power-tick/guest/wl_render.c)
  - [guest/wl_net.c](/home/manuel/code/wesen/2026-03-07--qemu-power-tick/guest/wl_net.c)
  - [guest/wl_wayland.c](/home/manuel/code/wesen/2026-03-07--qemu-power-tick/guest/wl_wayland.c)
  - [guest/wl_sleepdemo.c](/home/manuel/code/wesen/2026-03-07--qemu-power-tick/guest/wl_sleepdemo.c)
  - [guest/build-wl-sleepdemo.sh](/home/manuel/code/wesen/2026-03-07--qemu-power-tick/guest/build-wl-sleepdemo.sh)
  - [guest/init-phase2](/home/manuel/code/wesen/2026-03-07--qemu-power-tick/guest/init-phase2)
- Mirrored all of those files into the phase-2 ticket `scripts/` directory.
- Built and tested:
  - `guest/build-wl-sleepdemo.sh build build/wl_sleepdemo`
  - `guest/build-phase2-rootfs.sh build build/initramfs-phase2-suspend.cpio.gz build/wl_sleepdemo`
  - `guest/run-qemu-phase2.sh --kernel build/vmlinuz --initramfs build/initramfs-phase2-suspend.cpio.gz --results-dir results-phase2-suspend1`
  - `guest/run-qemu-phase2.sh --kernel build/vmlinuz --initramfs build/initramfs-phase2-suspend.cpio.gz --results-dir results-phase2-suspend3`
- Re-ran post-resume input checks with QMP key and pointer injection.

### Why
- The phase-2 assignment is not complete until the Wayland client survives the suspend path and produces timing data, not just screenshots and input.
- Doing the suspend work after the modularization pass keeps the complexity contained in one dedicated module instead of regrowing the earlier monolith.

### What worked
- `pm_test=devices` suspend/resume now works in the phase-2 guest.
- The guest emits:
  - `@@METRIC name=sleep_duration ...`
  - `@@METRIC name=suspend_resume_gap ...`
  - `@@METRIC name=resume_to_redraw ...`
- On `results-phase2-suspend3`, post-resume input still works:
  - `seat-capabilities=3`
  - `pointer-rebound`
  - `BUTTON 272 STATE 1`
  - `BUTTON 272 STATE 0`
  - `KEY=30 STATE=1`
  - `KEY=30 STATE=0`

### What didn't work
- A clean `resume_to_reconnect` metric is still missing from the meaningful measurement runs.
- The first reconnect attempt was invalid because I started the host server minutes after resume, which produced a meaningless latency number.
- The first “server active before boot” attempt kept the guest awake because packet traffic continually reset the idle timer.
- The delayed-server attempts were tricky to time cleanly enough to get both suspend and a short reconnect window in one run.

### What I learned
- The `pm_test=devices` path is stable enough to measure redraw and input continuity before attempting more ambitious suspend cases.
- Reconnect measurement is much more about scenario control than raw code correctness. The client logic is there, but the host timing has to be shaped so the socket is actually absent at resume and then available shortly after.

### What was tricky to build
- The sharp edge here was that the networking and suspend logic interact indirectly through time, not through one obvious code path. If the host sends packets too long, the idle timer never fires and suspend does not happen. If the host comes back too late, the reconnect metric exists but is not meaningful. Getting a scientifically useful reconnect number requires a controlled host-side scenario, not just “turn the server on at some point.”

### What warrants a second pair of eyes
- Whether `pm_test=devices` is the right default checkpoint for phase 2, or whether I should now pivot directly to `pm_test=freezer` or a more realistic path once the reconnect scenario is nailed down.
- Whether the reconnect timing scenario should become a dedicated host harness script instead of more ad hoc shell timing.

### What should be done in the future
- Commit this suspend-and-metrics slice.
- Add a small host-side timing harness that starts the drip server in a controlled post-resume window.
- Capture named screenshots around suspend/resume and parse the metrics into a summary file.

### Code review instructions
- Start with the new suspend module:
  - [guest/wl_suspend.c](/home/manuel/code/wesen/2026-03-07--qemu-power-tick/guest/wl_suspend.c)
  - [guest/wl_suspend.h](/home/manuel/code/wesen/2026-03-07--qemu-power-tick/guest/wl_suspend.h)
- Then review the hook points in:
  - [guest/wl_sleepdemo.c](/home/manuel/code/wesen/2026-03-07--qemu-power-tick/guest/wl_sleepdemo.c)
  - [guest/wl_net.c](/home/manuel/code/wesen/2026-03-07--qemu-power-tick/guest/wl_net.c)
  - [guest/wl_wayland.c](/home/manuel/code/wesen/2026-03-07--qemu-power-tick/guest/wl_wayland.c)
  - [guest/wl_render.c](/home/manuel/code/wesen/2026-03-07--qemu-power-tick/guest/wl_render.c)
- Validate with:
  - `guest/build-wl-sleepdemo.sh build build/wl_sleepdemo`
  - `guest/build-phase2-rootfs.sh build build/initramfs-phase2-suspend.cpio.gz build/wl_sleepdemo`
  - `guest/run-qemu-phase2.sh --kernel build/vmlinuz --initramfs build/initramfs-phase2-suspend.cpio.gz --results-dir results-phase2-suspend3`
  - `python3 host/qmp_harness.py --socket results-phase2-suspend3/qmp.sock send-key --key a`
  - `python3 host/qmp_harness.py --socket results-phase2-suspend3/qmp.sock pointer-button --button left --down`
  - `python3 host/qmp_harness.py --socket results-phase2-suspend3/qmp.sock pointer-button --button left`

### Technical details
- `results-phase2-suspend1/guest-serial.log` first proved the phase-2 suspend path:
  - `state=SUSPENDING cycle=1`
  - `state=RESUMED cycle=1 sleep_ms=5775 resume_gap_ms=5775`
  - `@@METRIC name=sleep_duration value_ms=5775 cycle=1`
  - `@@METRIC name=suspend_resume_gap value_ms=5775 cycle=1`
  - `@@METRIC name=resume_to_redraw value_ms=4 cycle=1`
- `results-phase2-suspend3/guest-serial.log` proved post-resume seat/input recovery:
  - `seat-capabilities=0`
  - `seat-capabilities=2`
  - `keyboard-rebound`
  - `seat-capabilities=3`
  - `pointer-rebound`
  - `BUTTON 272 STATE 1`
  - `BUTTON 272 STATE 0`
  - `KEY=30 STATE=1`
  - `KEY=30 STATE=0`

## Step 15: Added A Resume-Triggered Host Server To Get A Clean Reconnect Metric

The reconnect problem turned out not to be another guest bug. It was a measurement-orchestration bug. If the host drip server ran too long before suspend, packet traffic kept resetting the idle timer and the guest never slept. If the server started too late after resume, the reconnect metric existed but did not mean anything useful. The fix was to stop treating reconnect timing as an ad hoc shell timing problem and make it an explicit host-side scenario.

I added a small watcher script that waits for `state=RESUMED` in the guest serial log and only then starts listening on the host port. That gave the guest a clean “disconnected during suspend, server appears after resume” case and finally produced a meaningful `resume_to_reconnect` value.

### Prompt Context

**User prompt (verbatim):** (see Step 14)

**Assistant interpretation:** Keep going on the phase-2 suspend/measurement work until the reconnect timing gap is closed and the ticket reflects the actual measured state.

**Inferred user intent:** Finish the useful timing data, not just partial suspend plumbing.

**Commit (code):** Pending at this step

### What I did
- Added [host/resume_drip_server.py](/home/manuel/code/wesen/2026-03-07--qemu-power-tick/host/resume_drip_server.py).
- Mirrored it into the phase-2 ticket scripts directory:
  - [scripts/resume_drip_server.py](/home/manuel/code/wesen/2026-03-07--qemu-power-tick/ttmp/2026/03/07/QEMU-02-LAB-TWO--qemu-lab-two-wayland-client-analysis-and-implementation/scripts/resume_drip_server.py)
- Used it with:
  - `python3 host/resume_drip_server.py --serial-log results-phase2-suspend6/guest-serial.log --runtime 12`
  - `guest/run-qemu-phase2.sh --kernel build/vmlinuz --initramfs build/initramfs-phase2-suspend.cpio.gz --results-dir results-phase2-suspend6`
- Parsed the resulting metrics with:
  - `python3 scripts/measure_run.py --serial-log results-phase2-suspend6/guest-serial.log --json-out results-phase2-suspend6/metrics.json`
- Captured a post-resume screenshot:
  - `python3 host/qmp_harness.py --socket results-phase2-suspend6/qmp.sock screendump --file results-phase2-suspend6/02-post-resume.ppm`

### Why
- The remaining missing measurement was not in the guest anymore. It was in the way the host test scenario was being timed.
- A host watcher script is cheaper and more repeatable than continuing to guess sleep offsets with shell `sleep` commands.

### What worked
- `results-phase2-suspend6/guest-serial.log` now contains:
  - `@@METRIC name=sleep_duration value_ms=5755 cycle=1`
  - `@@METRIC name=suspend_resume_gap value_ms=5755 cycle=1`
  - `@@METRIC name=resume_to_redraw value_ms=5 cycle=1`
  - `@@METRIC name=resume_to_reconnect value_ms=1243 cycle=1`
- The measurement JSON at `results-phase2-suspend6/metrics.json` matches those values.
- The post-resume screenshot was captured successfully.

### What didn't work
- The first reconnect attempts failed for measurement purposes:
  - always-on server kept the guest awake,
  - late-start server gave a meaningless reconnect latency,
  - some delayed-start runs still raced with boot timing and connected too early.
- Those failures are exactly why the dedicated watcher script was worth adding.

### What I learned
- For this lab, reconnect latency is fundamentally a scenario-definition problem. Once the host behavior is defined cleanly, the guest metric falls out naturally.
- The best fix was not a more complicated guest state machine. It was a better host harness.

### What was tricky to build
- The tricky part was avoiding false confidence from bad numbers. A reconnect metric can exist and still be useless if the host server timing does not correspond to the scenario we claim to be measuring. The watcher script solves that by tying server start to an observable guest event instead of a guessed wall-clock delay.

### What warrants a second pair of eyes
- Whether the final report should treat `pm_test=devices` as the main measured baseline and keep “real freeze wake path” as a limitations section, which currently seems like the honest framing for this environment.

### What should be done in the future
- Record the final commit hash for this host harness slice.
- Capture the remaining screenshot set around earlier checkpoints.
- Write the phase-2 final report with the measured numbers and the reconnect-scenario explanation.

### Code review instructions
- Read [host/resume_drip_server.py](/home/manuel/code/wesen/2026-03-07--qemu-power-tick/host/resume_drip_server.py).
- Then inspect:
  - `results-phase2-suspend6/guest-serial.log`
  - `results-phase2-suspend6/metrics.json`
  - `results-phase2-suspend6/02-post-resume.ppm`
- Reproduce with:
  - `python3 host/resume_drip_server.py --serial-log results-phase2-suspend6/guest-serial.log --runtime 12`
  - `guest/run-qemu-phase2.sh --kernel build/vmlinuz --initramfs build/initramfs-phase2-suspend.cpio.gz --results-dir results-phase2-suspend6`
  - `python3 scripts/measure_run.py --serial-log results-phase2-suspend6/guest-serial.log --json-out results-phase2-suspend6/metrics.json`

### Technical details
- Clean measured values from `results-phase2-suspend6/metrics.json`:
  - `sleep_duration = 5755 ms`
  - `suspend_resume_gap = 5755 ms`
  - `resume_to_redraw = 5 ms`
  - `resume_to_reconnect = 1243 ms`

## Step 16: Applied The First Postmortem Cleanup Pass And Fixed The PID-1 Exit Bug

I took the review feedback seriously enough to validate it with a real boot instead of treating it as documentation-only cleanup. That turned out to be the right call. The “remove unconditional redraws” and “move scenario control out of `/init`” changes behaved as intended, but the validation run also exposed a correctness bug the postmortem had only identified as a risk: when `wl_sleepdemo` hit its runtime limit as PID 1, the guest panicked because init exited.

This step therefore became more valuable than a pure cleanup pass. It tightened the control boundary between host and guest, reduced artificial redraw activity, and converted a known risk into a concrete fix verified by a repeatable boot scenario.

### Prompt Context

**User prompt (verbatim):** "Ok, with that feedback in mind, we should go back to you, the little brother, and have you continue."

**Assistant interpretation:** Resume phase-2 implementation while acting on the postmortem feedback instead of just writing about it.

**Inferred user intent:** Improve the actual system based on the review findings, keep the ticket current, and avoid letting the postmortem become shelfware.

**Commit (code):** Pending at this step

### What I did
- Removed the unconditional redraw tick from [guest/wl_sleepdemo.c](/home/manuel/code/wesen/2026-03-07--qemu-power-tick/guest/wl_sleepdemo.c) so redraws are now driven by real state changes instead of a fixed timer.
- Changed [guest/init-phase2](/home/manuel/code/wesen/2026-03-07--qemu-power-tick/guest/init-phase2) to derive client suspend/runtime arguments from `/proc/cmdline` rather than hardcoding one scenario inside the init script.
- Extended [guest/run-qemu-phase2.sh](/home/manuel/code/wesen/2026-03-07--qemu-power-tick/guest/run-qemu-phase2.sh) with host-controlled `APPEND_EXTRA` and `--append-extra` support so the test scenario can be chosen outside the guest image.
- Booted a validation guest with:
  - `guest/build-wl-sleepdemo.sh build build/wl_sleepdemo`
  - `guest/build-phase2-rootfs.sh build build/initramfs-phase2-client.cpio.gz build/wl_sleepdemo`
  - `guest/run-qemu-phase2.sh --kernel build/vmlinuz --initramfs build/initramfs-phase2-client.cpio.gz --results-dir results-phase2-cleanup-validate --append-extra 'phase2_idle_seconds=3 phase2_max_suspend_cycles=1 phase2_wake_seconds=5 phase2_pm_test=devices phase2_runtime_seconds=18'`
- Verified from `results-phase2-cleanup-validate/guest-serial.log` that `/init` launched the client with cmdline-derived args and that suspend still worked.
- Observed the guest fail at runtime limit with:
  - `@@LOG kind=state runtime_limit_reached=true`
  - `Kernel panic - not syncing: Attempted to kill init! exitcode=0x00000000`
- Fixed that path in [guest/wl_suspend.c](/home/manuel/code/wesen/2026-03-07--qemu-power-tick/guest/wl_suspend.c) so PID 1 powers off the guest instead of simply returning.
- Rebuilt and reran the same scenario in `results-phase2-cleanup-validate2`, which now ends with:
  - `@@LOG kind=state runtime_limit_reached=true`
  - `@@LOG kind=state runtime_limit_action=poweroff`
  - `reboot: Power down`
- Mirrored the updated sources into the ticket-local `scripts/` directory:
  - `wl_sleepdemo.c`
  - `wl_suspend.c`
  - `init-phase2`
  - `run-qemu-phase2.sh`

### Why
- The postmortem called out concrete quality issues, and the right response was to either fix them or prove they were harmless.
- Moving scenario control to the host side makes later measurement runs much easier to vary without rebuilding the guest logic each time.
- The redraw change reduces background activity and makes future idle/sleep behavior easier to reason about.
- The PID-1 shutdown fix was required for correctness once runtime-limited validation runs were used as part of the workflow.

### What worked
- The cleaned-up control boundary works: `init-phase2` now prints and uses client args reconstructed from the kernel command line.
- `pm_test=devices` suspend still works after the cleanup.
- The validation rerun in `results-phase2-cleanup-validate2` produced:
  - `sleep_duration = 5757 ms`
  - `suspend_resume_gap = 5757 ms`
  - `resume_to_redraw = 2 ms`
- The runtime-limit path now powers down cleanly instead of triggering a kernel panic.

### What didn't work
- The first validation run after the cleanup failed at guest shutdown because PID 1 exited directly:
  - `Kernel panic - not syncing: Attempted to kill init! exitcode=0x00000000`
- That bug was not hypothetical. It only became visible once I used the new cmdline-driven runtime limit in a real booted guest.

### What I learned
- Review findings about “risk” are often worth validating immediately, because they can hide real correctness bugs.
- The host/guest scenario boundary matters a lot in this lab. Encoding test policy in the kernel command line is materially cleaner than freezing it into `/init`.
- Removing the forced redraw tick does not break suspend/resume in the current client path, which means the redraw loop was more policy than necessity.

### What was tricky to build
- The subtle part was that the cleanup looked behavior-preserving at first glance, so it would have been easy to skip a full boot and just commit the refactor. That would have missed the PID-1 failure entirely. The runtime-limit bug only manifests when the client is actually acting as init inside the guest, so ordinary local compilation or static review would not have caught it.

### What warrants a second pair of eyes
- Whether `runtime_limit_seconds` should remain a client concern at all, or whether long-run shutdown policy should eventually move to a thinner bootstrap or host harness.
- Whether any remaining periodic wakeups still exist elsewhere in the Wayland or network path and bias the later energy-style measurements.

### What should be done in the future
- Commit this cleanup checkpoint.
- Re-run the phase-2 screenshot matrix using the new host-driven scenario controls.
- Measure a few alternative idle/reconnect scenarios now that the redraw tick is gone.

### Code review instructions
- Start with:
  - [guest/wl_sleepdemo.c](/home/manuel/code/wesen/2026-03-07--qemu-power-tick/guest/wl_sleepdemo.c)
  - [guest/init-phase2](/home/manuel/code/wesen/2026-03-07--qemu-power-tick/guest/init-phase2)
  - [guest/run-qemu-phase2.sh](/home/manuel/code/wesen/2026-03-07--qemu-power-tick/guest/run-qemu-phase2.sh)
  - [guest/wl_suspend.c](/home/manuel/code/wesen/2026-03-07--qemu-power-tick/guest/wl_suspend.c)
- Then inspect:
  - `results-phase2-cleanup-validate/guest-serial.log`
  - `results-phase2-cleanup-validate2/guest-serial.log`
- Validate with:
  - `guest/build-wl-sleepdemo.sh build build/wl_sleepdemo`
  - `guest/build-phase2-rootfs.sh build build/initramfs-phase2-client.cpio.gz build/wl_sleepdemo`
  - `guest/run-qemu-phase2.sh --kernel build/vmlinuz --initramfs build/initramfs-phase2-client.cpio.gz --results-dir results-phase2-cleanup-validate2 --append-extra 'phase2_idle_seconds=3 phase2_max_suspend_cycles=1 phase2_wake_seconds=5 phase2_pm_test=devices phase2_runtime_seconds=18'`

### Technical details
- The key proof that scenario control moved out of `/init` is:
  - `[init-phase2] launching client=/bin/wl_sleepdemo args= --idle-seconds 3 --max-suspend-cycles 1 --wake-seconds 5 --pm-test devices --runtime-seconds 18`
- The failing shutdown sequence in `results-phase2-cleanup-validate/guest-serial.log` ended with:
  - `@@LOG kind=state runtime_limit_reached=true`
  - `Kernel panic - not syncing: Attempted to kill init! exitcode=0x00000000`
- The fixed shutdown sequence in `results-phase2-cleanup-validate2/guest-serial.log` ends with:
  - `@@LOG kind=state runtime_limit_reached=true`
  - `@@LOG kind=state runtime_limit_action=poweroff`
  - `reboot: Power down`
