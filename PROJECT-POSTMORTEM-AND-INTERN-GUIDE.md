# QEMU Sleep/Wake Lab
## Project Postmortem, Architecture Walkthrough, and Intern Guide

## Why This Exists

If you open this repository cold, the first thing you notice is that it does not describe one neat, finished product. It describes a sequence of experiments, deliverables, regressions, recoveries, and increasingly ambitious graphics stacks built on the same QEMU suspend/resume lab idea.

That is both the strength and the weakness of the project.

The strength is that the repository contains real engineering work:

- a minimal stage-1 suspend/resume prototype,
- a working Weston-based Wayland stack,
- a mostly working Chromium-on-Weston kiosk stack,
- a serious direct DRM/Ozone browser investigation,
- and a very detailed trail of evidence in diaries, reports, and scripts.

The weakness is that a new person can easily confuse:

- the stable path,
- the historically important path,
- and the currently experimental path.

This document is meant to solve that problem. It is the “big picture” explanation of what the system is, how it is organized, what was built, what went wrong, what was learned, and how to continue the work without repeating the same confusion.

If you are a new intern, read this file before you start reading code.

## The Short Version

This repository started as a small QEMU lab for a sleepy, mostly idle Linux guest process. It then grew, step by step, into a full graphical testbed:

1. stage 1 proved the suspend/resume semantics in a minimal initramfs guest,
2. stage 2 replaced the text-mode output with a real Weston compositor and a custom Wayland client,
3. stage 3 replaced the custom client with Chromium running under Weston kiosk shell,
4. stage 4 attempted to simplify the browser path further by removing Weston and running Chromium `content_shell` directly on Ozone DRM,
5. later investigation tickets tried to separate “guest graphics state is wrong” from “QEMU host-visible capture is wrong.”

The current practical state is:

- stage 1 is complete enough to trust,
- stage 2 is the strongest working graphical implementation,
- stage 3 is largely successful but not fully closed under realistic device-resume,
- stage 4 is still a real investigation, not a finished implementation.

If you want the safest working path, use the Weston-based path.

If you want the most interesting open problem, continue the direct DRM/Ozone path.

## Reading Order

This repository has many ticket docs. The best reading order for a newcomer is:

1. this file,
2. [README.md](./README.md),
3. [QEMU-01 final report](./ttmp/2026/03/07/QEMU-01-LAB-ONE--qemu-lab-one-analysis-and-implementation-guide/design-doc/02-final-implementation-report.md),
4. [QEMU-02 final report](./ttmp/2026/03/07/QEMU-02-LAB-TWO--qemu-lab-two-wayland-client-analysis-and-implementation/design-doc/03-phase-2-final-implementation-report.md),
5. [QEMU-04 stage-3 review](./ttmp/2026/03/07/QEMU-04-LAB-THREE--qemu-lab-three-chromium-kiosk-analysis-and-implementation/design/02-stage-3-postmortem-and-review-guide.md),
6. [QEMU-05 QMP capture postmortem](./ttmp/2026/03/07/QEMU-05-DEVICES-RESUME-INVESTIGATION--devices-resume-display-ownership-investigation/design-doc/02-qmp-capture-path-postmortem-report.md),
7. [QEMU-07 controller-discovery guide](./ttmp/2026/03/09/QEMU-07-CONTENT-SHELL-MODESET--content-shell-drm-modeset-and-controller-mapping-investigation/design/01-content-shell-modeset-mapping-investigation-guide.md).

If you want the raw chronology rather than the cleaned final reports, the most useful diaries are:

- [QEMU-02 diary](./ttmp/2026/03/07/QEMU-02-LAB-TWO--qemu-lab-two-wayland-client-analysis-and-implementation/reference/01-diary.md)
- [QEMU-04 diary](./ttmp/2026/03/07/QEMU-04-LAB-THREE--qemu-lab-three-chromium-kiosk-analysis-and-implementation/reference/01-diary.md)
- [QEMU-05 investigation diary](./ttmp/2026/03/07/QEMU-05-DEVICES-RESUME-INVESTIGATION--devices-resume-display-ownership-investigation/reference/01-investigation-diary.md)
- [QEMU-07 diary](./ttmp/2026/03/09/QEMU-07-CONTENT-SHELL-MODESET--content-shell-drm-modeset-and-controller-mapping-investigation/reference/01-diary.md)

## What The Repository Actually Contains

At a very high level, the repository contains four kinds of things:

- guest-side programs and bootstraps,
- host-side orchestration and capture tools,
- build/packaging scripts for initramfs-based guests,
- ticket-local documentation and investigation artifacts.

The top-level directories matter for different reasons:

- [guest/](./guest): guest user space, init scripts, QEMU launchers, browser launchers, DRM helpers
- [host/](./host): QMP tools, screenshot harnesses, Chromium build/payload helpers
- [scripts/](./scripts): smaller shared helpers, especially metric extraction
- [ttmp/](./ttmp): ticket workspaces, design docs, reports, tasks, diaries, playbooks

There are also many local `results-*` directories. These are not random junk. They are the forensic evidence of the project:

- screenshots,
- serial logs,
- metrics,
- extracted DRM debugfs state,
- parser outputs.

They are noisy, but they are how many of the final conclusions were earned.

## The Core Design Pattern

Across all stages, the project keeps reusing the same broad pattern:

```text
host tools
  -> boot a small Linux guest in QEMU
  -> stimulate it with network/input/timers
  -> suspend and resume it
  -> capture evidence from serial and screenshots
  -> compare the observed state against the intended state
```

The guest is almost always built as:

```text
kernel + initramfs + one small custom /init + a small set of guest binaries
```

That design is deliberate. It keeps the lab inspectable. The work rarely depends on a full distro image, a package manager inside the guest, or a large service graph. That made the project harder to package, but much easier to reason about.

## The Three System Shapes

The codebase really contains three related but distinct systems.

### System Shape 1: Minimal Single-Process Guest

This is the stage-1 model:

```text
host
  -> QEMU
     -> Linux kernel
        -> /init
           -> sleepdemo
```

The point of this system is not graphics. It is semantics:

- idle waiting,
- suspend entry,
- RTC wake,
- reconnect,
- redraw,
- shutdown,
- structured timing metrics.

Relevant files:

- [guest/sleepdemo.c](./guest/sleepdemo.c)
- [guest/init](./guest/init)
- [guest/build-initramfs.sh](./guest/build-initramfs.sh)
- [guest/run-qemu.sh](./guest/run-qemu.sh)
- [host/drip_server.py](./host/drip_server.py)
- [scripts/measure_run.py](./scripts/measure_run.py)

The important mental model is:

```text
/init
  mount core filesystems
  bring up guest network
  exec sleepdemo

sleepdemo
  epoll_wait()
  react to socket/timer/signal readiness
  enter suspend
  resume
  emit metrics
```

This stage is the cleanest fully working subsystem in the repository.

### System Shape 2: Weston Plus A Custom Wayland Client

This is the stage-2 model:

```text
host
  -> QEMU
     -> kernel + initramfs
        -> /init
           -> udev + seatd
           -> weston --backend=drm
           -> wl_sleepdemo
```

This was the first genuinely graphical stage. It replaced “serial/text redraw” with:

- real DRM device ownership,
- real compositor startup,
- real input device enumeration,
- a real Wayland surface,
- real host screenshots and input injection.

Relevant files:

- [guest/init-phase2](./guest/init-phase2)
- [guest/build-phase2-rootfs.sh](./guest/build-phase2-rootfs.sh)
- [guest/weston.ini](./guest/weston.ini)
- [guest/wl_sleepdemo.c](./guest/wl_sleepdemo.c)
- [guest/wl_wayland.c](./guest/wl_wayland.c)
- [guest/wl_render.c](./guest/wl_render.c)
- [guest/wl_net.c](./guest/wl_net.c)
- [guest/wl_suspend.c](./guest/wl_suspend.c)
- [guest/run-qemu-phase2.sh](./guest/run-qemu-phase2.sh)

The conceptual diagram is:

```text
guest /init
  -> load DRM/input modules
  -> start systemd-udevd
  -> start seatd
  -> start weston
  -> exec wl_sleepdemo

wl_sleepdemo
  -> Wayland registry
  -> xdg-shell toplevel
  -> shm buffers
  -> keyboard/pointer listeners
  -> reconnect and suspend logic
```

This is the strongest finished graphics path in the repo.

### System Shape 3: Browser Stacks

There are two browser variants:

- stage 3: Chromium on Wayland under Weston,
- stage 4: direct DRM/Ozone `content_shell` without Weston.

They are related, but they are not the same problem.

The stage-3 browser path is a client inside a working compositor platform:

```text
Weston
  -> Wayland socket
  -> Chromium --ozone-platform=wayland
```

The stage-4 browser path asks Chromium to own DRM directly:

```text
guest /init
  -> udev + DRM
  -> content_shell --ozone-platform=drm
```

That second path is much riskier, and currently much less mature.

## The Boot And Shipping Model

One of the most important design choices in the whole project was to use custom initramfs guests rather than full disk images.

At a high level, the shipping model is:

```text
build a rootfs tree
  -> copy binaries
  -> copy libraries
  -> copy configs
  -> copy selected kernel modules
  -> package with cpio + gzip
boot QEMU with:
  -kernel ...
  -initrd ...
  rdinit=/init
```

Pseudocode:

```text
rootfs = make_empty_tree()
copy_busybox(rootfs)
copy_guest_init(rootfs)
copy_custom_binaries(rootfs)
copy_runtime_deps(rootfs)
copy_kernel_modules(rootfs)
copy_data_files(rootfs)
pack_initramfs(rootfs)
run_qemu(kernel, initramfs)
```

This had clear advantages:

- iteration remained fast,
- boot contents stayed explicit,
- no hidden distro services confused the story,
- each stage could tailor its guest exactly.

But it also created recurring pain:

- packaging graphics stacks is harder than packaging a text-mode tool,
- missing data files cause strange runtime failures,
- library sonames and symlinks matter,
- browser payloads are much larger and more fragile than small C helpers.

## Host Control Plane

The host-side tools are one of the strongest architectural successes in the project.

The host does not just “launch QEMU.” It provides a reusable control plane.

### QMP

QMP, the QEMU Machine Protocol, is used for:

- screenshots via `screendump`,
- injecting keyboard events,
- injecting pointer events,
- querying QEMU state during investigations.

The central tool is [host/qmp_harness.py](./host/qmp_harness.py).

This mattered because it gave the project a standard way to:

- capture before/after images,
- drive UI tests from the host,
- compare the guest-visible state to QEMU-visible state.

### Deterministic network stimulus

The host drip server provides a small but extremely useful network model:

- periodic bytes,
- pauses,
- disconnect-on-pause behavior,
- reconnect timing tests.

Relevant files:

- [host/drip_server.py](./host/drip_server.py)
- [host/resume_drip_server.py](./host/resume_drip_server.py)

This is not fancy, but it gave the guest a deterministic external event source and made reconnect timing measurable rather than anecdotal.

### Structured evidence extraction

The guest side logs structured lines like:

- `@@LOG`
- `@@METRIC`
- `@@DISPLAY`
- `@@DRMSTATE`

Those are later parsed by tools like:

- [scripts/measure_run.py](./scripts/measure_run.py)
- [host/extract_drmstate_from_serial.py](./host/extract_drmstate_from_serial.py)

This was one of the best early decisions in the entire repository. Without it, the later graphics investigations would have been far messier.

## Stage 1 In Detail

Stage 1 is documented in:

- [QEMU-01 index](./ttmp/2026/03/07/QEMU-01-LAB-ONE--qemu-lab-one-analysis-and-implementation-guide/index.md)
- [QEMU-01 final report](./ttmp/2026/03/07/QEMU-01-LAB-ONE--qemu-lab-one-analysis-and-implementation-guide/design-doc/02-final-implementation-report.md)
- [QEMU-01 diary](./ttmp/2026/03/07/QEMU-01-LAB-ONE--qemu-lab-one-analysis-and-implementation-guide/reference/01-diary.md)

The stage-1 system proved that a mostly idle Linux guest process could:

- block on fd readiness,
- connect to a host data source,
- suspend after becoming idle,
- wake via RTC,
- redraw immediately after resume,
- reconnect cleanly,
- emit timing metrics that could be parsed later.

Representative result values:

- `sleep_duration` around `5024-5033 ms`
- `resume_to_redraw` around `3-4 ms`
- `resume_to_reconnect` around `4-5 ms`

Why stage 1 matters even now:

- it established the suspend state-machine pattern reused later,
- it established the metric format reused later,
- it gave a working semantic baseline before graphics complexity entered the picture.

In other words, stage 1 was not throwaway code. It was the conceptual seed of everything that followed.

## Stage 2 In Detail

Stage 2 is documented in:

- [QEMU-02 index](./ttmp/2026/03/07/QEMU-02-LAB-TWO--qemu-lab-two-wayland-client-analysis-and-implementation/index.md)
- [QEMU-02 final report](./ttmp/2026/03/07/QEMU-02-LAB-TWO--qemu-lab-two-wayland-client-analysis-and-implementation/design-doc/03-phase-2-final-implementation-report.md)
- [QEMU-02 postmortem](./ttmp/2026/03/07/QEMU-02-LAB-TWO--qemu-lab-two-wayland-client-analysis-and-implementation/design-doc/02-postmortem-and-review-guide.md)
- [QEMU-02 input playbook](./ttmp/2026/03/07/QEMU-02-LAB-TWO--qemu-lab-two-wayland-client-analysis-and-implementation/reference/02-input-bring-up-playbook.md)
- [QEMU-02 diary](./ttmp/2026/03/07/QEMU-02-LAB-TWO--qemu-lab-two-wayland-client-analysis-and-implementation/reference/01-diary.md)

This stage took the core stage-1 semantics and transplanted them into a real graphical stack.

The crucial architectural change was:

```text
text-mode redraw
  -> compositor-managed Wayland surface
```

The guest bootstrap in [guest/init-phase2](./guest/init-phase2) performs a very explicit sequence:

1. mount core filesystems,
2. bring up the simple guest network,
3. load `virtio-gpu`, HID, USB, and xHCI modules,
4. start `systemd-udevd`,
5. trigger and settle input/DRM devices,
6. start `seatd`,
7. start Weston on the DRM backend,
8. launch the client.

This stage taught an important lesson: once graphics are involved, `udevd` is not optional plumbing. It is part of the correctness story.

The client itself began as a large monolith and was later modularized. The important modules became:

- [guest/wl_sleepdemo.c](./guest/wl_sleepdemo.c): thin orchestration layer
- [guest/wl_wayland.c](./guest/wl_wayland.c): Wayland registry, seat, xdg-shell
- [guest/wl_render.c](./guest/wl_render.c): shm buffer allocation and drawing
- [guest/wl_net.c](./guest/wl_net.c): reconnect behavior
- [guest/wl_suspend.c](./guest/wl_suspend.c): suspend metrics and wake behavior

That modularization was not cosmetic. It made later reasoning and stage-3 reuse much easier.

What stage 2 achieved:

- Weston on DRM booted reliably,
- the custom client rendered visibly,
- host-side keyboard injection worked,
- host-side pointer injection worked,
- suspend/resume with `pm_test=devices` worked,
- metrics and screenshots were reproducible.

Representative measured values:

- `sleep_duration = 5755 ms`
- `suspend_resume_gap = 5755 ms`
- `resume_to_redraw = 5 ms`
- `resume_to_reconnect = 1243 ms`

That reconnect figure looks long, but the project correctly identified that it includes orchestration delay and reconnect cadence, not just raw socket cost.

The strongest qualitative result of stage 2 is simple:

- the compositor path is real,
- the input path is real,
- the suspend path is real,
- and the tooling around it is reusable.

## Stage 3 In Detail

Stage 3 is documented in:

- [QEMU-04 index](./ttmp/2026/03/07/QEMU-04-LAB-THREE--qemu-lab-three-chromium-kiosk-analysis-and-implementation/index.md)
- [QEMU-04 stage-3 guide](./ttmp/2026/03/07/QEMU-04-LAB-THREE--qemu-lab-three-chromium-kiosk-analysis-and-implementation/design/01-stage-3-chromium-kiosk-guide.md)
- [QEMU-04 stage-3 review](./ttmp/2026/03/07/QEMU-04-LAB-THREE--qemu-lab-three-chromium-kiosk-analysis-and-implementation/design/02-stage-3-postmortem-and-review-guide.md)
- [QEMU-04 diary](./ttmp/2026/03/07/QEMU-04-LAB-THREE--qemu-lab-three-chromium-kiosk-analysis-and-implementation/reference/01-diary.md)

This stage reused the working Weston path and replaced the custom client with Chromium.

That mattered because it tested a different proposition:

- not “can we render something under Weston?”
- but “can we run a real browser stack, drive it from the host, and keep suspend semantics?”

The project got quite far here.

What was validated:

- Chromium booted visibly under Weston kiosk shell,
- host keyboard injection was validated against a deterministic test page,
- host pointer injection was validated against a deterministic test page,
- the browser stack survived the higher-level `pm_test=freezer` suspend path.

What remained broken:

- visible continuity under `pm_test=devices`.

This is a key subtlety. Stage 3 was not “failed.” It was partially successful and very informative. The work isolated a real split:

- the higher-level orchestration path was fine,
- the realistic device-resume graphics path was not.

That distinction prevented the project from flattening all failures into one vague “Chromium is broken” conclusion.

## The QMP Capture Investigation

The most intellectually important investigation in the repo is probably QEMU-05:

- [QEMU-05 index](./ttmp/2026/03/07/QEMU-05-DEVICES-RESUME-INVESTIGATION--devices-resume-display-ownership-investigation/index.md)
- [QEMU-05 analysis guide](./ttmp/2026/03/07/QEMU-05-DEVICES-RESUME-INVESTIGATION--devices-resume-display-ownership-investigation/design-doc/01-devices-resume-analysis-guide.md)
- [QEMU-05 QMP postmortem](./ttmp/2026/03/07/QEMU-05-DEVICES-RESUME-INVESTIGATION--devices-resume-display-ownership-investigation/design-doc/02-qmp-capture-path-postmortem-report.md)
- [QEMU-05 diary](./ttmp/2026/03/07/QEMU-05-DEVICES-RESUME-INVESTIGATION--devices-resume-display-ownership-investigation/reference/01-investigation-diary.md)

This ticket changed the project’s mental model of the problem.

Before this investigation, it was easy to say:

- the guest resumed,
- but the screenshots are wrong,
- so the guest graphics stack must be wrong.

After this investigation, that became much harder to defend.

The evidence showed:

- guest DRM debugfs state could remain healthy,
- guest-side compositor screenshots could remain healthy,
- while QMP `screendump` still showed the wrong thing.

That is a major conceptual shift.

It means:

- guest truth and host-capture truth can diverge,
- `/dev/fb0` is not necessarily the truth source in the compositor path,
- QMP `screendump` is a witness, not an oracle.

That finding is why later tickets became sharper and more believable.

## Stage 4 And The Direct DRM/Ozone Branch

Stage 4 is documented in:

- [QEMU-06 guide](./ttmp/2026/03/08/QEMU-06-CONTENT-SHELL-DRM-OZONE--chromium-content-shell-direct-drm-ozone-analysis-and-implementation/design-doc/01-direct-drm-ozone-content-shell-analysis-and-implementation-guide.md)
- [QEMU-06 diary](./ttmp/2026/03/08/QEMU-06-CONTENT-SHELL-DRM-OZONE--chromium-content-shell-direct-drm-ozone-analysis-and-implementation/reference/01-diary.md)
- [QEMU-07 guide](./ttmp/2026/03/09/QEMU-07-CONTENT-SHELL-MODESET--content-shell-drm-modeset-and-controller-mapping-investigation/design/01-content-shell-modeset-mapping-investigation-guide.md)
- [QEMU-07 diary](./ttmp/2026/03/09/QEMU-07-CONTENT-SHELL-MODESET--content-shell-drm-modeset-and-controller-mapping-investigation/reference/01-diary.md)

This branch was motivated by a seductive idea:

- if Weston already works,
- maybe removing Weston will simplify the problem.

That is partly true and partly false.

It simplified the conceptual stack:

```text
Weston + Wayland + Chromium client
  -> content_shell owning DRM directly
```

But it also shifted responsibility onto Chromium’s Ozone DRM path and made packaging much harder.

The branch still achieved a lot:

- Chromium was cloned and built from source,
- Ozone DRM and Ozone headless support were configured,
- a payload staging path was built,
- headless `content_shell` worked,
- direct DRM startup reached real GPU initialization,
- source-level Chromium instrumentation was added and saved on a dedicated branch:
  - `qemu-07-content-shell-controller-debug`

The strongest current finding from that work is not a working browser frame. It is a diagnostic one:

```text
ScreenManager::AddWindow ... controller_count=0
UpdateControllerToWindowMapping begin controllers=0 windows=1
DrmWindow::SchedulePageFlip ... controller=null ... -> ack without real flip
```

That is valuable because it collapses a vague graphics mystery into a smaller question:

- why does `content_shell` begin flipping before Chromium’s DRM path has any controllers to map against?

This branch is still open. It is educational and promising, but not yet a finished implementation.

## The Story Of What Worked

Several things in this project went very well.

### The diaries

The diaries are not filler. They are one of the best assets in the repo.

They preserve:

- the order of experiments,
- the commands that mattered,
- the exact errors,
- the false starts,
- the corrected interpretations.

Without them, the later postmortems would be much weaker, and the direct DRM path would be much harder to continue safely.

### The incremental narrowing

The project generally got better when it shrank the question:

- stage 1 validated suspend semantics,
- stage 2 validated a real compositor path,
- stage 3 validated browser integration on the compositor path,
- QEMU-05 separated guest truth from host capture truth,
- QEMU-07 separated geometry theories from controller-discovery theories.

That is good systems debugging.

### The host tooling

The project built reusable host-side tools rather than relying only on one-off shell history:

- QMP harnesses,
- screenshot scripts,
- metric parsers,
- resume capture tools,
- Chromium payload staging helpers.

This made the repo much more reviewable and much less dependent on luck.

### The willingness to revise the story

The work repeatedly corrected itself:

- generic `--window-size` was not the real `content_shell` switch,
- a bad debug change could poison interpretation,
- QMP `screendump` was not always authoritative,
- some browser-looking failures turned out to be lower-level display problems.

That willingness to revise the model is exactly what kept the project from devolving into superstition.

## The Story Of What Went Badly

The project also had recurring weaknesses.

### The repo now contains multiple historical truths

This is the hardest onboarding problem.

A newcomer can easily mistake:

- the strongest working path,
- the most recently touched path,
- and the most interesting open problem

for the same thing. They are not the same thing.

### Packaging complexity consumed a lot of time

The initramfs model was a good architectural choice, but it made the cost of graphics stacks very obvious:

- missing sonames,
- missing fontconfig state,
- missing XKB data,
- missing input/runtime services,
- missing Chromium runtime bits.

None of these are intellectually glamorous, but they matter.

### Some debug experiments contaminated the signal

This happened more than once:

- invalid control runs,
- debug logging changes that changed behavior,
- assumptions based on the wrong browser flag,
- incomplete experimental isolation.

The work did recover from these mistakes, but they slowed the pace.

### The external Chromium checkout was initially too detached from the repo story

That improved later when the instrumentation was saved on:

- `qemu-07-content-shell-controller-debug`

but this should have been stabilized earlier.

## What We Learned

The project produced several technical lessons that are worth carrying forward.

### Lesson 1: the compositor path is the pragmatic path

Weston plus a custom client was not merely easier than direct DRM. It became the most dependable graphics path in the repository.

That matters because it tells a new contributor where to stand first.

### Lesson 2: `pm_test=freezer` and `pm_test=devices` are not interchangeable

They answer different questions.

- `freezer` tells you about higher-level orchestration with limited device exercise,
- `devices` tells you much more about realistic graphics/input/device resume.

Treating them as equivalent would have hidden real failures.

### Lesson 3: host-visible capture is not always display truth

This was one of the deepest lessons in the project.

QMP screenshots can disagree with:

- guest DRM debugfs state,
- guest compositor screenshots,
- the guest application’s own understanding of the world.

That means a screenshot failure is not automatically a guest failure.

### Lesson 4: structured logs are worth the effort

The decision to emit structured tags like `@@METRIC`, `@@DISPLAY`, and `@@DRMSTATE` paid off over and over again.

### Lesson 5: direct DRM browser startup is a different class of problem

Once Weston is removed, Chromium’s own display-controller and initialization logic becomes the center of the problem. That is not just “stage 3 minus one process.” It is a fundamentally different debugging problem.

## If You Are A New Intern

Here is the advice I would give you.

### First, choose your goal

Ask yourself which of these you actually need:

1. a working suspend/resume lab,
2. a working graphical lab,
3. a working browser on the compositor path,
4. progress on the direct DRM browser path.

Those are different goals.

### If you need a stable implementation

Use the Weston path.

Recommended sequence:

1. reproduce stage 1,
2. reproduce stage 2,
3. reproduce stage 3 with `pm_test=freezer`,
4. only then touch the harder investigations.

### If you need to continue the direct DRM work

Start from the saved Chromium instrumentation:

- Chromium checkout: `/home/manuel/chromium/src`
- branch: `qemu-07-content-shell-controller-debug`

Then do this:

1. keep the current controller-discovery hypothesis,
2. instrument the display-discovery/configuration path more aggressively,
3. prove whether controllers are discovered too late or never handed off,
4. only after that revisit window-bounds theories.

### Read the right artifacts

Do not start from random code first. Start from:

- this document,
- [QEMU-02 final report](./ttmp/2026/03/07/QEMU-02-LAB-TWO--qemu-lab-two-wayland-client-analysis-and-implementation/design-doc/03-phase-2-final-implementation-report.md),
- [QEMU-05 QMP postmortem](./ttmp/2026/03/07/QEMU-05-DEVICES-RESUME-INVESTIGATION--devices-resume-display-ownership-investigation/design-doc/02-qmp-capture-path-postmortem-report.md),
- [QEMU-07 guide](./ttmp/2026/03/09/QEMU-07-CONTENT-SHELL-MODESET--content-shell-drm-modeset-and-controller-mapping-investigation/design/01-content-shell-modeset-mapping-investigation-guide.md).

That order gives you the cleanest mental map.

## Practical File Guide

If you want to understand the repository through files rather than tickets, start here.

### Core stage-1 files

- [guest/sleepdemo.c](./guest/sleepdemo.c)
- [guest/init](./guest/init)
- [guest/build-initramfs.sh](./guest/build-initramfs.sh)
- [guest/run-qemu.sh](./guest/run-qemu.sh)

### Core Weston path files

- [guest/build-phase2-rootfs.sh](./guest/build-phase2-rootfs.sh)
- [guest/init-phase2](./guest/init-phase2)
- [guest/weston.ini](./guest/weston.ini)
- [guest/wl_wayland.c](./guest/wl_wayland.c)
- [guest/wl_render.c](./guest/wl_render.c)
- [guest/wl_suspend.c](./guest/wl_suspend.c)

### Core browser-on-Weston files

- [guest/build-phase3-rootfs.sh](./guest/build-phase3-rootfs.sh)
- [guest/init-phase3](./guest/init-phase3)
- [guest/chromium-wayland-launcher.sh](./guest/chromium-wayland-launcher.sh)

### Core direct DRM files

- [guest/build-phase4-rootfs.sh](./guest/build-phase4-rootfs.sh)
- [guest/init-phase4-drm](./guest/init-phase4-drm)
- [guest/content-shell-drm-launcher.sh](./guest/content-shell-drm-launcher.sh)
- [host/configure_phase4_chromium_build.sh](./host/configure_phase4_chromium_build.sh)
- [host/stage_phase4_chromium_payload.sh](./host/stage_phase4_chromium_payload.sh)

### Core evidence tools

- [host/qmp_harness.py](./host/qmp_harness.py)
- [host/capture_phase4_smoke.py](./host/capture_phase4_smoke.py)
- [host/extract_drmstate_from_serial.py](./host/extract_drmstate_from_serial.py)
- [scripts/measure_run.py](./scripts/measure_run.py)

## Recommended Next Steps

If the goal is “make the project more practically usable,” the best next work is:

1. keep the Weston path healthy and documented,
2. decide whether stage-3 `pm_test=devices` should be fixed or explicitly documented as a known limitation,
3. continue the direct DRM path only if there is a strong reason to own DRM directly.

If the goal is “solve the most interesting open systems problem,” the next work is:

1. keep the current Chromium debug branch,
2. instrument display discovery more strongly than `VLOG`,
3. prove exactly why `ScreenManager` has zero controllers at first-flip time,
4. only then revisit modeset and window-size hypotheses.

## Final Assessment

This repository is not a neat single-product codebase. It is a layered record of a systems project that kept moving forward by building small working systems, measuring them, and then climbing to more ambitious ones.

That means two things are true at once:

- it is messier than a greenfield polished codebase,
- and it is much more honest than one.

The strongest completed implementation is the Weston-based path. The most educational unresolved work is the direct DRM/Ozone path. The best project artifact is the documentation trail: the tickets, diaries, postmortems, and harnesses together tell a coherent story of real engineering progress.

If you are here to use something today, use the Weston path.

If you are here to learn, study the tickets.

If you are here to push the frontier, continue the direct DRM investigation carefully and keep writing the diary.
