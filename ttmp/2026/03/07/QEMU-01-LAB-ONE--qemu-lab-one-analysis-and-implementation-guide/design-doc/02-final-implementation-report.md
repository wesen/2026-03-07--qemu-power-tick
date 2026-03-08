---
Title: Final Implementation Report
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
    - Path: guest/init
      Note: |-
        Busybox-based early boot bootstrap for the initramfs guest
        Bootstrap path explained in the report
    - Path: guest/run-qemu.sh
      Note: |-
        Reproducible QEMU launcher used for measured runs
        QEMU execution path used for the reported results
    - Path: guest/sleepdemo.c
      Note: |-
        Main guest runtime implementation and metric emission
        Main implementation described in the report
    - Path: host/drip_server.py
      Note: |-
        Host-side byte source used to trigger reconnect and idle paths
        Host traffic generator used in the reported runs
    - Path: results/metrics.json
      Note: |-
        Parsed metric summary from the measured devices run
        Measured values cited in the report
    - Path: scripts/measure_run.py
      Note: Parser for extracting structured metrics from the guest serial log
    - Path: ttmp/2026/03/07/QEMU-01-LAB-ONE--qemu-lab-one-analysis-and-implementation-guide/design-doc/01-lab-one-detailed-analysis-and-implementation-guide.md
      Note: Original design and implementation guide
    - Path: ttmp/2026/03/07/QEMU-01-LAB-ONE--qemu-lab-one-analysis-and-implementation-guide/reference/01-diary.md
      Note: |-
        Chronological working diary
        Chronological source material synthesized into the report
ExternalSources:
    - local:01-lab-assignment.md
    - https://docs.kernel.org/admin-guide/pm/sleep-states.html
    - https://docs.kernel.org/power/basic-pm-debugging.html
    - https://man7.org/linux/man-pages/man7/epoll.7.html
    - https://man7.org/linux/man-pages/man2/timerfd_create.2.html
    - https://man7.org/linux/man-pages/man2/signalfd.2.html
    - https://www.qemu.org/docs/master/system/devices/net.html
Summary: Detailed final submission report for the QEMU stage-1 sleep/wake lab, including implementation, debugging, measurements, struggles, and lessons learned.
LastUpdated: 2026-03-07T16:02:45.041112503-05:00
WhatFor: Serve as the submission-ready end report for the implemented lab.
WhenToUse: Use when submitting, reviewing, or handing off the completed stage-1 lab.
---


# Final Implementation Report

## Executive Summary

This report documents the implementation and validation of the stage-1 QEMU sleep/wake prototype requested by the assignment. The final system uses a minimal initramfs guest, one primary long-running user-space program called `sleepdemo`, and a host-side TCP drip server. `sleepdemo` uses one `epoll` loop, one TCP socket, two `timerfd`s, and one `signalfd` to model idle waiting, suspend entry, resume continuation, redraw, reconnect, and clean shutdown.

The implementation successfully booted inside QEMU, reached the host server through QEMU user networking, rendered a visible text status screen, and emitted structured metric lines. Both `pm_test=freezer` and `pm_test=devices` resume paths worked and produced usable timing measurements. The strongest measured run recorded:

- `sleep_duration = 5028 ms`
- `suspend_resume_gap = 5028 ms`
- `resume_to_redraw = 3 ms`
- `resume_to_reconnect = 4 ms`

The main unresolved limitation is real `freeze` wake in this QEMU environment. Real suspend entry into `s2idle` works, but the wake strategies tried during implementation did not bring the guest back reliably. This report therefore distinguishes clearly between what was fully validated and what remains environment-limited.

## Problem Statement

The assignment did not ask for a polished application or a power-measurement product. It asked for a semantics prototype: a small Linux guest in QEMU that demonstrates how a mostly-idle, fd-driven process should behave across idle waiting, suspend entry, wake, reconnect, and redraw.

That leads to a precise technical problem:

- represent the application as one primary process
- keep the process blocked on file-descriptor readiness rather than polling
- integrate network input, timers, and signals into a single event loop
- enter Linux suspend-to-idle through the documented sysfs interface
- continue execution after resume and repair transient runtime state

The difficulty is not only writing the program. The difficulty is building a controlled environment where those semantics are observable, reproducible, and measurable.

## Proposed Solution

The implemented solution uses three cooperating pieces:

1. A host-side TCP drip server.
2. A QEMU guest booted from a custom initramfs.
3. A statically linked guest executable named `sleepdemo`.

High-level diagram:

```text
+---------------------------------------------------------------+
| Host                                                          |
|                                                               |
|  host/drip_server.py                                          |
|    - sends periodic bytes                                     |
|    - can pause and disconnect                                 |
|                                                               |
|  guest/run-qemu.sh                                            |
|    - launches kernel + initramfs                              |
|    - captures serial log                                      |
+-------------------------------+-------------------------------+
                                |
                                | QEMU user networking
                                v
+---------------------------------------------------------------+
| Guest                                                         |
|                                                               |
|  /init                                                        |
|    - mounts proc/sys/dev                                      |
|    - configures static QEMU user-net route                    |
|    - launches sleepdemo                                       |
|                                                               |
|  /bin/sleepdemo                                               |
|    - epoll loop                                               |
|    - socket fd                                                |
|    - redraw timerfd                                           |
|    - idle timerfd                                             |
|    - signalfd                                                 |
|    - suspend helper                                           |
|    - metric emission                                          |
+---------------------------------------------------------------+
```

The design keeps the bootstrap honest and small. `busybox` is used only to make the initramfs capable of mounting filesystems, configuring the expected guest network, and launching `sleepdemo`. Once the guest is running, `sleepdemo` is the only meaningful long-running user-space process.

## Design Decisions

### Use a custom initramfs instead of a full guest disk image

This made the guest reproducible and fully under local control. The tradeoff was that early-boot setup had to be handled explicitly instead of relying on a distro environment.

### Keep the runtime in C

This kept the mapping from the design to the Linux APIs explicit: `epoll`, `timerfd`, `signalfd`, nonblocking sockets, sysfs writes, and timing capture all remain close to the operating system model the lab is meant to teach.

### Emit structured metrics directly from the guest

This was one of the most useful decisions. It turned runtime validation into a parsing problem instead of a log-forensics problem.

### Treat `pm_test` as a first-class validation path

This allowed the work to produce meaningful suspend/resume timing results even though real-freeze wake remains limited by the VM environment.

## Alternatives Considered

### Full distro guest image

Rejected for the initial implementation because it would have increased boot complexity and made early-user-space behavior harder to reason about.

### No busybox, make `sleepdemo` do everything immediately

Not rejected as a long-term idea, but deferred. The ticket now includes this as a bonus-point path. It would create a cleaner near-single-process story, but it would also push more bootstrapping work into the app before the core suspend semantics were validated.

### Use async signal handlers

Rejected because `signalfd` integrates cleanly into the single event loop and avoids out-of-band control flow.

### Polling loop with sleeps

Rejected because it defeats the purpose of the lab. The lab specifically exists to validate a readiness-driven architecture.

## Implementation Plan

The implementation followed these phases in practice:

1. Create the ticket and write the architecture guide.
2. Scaffold the repo with `Makefile`, guest code, host server, and QEMU scripts.
3. Pass a host-only smoke test outside QEMU.
4. Bring up the initramfs guest in QEMU.
5. Fix guest bootstrap and networking issues.
6. Capture a working no-suspend QEMU run.
7. Validate `pm_test=freezer`.
8. Validate `pm_test=devices` with reconnect-after-resume behavior.
9. Attempt real `freeze` and document the wake limitation.
10. Produce metric summaries, update ticket docs, and prepare submission output.

The practical implementation files are:

- [sleepdemo.c](/home/manuel/code/wesen/2026-03-07--qemu-power-tick/guest/sleepdemo.c)
- [init](/home/manuel/code/wesen/2026-03-07--qemu-power-tick/guest/init)
- [build-initramfs.sh](/home/manuel/code/wesen/2026-03-07--qemu-power-tick/guest/build-initramfs.sh)
- [run-qemu.sh](/home/manuel/code/wesen/2026-03-07--qemu-power-tick/guest/run-qemu.sh)
- [drip_server.py](/home/manuel/code/wesen/2026-03-07--qemu-power-tick/host/drip_server.py)
- [measure_run.py](/home/manuel/code/wesen/2026-03-07--qemu-power-tick/scripts/measure_run.py)

### Core Runtime Behavior

The runtime loop is the center of the system:

```c
for (;;) {
    int n = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);
    for each ready event:
        if socket: connect, read, detect disconnect, or reconnect
        if redraw timer: repaint terminal state
        if idle timer: attempt suspend
        if signalfd: shut down cleanly
}
```

When suspend is triggered, `sleepdemo`:

1. records timing baselines,
2. optionally programs `pm_test`,
3. optionally programs an RTC wakealarm,
4. writes `freeze\n` to `/sys/power/state`,
5. treats return from that call as post-resume,
6. emits timing metrics,
7. redraws,
8. reconnects if necessary.

## What Worked

Several things worked cleanly by the end:

- Static guest build and initramfs packaging.
- QEMU boot with serial logging.
- QEMU user networking once the guest network was configured explicitly.
- Host-to-guest TCP traffic and visible byte counters.
- Idle-triggered suspend entry.
- Automatic resume under `pm_test=freezer`.
- Automatic resume under `pm_test=devices`.
- Reconnect-after-resume timing capture.
- Metric parsing into [metrics.json](/home/manuel/code/wesen/2026-03-07--qemu-power-tick/results/metrics.json).

## What I Struggled With

### Busybox symlink installation inside the initramfs

The first initramfs build used a busybox install mode that created absolute symlinks back into the host build directory. Inside the guest, those paths were invalid. The result was a broken `/init` where commands like `mount` were not found even though busybox had been copied in.

The fix was to perform the busybox applet install from inside the rootfs so the resulting applet paths resolved locally.

### DHCP appeared successful but did not configure the guest

`udhcpc` printed a successful lease acquisition from `10.0.2.2`, which looked correct at first glance. But because the initramfs had no DHCP hook script, the route and address were not actually installed in a useful way. The guest still could not connect to the host server.

The fix was to stop pretending the initramfs was a mini distro and configure the well-known QEMU user-net guest IP and default route directly in `/init`.

### QEMU wake semantics for real `freeze`

This was the hardest unresolved issue. The guest entered `s2idle` and kernel logs confirmed real suspend entry. But:

- `system_wakeup` did not apply
- virtual keyboard injection did not resume the guest
- the RTC wakealarm path did not wake the guest from real `freeze` in this VM setup

This showed that guest kernel sleep state and QEMU VM wake behavior are not interchangeable concepts.

## What I Learned

### The event-driven design was the right model

The combination of `epoll`, `timerfd`, `signalfd`, and nonblocking sockets made the implementation align naturally with the lab goal.

### Small bootstrap layers are acceptable if they stay minimal

The `busybox`-based initramfs did not undermine the assignment. It enabled a quick, controlled bring-up without turning the prototype into a distro integration exercise.

### Build measurement hooks into the first implementation

The structured `@@METRIC` lines made the later reporting step trivial by comparison. This was one of the highest-value design choices in the project.

### `pm_test` is essential for suspend debugging

Without `pm_test`, the project would have stalled on the wake limitation of real `freeze`. With `pm_test`, the suspend/resume logic could still be validated and measured.

## Measurements

The strongest measured run was the `pm_test=devices` reconnect case, parsed into [metrics.json](/home/manuel/code/wesen/2026-03-07--qemu-power-tick/results/metrics.json):

- `sleep_duration = 5028 ms`
- `suspend_resume_gap = 5028 ms`
- `resume_to_redraw = 3 ms`
- `resume_to_reconnect = 4 ms`

The successful `pm_test=freezer` run measured:

- `sleep_duration = 5033 ms`
- `suspend_resume_gap = 5033 ms`
- `resume_to_redraw = 3 ms`

Interpretation:

- The guest survives the resume path correctly under staged suspend validation.
- Redraw after resume is effectively immediate for this small prototype.
- Reconnect after resume is also effectively immediate once the server becomes reachable again.

## Current Limitations

The implementation is complete enough to submit, but not complete enough to overstate.

Current limitation:

- Real `freeze` suspend-to-idle entry works.
- Real wake from that state was not achieved reliably in this QEMU environment.
- Because of that, the final measured resume numbers come from `pm_test` validation modes rather than from a full real-freeze wake cycle.

That limitation should be reported honestly. It does not invalidate the working runtime model or the measured `pm_test` results, but it does leave one final environment-specific issue open.

## Open Questions

1. What is the correct wake source for real guest `s2idle` in this QEMU setup?
2. Is there a machine-model or ACPI configuration change that would make RTC or monitor wake effective here?
3. Should the next iteration prioritize solving real-freeze wake, or the bonus-point near-single-process guest refinement?

## References

- Original architecture guide: [01-lab-one-detailed-analysis-and-implementation-guide.md](./01-lab-one-detailed-analysis-and-implementation-guide.md)
- Working diary: [01-diary.md](../reference/01-diary.md)
- Imported assignment: [01-lab-assignment.md](../sources/local/01-lab-assignment.md)
- Metric summary: [metrics.json](/home/manuel/code/wesen/2026-03-07--qemu-power-tick/results/metrics.json)
- Guest runtime: [sleepdemo.c](/home/manuel/code/wesen/2026-03-07--qemu-power-tick/guest/sleepdemo.c)
- Guest bootstrap: [init](/home/manuel/code/wesen/2026-03-07--qemu-power-tick/guest/init)
- QEMU launcher: [run-qemu.sh](/home/manuel/code/wesen/2026-03-07--qemu-power-tick/guest/run-qemu.sh)
- Host drip server: [drip_server.py](/home/manuel/code/wesen/2026-03-07--qemu-power-tick/host/drip_server.py)
