---
Title: Postmortem And Review Guide
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
RelatedFiles:
    - Path: guest/init-phase2
      Note: Bootstrap policy and hardcoded scenario configuration discussed in the findings
    - Path: guest/wl_sleepdemo.c
      Note: Main phase-2 orchestration file reviewed in the findings section
    - Path: guest/wl_suspend.c
      Note: Suspend path and runtime-limit behavior evaluated in the review
    - Path: host/resume_drip_server.py
      Note: Reconnect harness measurement limitations and strengths discussed in the postmortem
    - Path: ttmp/2026/03/07/QEMU-02-LAB-TWO--qemu-lab-two-wayland-client-analysis-and-implementation/reference/01-diary.md
      Note: Primary process diary used as source evidence
    - Path: ttmp/2026/03/07/QEMU-03-PHASE2-CLEANUP--phase-2-wayland-client-modularization-cleanup/reference/01-diary.md
      Note: Cleanup diary used to evaluate the modularization decision
ExternalSources: []
Summary: Postmortem analysis of the phase-1, phase-2, and cleanup work, with a code-review section, process assessment, system walkthrough, and review guide for the next engineer.
LastUpdated: 2026-03-07T19:34:01.111907134-05:00
WhatFor: Evaluate the quality of the delivered QEMU lab work, explain where the implementation is strong or weak, and provide a structured review guide for continuation.
WhenToUse: Use this when judging the quality of the current lab implementation, onboarding a reviewer, or deciding what should be fixed before treating the work as final.
---


# Postmortem And Review Guide

## Executive Summary

The short version is that the work is not garbage. It is materially useful, technically real, and much further along than a paper design or superficial demo. Stage 1 has a functioning harness and measured suspend-like behavior. Stage 2 has a real Weston guest, a custom Wayland client, host screenshots, host keyboard and pointer injection, a modularized client codebase, and measured `pm_test=devices` suspend/resume with redraw and reconnect timing. The diaries are unusually strong and make the work auditable.

That said, the result is not finished in the “submission-grade and low-risk” sense. The biggest remaining weaknesses are not fake progress but they are real weaknesses:

- the phase-2 screenshot/checkpoint set is incomplete,
- the reconnect metric is partly harness-shaped rather than purely application-shaped,
- the client still redraws once per second even though the intended design says redraw only on meaningful state change,
- the phase-2 bootstrap currently hardcodes one suspend scenario into `/init`,
- the current runtime-limit behavior is risky if someone starts using it while the client is PID 1.

The process quality is above average. The engineering quality is mixed but respectable. The documentation quality is strong. The testing quality is good for exploratory systems work but still below what I would call “done.”

## Problem Statement

The user asked for a postmortem of the work done so far by the earlier agent, including:

- whether the resulting system is actually good or merely noisy,
- where the implementation struggled,
- what was done well,
- what should be improved in the engineering process,
- what should be improved in the lab itself,
- and a detailed guide that explains the whole system so a reviewer can judge it correctly.

This document addresses that by combining:

1. a code-review section with concrete findings,
2. a process review based on the diaries and git history,
3. a deep system walkthrough of the current architecture,
4. and a reviewer playbook for continuation.

## Proposed Solution

The right way to evaluate this work is not as a binary “works” or “does not work.” It should be evaluated as a layered systems effort with four distinct outputs:

1. Stage-1 lab harness and measurements.
2. Stage-2 guest graphics/input platform and custom Wayland client.
3. Phase-2 code cleanup that created workable module boundaries.
4. Host-side measurement harnesses and diaries that make the work reproducible.

The rest of this report therefore does four things:

- it identifies the highest-value technical findings first,
- it reconstructs the path the agent took and why that path was mostly reasonable,
- it explains the system in enough detail that another engineer can review it without rereading every diary step,
- and it closes with concrete recommendations for both the engineer and the lab itself.

## Findings

These are the main issues I would want fixed or explicitly accepted before calling the phase-2 result “final.”

### 1. The phase-2 client still redraws once per second, which biases idle and energy behavior

Severity: medium

Why this matters:

- The task list and intended design point toward redraw-on-change behavior.
- The current implementation arms a periodic redraw timer and turns every timer tick into a redraw trigger.
- That creates unnecessary guest wakeups and makes energy or idle behavior look worse than it should.

Code:

- [guest/wl_sleepdemo.c](/home/manuel/code/wesen/2026-03-07--qemu-power-tick/guest/wl_sleepdemo.c#L57)
- [guest/wl_sleepdemo.c](/home/manuel/code/wesen/2026-03-07--qemu-power-tick/guest/wl_sleepdemo.c#L185)

Why I believe it:

- `arm_redraw_timer(app, 1000, 1000)` creates a repeating timer.
- `EV_REDRAW` calls `set_redraw_reason(&app, "timer")`, which marks a redraw pending every second.

Practical effect:

- The UI is not truly quiescent.
- Measurements related to “idle” or “energy-like” behavior are polluted.
- The current reconnect timing includes some wakefulness that is not inherent to suspend/resume.

Recommended fix:

- replace the periodic redraw timer with either:
  - a one-shot timer only when a future redraw is actually needed, or
  - no redraw timer at all if the dashboard only needs redraw on event boundaries.

### 2. The phase-2 bootstrap hardcodes a specific suspend scenario into `/init`

Severity: medium

Why this matters:

- Experiment configuration should not be hidden in the guest bootstrap unless that is explicitly the lab contract.
- Right now, the guest always launches the client with one specific suspend configuration.
- That makes it harder to run alternate scenarios cleanly and makes the guest image itself part of the test-case definition.

Code:

- [guest/init-phase2](/home/manuel/code/wesen/2026-03-07--qemu-power-tick/guest/init-phase2#L99)

Practical effect:

- the current image is implicitly “the `pm_test=devices` image,”
- alternate scenarios require editing and rebuilding the guest,
- review and reproduction are less flexible than they should be.

Recommended fix:

- move client scenario parameters to kernel cmdline parsing, a small config file, or an explicit host-side launcher argument path.

### 3. Runtime-limit exit is unsafe if the client is PID 1

Severity: medium

Why this matters:

- `init-phase2` uses `exec "$CLIENT_BIN" ...`, so the client process becomes PID 1.
- `maybe_exit_on_runtime_limit()` only sets `app->running = false`.
- If someone starts relying on `--runtime-seconds`, the client may exit as PID 1 instead of powering down cleanly.

Code:

- [guest/wl_suspend.c](/home/manuel/code/wesen/2026-03-07--qemu-power-tick/guest/wl_suspend.c#L95)
- [guest/init-phase2](/home/manuel/code/wesen/2026-03-07--qemu-power-tick/guest/init-phase2#L101)

Practical effect:

- likely harmless right now because runtime-limit is not the main control path,
- but this is a trap for later automation or final-report scripts.

Recommended fix:

- if runtime limit is used in the final harness, exit via explicit `poweroff -f`, `reboot`, or a small PID-1 wrapper.

### 4. The reconnect metric is real enough to be useful, but it is not a pure application/network latency

Severity: low to medium

Why this matters:

- the recorded `resume_to_reconnect = 1243 ms` is meaningful as a scenario metric,
- but it includes host-harness latency from serial-log observation and server startup,
- not only guest-side reconnect behavior.

Code:

- [host/resume_drip_server.py](/home/manuel/code/wesen/2026-03-07--qemu-power-tick/host/resume_drip_server.py#L14)
- [host/resume_drip_server.py](/home/manuel/code/wesen/2026-03-07--qemu-power-tick/host/resume_drip_server.py#L77)

Practical effect:

- the metric is valid for “time from resume to restored host connectivity in this harness,”
- but not a clean isolated reconnect-loop benchmark.

Recommended fix:

- explicitly label it as a harness-level metric in the final report,
- or replace the serial-log watcher with a tighter host-side event trigger.

## Design Decisions

The earlier agent made several good decisions that materially improved the result.

### Decision 1: Use a hand-built initramfs rather than a full guest distro image

Why it was good:

- iteration stayed fast,
- the runtime surface stayed inspectable,
- boot logic remained transparent in [guest/init](/home/manuel/code/wesen/2026-03-07--qemu-power-tick/guest/init) and [guest/init-phase2](/home/manuel/code/wesen/2026-03-07--qemu-power-tick/guest/init-phase2).

Cost:

- packaging fidelity became a major pain point,
- missing libraries and kernel modules caused many early failures.

Assessment:

- correct choice for this lab,
- but only because the agent was disciplined enough to debug the image assembly properly.

### Decision 2: Bring up Weston in phase 2 instead of deferring it

Why it was good:

- it matched the actual lab requirements,
- it forced the platform layer to be real before Chromium,
- it made input, screenshots, and suspend/resume issues visible early.

Assessment:

- clearly correct.

### Decision 3: Build a tiny custom Wayland client instead of using a heavy UI stack

Why it was good:

- the client is easy to reason about,
- there are very few guest-side dependencies,
- the dashboard is directly tied to measured state,
- the agent could instrument exactly the events the lab cares about.

Assessment:

- one of the best decisions in the whole effort.

### Decision 4: Stop and modularize the client before layering suspend logic on top

Why it was good:

- the phase-2 monolith had already reached friction size,
- the cleanup ticket created clear boundaries for render, net, Wayland, and suspend,
- later changes became cheaper.

Assessment:

- very good engineering instinct.

## Alternatives Considered

### Alternative: keep the monolith and continue adding features

Rejected because:

- it would have produced faster short-term progress,
- but made suspend and measurement changes harder to trust and review.

### Alternative: use a full distro image with systemd and packages already installed

Rejected because:

- it would reduce packaging pain,
- but make the runtime much less legible,
- and add more uncontrolled background behavior to a lab that is supposed to be minimal.

### Alternative: postpone formal diaries until the code was “done”

Rejected in practice because:

- the debugging path was too branchy,
- without the diaries, a reviewer would have no reliable map of what failed, why it failed, and how it was fixed.

This was the right rejection. The diaries are one of the strongest parts of the deliverable.

## Implementation Plan

For the next engineer, the correct continuation plan is:

1. Fix the always-redraw behavior.
2. Move phase-2 scenario parameters out of hardcoded `/init`.
3. Decide whether runtime-limit exit is needed at all. If yes, make it power off cleanly.
4. Finish the screenshot checkpoint set.
5. Write the phase-2 final report with the right honesty about what is and is not measured cleanly.
6. Only then move on to the Chromium stage.

## Process Review

### What the agent did well

- Read the brief carefully enough to avoid building the wrong phase.
- Used small intermediate checkpoints instead of pretending everything worked at once.
- Wrote diaries that recorded not only outcomes but failed hypotheses.
- Added instrumentation instead of hand-waving when stuck.
- Split cleanup work into a separate ticket rather than letting the main delivery ticket become incoherent.
- Committed at sensible boundaries:
  - `78baa4d` stage-1 scaffold
  - `83f1868` stage-1 measurement validation
  - `556fbc4` phase-2 Weston scaffold
  - `f0923d1` custom phase-2 client
  - `52a6391` input-complete milestone
  - `91076d0` modularization cleanup
  - `ba8f6d9` suspend metrics path
  - `78bb516` reconnect timing harness

### Where the agent struggled

The struggles were mostly normal for this kind of lab and mostly handled well.

#### 1. Packaging fidelity

Symptoms:

- missing `/dev/dri`,
- broken dynamic loader path,
- missing XKB data,
- missing HID/xHCI pieces.

Why it happened:

- a hand-assembled initramfs is unforgiving,
- graphics and input completeness are separate problems.

How well it was handled:

- well.
- The agent kept the problem narrow and used logs to avoid magical thinking.

#### 2. Input debugging across many layers

Symptoms:

- QMP input accepted but client saw nothing,
- seat capabilities missing,
- pointer and keyboard failed for different reasons.

Why it happened:

- the stack spans QEMU device model, kernel input, udev, libinput, Weston, seat negotiation, and client listeners.

How well it was handled:

- very well.
- The agent added the right probes and produced a reusable input playbook.

#### 3. Reconnect measurement orchestration

Symptoms:

- server alive too long prevented suspend,
- server started too late made reconnect metric meaningless.

Why it happened:

- this is not just a code problem; it is a scenario-design problem.

How well it was handled:

- eventually well.
- The addition of [resume_drip_server.py](/home/manuel/code/wesen/2026-03-07--qemu-power-tick/host/resume_drip_server.py) was the right move.

### What the agent could improve

- tighten the loop between measured intent and implementation intent.
  Example: the task said redraw-on-change, but the code still redraws every second.
- separate “benchmark metric” from “harness metric” more explicitly in code and docs.
- make scenario control host-configurable earlier instead of baking more policy into `/init`.
- update ticket status/tasks as soon as a milestone is actually committed. A few diary/task moments lagged reality slightly.

## System Walkthrough

This section explains the system as it exists now.

### 1. High-Level Architecture

```text
+---------------- Host ----------------+
| QEMU launcher                        |
| QMP harness                          |
| drip server / resume watcher         |
+----------------+---------------------+
                 |
                 v
+---------------- Guest -----------------------------+
| Linux kernel + initramfs                           |
| /init mounts /proc /sys /dev, loads modules        |
| systemd-udevd + seatd + Weston                     |
|                                                    |
| wl_sleepdemo                                       |
|  - Wayland surface + listeners                     |
|  - reconnecting TCP client                         |
|  - suspend logic                                   |
|  - metric emission                                 |
+----------------------------------------------------+
```

### 2. Boot Path

Relevant files:

- [guest/run-qemu-phase2.sh](/home/manuel/code/wesen/2026-03-07--qemu-power-tick/guest/run-qemu-phase2.sh)
- [guest/build-phase2-rootfs.sh](/home/manuel/code/wesen/2026-03-07--qemu-power-tick/guest/build-phase2-rootfs.sh)
- [guest/init-phase2](/home/manuel/code/wesen/2026-03-07--qemu-power-tick/guest/init-phase2)

Pseudocode:

```text
host builds initramfs
host starts qemu with kernel + initrd + qmp socket
guest /init:
  mount proc/sys/dev
  configure static network
  load gpu + hid + xhci modules
  start udev
  start seatd
  start Weston
  wait for Wayland socket
  exec wl_sleepdemo
```

### 3. Wayland Client Structure

Relevant files:

- [guest/wl_app_core.h](/home/manuel/code/wesen/2026-03-07--qemu-power-tick/guest/wl_app_core.h)
- [guest/wl_render.c](/home/manuel/code/wesen/2026-03-07--qemu-power-tick/guest/wl_render.c)
- [guest/wl_net.c](/home/manuel/code/wesen/2026-03-07--qemu-power-tick/guest/wl_net.c)
- [guest/wl_suspend.c](/home/manuel/code/wesen/2026-03-07--qemu-power-tick/guest/wl_suspend.c)
- [guest/wl_wayland.c](/home/manuel/code/wesen/2026-03-07--qemu-power-tick/guest/wl_wayland.c)
- [guest/wl_sleepdemo.c](/home/manuel/code/wesen/2026-03-07--qemu-power-tick/guest/wl_sleepdemo.c)

Module map:

- `wl_app_core`: shared state, time helpers, metric/log helpers
- `wl_render`: `wl_shm` buffer creation and pixel dashboard drawing
- `wl_net`: TCP connect/reconnect state machine
- `wl_suspend`: idle timer, wakealarm programming, `freeze`, metric capture
- `wl_wayland`: registry, seat, keyboard, pointer, `xdg-shell`
- `wl_sleepdemo`: epoll loop and composition

### 4. Event Loop Model

Current event sources:

- Wayland fd
- socket fd
- redraw timer
- idle timer
- signal fd

Current logic:

```text
while running:
  attempt_connect()
  if configured and redraw_pending:
    render()
  epoll_wait(...)
  dispatch each ready fd
```

Important review note:

- this is clean and understandable,
- but the redraw timer currently forces periodic redraw.

### 5. Suspend Path

Relevant Linux interfaces:

- `/sys/power/state`
- `/sys/power/pm_test`
- `/sys/class/rtc/rtc0/wakealarm`

Pseudocode:

```text
idle timer fires
if suspend allowed:
  write pm_test
  program wakealarm
  log SUSPENDING
  write "freeze" to /sys/power/state
  on return:
    compute sleep and resume gap
    emit metrics
    inspect socket health
    request redraw
```

### 6. Host Harness

Relevant files:

- [host/qmp_harness.py](/home/manuel/code/wesen/2026-03-07--qemu-power-tick/host/qmp_harness.py)
- [host/drip_server.py](/home/manuel/code/wesen/2026-03-07--qemu-power-tick/host/drip_server.py)
- [host/resume_drip_server.py](/home/manuel/code/wesen/2026-03-07--qemu-power-tick/host/resume_drip_server.py)
- [scripts/measure_run.py](/home/manuel/code/wesen/2026-03-07--qemu-power-tick/scripts/measure_run.py)

Roles:

- `qmp_harness.py`: screenshots and host input injection
- `drip_server.py`: generic traffic source
- `resume_drip_server.py`: reconnect-timing scenario source
- `measure_run.py`: parse `@@METRIC` lines from serial logs

### 7. Measured State So Far

Stage 1:

- devices/freezer-style measurements exist and are credible for the stage-1 harness

Stage 2:

From `results-phase2-suspend6/metrics.json`:

- `sleep_duration = 5755 ms`
- `suspend_resume_gap = 5755 ms`
- `resume_to_redraw = 5 ms`
- `resume_to_reconnect = 1243 ms`

Interpretation:

- suspend/resume path is materially working in the measured `pm_test=devices` scenario
- redraw after resume is fast
- reconnect is acceptable for the current harness, but not yet a pure app-only metric

## Reviewer Playbook

If I were reviewing this work from scratch, I would do it in this order:

1. Read the phase-2 design doc:
   - [01-phase-2-wayland-client-analysis-and-implementation-guide.md](./01-phase-2-wayland-client-analysis-and-implementation-guide.md)
2. Read the phase-2 diary:
   - [../reference/01-diary.md](../reference/01-diary.md)
3. Read the cleanup diary:
   - [../../QEMU-03-PHASE2-CLEANUP--phase-2-wayland-client-modularization-cleanup/reference/01-diary.md](../../QEMU-03-PHASE2-CLEANUP--phase-2-wayland-client-modularization-cleanup/reference/01-diary.md)
4. Review the current code in this order:
   - [guest/init-phase2](/home/manuel/code/wesen/2026-03-07--qemu-power-tick/guest/init-phase2)
   - [guest/wl_app_core.h](/home/manuel/code/wesen/2026-03-07--qemu-power-tick/guest/wl_app_core.h)
   - [guest/wl_wayland.c](/home/manuel/code/wesen/2026-03-07--qemu-power-tick/guest/wl_wayland.c)
   - [guest/wl_net.c](/home/manuel/code/wesen/2026-03-07--qemu-power-tick/guest/wl_net.c)
   - [guest/wl_suspend.c](/home/manuel/code/wesen/2026-03-07--qemu-power-tick/guest/wl_suspend.c)
   - [guest/wl_render.c](/home/manuel/code/wesen/2026-03-07--qemu-power-tick/guest/wl_render.c)
   - [guest/wl_sleepdemo.c](/home/manuel/code/wesen/2026-03-07--qemu-power-tick/guest/wl_sleepdemo.c)
5. Validate one clean run with:
   - client build
   - initramfs build
   - QEMU run
   - QMP screenshot
   - post-resume metric parse
6. Then check whether the remaining gaps matter for the intended submission.

## Lab Improvement Suggestions

The lab itself could be improved.

### 1. Separate platform bring-up from measurement deliverables more explicitly

Right now, students can lose time wondering whether Weston belongs in phase 2. The brief should say plainly:

- phase 2 includes compositor bring-up,
- phase 2 does not include Chromium yet,
- stage 3 adds Chromium on top.

### 2. Provide a recommended host measurement scenario

The reconnect metric issue was not obvious from the brief. A short recommended scenario matrix would help:

- no server before boot
- server appears after resume
- input replay after resume

### 3. State whether “redraw on change only” is a hard requirement

If it is hard, the lab should say so because it directly affects energy behavior and measurement validity.

### 4. Clarify what counts as an acceptable suspend result in QEMU

For example:

- is `pm_test=devices` enough,
- is `freezer` required,
- is full wake from `freeze` expected in this environment,
- or should environment limits be documented as long as the path is well explained?

## Bottom Line

The earlier agent did good work.

This is not fake progress. The code and artifacts show real system bring-up, real debugging, real measurement, and real documentation. The strongest parts are:

- the diaries,
- the input investigation,
- the modularization timing,
- and the host-side scenario work needed to get a clean reconnect metric.

The biggest weaknesses are:

- one misleading “idle” characteristic in the redraw loop,
- one hardcoded scenario in `/init`,
- one unsafe runtime-limit exit path,
- and some remaining incompleteness in screenshots/final reporting.

My overall assessment:

- process quality: high
- documentation quality: high
- implementation quality: medium to high
- final-submission completeness: not there yet, but close enough that the remaining work is clear and bounded

## Open Questions

- Should the final submission include the redraw-loop fix before being treated as complete?
- Should the final report present `resume_to_reconnect` as a harness-level metric explicitly?
- Is a real non-`pm_test` wake path required by the grader, or is documented environment limitation acceptable?

## References

- [01-phase-2-wayland-client-analysis-and-implementation-guide.md](./01-phase-2-wayland-client-analysis-and-implementation-guide.md)
- [../reference/01-diary.md](../reference/01-diary.md)
- [../reference/02-input-bring-up-playbook.md](../reference/02-input-bring-up-playbook.md)
- [../../QEMU-03-PHASE2-CLEANUP--phase-2-wayland-client-modularization-cleanup/design/01-phase-2-modularization-guide.md](../../QEMU-03-PHASE2-CLEANUP--phase-2-wayland-client-modularization-cleanup/design/01-phase-2-modularization-guide.md)
- [../../QEMU-03-PHASE2-CLEANUP--phase-2-wayland-client-modularization-cleanup/reference/01-diary.md](../../QEMU-03-PHASE2-CLEANUP--phase-2-wayland-client-modularization-cleanup/reference/01-diary.md)
