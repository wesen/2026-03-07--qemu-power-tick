---
Title: Imported Lab Assignment
Ticket: QEMU-01-LAB-ONE
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
    - /tmp/lab-assignment.md
Summary: "Imported source assignment for the stage-1 QEMU sleep/wake lab."
LastUpdated: 2026-03-07T14:58:13-05:00
WhatFor: "Preserve the original assignment text inside the ticket workspace."
WhenToUse: "Use as the source brief for the stage-1 lab design and implementation work."
---

# Imported Lab Assignment

Here’s a stage-1 lab brief you can hand to an intern.

## Lab: Single-Process Sleep/Wake Prototype in QEMU

### Purpose

Build a tiny Linux guest in QEMU that runs **one primary user-space process** and demonstrates the **semantics of idle, suspend, wake, and reconnect**.

The prototype should:

* maintain a network connection to a host-side test server,
* display a simple “offline / status” screen periodically,
* stay blocked in a single event loop when idle,
* enter Linux **suspend-to-idle** by writing `freeze` to `/sys/power/state`,
* resume correctly when the VM is woken,
* reconnect and redraw after resume. Linux documents that writing `freeze` to `/sys/power/state` enters suspend-to-idle, and the suspend flow freezes tasks, suspends devices, and resumes them on wake. ([Kernel Documentation][1])

This stage is **not** about real power measurement. It is about getting the **behavior model** right.

---

## Why this lab exists

The long-term system idea is “do almost nothing unless there is interaction or incoming data.” The cleanest way to explore that is to make a process that is almost entirely **fd-driven**:

* socket becomes readable,
* timer expires,
* signal arrives,
* idle timeout fires and the system suspends.

Linux’s `epoll` API is designed exactly for this kind of readiness-driven architecture. `timerfd` exposes timers as file descriptors that can be monitored by `epoll`, and `signalfd` does the same for signals. ([man7.org][2])

The important semantic to internalize is that during real system suspend, **user-space does not keep running**. The kernel’s suspend flow freezes tasks and later resumes them; your process continues only after wake. ([Kernel Documentation][3])

---

## Scope

### In scope

Build a single executable that:

* runs as the main app in the guest,
* uses one `epoll` loop,
* uses one TCP connection to the host,
* uses timerfds for redraw and idle timeout,
* uses signalfd for shutdown,
* triggers suspend with `freeze`,
* logs pre-suspend and post-resume behavior,
* redraws a trivial text UI after wake. ([man7.org][4])

### Out of scope

Do **not** add any of this in stage 1:

* Wayland compositor or client
* browser embedding
* graphics stack work
* wake-on-LAN correctness
* battery or wattage measurement
* raw evdev integration unless needed for a stretch goal

---

## Big concepts the intern should understand first

### 1. Readiness-driven design

The app should not poll in a loop. It should block in `epoll_wait()` until something is ready. `epoll` maintains an interest list and a ready list, making it the right primitive for a small event-driven process. ([man7.org][2])

### 2. Timers as file descriptors

Use `timerfd_create()` and `timerfd_settime()` instead of signal timers or ad hoc sleeps. Timerfds can be monitored by `select`, `poll`, and `epoll`. ([man7.org][4])

### 3. Signals as file descriptors

Use `signalfd()` rather than async signal handlers. This keeps shutdown inside the normal event loop. `signalfd` is specifically documented as an alternative to signal handlers and can also be monitored by `epoll`. ([man7.org][5])

### 4. Suspend-to-idle semantics

For this lab, the target sleep state is **suspend-to-idle**. Linux documents that `freeze` written to `/sys/power/state` enters this state. The suspend flow freezes tasks and suspends devices, then resumes them when the system wakes. ([Kernel Documentation][1])

### 5. QEMU networking model

Use QEMU user networking first. In that mode, the guest can usually reach the host at `10.0.2.2`, and host port forwarding can expose a host-side test server to the guest. ([QEMU][6])

### 6. Suspend testing without fully trusting suspend

Linux provides `/sys/power/pm_test` for staged suspend testing. The documented test modes include `freezer`, `devices`, `platform`, `processors`, and `core`, which is ideal for validating the path incrementally before relying on full suspend. ([Kernel Documentation][7])

---

## High-level architecture

The guest should boot into a small Linux environment and run one main app, for example `sleepdemo`.

`sleepdemo` owns:

* one `epoll` instance,
* one TCP socket,
* one redraw timerfd,
* one idle timerfd,
* one signalfd,
* one simple screen renderer.

The renderer should be minimal. For stage 1, a terminal UI is enough: ANSI escape sequences or `ncurses` are fine. The point is not graphics sophistication; the point is to make visible when the process is alive, idle, suspended, resumed, disconnected, and reconnected.

### State model

The app should track:

* `CONNECTED`
* `DISCONNECTED`
* `IDLE_AWAKE`
* `SUSPENDING`
* `RESUMED`
* `SHUTTING_DOWN`

Incoming network bytes should update a counter and last-seen timestamp. The periodic redraw should display those values.

---

## Required deliverables

1. **Source tree**

   * one main executable
   * simple build system (`Makefile` or `meson`)
   * README with run instructions

2. **Host-side test server**

   * a tiny script that accepts a connection and periodically sends bytes
   * can be Python for speed of implementation

3. **Guest run configuration**

   * QEMU command line
   * any guest startup scripts or systemd unit files

4. **Short design note**

   * event loop design
   * suspend entry point
   * resume handling strategy
   * known limitations

5. **Test report**

   * log excerpts for normal operation
   * log excerpts for suspend/resume
   * at least one `pm_test` result
   * observed reconnect behavior after wake

---

## Functional requirements

### R1. Single-process event loop

The main app must use `epoll` as the central wait mechanism. The loop should monitor:

* TCP socket
* redraw timerfd
* idle timerfd
* signalfd

No busy loop is allowed. ([man7.org][2])

### R2. Periodic screen update

Redraw the screen every 2 to 5 seconds while awake. The screen should show:

* current state
* connection status
* bytes received
* last packet time
* last suspend time
* last resume time
* how many suspend cycles have completed

### R3. Network input

The app must establish a TCP connection to a host-side server and consume arbitrary bytes. For QEMU user networking, plan to connect to the host through `10.0.2.2` plus a forwarded or directly reachable port. ([QEMU][6])

### R4. Idle-triggered suspend

After a configurable period with no local input and no network traffic, the app should attempt suspend-to-idle by writing `freeze` to `/sys/power/state`. Linux documents this as a valid path into suspend-to-idle. ([Kernel Documentation][1])

### R5. Resume continuity

After wake, the same process should continue running, detect that it has resumed, redraw the screen, and reconnect if the TCP connection broke during suspend. Linux’s documented suspend/resume flow supports this model of “execution continues after resume.” ([Kernel Documentation][3])

### R6. Clean shutdown

The app must exit cleanly on `SIGINT` and `SIGTERM` using signalfd. ([man7.org][5])

---

## Non-functional requirements

* Keep dependencies minimal.
* Prefer C or Rust.
* Log every state transition with timestamps.
* Keep the app understandable enough that stage 2 can reuse most of it unchanged.

---

## Suggested implementation plan

### Phase 0: Environment setup

Set up a Linux guest in QEMU with:

* x86_64 machine
* KVM enabled if available
* user networking
* serial console available
* a persistent disk image

QEMU’s networking docs describe user-mode networking and note that the guest typically sees the host at `10.0.2.2`. ([QEMU][6])

Use a QEMU monitor channel or monitor on stdio, because QEMU exposes a `system_wakeup` command in its management interfaces, which is useful as a deterministic fallback during testing. ([QEMU][8])

### Phase 1: Event loop skeleton

Implement:

* `epoll_create1`
* nonblocking TCP socket
* one redraw timerfd
* one idle timerfd
* signalfd

At this point, print log lines only.

### Phase 2: Terminal renderer

Add a primitive renderer:

* clear screen
* print app status
* print counters
* print timestamps

A terminal renderer is enough for stage 1.

### Phase 3: Host-side byte source

Implement a simple host-side server that:

* listens on a TCP port,
* accepts one client,
* sends bytes every few seconds,
* optionally pauses and resumes,
* optionally sends bursts.

This simulates “internet data arriving.”

### Phase 4: Suspend plumbing

Add suspend entry:

* on idle timeout, log “about to suspend”
* optionally flush logs
* write `freeze\n` to `/sys/power/state`
* on return from the write path, treat that as post-resume
* log “resumed”

Linux documents `freeze` as the direct suspend-to-idle entry path. ([Kernel Documentation][1])

### Phase 5: Resume repair

On resume:

* re-check socket health
* reconnect if needed
* force immediate redraw
* restart idle timer

### Phase 6: `pm_test` support

Before using real suspend in the guest, validate incrementally with `/sys/power/pm_test`. Linux documents the `freezer`, `devices`, `platform`, `processors`, and `core` modes for staged debugging. ([Kernel Documentation][7])

---

## Suggested file layout

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

---

## APIs and interfaces that are probably relevant

These are the interfaces the intern should read before coding.

### Core Linux userspace APIs

* `epoll(7)` for the main readiness loop. ([man7.org][2])
* `timerfd_create(2)` for redraw and idle timers. ([man7.org][4])
* `signalfd(2)` for clean signal handling inside the event loop. ([man7.org][5])
* nonblocking sockets with `read(2)` / `recv(2)` semantics. The ordinary file-descriptor model applies to the socket and timerfd reads. ([man7.org][9])
* optionally `eventfd(2)` for internal wakeups if the design later grows background worker threads. ([man7.org][10])

### Power-management interfaces

* `/sys/power/state` for entering suspend-to-idle with `freeze`. ([Kernel Documentation][1])
* `/sys/power/pm_test` for staged suspend debugging. ([Kernel Documentation][7])

### QEMU interfaces

* user-mode networking and `10.0.2.2` host reachability. ([QEMU][6])
* QEMU monitor / QMP `system_wakeup` as a test wake mechanism. ([QEMU][8])

---

## Concrete behavior spec

The app should behave like this:

### Startup

* start
* connect to server
* print startup screen
* arm redraw timer
* arm idle timer

### On network bytes

* read all available bytes
* increment byte counter
* update last-network-activity timestamp
* reset idle timer
* redraw soon

### On redraw timer

* repaint the status screen
* rearm redraw timer

### On idle timeout

* log state transition to `SUSPENDING`
* write `freeze` to `/sys/power/state`
* once execution returns, log `RESUMED`
* reinitialize any timers if needed
* reconnect socket if broken
* redraw immediately

### On signal

* close descriptors
* print final counters
* exit

---

## Recommended QEMU setup

Use a simple baseline VM first.

Example ingredients:

* `-machine q35`
* `-enable-kvm`
* `-cpu host`
* `-m 1024` or `2048`
* one virtio disk
* user networking
* monitor on stdio or a socket

The exact command line can vary; the important part is that the guest has basic ACPI-capable system behavior and convenient access to the monitor. QEMU’s docs cover the monitor and networking pieces; the user networking docs specifically describe the guest/host addressing model. ([QEMU][8])

---

## Testing plan

### Test A: Event loop sanity

Verify:

* no busy CPU use at idle
* redraw timer fires
* network bytes are received
* signal handling works

Success means the process spends most of its time blocked in `epoll_wait()`.

### Test B: Network reconnection

Stop and restart the host-side server. Verify the guest app notices disconnect and reconnects.

### Test C: `pm_test=freezer`

Use `/sys/power/pm_test` with `freezer` first. Linux documents this as a mode that tests process freezing without deeper suspend stages. ([Kernel Documentation][7])

### Test D: `pm_test=devices`

Advance to `devices`, then `processors`, then `core` if supported. ([Kernel Documentation][7])

### Test E: Real `freeze`

Run actual suspend-to-idle by writing `freeze` to `/sys/power/state`. Wake the guest using whichever wake path proves reliable in the VM. If interactive input is unreliable, use QEMU’s `system_wakeup`. ([Kernel Documentation][1])

### Test F: Suspend during active network flow

Suspend while the host server is still sending bytes. On resume, verify:

* process is still alive,
* connection state is handled correctly,
* redraw occurs,
* counters remain coherent.

---

## Acceptance criteria

The lab is complete when all of the following are true:

* One process owns the main loop.
* That process uses `epoll`, `timerfd`, and `signalfd`.
* The app receives host-sent bytes over TCP.
* The app renders a visible status screen in the guest.
* The app enters `freeze` after idle.
* The guest can be resumed and the app continues.
* The app reconnects if needed after wake.
* The README is sufficient for someone else to reproduce the demo.

---

## Suggested README outline for the intern

Ask the intern to include these sections:

1. What the prototype demonstrates
2. Guest OS choice
3. How to build the app
4. How to run the host byte server
5. How to start QEMU
6. How to enable and use `pm_test`
7. How to trigger idle suspend
8. How to wake the guest
9. Known limitations
10. Next-step ideas for stage 2

---

## Stretch goals

After the base version works:

* Add optional raw keyboard input in the app itself.
* Swap terminal rendering for a DRM dumb-buffer renderer.
* Add an internal eventfd for synthetic wakeups or test injection.
* Add structured logs with suspend/resume cycle IDs.
* Add a fake “offline mode” page and “incoming message” page.

---

## Resources to assign as preread

Have the intern read these in roughly this order:

* `epoll(7)` for readiness-driven architecture. ([man7.org][2])
* `timerfd_create(2)` for timer integration into the fd loop. ([man7.org][4])
* `signalfd(2)` for event-loop-friendly signal handling. ([man7.org][5])
* Linux system sleep states, especially `freeze`. ([Kernel Documentation][1])
* Linux suspend code flows. ([Kernel Documentation][3])
* Linux basic PM debugging and `/sys/power/pm_test`. ([Kernel Documentation][7])
* QEMU networking docs for host/guest connection setup. ([QEMU][6])
* QEMU monitor docs and `system_wakeup`. ([QEMU][8])

---

## Manager notes / guidance to the intern

Tell them this explicitly:

The purpose of stage 1 is **not** to build a graphics stack. It is to prove that a mostly-idle, fd-driven process can:

* wait efficiently,
* suspend the system,
* survive resume,
* reconnect and redraw.

If they are tempted to add Wayland now, they should stop. Stage 1 should make stage 2 boring.

If you want, I can turn this into a more formal assignment handout with sections like “Context,” “Tasks,” “Milestones,” “Rubric,” and “Submission Checklist.”

[1]: https://docs.kernel.org/admin-guide/pm/sleep-states.html?utm_source=chatgpt.com "System Sleep States"
[2]: https://man7.org/linux/man-pages/man7/epoll.7.html?utm_source=chatgpt.com "epoll(7) - Linux manual page"
[3]: https://docs.kernel.org/admin-guide/pm/suspend-flows.html?utm_source=chatgpt.com "System Suspend Code Flows"
[4]: https://man7.org/linux/man-pages/man2/timerfd_create.2.html?utm_source=chatgpt.com "timerfd_create(2) - Linux manual page"
[5]: https://man7.org/linux/man-pages/man2/signalfd.2.html?utm_source=chatgpt.com "signalfd(2) - Linux manual page"
[6]: https://www.qemu.org/docs/master/system/devices/net.html?utm_source=chatgpt.com "Network emulation — QEMU documentation"
[7]: https://docs.kernel.org/power/basic-pm-debugging.html?utm_source=chatgpt.com "Debugging hibernation and suspend"
[8]: https://www.qemu.org/docs/master/system/monitor.html?utm_source=chatgpt.com "QEMU Monitor"
[9]: https://man7.org/linux/man-pages/man2/read.2.html?utm_source=chatgpt.com "read(2) - Linux manual page"
[10]: https://man7.org/linux/man-pages/man2/eventfd.2.html?utm_source=chatgpt.com "eventfd(2) - Linux manual page"
