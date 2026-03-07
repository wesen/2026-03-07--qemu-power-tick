---
Title: Lab One Detailed Analysis and Implementation Guide
Ticket: QEMU-01-LAB-ONE
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
    - Path: ttmp/2026/03/07/QEMU-01-LAB-ONE--qemu-lab-one-analysis-and-implementation-guide/index.md
      Note: Ticket overview and entry point for the lab analysis
    - Path: ttmp/2026/03/07/QEMU-01-LAB-ONE--qemu-lab-one-analysis-and-implementation-guide/sources/local/01-lab-assignment.md
      Note: Imported lab brief that defines the required stage-1 behavior
ExternalSources:
    - local:01-lab-assignment.md
    - https://docs.kernel.org/admin-guide/pm/sleep-states.html
    - https://docs.kernel.org/admin-guide/pm/suspend-flows.html
    - https://docs.kernel.org/power/basic-pm-debugging.html
    - https://man7.org/linux/man-pages/man7/epoll.7.html
    - https://man7.org/linux/man-pages/man2/timerfd_create.2.html
    - https://man7.org/linux/man-pages/man2/signalfd.2.html
    - https://www.qemu.org/docs/master/system/devices/net.html
    - https://www.qemu.org/docs/master/system/monitor.html
Summary: Detailed stage-1 design and implementation plan for a single-process QEMU sleep/wake prototype.
LastUpdated: 2026-03-07T14:58:21.843455535-05:00
WhatFor: Translate the imported lab brief into a concrete implementation plan, architecture guide, and validation strategy.
WhenToUse: Use when implementing or reviewing the QEMU suspend/resume lab.
---



# Lab One Detailed Analysis and Implementation Guide

## Executive Summary

This ticket turns the imported lab brief into a detailed stage-1 engineering plan for a QEMU guest that runs one primary user-space program, stays blocked in a readiness-driven event loop while idle, enters Linux suspend-to-idle by writing `freeze` to `/sys/power/state`, and resumes cleanly after wake. The core idea is deliberately narrow: prove the behavior model for sleep, wake, reconnect, and redraw before introducing graphics, multiple processes, or power-measurement concerns.

The simplest correct architecture is a single executable, `sleepdemo`, inside a minimal Linux guest. `sleepdemo` owns one `epoll` instance, one nonblocking TCP socket connected to a host-side drip server, one redraw `timerfd`, one idle-timeout `timerfd`, and one `signalfd` for shutdown. The process should spend most of its awake life inside `epoll_wait()`, transition to `SUSPENDING` only on a true idle timeout, and treat return from the `freeze` write path as the boundary between pre-suspend and post-resume handling. That model matches both the imported assignment and the Linux suspend-flow documentation.

This document is intentionally implementation-oriented. It explains the relevant Linux and QEMU interfaces, proposes a concrete file layout, describes the runtime state machine and event flow, identifies the main failure modes, and breaks the work into phases that an intern could execute in order without inventing architecture as they go.

## Problem Statement

The imported assignment in [01-lab-assignment.md](../sources/local/01-lab-assignment.md) asks for a prototype that demonstrates semantics rather than polish. The goal is not to build a production power-management stack, but to prove several precise behaviors:

1. One primary process can represent the whole guest-facing application.
2. That process can block efficiently on file-descriptor readiness instead of polling.
3. The guest can enter suspend-to-idle through the documented Linux power-management interface.
4. After wake, the same process continues, repairs transient state if needed, and redraws a visible status UI.
5. The networking story is simple enough to reproduce inside QEMU without introducing unnecessary infrastructure.

The main design challenge is semantic clarity. Suspend/resume behavior is easy to misunderstand if the program is written as if user space continues running during suspend. The Linux suspend documentation makes clear that tasks are frozen during the suspend path and continue only after wake. That means the app should not try to "do work while asleep"; it should instead prepare for suspend, block in the kernel suspend path, and run repair logic immediately after return.

There is also an execution-environment challenge. A lab that is hard to boot, hard to wake, or hard to observe will fail even if the program logic is reasonable. The design therefore needs both an application architecture and a lab harness: a host byte server, a reproducible QEMU launch command, a guest startup strategy, and an incremental validation sequence using `/sys/power/pm_test` before relying on full suspend.

## Scope And Non-Goals

The intended scope follows the imported assignment closely.

In scope:

- Minimal Linux guest in QEMU.
- One primary user-space executable.
- One host-side TCP server that periodically emits bytes.
- Terminal UI or equivalent text-mode renderer.
- Suspend-to-idle via `/sys/power/state`.
- Resume detection, reconnect, redraw, and logging.
- Reproducible test steps and evidence capture.

Out of scope for stage 1:

- Wayland, X11, browser embedding, GPU stacks, or advanced graphics.
- Multi-process orchestration.
- Precise energy or battery measurements.
- Wake-on-LAN correctness.
- Device-input plumbing beyond what is necessary to test the suspend cycle.

## Current-State Analysis

There is no implementation in this repository yet. The concrete artifact set currently consists of the ticket workspace, the imported assignment, and this documentation effort. That matters because the design is unconstrained by existing code but must be explicit enough to serve as the system blueprint.

The imported assignment establishes several strong constraints:

- The program should be almost entirely fd-driven, specifically around socket readiness, timer expiration, signal delivery, and idle-triggered suspend; see [01-lab-assignment.md](../sources/local/01-lab-assignment.md).
- The guest should use QEMU user networking first, relying on the usual guest-to-host address `10.0.2.2`.
- The power-management path should start with `freeze`, not deeper sleep states.
- Validation should include `/sys/power/pm_test` modes before real suspend.

The external interfaces reinforce the same shape:

- `epoll(7)` defines an interest list and a ready list, which is exactly the event-dispatch model needed for a single-process readiness loop.
- `timerfd_create(2)` turns timer expiration into readable file-descriptor events that can be consumed through the same event loop.
- `signalfd(2)` lets `SIGINT` and `SIGTERM` arrive as readable records instead of forcing asynchronous signal-handler logic.
- The Linux power-management docs define `freeze` as the user-space entry to suspend-to-idle and describe the suspend flow in terms of freezing tasks, suspending devices, and resuming them on wake.
- QEMU documentation provides the simplest initial network model and the `system_wakeup` monitor command as a deterministic wake path when guest-side input is unreliable.

The absence of code is therefore a benefit here: the design can adopt the cleanest architecture directly instead of retrofitting around an unsuitable polling loop or multi-threaded control plane.

## Gap Analysis

The assignment gives a solid behavioral brief but leaves several implementation details open. Those gaps need to be closed before coding starts.

Gap 1: Resume detection strategy.

The brief says to treat return from the `freeze` write path as post-resume. That should be the primary mechanism. The implementation should not depend on a separate kernel event source for resume detection in stage 1.

Gap 2: TCP lifecycle policy.

The brief requires reconnect-if-needed behavior, but not a detailed socket state machine. The design needs explicit rules for connect in progress, peer close, read errors, backoff, and post-resume health checks.

Gap 3: Idle semantics.

The brief references both lack of local input and lack of network traffic. Stage 1 does not require a local input subsystem, so the implementation should define idleness as "no network traffic and no local synthetic interaction events" unless a keyboard stretch goal is added.

Gap 4: Guest bootstrap.

The brief requires the app to run as the main app in the guest, but does not prescribe whether that means a shell profile, initramfs script, or systemd service. A systemd unit is the simplest maintainable answer for a persistent disk-based guest.

Gap 5: Observability.

The brief asks for logs and a status screen but does not define the log schema, timestamp format, or evidence capture method. The design should specify those so the test report is straightforward.

## Proposed Solution

The recommended stage-1 solution is a small project named `sleep-lab` with a minimal host/guest split:

```text
sleep-lab/
  README.md
  Makefile
  host/
    drip_server.py
  guest/
    sleepdemo.c
    sleepdemo.service
    run-qemu.sh
    notes.md
```

The implementation language should be C unless there is a strong reason to prefer Rust. The assignment explicitly values minimal dependencies and close contact with Linux fd APIs. C keeps the mapping between design and runtime obvious: `epoll_wait`, `timerfd_create`, `signalfd`, nonblocking sockets, and sysfs writes all stay visible without runtime abstraction layers.

### System Overview

At runtime, the system has four major parts:

1. Host test server.
2. QEMU VM process.
3. Linux guest userspace.
4. `sleepdemo` inside the guest.

ASCII architecture diagram:

```text
+---------------- Host Machine ----------------+
|                                              |
|  drip_server.py : TCP listen/send loop       |
|            ^                                 |
|            | TCP via QEMU user networking    |
|            v                                 |
|  qemu-system-x86_64 ---------------------+   |
|    monitor/QMP: system_wakeup            |   |
+------------------------------------------|---+
                                           |
                             +-------------v--------------+
                             | Linux Guest                |
                             |                            |
                             | systemd -> sleepdemo       |
                             |   epoll fd                 |
                             |   socket fd                |
                             |   redraw timerfd           |
                             |   idle timerfd             |
                             |   signalfd                 |
                             |   terminal renderer        |
                             +----------------------------+
```

### Primary Runtime Model

All application-visible events should be normalized into one loop:

- Socket readable or error/hangup.
- Redraw timer expiration.
- Idle timer expiration.
- Signal delivery through `signalfd`.

The program should not perform background polling. If no descriptor is ready, it should remain blocked in `epoll_wait()`. This design directly reflects the assignment's emphasis on a mostly idle, fd-driven process.

### State Machine

A minimal state model is enough and should remain explicit.

```text
STARTING
  -> CONNECTING
  -> CONNECTED
  -> IDLE_AWAKE
  -> SUSPENDING
  -> RESUMED
  -> CONNECTING or CONNECTED
  -> SHUTTING_DOWN
```

Recommended state meanings:

- `STARTING`: process setup, signal mask, fd creation, initial render.
- `CONNECTING`: nonblocking connect in progress or reconnect backoff window.
- `CONNECTED`: socket established and healthy.
- `IDLE_AWAKE`: guest is awake, app is quiescent, waiting in epoll.
- `SUSPENDING`: logging/flush boundary just before writing `freeze`.
- `RESUMED`: immediate post-wake repair path.
- `SHUTTING_DOWN`: orderly teardown after `SIGINT` or `SIGTERM`.

Separate the transport state from the power/UI state in code even if both are rendered together. A single `app_state` struct can hold both, but the design should avoid conflating "socket disconnected" with "system asleep."

### Core Data Model

Suggested core structure:

```c
struct app_state {
    int epoll_fd;
    int sock_fd;
    int redraw_timer_fd;
    int idle_timer_fd;
    int signal_fd;

    enum power_state power_state;
    enum conn_state conn_state;

    uint64_t bytes_received;
    uint64_t packets_received;
    uint64_t suspend_cycles;

    struct timespec started_at;
    struct timespec last_redraw_at;
    struct timespec last_network_at;
    struct timespec last_suspend_at;
    struct timespec last_resume_at;

    int reconnect_attempt;
    bool redraw_pending;
    bool shutdown_requested;
};
```

This is intentionally small. The app needs just enough state to render, log transitions, decide idleness, and recover after wake.

## Linux API References And How They Apply

### `epoll(7)`

Use `epoll` as the single dispatcher. The man page describes an interest list and a ready list; that is the exact conceptual model for this lab. Register:

- socket fd with `EPOLLIN | EPOLLRDHUP | EPOLLERR`
- redraw timerfd with `EPOLLIN`
- idle timerfd with `EPOLLIN`
- signalfd with `EPOLLIN`

Recommended API usage:

```c
int efd = epoll_create1(EPOLL_CLOEXEC);
epoll_ctl(efd, EPOLL_CTL_ADD, sock_fd, &sock_event);
epoll_ctl(efd, EPOLL_CTL_ADD, redraw_timer_fd, &redraw_event);
epoll_ctl(efd, EPOLL_CTL_ADD, idle_timer_fd, &idle_event);
epoll_ctl(efd, EPOLL_CTL_ADD, signal_fd, &signal_event);
```

### `timerfd_create(2)`

Use two timerfds:

- redraw timer: periodic, for example every 3 seconds
- idle timer: one-shot or re-armed relative timer based on latest activity

Timerfd reads return an 8-byte expiration counter, not just a boolean. Always drain that value so the fd becomes non-readable again.

Recommended timer policy:

- Redraw timer is periodic while awake.
- Idle timer is reset after any meaningful activity.
- Post-resume path forces an immediate redraw and restarts the idle timer.

### `signalfd(2)`

Block `SIGINT` and `SIGTERM` with `sigprocmask`, then consume them from the signalfd inside the event loop. This keeps shutdown semantics synchronous and visible in logs.

Suggested setup:

```c
sigset_t mask;
sigemptyset(&mask);
sigaddset(&mask, SIGINT);
sigaddset(&mask, SIGTERM);
sigprocmask(SIG_BLOCK, &mask, NULL);
int sfd = signalfd(-1, &mask, SFD_CLOEXEC | SFD_NONBLOCK);
```

### `/sys/power/state`

Stage 1 should write `freeze\n` to `/sys/power/state`. This is the documented user-space path for suspend-to-idle. The program should log just before the write, perform the write synchronously, and treat successful return from the write path as the earliest reliable post-resume hook in user space.

Practical guardrails:

- Open with `O_WRONLY | O_CLOEXEC`.
- Write the exact string `freeze\n`.
- Handle permission errors explicitly.
- Update timestamps around the operation.
- Avoid assuming the socket survives unchanged.

### `/sys/power/pm_test`

Use `/sys/power/pm_test` during bring-up before trusting full suspend. Start with `freezer`, then `devices`, then `processors`, then `core` if supported by the guest. This reduces debugging scope:

- `freezer` verifies task freezing.
- `devices` adds device suspend/resume.
- deeper modes progressively cover more of the suspend pipeline.

### QEMU user networking

The guest should initially connect to `10.0.2.2:<port>`, which QEMU user networking exposes as the host side. This avoids bridge setup and keeps the host byte server simple.

### QEMU monitor `system_wakeup`

Wake reliability is often the most annoying lab issue. The design should therefore include a deterministic wake path through the QEMU monitor. This is not a production wake mechanism; it is a lab-control mechanism that reduces flakiness during stage 1.

## Detailed Event Flow

### Startup Flow

```text
process starts
  -> initialize logging and timestamps
  -> block SIGINT/SIGTERM
  -> create signalfd
  -> create epoll fd
  -> create timerfds
  -> create nonblocking socket
  -> start connect attempt
  -> arm redraw timer
  -> arm idle timer
  -> render initial screen
  -> enter epoll_wait loop
```

### Network Read Flow

```text
epoll says socket readable
  -> read until EAGAIN or peer close
  -> add to byte counter
  -> update last_network_at
  -> if first successful I/O after reconnect, mark CONNECTED
  -> reset idle timer
  -> request redraw
```

Important design choice: drain all available bytes in a loop. A partial single read may leave the fd readable and create unnecessary wakeups.

### Idle Suspend Flow

```text
idle timer expires
  -> read timerfd expiration count
  -> state = SUSPENDING
  -> last_suspend_at = now
  -> log transition
  -> flush stdio/log output
  -> write "freeze\n" to /sys/power/state
  -> on return:
       state = RESUMED
       last_resume_at = now
       suspend_cycles++
       verify or repair socket
       rearm timers
       force redraw
       state = IDLE_AWAKE or CONNECTING/CONNECTED
```

This path is the semantic center of the lab. Do not hide it behind multiple abstraction layers. The intern should be able to read one function and see exactly how the suspend boundary is handled.

### Signal Shutdown Flow

```text
epoll says signalfd readable
  -> read signalfd_siginfo
  -> log requested signal
  -> state = SHUTTING_DOWN
  -> close socket and other fds
  -> print final counters
  -> exit(0)
```

## Proposed Internal Module Layout

Even in a single C file, logical boundaries should be visible. If the code grows, split later along these lines:

- `main.c` or `sleepdemo.c`
  - argument parsing
  - bootstrap
  - event loop
- `net.c`
  - nonblocking connect
  - read/drain
  - disconnect detection
  - reconnect strategy
- `timers.c`
  - timerfd creation
  - arm/rearm helpers
- `render.c`
  - terminal clear
  - status presentation
  - timestamp formatting
- `power.c`
  - suspend entry helper
  - pm_test helpers for lab bring-up
- `log.c`
  - timestamped transition logs

For stage 1, keeping everything in `guest/sleepdemo.c` is acceptable if the code remains under control. The important point is conceptual separation, not forced file count.

## Detailed Implementation Plan

### Phase 0: Guest Environment

Choose a small Linux guest with:

- systemd available
- serial console enabled
- `/sys/power/state` exposed
- easy package installation for a compiler and basic tools

The guest should be persistent, not rebuilt from scratch for every run, unless the team prefers an initramfs workflow later. For an intern lab, a small disk-backed guest is easier to debug.

Recommended outputs for this phase:

- working disk image
- documented login method
- verified serial console
- verified access to `/sys/power/state`
- verified host reachability at `10.0.2.2`

### Phase 1: Event Loop Skeleton

Implement only the infrastructure:

- create epoll instance
- block and route signals through signalfd
- create timerfds
- create nonblocking TCP socket
- print log lines on each event

No renderer sophistication yet. The success condition is that the process blocks in `epoll_wait()` and event dispatch behaves correctly.

Pseudocode:

```c
for (;;) {
    int n = epoll_wait(state.epoll_fd, events, MAX_EVENTS, -1);
    for (int i = 0; i < n; i++) {
        switch (events[i].data.u32) {
        case EV_SOCKET:  handle_socket_event(&state, events[i].events); break;
        case EV_REDRAW:  handle_redraw_timer(&state); break;
        case EV_IDLE:    handle_idle_timer(&state); break;
        case EV_SIGNAL:  handle_signal(&state); break;
        }
    }
    if (state.shutdown_requested) break;
}
```

### Phase 2: Terminal Renderer

Add a stable text UI. ANSI escapes are enough:

- clear screen
- move cursor to home
- print current states
- print counters and timestamps
- print most recent log summary

Screen contents should include at least:

- power state
- connection state
- byte count
- packet count
- last network activity
- last suspend time
- last resume time
- suspend cycle count
- idle timeout configuration

### Phase 3: Host Byte Server

Create `host/drip_server.py` with a simple loop:

- bind/listen on a port
- accept one client
- send a short byte string every N seconds
- optionally pause or burst based on CLI flags
- log connect/disconnect/send events

Minimal pseudocode:

```python
while True:
    conn, addr = sock.accept()
    try:
        while True:
            conn.sendall(payload)
            time.sleep(interval)
    except OSError:
        conn.close()
```

The server should be intentionally boring. Its only purpose is to generate observable network activity and reconnection cases.

### Phase 4: Suspend Plumbing

Implement the suspend helper and connect it to idle timeout. Add a command-line option such as `--idle-seconds 20` so the timeout is short enough for lab iteration.

Recommended helper contract:

```c
int enter_suspend_to_idle(struct app_state *s);
```

Responsibilities:

- update state and timestamps
- log pre-suspend transition
- flush logs
- write `freeze\n`
- record resume timestamp on return
- increment cycle counter
- schedule immediate redraw

### Phase 5: Post-Resume Repair

This phase is where most subtle bugs appear. Keep the repair policy explicit:

1. Assume timers may need rearming from user-space perspective.
2. Assume the socket may be broken even if the fd still exists.
3. Force a redraw immediately after resume.
4. Restart idle accounting from the resume moment.

Socket health policy:

- if read returns `0`, peer closed: close fd and reconnect
- if `EPOLLERR` or `EPOLLRDHUP` appears: query `SO_ERROR`, log it, reconnect
- if reconnect fails: stay in `CONNECTING`, log attempt count, retry with modest backoff

### Phase 6: `pm_test` Validation

Before full `freeze`, add a helper script or documentation sequence:

```sh
echo freezer > /sys/power/pm_test
echo freeze > /sys/power/state
```

Then repeat with `devices`, then deeper modes if the guest supports them. Record exact observations for each step.

### Phase 7: Final Lab Packaging

Prepare:

- README with exact build/run/test steps
- guest service unit
- QEMU run script
- captured logs
- short test report

At this point the deliverable should be reproducible by another engineer without oral knowledge transfer.

## QEMU Lab Harness Design

### Recommended QEMU Command Shape

The exact command can vary by local environment, but the essential pieces are:

```sh
qemu-system-x86_64 \
  -machine q35 \
  -enable-kvm \
  -cpu host \
  -m 1024 \
  -drive file=guest.img,if=virtio,format=qcow2 \
  -nic user,model=virtio-net-pci \
  -serial mon:stdio
```

Why these choices:

- `q35` is a common modern machine model.
- `-enable-kvm` keeps iteration fast when available.
- virtio disk/net reduce emulation friction.
- monitor on stdio makes `system_wakeup` immediately accessible during testing.

If monitor/stdin multiplexing becomes awkward, split serial and monitor into separate sockets later. Stage 1 should optimize for easy observation, not elegance.

### Guest Startup Strategy

Use a systemd service:

```ini
[Unit]
Description=Sleep/Wake demo
After=network-online.target

[Service]
ExecStart=/usr/local/bin/sleepdemo --host 10.0.2.2 --port 5555 --idle-seconds 20
Restart=always

[Install]
WantedBy=multi-user.target
```

This is better than ad hoc shell startup because it makes boot behavior consistent and logs easier to collect.

## Bonus Point: Move Toward A Near-Single-Process Guest

The current implementation path uses a tiny `busybox`-based initramfs because it is the fastest way to get a reproducible guest: `/init` mounts kernel filesystems, brings up networking, launches `sleepdemo`, and powers the VM off after the run. That still preserves the spirit of the lab because `sleepdemo` remains the only meaningful long-running user process, while the bootstrap layer is just early-boot glue.

For a stronger interpretation of "single-process," a bonus-point follow-up can move more of that bootstrap behavior into `sleepdemo` itself. The target end state would be a very thin PID 1 shim that does as little as possible before `exec`-ing `sleepdemo`, with `sleepdemo` directly handling more of the environment setup and shutdown path.

Possible bonus-point changes:

- replace shell-driven networking setup with direct setup in `sleepdemo`
- avoid `udhcpc` and either hardcode the QEMU user-net assumptions or implement minimal DHCP/client setup in C
- move test-run shutdown logic from the shell bootstrap into `sleepdemo`
- reduce the bootstrap layer to only the kernel-mandated early mounts that are awkward to avoid entirely

Tradeoffs:

- fewer helper tools and a cleaner single-binary story
- more code inside `sleepdemo`, including bootstrapping concerns that are not central to the sleep/wake semantics lab
- more implementation complexity earlier in the project

Recommended position for this ticket:

- keep the thin `busybox` bootstrap for the initial working prototype
- record the near-single-process approach as a bonus-point refinement after the suspend/resume measurements are working end to end

## Testing And Validation Strategy

### Test Matrix

1. Event loop idle test.
2. Network receive test.
3. Shutdown signal test.
4. Reconnect-after-server-restart test.
5. `pm_test=freezer`.
6. `pm_test=devices`.
7. Real `freeze` suspend/resume.
8. Suspend while server is actively sending.

### What To Capture

For every test, capture:

- exact command lines
- guest log lines with timestamps
- host server log lines
- whether the UI redrew correctly
- whether reconnect happened automatically

### Success Criteria

The lab should be considered successful only when:

- CPU usage is low at idle because the process blocks in `epoll_wait()`.
- Timer, signal, and socket events all arrive through the single loop.
- The guest enters `freeze` through the documented sysfs path.
- After wake, the process continues in the same PID and repairs the connection if needed.
- The screen visibly reflects suspend/resume and reconnect state.

## Current Validation Results

The implementation work in this ticket has now progressed beyond planning. The initramfs guest boots in QEMU, reaches the host-side drip server over QEMU user networking, redraws continuously while awake, and records structured timing metrics on suspend/resume.

Measured successful runs:

- `pm_test=freezer`
  - `sleep_duration`: about `5033 ms`
  - `suspend_resume_gap`: about `5033 ms`
  - `resume_to_redraw`: about `3 ms`
- `pm_test=devices`
  - `sleep_duration`: about `5028 ms`
  - `suspend_resume_gap`: about `5028 ms`
  - `resume_to_redraw`: about `3 ms`
  - `resume_to_reconnect`: about `4 ms`

Observed current limitation:

- Real `freeze` suspend-to-idle entry works and the guest reaches the `s2idle` suspend path.
- In this VM environment, the wake methods tried so far did not reliably resume the guest from real `freeze`.
- Because of that, the measured end-to-end resume numbers currently come from `pm_test` validation modes rather than a successful real-freeze wake cycle.

## Risks, Failure Modes, And Mitigations

### Risk 1: Suspend does not actually occur in the guest

Symptoms:

- `freeze` write fails
- guest logs permission or unsupported-state errors
- VM appears to hang or immediately returns without meaningful suspend

Mitigation:

- verify `/sys/power/state` contents first
- use `pm_test` modes to isolate failure stage
- confirm guest kernel/config and ACPI behavior

### Risk 2: Wake path is unreliable

Symptoms:

- guest never resumes from an attempted suspend
- keyboard/mouse wake is inconsistent in the VM

Mitigation:

- use QEMU monitor `system_wakeup`
- document the exact wake method in README and test report

### Risk 3: Socket appears open but is unusable after resume

Symptoms:

- no bytes arrive after wake
- next write/read reveals connection failure late

Mitigation:

- inspect `EPOLLERR` and `EPOLLRDHUP`
- perform an explicit reconnect policy after resume when health is uncertain
- keep the protocol stateless in stage 1

### Risk 4: Timer semantics become confused across suspend

Symptoms:

- immediate repeated suspend cycles
- redraw or idle timers fire unexpectedly after resume

Mitigation:

- fully read timerfd expiration counts
- always rearm idle timer after resume
- force redraw through explicit state rather than relying on old timer state

### Risk 5: Scope creep

Symptoms:

- renderer complexity begins to dominate the lab
- extra processes or graphical stacks are introduced

Mitigation:

- keep terminal rendering only
- reject stage-2 features during stage 1

## Alternatives Considered

### Alternative 1: Polling loop with sleeps

Rejected because it defeats the point of the lab. The lab exists to validate a readiness-driven idle model, not to emulate one loosely.

### Alternative 2: Async signal handlers instead of signalfd

Rejected because it introduces out-of-band control flow and makes shutdown logic harder to reason about in a small teaching lab.

### Alternative 3: Multiple processes for UI and network handling

Rejected because the assignment is specifically about one primary process and because a multi-process design would blur suspend/resume semantics during stage 1.

### Alternative 4: Start with bridged networking

Rejected because QEMU user networking is faster to reproduce and adequate for the host-to-guest test server requirement.

### Alternative 5: Start with a graphical renderer

Rejected because it increases debug surface without improving understanding of suspend/resume behavior.

## Detailed File-Level Guidance

Recommended implementation targets once coding begins:

- `sleep-lab/README.md`
  - reproducible host and guest instructions
- `sleep-lab/Makefile`
  - build `guest/sleepdemo` and run convenience targets
- `sleep-lab/host/drip_server.py`
  - deterministic host data source
- `sleep-lab/guest/sleepdemo.c`
  - event loop, suspend logic, reconnect logic, renderer
- `sleep-lab/guest/sleepdemo.service`
  - guest boot integration
- `sleep-lab/guest/run-qemu.sh`
  - known-good QEMU invocation
- `sleep-lab/guest/notes.md`
  - quick operator notes or captured caveats

Suggested symbol-level breakdown for `sleepdemo.c`:

- `setup_signals()`
- `setup_epoll()`
- `setup_timers()`
- `connect_socket()`
- `reset_idle_timer()`
- `request_redraw()`
- `render_screen()`
- `handle_socket_event()`
- `handle_redraw_timer()`
- `handle_idle_timer()`
- `enter_suspend_to_idle()`
- `repair_after_resume()`
- `handle_signal_event()`
- `main_loop()`

## Review Checklist For The Future Implementation

When reviewing the eventual code, verify:

1. Every external event source is fd-based and registered in epoll.
2. No busy loop or periodic polling exists outside the timerfds.
3. The suspend helper writes exactly `freeze\n` and treats return as resume.
4. Timerfd expiration counters are always drained.
5. Signal handling stays inside signalfd-based dispatch.
6. Reconnect logic is deterministic and logged.
7. The UI redraw path is side-effect-light and easy to trigger after resume.
8. The README is sufficient for a fresh operator to reproduce the demo.

## Open Questions

The assignment is detailed enough that most design choices are now resolved. The remaining open questions are practical rather than architectural:

1. Which guest distribution or image should be used for the fastest setup while preserving suspend support?
2. Should the app be written in C, as recommended here, or Rust if the intern is materially stronger there?
3. What exact wake method is most reliable in the target local environment: QEMU monitor, input event, or another mechanism?
4. Does the team want the stage-1 deliverable to be disk-image based, or should it later move to initramfs for faster resets?

## References

- Imported assignment: [01-lab-assignment.md](../sources/local/01-lab-assignment.md)
- Ticket overview: [index.md](../index.md)
- Diary: [01-diary.md](../reference/01-diary.md)
- Linux sleep states: <https://docs.kernel.org/admin-guide/pm/sleep-states.html>
- Linux suspend flows: <https://docs.kernel.org/admin-guide/pm/suspend-flows.html>
- Linux PM debugging: <https://docs.kernel.org/power/basic-pm-debugging.html>
- `epoll(7)`: <https://man7.org/linux/man-pages/man7/epoll.7.html>
- `timerfd_create(2)`: <https://man7.org/linux/man-pages/man2/timerfd_create.2.html>
- `signalfd(2)`: <https://man7.org/linux/man-pages/man2/signalfd.2.html>
- QEMU networking: <https://www.qemu.org/docs/master/system/devices/net.html>
- QEMU monitor: <https://www.qemu.org/docs/master/system/monitor.html>
