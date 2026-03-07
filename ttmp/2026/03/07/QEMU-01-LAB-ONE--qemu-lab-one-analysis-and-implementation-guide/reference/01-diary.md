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
    - Path: guest/init
      Note: Initramfs bootstrap that now uses static QEMU networking
    - Path: guest/run-qemu.sh
      Note: QEMU launcher used for the measured runs
    - Path: guest/sleepdemo.c
      Note: Core guest runtime with suspend and measurement logic
    - Path: host/drip_server.py
      Note: Host traffic generator used to force idle and reconnect behavior
    - Path: results/metrics.json
      Note: Parsed measurement summary from the devices run
    - Path: scripts/measure_run.py
      Note: Metric parser used on the serial log
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

**Commit (code):** `78baa4d` — "Scaffold QEMU sleep lab harness"

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

## Step 3: Add The Bonus-Point Near-Single-Process Track

I recorded an explicit bonus-point direction in the ticket: keep the current `busybox` bootstrap for the fastest path to a working suspend/resume prototype, but track a follow-up that pushes more early-boot and shutdown responsibilities into `sleepdemo` itself. That preserves momentum while making the stricter "sleepdemo does almost everything" interpretation visible in the documentation and task list.

This step did not change runtime behavior. Its purpose was to clarify that the current initramfs design is a pragmatic bootstrap layer rather than the final philosophical endpoint of the lab.

### Prompt Context

**User prompt (verbatim):** "ok, let's start with busybox then. but make a \"bonus point\" section in the implementation guide and in the tasks to make sleepdemo do everything itself as much as possible.

Then continue."

**Assistant interpretation:** Keep the current busybox-based implementation path, but document a concrete stretch goal that reduces bootstrap dependencies and moves more work into `sleepdemo`.

**Inferred user intent:** Preserve the fast path without losing sight of the cleaner near-single-process architecture.

**Commit (code):** `83f1868` — "Validate QEMU suspend measurements"

### What I did

- Added a `Bonus Point` section to `tasks.md`.
- Added a `Bonus Point: Move Toward A Near-Single-Process Guest` section to the primary design doc.
- Framed the current busybox-based initramfs as a thin bootstrap layer, not a rejection of the more aggressive single-process idea.

### Why

- The current bootstrap is the fastest way to get real suspend/resume data.
- The stricter near-single-process model is still valuable and should remain visible as an engineering target.

### What worked

- The documentation now distinguishes between the initial pragmatic implementation and the bonus-point refinement path.

### What didn't work

- No runtime changes were made in this step.

### What I learned

- The user’s preference is clear: the thin-init approach is acceptable as long as the stronger “sleepdemo does more itself” goal remains explicit.

### What was tricky to build

- The subtle part was framing the bonus-point work so it stays ambitious without derailing the primary milestone sequence.

### What warrants a second pair of eyes

- The eventual boundary between mandatory early-boot setup and what should move into `sleepdemo` will be worth revisiting after the end-to-end measurements are in place.

### What should be done in the future

- Resume QEMU bring-up and complete suspend/resume validation.
- Revisit the bonus-point path once the baseline implementation is stable and measured.

### Code review instructions

- Read the `Bonus Point` section in `tasks.md`.
- Read the corresponding bonus-point section in the primary design doc and verify that it aligns with the intended scope.

### Technical details

- Updated files:
  - `ttmp/.../tasks.md`
  - `ttmp/.../design-doc/01-lab-one-detailed-analysis-and-implementation-guide.md`

## Step 4: Bring Up QEMU, Fix The Guest Harness, And Capture Suspend/Resume Measurements

This step converted the scaffold into a working QEMU-based lab harness and produced the first real timing measurements. The guest now boots from the custom initramfs, configures networking for QEMU user mode, runs `sleepdemo`, reaches the suspend path, and successfully resumes under `pm_test` validation modes with parsed metric output.

The work also surfaced two environment-specific constraints that matter for future readers. First, the original busybox install step created absolute symlinks back into the host build directory, which broke `/init` inside the guest. Second, the initial `udhcpc` approach obtained a lease message but did not actually configure the interface because there was no DHCP hook script in the initramfs. Both problems were fixed, and the resulting harness is now stable enough to measure `freezer` and `devices` suspend paths end to end.

### Prompt Context

**User prompt (verbatim):** (see Step 1, plus the follow-up request to continue implementation and keep a detailed diary)

**Assistant interpretation:** Continue through QEMU bring-up, runtime validation, measurement capture, and ticket-quality documentation while keeping the work traceable.

**Inferred user intent:** Produce a functioning and measured prototype, not just a code scaffold.

**Commit (code):** `83f1868` — "Validate QEMU suspend measurements"

### What I did

- Fixed the initramfs busybox applet installation to avoid absolute symlinks that were invalid inside the guest.
- Changed guest network setup from `udhcpc`-only to explicit static QEMU user-net configuration:
  - guest IP `10.0.2.15/24`
  - default route via `10.0.2.2`
- Verified a no-suspend QEMU smoke run:
  - guest booted,
  - `sleepdemo` connected to the host drip server,
  - counters increased,
  - periodic redraws happened,
  - runtime-limited exit shut the VM down cleanly.
- Attempted real `freeze` suspend-to-idle and confirmed entry into `s2idle`.
- Tried wake strategies for real `freeze`:
  - QEMU monitor `system_wakeup`
  - QEMU `sendkey ret`
  - guest-programmed RTC wakealarm
- Determined that those did not wake this guest cleanly in the current VM environment.
- Added `no_console_suspend` and RTC wakealarm support in `sleepdemo` so suspend/resume timing became visible when `pm_test` auto-resumed.
- Ran and captured successful measurement passes for:
  - `pm_test=freezer`
  - `pm_test=devices`
- Added a metric parser in `scripts/measure_run.py`.
- Copied the implementation scripts into the ticket `scripts/` directory:
  - `01-build-initramfs.sh`
  - `02-run-qemu.sh`
  - `03-init`
  - `04-drip_server.py`
  - `05-hmp-command.sh`
  - `06-measure_run.py`
  - `07-guestfwd-to-drip.sh`

### Why

- QEMU bring-up had to be stable before any suspend measurements were trustworthy.
- `pm_test` provides a controlled auto-resume path even when true VM wake behavior is unreliable.
- The ticket needed to include the exact scripts that shaped the runtime evidence, not just the narrative description.

### What worked

- Static binary + custom initramfs booted successfully in QEMU.
- Explicit guest networking made the host server reachable through `10.0.2.2`.
- `pm_test=freezer` resumed automatically and produced usable metrics.
- `pm_test=devices` resumed automatically and produced usable metrics, including reconnect-after-resume timing.
- The metric parser extracted the latest measurements from `results/guest-serial.log`.

### What didn't work

- Real `freeze` entered `s2idle` correctly but did not wake again in this QEMU environment with the wake methods tried so far.
- QEMU monitor `system_wakeup` reported that the guest was not in a suspended QEMU power state, which is consistent with Linux guest `s2idle` not mapping to a QEMU-level suspend state.
- The initial `busybox --install -s` strategy broke the guest because the generated symlinks were absolute host paths.
- The initial DHCP-only setup looked successful in logs but did not configure a usable route inside the initramfs.

### What I learned

- For this guest, `pm_test` is the reliable way to exercise and measure the suspend/resume path.
- The real `freeze` path can be validated for entry, but its wake path is currently a VM/platform issue rather than an application control-flow issue.
- A custom initramfs must avoid assumptions about distro helper scripts; early-boot networking needs to be explicitly handled.

### What was tricky to build

- The networking bug was subtle because `udhcpc` printed a successful lease message, which initially looked like success even though the route never became active. I fixed that by using the known QEMU user-net addresses directly in `/init`.
- The wake problem was also tricky because the guest clearly entered suspend, but the usual QEMU wake commands did not apply to the guest’s `s2idle` state. I worked around that by using `pm_test` to get controlled resumes and still collect the required latency data.

### What warrants a second pair of eyes

- The real `freeze` wake limitation should be reviewed with someone who has deeper QEMU/ACPI wake expertise; there may be a machine-model or wake-source detail still missing.
- The current reconnect metric is excellent for the `pm_test=devices` case, but the real-freeze reconnect path remains blocked on wake reliability.

### What should be done in the future

- Investigate a reliable wake source for real guest `s2idle` in QEMU.
- Refresh the ticket bundle and upload the updated version to reMarkable.
- Consider whether the bonus-point near-single-process path should subsume more of `/init`.

### Code review instructions

- Review `guest/build-initramfs.sh`, `guest/init`, and `guest/run-qemu.sh` together as one boot path.
- Review `guest/sleepdemo.c` around the suspend helper, reconnect scheduling, and metric emission.
- Review `scripts/measure_run.py` against `results/guest-serial.log`.
- Verify the successful measured runs with:
  - `PM_TEST=freezer`
  - `PM_TEST=devices`

### Technical details

- Successful measured metrics from the `pm_test=devices` reconnect run:
  - `sleep_duration = 5028 ms`
  - `suspend_resume_gap = 5028 ms`
  - `resume_to_redraw = 3 ms`
  - `resume_to_reconnect = 4 ms`
- Successful measured metrics from the `pm_test=freezer` run:
  - `sleep_duration = 5033 ms`
  - `suspend_resume_gap = 5033 ms`
  - `resume_to_redraw = 3 ms`
- Key commands run:
  - `make build initramfs`
  - `python3 host/drip_server.py --host 0.0.0.0 --port 5555 --interval 0.25 --active-seconds 3 --pause-seconds 8 --disconnect-on-pause --stop-after 20`
  - `RUNTIME_SECONDS=15 IDLE_SECONDS=3 WAKE_SECONDS=4 PM_TEST=freezer RESULTS_DIR=results guest/run-qemu.sh --kernel build/vmlinuz --initramfs build/initramfs.cpio.gz`
  - `RUNTIME_SECONDS=17 IDLE_SECONDS=3 RECONNECT_MS=7000 WAKE_SECONDS=4 PM_TEST=devices RESULTS_DIR=results guest/run-qemu.sh --kernel build/vmlinuz --initramfs build/initramfs.cpio.gz`
  - `python3 scripts/measure_run.py --serial-log results/guest-serial.log --json-out results/metrics.json`
