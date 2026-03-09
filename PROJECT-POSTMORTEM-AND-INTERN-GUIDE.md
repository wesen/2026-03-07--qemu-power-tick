# QEMU Sleep/Wake Lab Project Postmortem And Intern Guide

## What This Document Is

This document is the project-level end result of the work in this repository. It is meant to serve four jobs at once:

- a retrospective on what was actually built,
- a design guide for a new intern who needs to understand the stack quickly,
- an implementation map showing how the repository is organized,
- a postmortem explaining what worked, what failed, what remains unfinished, and why.

This is not just a stage-1 summary. The repository evolved through several distinct efforts:

- stage 1: single-process suspend/resume lab,
- stage 2: Weston plus a custom Wayland client,
- stage 3: Chromium on Weston kiosk mode,
- stage 4: direct DRM/Ozone `content_shell`,
- follow-up investigations into resume capture and direct-DRM controller discovery.

The important outcome is that the repository now contains multiple partially overlapping system models. A new contributor must understand which parts are stable, which parts are experimental, and which parts are still open problems.

## Executive Summary

The project succeeded in building a real suspend/resume lab environment inside QEMU. The strongest completed results are:

- a stage-1 single-process guest with measured suspend/resume behavior,
- a stage-2 Weston-based guest GUI stack with a working custom Wayland client,
- host-side QMP screenshot and input automation,
- measured suspend/resume and reconnect timings,
- a stage-3 Chromium-on-Weston path that visibly boots and accepts host keyboard and pointer input.

The project did not fully close every target. The most important unresolved areas are:

- `pm_test=devices` display continuity in the Weston-based graphics path,
- host-side QEMU screenshot/capture ambiguity after device-resume,
- direct DRM/Ozone `content_shell` visible scanout, where Chromium currently begins page-flipping before `ScreenManager` has any controllers.

The short version is:

- the infrastructure is real,
- the Wayland compositor path is much more mature than the direct DRM path,
- the repository contains strong evidence and good experiments,
- but the direct Ozone DRM branch remains a debug target rather than a finished solution.

## Repository Map

The main directories matter for different reasons:

- [guest/](./guest) contains guest bootstraps, guest binaries, and QEMU launchers.
- [host/](./host) contains host-side orchestration, QMP helpers, capture tools, and smoke harnesses.
- [scripts/](./scripts) contains reusable helper scripts shared across phases.
- [ttmp/](./ttmp) contains ticket workspaces, long-form docs, tasks, diaries, playbooks, and investigation artifacts.
- `results-*` directories contain local runtime evidence from many experiments and should be treated as uncommitted investigation artifacts.

The most important guest-side files are:

- [guest/sleepdemo.c](./guest/sleepdemo.c)
- [guest/init](./guest/init)
- [guest/build-initramfs.sh](./guest/build-initramfs.sh)
- [guest/run-qemu.sh](./guest/run-qemu.sh)
- [guest/init-phase2](./guest/init-phase2)
- [guest/build-phase2-rootfs.sh](./guest/build-phase2-rootfs.sh)
- [guest/wl_sleepdemo.c](./guest/wl_sleepdemo.c)
- [guest/wl_suspend.c](./guest/wl_suspend.c)
- [guest/init-phase3](./guest/init-phase3)
- [guest/build-phase3-rootfs.sh](./guest/build-phase3-rootfs.sh)
- [guest/init-phase4-drm](./guest/init-phase4-drm)
- [guest/content-shell-drm-launcher.sh](./guest/content-shell-drm-launcher.sh)
- [guest/build-phase4-rootfs.sh](./guest/build-phase4-rootfs.sh)

The most important host-side files are:

- [host/drip_server.py](./host/drip_server.py)
- [host/qmp_harness.py](./host/qmp_harness.py)
- [host/capture_phase2_checkpoints.py](./host/capture_phase2_checkpoints.py)
- [host/capture_phase3_checkpoints.py](./host/capture_phase3_checkpoints.py)
- [host/capture_phase4_smoke.py](./host/capture_phase4_smoke.py)
- [host/extract_drmstate_from_serial.py](./host/extract_drmstate_from_serial.py)
- [host/stage_phase4_chromium_payload.sh](./host/stage_phase4_chromium_payload.sh)
- [host/configure_phase4_chromium_build.sh](./host/configure_phase4_chromium_build.sh)

The most important documentation tickets are:

- [QEMU-01](./ttmp/2026/03/07/QEMU-01-LAB-ONE--qemu-lab-one-analysis-and-implementation-guide/index.md)
- [QEMU-02](./ttmp/2026/03/07/QEMU-02-LAB-TWO--qemu-lab-two-wayland-client-analysis-and-implementation/index.md)
- [QEMU-04](./ttmp/2026/03/07/QEMU-04-LAB-THREE--qemu-lab-three-chromium-kiosk-analysis-and-implementation/index.md)
- [QEMU-05](./ttmp/2026/03/07/QEMU-05-DEVICES-RESUME-INVESTIGATION--devices-resume-display-ownership-investigation/index.md)
- [QEMU-06](./ttmp/2026/03/08/QEMU-06-CONTENT-SHELL-DRM-OZONE--chromium-content-shell-direct-drm-ozone-analysis-and-implementation/index.md)
- [QEMU-07](./ttmp/2026/03/09/QEMU-07-CONTENT-SHELL-MODESET--content-shell-drm-modeset-and-controller-mapping-investigation/index.md)

## System Model

There are three system shapes in this repository.

### 1. Minimal single-process guest

This is the stage-1 shape:

```text
host
  -> QEMU
     -> Linux kernel
        -> /init
           -> sleepdemo
```

This path is intentionally tiny. It exists to test suspend timing, reconnect behavior, redraw timing, and the mechanics of booting a minimal guest out of a custom initramfs.

### 2. Weston plus a custom Wayland client

This is the stage-2 shape:

```text
host
  -> QEMU
     -> kernel + initramfs
        -> /init (mounts fs, loads modules, starts udev + seatd)
           -> weston --backend=drm
              -> wl_sleepdemo
```

This path is much closer to a real UI system. It adds:

- a compositor,
- input device enumeration,
- a real Wayland surface,
- host screenshots and host input injection.

### 3. Chromium browser stack

There are two Chromium variants in the repository:

- stage 3: Chromium on Wayland under Weston kiosk shell,
- stage 4: `content_shell` on direct Ozone DRM without Weston.

That split is important because the Wayland compositor path is far more mature than the direct DRM path.

## Boot And Shipping Model

The shipping model for most of the repository is not a disk image. It is:

- a host kernel,
- a compressed initramfs,
- a QEMU command line.

At a high level:

```text
build rootfs tree
  -> copy binaries, libs, configs, modules
  -> pack with cpio+gzip
  -> boot with qemu -kernel ... -initrd ...
```

Pseudocode:

```text
rootfs = make_empty_tree()
copy_busybox(rootfs)
copy_guest_init(rootfs)
copy_custom_binaries(rootfs)
copy_runtime_libraries(rootfs)
copy_selected_kernel_modules(rootfs)
copy_data_files(rootfs)
pack_rootfs_to_initramfs(rootfs)
boot_qemu(kernel, initramfs)
```

Why this was a good choice:

- iteration is fast,
- system contents are explicit,
- there is no full guest distro to fight,
- the lab is reproducible from scripts.

Why this was painful:

- packaging dependencies is manual,
- missing symlinks or missing data files break runtime in non-obvious ways,
- graphics stacks want more runtime baggage than text-mode test programs.

## Host Control Plane

The host-side control plane is one of the strongest parts of the project.

It has three main jobs:

- control QEMU through QMP,
- generate synthetic network traffic,
- capture evidence from guest serial logs and screenshots.

### QMP

QMP is the QEMU Machine Protocol. In this project it is used for:

- `screendump`,
- key injection,
- pointer motion and click injection,
- monitor queries during investigation.

Relevant file:

- [host/qmp_harness.py](./host/qmp_harness.py)

### Network source

The host drip server provides a deterministic network stimulus:

- active interval,
- paused interval,
- disconnect behavior,
- resume-triggered reconnect tests.

Relevant files:

- [host/drip_server.py](./host/drip_server.py)
- [host/resume_drip_server.py](./host/resume_drip_server.py)

### Evidence extraction

The guest logs structured lines to serial:

- `@@LOG`
- `@@METRIC`
- `@@DISPLAY`
- `@@DRMSTATE`

These logs are later parsed into screenshots, metrics, and debugfs snapshots.

Relevant files:

- [scripts/measure_run.py](./scripts/measure_run.py)
- [host/extract_drmstate_from_serial.py](./host/extract_drmstate_from_serial.py)

## Stage 1: Single-Process Suspend Lab

The stage-1 deliverable is the cleanest completed subsystem in the repository.

Architecture:

```text
/init
  -> mount proc/sys/dev
  -> bring up basic networking
  -> exec sleepdemo

sleepdemo
  -> connect to host drip server
  -> draw to serial/text state
  -> suspend after idle delay
  -> wake via rtc wakealarm
  -> reconnect
  -> emit metrics
```

Important files:

- [guest/sleepdemo.c](./guest/sleepdemo.c)
- [guest/init](./guest/init)
- [guest/build-initramfs.sh](./guest/build-initramfs.sh)
- [guest/run-qemu.sh](./guest/run-qemu.sh)

Important result:

- this stage is working and reproducible,
- suspend/resume intervals were measured,
- reconnect behavior was measured,
- the system became the conceptual template for later stages.

Representative measured values from the stage-1 work:

- `sleep_duration` around `5024-5033 ms`
- `resume_to_redraw` around `3-4 ms`
- `resume_to_reconnect` around `4-5 ms` in the simple reconnect path

What stage 1 did well:

- minimal scope,
- deterministic metrics,
- small enough code surface to reason about.

What stage 1 taught:

- RTC wakealarm plus `pm_test=*` is a practical way to test suspend in QEMU,
- it is worth logging explicit structured metrics early,
- a tiny userspace is enough to validate the basic lab.

## Stage 2: Weston Plus Custom Wayland Client

Stage 2 is where the project became a real graphics stack instead of a text-mode suspend harness.

Important ticket:

- [QEMU-02 final report](./ttmp/2026/03/07/QEMU-02-LAB-TWO--qemu-lab-two-wayland-client-analysis-and-implementation/design-doc/03-phase-2-final-implementation-report.md)

Architecture:

```text
guest /init
  -> load DRM + input modules
  -> start systemd-udevd
  -> start seatd
  -> start weston
  -> launch wl_sleepdemo

wl_sleepdemo
  -> wl_display
  -> wl_compositor + xdg-shell
  -> wl_seat keyboard/pointer
  -> shm buffer rendering
  -> network reconnect loop
  -> suspend/resume logic
```

Important files:

- [guest/init-phase2](./guest/init-phase2)
- [guest/build-phase2-rootfs.sh](./guest/build-phase2-rootfs.sh)
- [guest/weston.ini](./guest/weston.ini)
- [guest/wl_sleepdemo.c](./guest/wl_sleepdemo.c)
- [guest/wl_wayland.c](./guest/wl_wayland.c)
- [guest/wl_render.c](./guest/wl_render.c)
- [guest/wl_net.c](./guest/wl_net.c)
- [guest/wl_suspend.c](./guest/wl_suspend.c)

What worked:

- Weston booted on DRM,
- the custom client rendered visibly,
- keyboard input worked,
- pointer input worked,
- suspend/resume worked with `pm_test=devices`,
- metrics and screenshots were captured.

Representative measured values:

- `sleep_duration = 5755 ms`
- `resume_to_redraw = 5 ms`
- `resume_to_reconnect = 1243 ms`

Interpretation:

- redraw recovery was fast,
- reconnect looked long mainly because the harness and reconnect cadence were coarse,
- the compositor path was stable enough to measure.

What stage 2 did especially well:

- input debugging discipline,
- modularization after the initial monolith got too large,
- building reusable harnesses and screenshot capture scripts,
- keeping a diary detailed enough to support later reports.

What stage 2 struggled with:

- packaging Weston and its runtime deps into the initramfs,
- getting `wl_seat` to appear correctly,
- the many layers in host input injection:
  - QMP,
  - QEMU device model,
  - guest evdev,
  - Weston/libinput,
  - client bindings.

What stage 2 taught:

- `systemd-udevd` matters for correctness in the graphical stack,
- a compositor-based path is much easier to make visibly real than direct DRM/Ozone,
- once input works end-to-end, a lot of later system questions become easier.

## Stage 3: Chromium On Weston Kiosk

Stage 3 reused the working Weston stack and replaced the custom client with Chromium.

Important ticket:

- [QEMU-04 index](./ttmp/2026/03/07/QEMU-04-LAB-THREE--qemu-lab-three-chromium-kiosk-analysis-and-implementation/index.md)

Architecture:

```text
guest /init
  -> start weston kiosk-shell
  -> launch Chromium on Wayland

host
  -> inject keyboard and pointer via QMP
  -> capture screenshots via QMP
```

What worked:

- Chromium booted visibly under Weston,
- host keyboard injection was validated,
- host pointer injection was validated,
- `pm_test=freezer` continuity worked.

What did not fully work:

- `pm_test=devices` continuity still failed.

That failure was not fake progress. The work established a real split:

- the higher-level orchestration path worked,
- the realistic device-resume graphics path still had a visible continuity failure.

That is an important engineering result because it narrowed the problem from “Chromium is broken” to “the graphics/display stack under realistic device resume is broken.”

## QEMU-05: QMP Capture And Resume Investigation

The resume investigation changed the project’s understanding of the display problem in an important way.

Important ticket:

- [QEMU-05 analysis guide](./ttmp/2026/03/07/QEMU-05-DEVICES-RESUME-INVESTIGATION--devices-resume-display-ownership-investigation/design-doc/01-devices-resume-analysis-guide.md)

The critical finding was:

- guest-side DRM/KMS state could stay healthy,
- guest-side Weston screenshots could stay healthy,
- while QMP `screendump` still showed the wrong post-resume surface.

That means:

- guest graphics state and host capture state can diverge,
- QMP `screendump` is not always trustworthy as the final truth for visible scanout after `pm_test=devices`,
- some of the “resume is broken” conclusions had to be reframed as “host-visible capture path is broken or ambiguous.”

This was a major conceptual improvement in the project.

## Stage 4: Direct DRM/Ozone content_shell

The direct DRM branch was created to simplify the graphics stack by removing Weston.

Important tickets:

- [QEMU-06 direct DRM/Ozone guide](./ttmp/2026/03/08/QEMU-06-CONTENT-SHELL-DRM-OZONE--chromium-content-shell-direct-drm-ozone-analysis-and-implementation/design-doc/01-direct-drm-ozone-content-shell-analysis-and-implementation-guide.md)
- [QEMU-07 modeset/controller investigation](./ttmp/2026/03/09/QEMU-07-CONTENT-SHELL-MODESET--content-shell-drm-modeset-and-controller-mapping-investigation/design/01-content-shell-modeset-mapping-investigation-guide.md)

This branch achieved a lot of infrastructure:

- Chromium was cloned and built from source,
- Ozone DRM and Ozone headless support were configured,
- a staged Chromium payload was packaged into the guest,
- headless `content_shell` worked,
- direct DRM startup reached real GPU initialization,
- source-level instrumentation was added to Chromium’s DRM path.

But this branch is not finished.

Current strongest finding:

- `content_shell` allocates scanout-capable buffers,
- but begins page-flipping while `ScreenManager` still has zero controllers,
- so `DrmWindow::SchedulePageFlip()` ACKs without a real flip.

That means the bug is currently understood as a controller discovery or initialization-order problem, not just a geometry mismatch.

Representative branch result:

```text
ScreenManager::AddWindow ... controller_count=0
UpdateControllerToWindowMapping begin controllers=0 windows=1
DrmWindow::SchedulePageFlip ... controller=null ... -> ack without real flip
```

This is useful progress, but it is still a debug path, not a working implementation.

## What Was Good

Several things went unusually well for a project with this many moving parts.

### 1. Diary discipline

The diaries are one of the strongest assets in the repository.

Why they mattered:

- they preserved failed experiments,
- they recorded exact commands and artifacts,
- they prevented false conclusions from surviving too long,
- they made later postmortems and review guides possible.

### 2. Incremental narrowing

The work generally improved by shrinking the problem:

- stage 1 proved suspend mechanics,
- stage 2 proved a real compositor path,
- stage 3 proved browser integration,
- QEMU-05 separated guest health from host capture ambiguity,
- QEMU-07 separated geometry questions from controller-discovery questions.

That is good engineering behavior.

### 3. Reusable automation

The project did not rely only on ad-hoc shell history. It accumulated reusable harnesses for:

- QMP,
- screenshot capture,
- suspend timing,
- results parsing,
- payload staging,
- rootfs building.

### 4. Willingness to revisit wrong assumptions

Several earlier ideas were corrected rather than defended:

- generic `--window-size` was not the right `content_shell` flag,
- QMP `screendump` was not always ground truth,
- some apparent stage-3-specific failures were actually shared lower-level issues,
- some debug changes contaminated results and had to be rolled back conceptually.

That is exactly how this kind of systems debugging should work.

## What Was Bad

The project also had consistent weaknesses.

### 1. Too many overlapping branches of work

By the end, the repository had:

- stage-1 artifacts,
- stage-2 working compositor code,
- stage-3 working-ish browser/compositor code,
- stage-4 direct DRM code,
- multiple follow-up investigations.

This is useful historically, but it increases cognitive load for a new person.

### 2. Packaging complexity

The initramfs model was good for control, but expensive in packaging effort.

Typical recurring problems:

- missing symlinks,
- missing runtime data,
- missing library sonames,
- missing fontconfig assets,
- missing device-manager pieces.

### 3. Debug contamination

At several points, a debug change made interpretation harder:

- extra log tails,
- incomplete control runs,
- assumptions based on the wrong switch.

This was usually corrected, but it cost time.

### 4. Detached external state

The Chromium checkout lived outside the lab repo. That was necessary, but it made provenance weaker until the instrumentation was saved on a dedicated branch:

- `qemu-07-content-shell-controller-debug`

That should have happened earlier.

## What We Learned

The project produced several durable lessons.

### Technical lessons

- A compositor-based path is much easier to stabilize than direct DRM/Ozone browser startup.
- QMP `screendump` can disagree with guest-visible truth after device-resume.
- `pm_test=freezer` and `pm_test=devices` answer different questions and should not be treated as interchangeable.
- `content_shell` is not a perfect kiosk probe; its shell/window behavior matters.
- `systemd-udevd` is not optional once Weston/libinput are involved.

### Process lessons

- Keep the system small at first.
- Add measurement and structured logs early.
- Split tickets when the question changes.
- Preserve failed experiments in writing.
- Save temporary scripts and patched external sources before they get lost.

## If A New Intern Starts Tomorrow

Start here:

1. Read this file.
2. Read [README.md](./README.md).
3. Read [QEMU-02 final report](./ttmp/2026/03/07/QEMU-02-LAB-TWO--qemu-lab-two-wayland-client-analysis-and-implementation/design-doc/03-phase-2-final-implementation-report.md).
4. Read [QEMU-05 analysis guide](./ttmp/2026/03/07/QEMU-05-DEVICES-RESUME-INVESTIGATION--devices-resume-display-ownership-investigation/design-doc/01-devices-resume-analysis-guide.md).
5. Read [QEMU-07 guide](./ttmp/2026/03/09/QEMU-07-CONTENT-SHELL-MODESET--content-shell-drm-modeset-and-controller-mapping-investigation/design/01-content-shell-modeset-mapping-investigation-guide.md).

Recommended mental model:

- stage 1 is the clean suspend baseline,
- stage 2 is the strongest working graphics path,
- stage 3 proves Chromium on the compositor path,
- stage 4 is the experimental direct DRM branch,
- QEMU-05 and QEMU-07 explain the current graphics investigations.

Recommended implementation order if you need working results soon:

1. Prefer the Weston path.
2. Reproduce stage 2 cleanly.
3. Reproduce stage 3 cleanly with `freezer`.
4. Only then return to direct DRM/Ozone if you need it specifically.

Recommended implementation order if you need to continue the direct DRM branch:

1. Continue from the saved Chromium branch:
   - `qemu-07-content-shell-controller-debug`
2. Instrument display discovery with stronger logs than `VLOG`.
3. Prove whether controller discovery happens too late or not at all.
4. Only after that revisit bounds/modeset hypotheses.

## Practical Build And Test References

### Stage 1

Build:

```sh
make build initramfs
```

Run:

```sh
python3 host/drip_server.py --host 0.0.0.0 --port 5555 --interval 0.25 --active-seconds 30 --pause-seconds 2 --stop-after 12
RUNTIME_SECONDS=5 NO_SUSPEND=1 RESULTS_DIR=results guest/run-qemu.sh --kernel build/vmlinuz --initramfs build/initramfs.cpio.gz
```

### Stage 2

Boot artifact model:

```text
build/vmlinuz
build/initramfs-phase2.cpio.gz
```

Launcher:

- [guest/run-qemu-phase2.sh](./guest/run-qemu-phase2.sh)

### Stage 4

Chromium checkout:

```text
/home/manuel/chromium/src
```

Payload staging:

- [host/stage_phase4_chromium_payload.sh](./host/stage_phase4_chromium_payload.sh)

Rootfs build:

- [guest/build-phase4-rootfs.sh](./guest/build-phase4-rootfs.sh)

Direct DRM launcher:

- [guest/init-phase4-drm](./guest/init-phase4-drm)
- [guest/content-shell-drm-launcher.sh](./guest/content-shell-drm-launcher.sh)

## Open Problems

The most important open problems are:

- close or clearly document the `pm_test=devices` graphics continuity issue in the compositor path,
- understand QEMU host-visible capture versus guest-visible scanout more rigorously,
- finish or abandon the direct DRM `content_shell` path based on whether controller discovery can be made to work reasonably.

## Final Assessment

This project is not garbage, and it is not “just notes.” It built real systems:

- a minimal suspend lab,
- a real Wayland compositor client stack,
- a working browser-on-Weston path,
- a partially working direct Ozone DRM browser path with saved source-level instrumentation.

The strongest completed implementation is the Wayland compositor approach. The most educational open problem is the direct DRM path. The best documentation asset is the ticket diary/report system. The biggest practical risk is that a new contributor confuses the stable Weston path with the still-experimental direct DRM branch.

The right overall conclusion is:

- stage 1: complete enough to trust,
- stage 2: complete enough to use,
- stage 3: mostly working, but not fully closed under realistic device-resume,
- stage 4: still an investigation.
