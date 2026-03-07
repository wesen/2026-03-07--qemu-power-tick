---
Title: Diary
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
RelatedFiles:
    - Path: ttmp/2026/03/07/QEMU-01-LAB-ONE--qemu-lab-one-analysis-and-implementation-guide/design-doc/01-lab-one-detailed-analysis-and-implementation-guide.md
      Note: Primary deliverable produced during this documentation step
    - Path: ttmp/2026/03/07/QEMU-01-LAB-ONE--qemu-lab-one-analysis-and-implementation-guide/sources/local/01-lab-assignment.md
      Note: Imported assignment that shaped the diary conclusions
ExternalSources:
    - local:01-lab-assignment.md
Summary: Chronological diary for ticket setup, source import, analysis writing, and delivery.
LastUpdated: 2026-03-07T14:58:21.831611743-05:00
WhatFor: Capture what was done for the QEMU stage-1 lab analysis ticket and how to review it.
WhenToUse: Use when continuing or reviewing work on this ticket.
---



# Diary

## Goal

Capture the documentation work for the stage-1 QEMU sleep/wake lab ticket: ticket creation, source import, evidence gathering, analysis writing, ticket bookkeeping, validation, and reMarkable delivery.

## Step 1: Create The Ticket And Produce The Detailed Lab Guide

This step established the ticket workspace, imported the assignment brief from `/tmp/lab-assignment.md`, read the imported material in detail, and converted it into a concrete design and implementation guide for the proposed lab. The work focused on making the stage-1 scope operational: one primary process, one `epoll` loop, suspend-to-idle via `freeze`, deterministic wake testing, and clear reconnect/redraw behavior after resume.

The resulting design doc explains the system boundary, the Linux and QEMU APIs involved, the runtime state machine, the file layout, the testing sequence, and the main risks. It is intended to be sufficient for an unfamiliar engineer to start implementation without re-deriving the architecture from the assignment.

### Prompt Context

**User prompt (verbatim):** "Create a new docmgr ticket QEMU-01-LAB-ONE and import the file /tmp/lab-assignment.md with `docmgr import file --file ... --ticket ...` and read it in detail, Then create your own implementation plan that goes through the lab in detail. 

Create a detailed analysis / design / implementation guide that is very detailed, explaining all the parts of the system needed to understand what it is, with prose paragraphs and bullet point sand pseudocode and diagrams and api references and file
  references.
  It should be very clear and detailed. Store in the ticket and the nupload to remarkable."

**Assistant interpretation:** Create a new ticket, import the provided assignment file, read it closely, write a detailed implementation/design guide grounded in the assignment and supporting references, keep ticket bookkeeping accurate, and upload the final documentation bundle to reMarkable.

**Inferred user intent:** Produce a durable, high-quality lab handoff document that can guide actual implementation and review rather than just summarize the assignment.

**Commit (code):** N/A

### What I did

- Created ticket `QEMU-01-LAB-ONE`.
- Imported `/tmp/lab-assignment.md` into the ticket source area.
- Created the primary design doc and diary doc in the ticket.
- Read the imported assignment and extracted the behavioral requirements, scope boundaries, and suggested phases.
- Consulted the referenced Linux and QEMU API documentation to anchor the guide to the actual interfaces: `epoll`, `timerfd`, `signalfd`, `/sys/power/state`, `/sys/power/pm_test`, QEMU user networking, and QEMU monitor wake.
- Wrote a detailed design and implementation guide covering architecture, event flow, state model, pseudocode, testing, risks, and file-level guidance.

### Why

- The assignment is specific about behavioral outcomes but still leaves implementation choices open. A detailed design doc reduces ambiguity before coding begins.
- The ticket needs enough structure that another engineer can pick it up later without rereading all external materials first.

### What worked

- `docmgr ticket create-ticket` created the workspace cleanly.
- `docmgr import file --file /tmp/lab-assignment.md --ticket QEMU-01-LAB-ONE` imported the assignment into the ticket and updated the index.
- The imported assignment provided enough detail to derive a concrete state machine and phased implementation plan without inventing scope.
- The external Linux and QEMU docs aligned with the assignment and supported the recommended architecture directly.

### What didn't work

- No functional blockers occurred in this step.
- No implementation code was written yet, so suspend behavior, reconnect behavior, and wake reliability remain untested.

### What I learned

- The assignment is strongest when interpreted as a semantics lab, not a UI lab or a general VM-management lab.
- The cleanest teaching model is to treat return from the `freeze` write path as the explicit resume boundary and perform all repair logic immediately after that point.
- QEMU monitor `system_wakeup` should be part of the lab harness from the start to keep wake testing deterministic.

### What was tricky to build

- The main difficulty was choosing the right level of detail. The assignment already included a high-level plan, so the design doc needed to add concrete engineering guidance rather than restate the brief. I addressed that by explicitly defining the runtime state machine, internal data model, handler breakdown, file layout, and failure mitigations.

### What warrants a second pair of eyes

- The recommendation to prefer C for stage 1 is pragmatic, but the final language choice still depends on who will implement the lab.
- The suggested QEMU command shape is intentionally generic; it should be checked against the local guest image and host virtualization setup before being treated as the single canonical run command.

### What should be done in the future

- Implement the actual lab source tree described in the design doc.
- Run the `pm_test` sequence and confirm how wake should be triggered in the local environment.
- Capture real logs for the eventual test report.

### Code review instructions

- Start with the primary design document at `design-doc/01-lab-one-detailed-analysis-and-implementation-guide.md`.
- Cross-check it against the imported source `sources/local/01-lab-assignment.md`.
- Verify that the plan preserves the stage-1 scope and does not introduce unnecessary complexity.
- Validate the final ticket with `docmgr doctor --ticket QEMU-01-LAB-ONE --stale-after 30`.

### Technical details

- Ticket path: `ttmp/2026/03/07/QEMU-01-LAB-ONE--qemu-lab-one-analysis-and-implementation-guide`
- Imported source: `sources/local/01-lab-assignment.md`
- Primary design doc: `design-doc/01-lab-one-detailed-analysis-and-implementation-guide.md`
- Key external interfaces:
  - `epoll(7)`
  - `timerfd_create(2)`
  - `signalfd(2)`
  - `/sys/power/state`
  - `/sys/power/pm_test`
  - QEMU user networking
  - QEMU monitor `system_wakeup`

## Step 2: Scaffold The Lab Harness And Pass A Host-Side Smoke Test

I created the actual implementation scaffold in the repository root and filled in the first working version of the runtime pieces: the static guest binary, the host drip server, the initramfs builder, the guest `/init` script, and the QEMU launcher. The intent of this step was to get the real event loop and timing instrumentation compiled and exercised in a controlled environment before introducing VM boot complexity.

The host-side smoke test validated the core behavior of the guest binary without QEMU: the app connected to the local drip server, rendered status updates, tracked byte counts, ran its reconnect path on disconnect, and exited cleanly after a runtime limit. That gave me a stable baseline for the next step, which is booting the same binary inside the initramfs guest.

### Prompt Context

**User prompt (verbatim):** (see Step 1)

**Assistant interpretation:** Convert the plan into an actual implementation, track work in detail, commit incrementally, and gather real runtime measurements.

**Inferred user intent:** Move from documentation to a functioning, measured prototype while preserving traceability in the ticket.

**Commit (code):** pending

### What I did

- Added `.gitignore`, `Makefile`, and a root `README.md`.
- Added `guest/build-initramfs.sh`, `guest/init`, and `guest/run-qemu.sh`.
- Implemented `guest/sleepdemo.c` as a static binary with:
  - one `epoll` loop,
  - one TCP socket,
  - redraw and idle `timerfd`s,
  - `signalfd` shutdown handling,
  - suspend/resume metric emission,
  - reconnect scheduling,
  - a terminal status renderer.
- Implemented `host/drip_server.py` with pause/disconnect behavior that can later force an idle suspend and a reconnect case.
- Added a placeholder `scripts/measure_run.py` for the end-to-end orchestration step.
- Ran:
  - `make build`
  - `python3 -m py_compile host/drip_server.py scripts/measure_run.py`
  - `python3 host/drip_server.py --host 127.0.0.1 --port 5555 --interval 0.2 --active-seconds 30 --pause-seconds 1 --stop-after 6`
  - `./build/sleepdemo --host 127.0.0.1 --port 5555 --idle-seconds 2 --redraw-ms 500 --runtime-seconds 3 --no-suspend`

### Why

- The initramfs and QEMU harness need a working guest binary first; otherwise VM debugging would conflate guest boot issues with application issues.
- A host-side smoke test is the fastest way to validate the fd-driven loop, the renderer, the TCP logic, and the `signalfd` path.

### What worked

- Static compilation with `gcc -static` worked on this host.
- The drip server accepted the client and sent repeated payloads.
- `sleepdemo` connected immediately, rendered its status screen, updated byte and packet counters, and emitted structured log lines.
- The app observed server shutdown, transitioned to `DISCONNECTED`, and exited on its runtime limit.

### What didn't work

- The measurement runner is still a placeholder.
- The QEMU path has not been exercised yet, so suspend behavior, wake control, and guest networking are still unverified in the VM environment.

### What I learned

- The host kernel configuration is favorable for a minimal initramfs guest: `CONFIG_VIRTIO_NET=y`, `CONFIG_DEVTMPFS=y`, `CONFIG_SERIAL_8250_CONSOLE=y`, and the required suspend options are enabled.
- A custom initramfs with static `busybox` is viable here, which should keep the guest boot path short and reproducible.

### What was tricky to build

- The main tricky part was getting the right minimal guest shape. I needed to confirm whether a custom initramfs could rely on built-in kernel drivers rather than reusing the host's distro initrd. Kernel config checks showed that virtio-net and the serial console are built in, which makes the direct initramfs route practical.

### What warrants a second pair of eyes

- The reconnect policy currently retries from the redraw-timer cadence rather than from a dedicated reconnect timer. That keeps the fd set small, but it is worth reviewing once the QEMU path is working.
- The guest log stream mixes ANSI rendering with structured metric lines; the measurement parser will need to be tolerant of that.

### What should be done in the future

- Boot the initramfs guest in QEMU and verify that networking and `/sys/power/state` behave as expected.
- Implement the measurement runner and parse structured metric lines into a report.
- Expand the README once the exact run and measurement workflow has stabilized.

### Code review instructions

- Start in `guest/sleepdemo.c` to inspect the runtime model.
- Then review `host/drip_server.py`, `guest/init`, and `guest/run-qemu.sh` as the surrounding harness.
- Validate the host-side smoke test by rerunning the build and local server/app commands listed above.

### Technical details

- New files:
  - `.gitignore`
  - `Makefile`
  - `README.md`
  - `guest/build-initramfs.sh`
  - `guest/init`
  - `guest/run-qemu.sh`
  - `guest/sleepdemo.c`
  - `host/drip_server.py`
  - `scripts/hmp-command.sh`
  - `scripts/measure_run.py`
- Host-side smoke result:
  - connection established
  - repeated redraws observed
  - byte counters increased
  - disconnect path triggered when the server stopped
  - runtime-limited exit succeeded
