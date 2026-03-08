---
Title: Stage 3 Postmortem And Review Guide
Ticket: QEMU-04-LAB-THREE
Status: active
Topics:
    - qemu
    - virtualization
    - lab
    - systems
DocType: design
Intent: long-term
Owners: []
RelatedFiles:
    - Path: guest/build-phase3-rootfs.sh
      Note: Rootfs assembly and payload-trimming strategy reviewed in the postmortem
    - Path: guest/init-phase3
      Note: Stage-3 orchestration complexity and suspend scheduling reviewed in the postmortem
    - Path: guest/suspendctl.c
      Note: Suspend helper and metrics path reviewed in the postmortem
    - Path: results-phase3-suspend-freezer1/guest-serial.log
      Note: Evidence for the pm_test=freezer passing continuity path
    - Path: results-phase3-suspend2/guest-serial.log
      Note: Evidence for the pm_test=devices virtio-gpu resume failure
ExternalSources: []
Summary: Evidence-based postmortem and review guide for the stage-3 Chromium kiosk work, covering result quality, struggles, strengths, gaps, and concrete recommendations for the next engineer.
LastUpdated: 2026-03-07T23:15:00-05:00
WhatFor: Assess the quality of the stage-3 result and explain the system, process, and next steps clearly enough for a follow-on engineer to continue confidently.
WhenToUse: Read this when reviewing the stage-3 work quality, onboarding a new engineer to the Chromium stage, or deciding what to improve next.
---


# Stage 3 Postmortem And Review Guide

## Executive Summary

This work is not fake progress and it is not garbage. It established a real stage-3 Chromium-on-Wayland guest, proved visible rendering under Weston, proved host-driven keyboard and pointer input into Chromium, and reintroduced suspend plumbing with measured results. Those are substantial outcomes.

The quality is mixed in a useful, normal engineering way:

- the runtime bring-up work is strong,
- the evidence discipline is better than average,
- the ticketing and diary discipline is strong in spirit but not perfect in bookkeeping,
- the stage remains incomplete because `pm_test=devices` still breaks visible display continuity after resume.

My overall judgment is:

- **Technical result quality:** good
- **Evidence quality:** good to very good
- **Process quality:** good, with some bookkeeping drift
- **Completion level for stage 3:** partial but meaningful

The strongest positive signal is that the engineer repeatedly replaced ambiguous validation with deterministic validation. The strongest negative signal is that some documentation bookkeeping drifted out of sync with the actual commits, and the current stage still has a real unresolved graphics-resume problem.

## Bottom-Line Verdict

If I had to summarize the stage-3 result in one sentence, it would be:

> The brother produced a credible stage-3 Chromium platform bring-up with reproducible evidence, but did not yet close the hardest device-resume bug.

That is a good place to be. It means the work reduced uncertainty and produced a usable base for the next iteration. It does not mean the assignment is fully finished.

## Findings

### Finding 1: The runtime platform work is real and technically meaningful

The current codebase really does boot a guest, launch Weston, launch Chromium with the Wayland backend, and render a visible browser surface. The key implementation path is:

- rootfs assembly in [guest/build-phase3-rootfs.sh](/home/manuel/code/wesen/2026-03-07--qemu-power-tick/guest/build-phase3-rootfs.sh#L9)
- guest orchestration in [guest/init-phase3](/home/manuel/code/wesen/2026-03-07--qemu-power-tick/guest/init-phase3#L1)
- browser launch policy in [guest/chromium-wayland-launcher.sh](/home/manuel/code/wesen/2026-03-07--qemu-power-tick/guest/chromium-wayland-launcher.sh#L1)

This is supported by the successful visible screenshot at [01-stage3.png](/home/manuel/code/wesen/2026-03-07--qemu-power-tick/results-phase3-smoke3/01-stage3.png) and by the serial-log evidence in [guest-serial.log](/home/manuel/code/wesen/2026-03-07--qemu-power-tick/results-phase3-smoke3/guest-serial.log).

### Finding 2: The engineer improved validation quality instead of settling for ambiguous screenshots

This is the single best process decision in the stage.

The engineer did not stop at:

- `about:blank`
- `chrome://version`
- ad hoc screenshots

Instead, they created a deterministic `data:` URL test page via [host/make_phase3_test_url.py](/home/manuel/code/wesen/2026-03-07--qemu-power-tick/host/make_phase3_test_url.py#L1) and a repeatable input harness via [host/capture_phase3_checkpoints.py](/home/manuel/code/wesen/2026-03-07--qemu-power-tick/host/capture_phase3_checkpoints.py#L1).

That changed the evidence model from “I think the browser got input” to “the input field visibly contains `hello` and the page visibly flips to `clicked` and green.” That is exactly the right move in a system lab where false positives are easy.

### Finding 3: Stage 3 is not complete because `pm_test=devices` still breaks visible continuity

This is the main unresolved technical issue.

The suspend helper is real and emits sane metrics in [guest/suspendctl.c](/home/manuel/code/wesen/2026-03-07--qemu-power-tick/guest/suspendctl.c#L88), but the stronger device-resume path is still broken. The best direct evidence is in [guest-serial.log](/home/manuel/code/wesen/2026-03-07--qemu-power-tick/results-phase3-suspend2/guest-serial.log#L616), which shows:

- xHCI resume errors
- root hub reset
- `virtio_gpu_dequeue_ctrl_func` DRM errors at [guest-serial.log](/home/manuel/code/wesen/2026-03-07--qemu-power-tick/results-phase3-suspend2/guest-serial.log#L619)

By contrast, the `freezer` path resumes cleanly in [guest-serial.log](/home/manuel/code/wesen/2026-03-07--qemu-power-tick/results-phase3-suspend-freezer1/guest-serial.log#L615) and preserves the rendered surface. So the brother did not “fake” suspend; they found a real split in the stack.

### Finding 4: Documentation discipline is strong, but commit bookkeeping drifted

The diary is detailed and useful, which is a real strength. But the commit references inside it are not fully trustworthy right now.

For example:

- Step 2 is the browser bring-up step, but the diary currently records the input-validation commit at [01-diary.md](/home/manuel/code/wesen/2026-03-07--qemu-power-tick/ttmp/2026/03/07/QEMU-04-LAB-THREE--qemu-lab-three-chromium-kiosk-analysis-and-implementation/reference/01-diary.md#L116)
- Step 3 is the input-validation step, but the diary currently records the suspend-plumbing commit at [01-diary.md](/home/manuel/code/wesen/2026-03-07--qemu-power-tick/ttmp/2026/03/07/QEMU-04-LAB-THREE--qemu-lab-three-chromium-kiosk-analysis-and-implementation/reference/01-diary.md#L226)
- Step 4 still shows a pending commit marker at [01-diary.md](/home/manuel/code/wesen/2026-03-07--qemu-power-tick/ttmp/2026/03/07/QEMU-04-LAB-THREE--qemu-lab-three-chromium-kiosk-analysis-and-implementation/reference/01-diary.md#L337)

This does not invalidate the engineering work. It does weaken traceability, especially if another engineer tries to reconstruct the exact change history from the diary alone.

### Finding 5: The stage-3 orchestration is pragmatic, but still shell-heavy and brittle

The rootfs builder and init script got the job done quickly. That was the right tradeoff for initial bring-up. But they are still fragile in ways that will slow future iterations.

Examples:

- The rootfs builder manually copies a curated Chromium payload in [guest/build-phase3-rootfs.sh](/home/manuel/code/wesen/2026-03-07--qemu-power-tick/guest/build-phase3-rootfs.sh#L57), which is fast and explicit but easy to drift out of sync with actual runtime needs.
- `init-phase3` parses the kernel command line by flattening values into a pipe-delimited string in [guest/init-phase3](/home/manuel/code/wesen/2026-03-07--qemu-power-tick/guest/init-phase3#L85), which works but is not robust or pleasant to extend.
- Background process supervision is minimal in [guest/init-phase3](/home/manuel/code/wesen/2026-03-07--qemu-power-tick/guest/init-phase3#L123), which is acceptable in a tiny initramfs but will become harder to reason about as more stage-3 behaviors are added.

This is not a criticism of the direction. It is a sign that the work has reached the point where another cleanup pass would pay off.

## What The Brother Did Well

### 1. Chose the right order of attack

The order was mostly correct:

1. packaging reality,
2. first visible render,
3. deterministic input validation,
4. suspend integration,
5. root-cause narrowing on device-resume failure.

That is a pragmatic sequence because each step reduces a major unknown before layering on the next one.

### 2. Replaced ambiguity with determinism

This happened repeatedly:

- `about:blank` replaced by `chrome://version`
- generic Chromium UI replaced by a deterministic `data:` URL page
- manual input experiments replaced by a saved harness
- suspend intuition replaced by measured `sleep_duration` and `suspend_resume_gap`

That is senior-engineer behavior. It is the most convincing sign that the result is worth trusting.

### 3. Preserved evidence

The brother kept:

- result directories,
- serial logs,
- screenshots,
- ticket-local script copies,
- diary entries,
- commit history.

That matters more than people think. A lab like this becomes unreviewable very quickly if the evidence is lost.

### 4. Found a real failure mode instead of papering over it

The `pm_test=devices` failure is inconvenient, but the work did not hide it. The brother separated:

- “suspend helper works”
- “freezer path works”
- “device resume path still broken”

That honesty is important. The current stage is useful exactly because it shows the real boundary of what is known.

## Where The Brother Struggled

### 1. Packaging and initramfs size

The first stage-3 image failed because the rootfs packaging strategy started too broad. That was visible in the oversized initramfs and the early-boot failure described in the diary.

This was a good struggle, not a bad one. It forced the engineer to:

- measure actual directory sizes,
- trim assets explicitly,
- think about what Chromium really needs.

### 2. UI ambiguity

The early screenshots were too ambiguous to carry strong conclusions. The brother recognized that and corrected course. Again, this is the right kind of struggle: the validation method improved rather than the evidence being oversold.

### 3. Suspend screenshot timing and graphics continuity

This is the hardest current struggle.

The suspend work revealed two different problems:

- harness timing can produce misleading captures if the screenshot is taken too close to a transition,
- the `devices` path appears to have a real graphics-resume problem beyond harness timing.

The second one is the actual blocker. The first one made the second one harder to interpret at first.

### 4. Documentation and commit bookkeeping

The brother clearly tried to keep the diary current, but the commit backfills drifted. That suggests the documentation loop was running slightly behind the code loop. This is a very common failure mode in long, fast iterative sessions.

## What The Brother Could Improve

### 1. Keep commit attribution aligned with diary steps

This is the biggest process improvement to make immediately.

Recommended rule:

```text
finish coherent code slice
commit code
record exact commit hash in the matching diary step immediately
only then start the next slice
```

The current diary is good, but its auditability would be much better if each step’s commit reference were updated before moving on.

### 2. Move more orchestration out of `init-phase3`

Right now, [guest/init-phase3](/home/manuel/code/wesen/2026-03-07--qemu-power-tick/guest/init-phase3#L1) is doing too many jobs:

- filesystem boot,
- device bring-up,
- environment setup,
- Weston launch,
- Chromium launch,
- cmdline parsing,
- suspend scheduling,
- runtime-limit shutdown.

That is acceptable for a bring-up script, but it is already close to the point where debugging becomes slower than modularizing.

The next cleanup direction should look like:

```text
init-phase3
  -> boot_env.sh
  -> launch_weston.sh
  -> launch_chromium.sh
  -> schedule_suspend.sh or suspendctl
```

or, if staying in C for critical parts:

```text
init-phase3
  -> one small launcher binary for policy and supervision
```

### 3. Unify the host harness layer

There are now multiple host-side helpers:

- [host/qmp_harness.py](/home/manuel/code/wesen/2026-03-07--qemu-power-tick/host/qmp_harness.py)
- [host/capture_phase3_checkpoints.py](/home/manuel/code/wesen/2026-03-07--qemu-power-tick/host/capture_phase3_checkpoints.py#L1)
- [host/capture_phase3_suspend_checkpoints.py](/home/manuel/code/wesen/2026-03-07--qemu-power-tick/host/capture_phase3_suspend_checkpoints.py#L1)

That is fine for now, but the shared patterns are obvious:

- wait for socket,
- send QMP actions,
- capture screenshots,
- compare images,
- wait on serial-log markers.

These should eventually become one small reusable harness layer with subcommands, not a growing family of adjacent scripts.

### 4. Distinguish “known good” from “known sensitive” defaults

The suspend screenshot helper still encodes timing assumptions in [host/capture_phase3_suspend_checkpoints.py](/home/manuel/code/wesen/2026-03-07--qemu-power-tick/host/capture_phase3_suspend_checkpoints.py#L53). That is not wrong, but the doc or script should explicitly say:

- which defaults are empirically stable,
- which ones are merely convenient first guesses,
- which scenarios require overriding them.

## What We Could Improve In The Lab

The lab itself is mostly good, but it could be improved in ways that would reduce wasted effort without removing the interesting systems work.

### 1. Make the stage boundaries more explicit

The lab should say more clearly:

- stage 2 proves the Wayland platform,
- stage 3 proves the Chromium platform on top of it,
- the wake-study is a separate investigation track, not a prerequisite for first browser bring-up.

This would reduce back-and-forth about Weston versus Chromium responsibilities.

### 2. Give a recommended validation ladder

The lab would benefit from explicitly recommending:

1. render a non-blank internal or deterministic page,
2. validate keyboard with an autofocus input,
3. validate pointer with a visible DOM change,
4. validate `freezer`,
5. validate `devices`,
6. only then move to wake-study experiments.

That ladder mirrors what ended up working anyway.

### 3. Give a minimal supported browser packaging strategy

If the environment is expected to rely on a snap-installed Chromium, the lab should say so. Otherwise, students can lose a lot of time assuming a normal distro package exists when it does not.

### 4. Separate platform correctness from energy-study correctness

The lab currently mixes:

- compositor/browser bring-up,
- host automation,
- suspend correctness,
- energy/wakeup investigation.

Those are related, but not identical. A small note distinguishing “platform complete” from “study complete” would help teams know when they have a valid base versus a finished investigation.

## System Walkthrough

### What This Stage-3 System Actually Is

At a high level, stage 3 is a layered host-guest integration system:

```text
Host side
  |
  |  QMP screenshots / QMP input / serial-log inspection
  v
QEMU virtual hardware
  |
  |  virtio-vga, xHCI, usb-kbd, usb-tablet, virtio-net
  v
Guest Linux kernel + initramfs
  |
  |  /init bootstraps device and compositor environment
  v
udevd + seatd + Weston DRM backend
  |
  |  Wayland compositor + input routing
  v
Chromium Ozone Wayland client
```

The important thing to understand is that Chromium is not the whole system. It is just the final client in a chain that depends on:

- the guest kernel,
- the DRM device,
- udev device discovery,
- seat management,
- Weston’s DRM backend,
- Wayland socket setup,
- host-side QEMU control.

That is why failures in this lab can come from many layers even if the symptom is “the browser looks wrong.”

### Runtime Boot Flow

```text
build-phase3-rootfs.sh
  creates initramfs with BusyBox + Weston + Chromium payload + helper binaries

QEMU boots kernel with rdinit=/init
  -> init-phase3 mounts proc/sys/dev/run/tmp
  -> loads kernel modules
  -> starts systemd-udevd
  -> starts seatd
  -> starts Weston DRM backend
  -> waits for WAYLAND_DISPLAY socket
  -> launches Chromium with Ozone Wayland
  -> optionally schedules suspendctl
```

### Input Path

```text
host/capture_phase3_checkpoints.py
  -> host/qmp_harness.py send-key / input-send-event
  -> QEMU virtual USB keyboard/tablet
  -> guest input subsystem + udev + libinput
  -> Weston seat/input dispatch
  -> Chromium Wayland client
  -> visible DOM change on deterministic test page
```

### Suspend Path

```text
init-phase3
  -> launch Chromium
  -> background suspendctl after delay

suspendctl
  -> optionally write /sys/power/pm_test
  -> program /sys/class/rtc/rtc0/wakealarm
  -> write "freeze" to /sys/power/state
  -> on return, emit @@LOG and @@METRIC lines

host
  -> parse serial log
  -> capture pre/post screenshots
  -> compare continuity
```

## File-Level Map

### Core Stage-3 Guest Files

- [guest/build-phase3-rootfs.sh](/home/manuel/code/wesen/2026-03-07--qemu-power-tick/guest/build-phase3-rootfs.sh#L1)
  - assembles the initramfs
  - trims Chromium payload
  - copies runtime dependencies
- [guest/init-phase3](/home/manuel/code/wesen/2026-03-07--qemu-power-tick/guest/init-phase3#L1)
  - boot-time orchestrator
  - device, compositor, browser, and suspend wiring
- [guest/chromium-wayland-launcher.sh](/home/manuel/code/wesen/2026-03-07--qemu-power-tick/guest/chromium-wayland-launcher.sh#L1)
  - Chromium launch policy
- [guest/suspendctl.c](/home/manuel/code/wesen/2026-03-07--qemu-power-tick/guest/suspendctl.c#L88)
  - suspend helper and metric emitter

### Core Host Validation Files

- [host/qmp_harness.py](/home/manuel/code/wesen/2026-03-07--qemu-power-tick/host/qmp_harness.py)
  - low-level QMP control layer
- [host/make_phase3_test_url.py](/home/manuel/code/wesen/2026-03-07--qemu-power-tick/host/make_phase3_test_url.py#L1)
  - deterministic input-validation page generator
- [host/capture_phase3_checkpoints.py](/home/manuel/code/wesen/2026-03-07--qemu-power-tick/host/capture_phase3_checkpoints.py#L55)
  - browser input checkpoint runner
- [host/capture_phase3_suspend_checkpoints.py](/home/manuel/code/wesen/2026-03-07--qemu-power-tick/host/capture_phase3_suspend_checkpoints.py#L53)
  - suspend screenshot runner

### Core Evidence Files

- [01-diary.md](/home/manuel/code/wesen/2026-03-07--qemu-power-tick/ttmp/2026/03/07/QEMU-04-LAB-THREE--qemu-lab-three-chromium-kiosk-analysis-and-implementation/reference/01-diary.md#L22)
- [01-stage-3-chromium-kiosk-guide.md](/home/manuel/code/wesen/2026-03-07--qemu-power-tick/ttmp/2026/03/07/QEMU-04-LAB-THREE--qemu-lab-three-chromium-kiosk-analysis-and-implementation/design/01-stage-3-chromium-kiosk-guide.md#L22)
- [tasks.md](/home/manuel/code/wesen/2026-03-07--qemu-power-tick/ttmp/2026/03/07/QEMU-04-LAB-THREE--qemu-lab-three-chromium-kiosk-analysis-and-implementation/tasks.md#L1)
- [changelog.md](/home/manuel/code/wesen/2026-03-07--qemu-power-tick/ttmp/2026/03/07/QEMU-04-LAB-THREE--qemu-lab-three-chromium-kiosk-analysis-and-implementation/changelog.md#L1)

## Quality Review By Topic

### Packaging

Assessment: good

Why:

- The brother discovered the snap reality early.
- The brother measured size instead of guessing.
- The brother created an explicit payload allowlist.

Risk:

- The allowlist will be fragile if Chromium changes runtime asset requirements.

### Input

Assessment: very good

Why:

- deterministic page
- deterministic harness
- visible proof
- repeated-character pacing correction

### Suspend

Assessment: good but incomplete

Why:

- helper exists
- metrics are real
- `freezer` path is validated
- `devices` path has a concrete unresolved failure

### Documentation

Assessment: good with bookkeeping issues

Why:

- strong narrative detail
- strong capture of failures and commands
- weak exact commit-to-step alignment

## Suggested Next Engineering Moves

### Immediate next move

Investigate the `virtio_gpu_dequeue_ctrl_func` resume errors in the `devices` path before broadening the stage.

### After that

Either:

1. recover device-resume continuity, or
2. explicitly document that `freezer` is the current passing path and `devices` is an open blocker.

### Cleanup move

Split `init-phase3` into smaller responsibility units once the current device-resume bug is understood. Do not do a large cleanup before that bug is narrowed, or the signal will get muddier.

## Review Playbook For The Next Engineer

Start in this order:

1. Read the overview guide in [01-stage-3-chromium-kiosk-guide.md](/home/manuel/code/wesen/2026-03-07--qemu-power-tick/ttmp/2026/03/07/QEMU-04-LAB-THREE--qemu-lab-three-chromium-kiosk-analysis-and-implementation/design/01-stage-3-chromium-kiosk-guide.md#L22).
2. Read the chronological diary in [01-diary.md](/home/manuel/code/wesen/2026-03-07--qemu-power-tick/ttmp/2026/03/07/QEMU-04-LAB-THREE--qemu-lab-three-chromium-kiosk-analysis-and-implementation/reference/01-diary.md#L22), but do not trust the commit mapping blindly.
3. Inspect the three stage-3 commits:
   - `b88e9d9`
   - `6c2b1c0`
   - `1ce649c`
4. Re-run the deterministic input check.
5. Re-run the `freezer` suspend check.
6. Reproduce the `devices` failure and inspect the DRM errors.

Useful command sequence:

```bash
URL=$(python3 host/make_phase3_test_url.py)

guest/build-phase3-rootfs.sh build build/initramfs-phase3.cpio.gz

guest/run-qemu-phase3.sh \
  --kernel build/vmlinuz \
  --initramfs build/initramfs-phase3.cpio.gz \
  --results-dir results-phase3-checkpoints1 \
  --append-extra "phase3_runtime_seconds=35 phase3_url=$URL"

python3 host/capture_phase3_checkpoints.py \
  --socket results-phase3-checkpoints1/qmp.sock \
  --results-dir results-phase3-checkpoints1 \
  --text hello \
  --settle-seconds 9

guest/run-qemu-phase3.sh \
  --kernel build/vmlinuz \
  --initramfs build/initramfs-phase3.cpio.gz \
  --results-dir results-phase3-suspend-freezer1 \
  --append-extra "phase3_runtime_seconds=40 phase3_url=$URL phase3_suspend_delay_seconds=10 phase3_wake_seconds=5 phase3_pm_test=freezer"

python3 host/capture_phase3_suspend_checkpoints.py \
  --socket results-phase3-suspend-freezer1/qmp.sock \
  --serial-log results-phase3-suspend-freezer1/guest-serial.log \
  --results-dir results-phase3-suspend-freezer1 \
  --pre-name 00-pre \
  --post-name 01-post
```

## Final Assessment

The brother’s work is worth building on.

The strongest reasons are:

- they reduced ambiguity instead of amplifying it,
- they preserved evidence,
- they got a real Chromium guest running,
- they proved real input,
- they isolated a real suspend-resume split.

The main reasons to stay cautious are:

- the stage is not fully complete,
- the diary’s commit bookkeeping needs correction,
- the current shell-heavy orchestration will eventually need cleanup,
- the `devices` path still fails in a way that likely involves the graphics stack rather than just the browser.

If another engineer took over from here, they would be starting from a useful, evidence-backed platform rather than from noise.
