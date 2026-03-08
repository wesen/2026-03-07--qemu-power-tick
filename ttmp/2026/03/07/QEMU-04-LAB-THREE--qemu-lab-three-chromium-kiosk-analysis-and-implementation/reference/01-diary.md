---
Title: Diary
Ticket: QEMU-04-LAB-THREE
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
Summary: Chronological implementation diary for stage 3, including Chromium packaging, runtime bring-up, failures, measurements, and lessons learned.
LastUpdated: 2026-03-07T22:55:00-05:00
WhatFor: Preserve the stage-3 implementation trail and exact debugging outcomes while Chromium kiosk work is in progress.
WhenToUse: Update this after every meaningful stage-3 design, packaging, implementation, or validation step.
---

# Diary

## Goal

Capture the stage-3 Chromium kiosk work chronologically so another engineer can understand what changed, what failed, what was learned, and how to reproduce the current state.

## Context

Stage 3 builds directly on the completed phase-2 Weston plus Wayland-client stack. The new target is Chromium on Wayland under Weston `kiosk-shell`, followed by wake/input/URL validation and later wake-study work.

## Step 1: Stage-3 Ticket Initialization And Chromium Packaging Constraint

I created the stage-3 ticket immediately after finishing the phase-2 bundle so the Chromium work starts from a documented state instead of drifting into another pile of shell history. The first investigation result is already important: on this host, `chromium-browser` is not a normal distro browser package. It is only a shell launcher that expects the Chromium snap to be installed.

That packaging fact changes the initial stage-3 plan. The practical first implementation path is to reuse the installed snap payload under `/snap/chromium/current` and copy the real Chromium runtime into the guest rootfs, rather than spending time chasing a non-existent native `chromium` package in the current apt repositories.

### Prompt Context

**User prompt (verbatim):** "ok continue"

**Assistant interpretation:** Move on from the completed phase-2 ticket and start the next logical lab step rather than stopping at documentation.

**Inferred user intent:** Maintain forward momentum into stage 3, while preserving the same diary/ticket discipline used in the earlier stages.

**Commit (code):** `b88e9d9` `Bootstrap stage 3 chromium guest`

### What I did
- Created ticket `QEMU-04-LAB-THREE`.
- Copied the imported `lab2.md` source into the new ticket as `sources/local/01-lab2.md`.
- Wrote the initial stage-3 index, guide, tasks, and diary baseline.
- Checked host Chromium availability with:
  - `apt-cache policy chromium-browser chromium`
  - `command -v chromium-browser`
  - `snap list chromium`
- Verified:
  - `/usr/bin/chromium-browser` is a shell wrapper,
  - the installed snap exists,
  - the real browser binary is available from the mounted snap payload.

### Why
- The packaging strategy is the main early risk for stage 3, just like Weston packaging was the main early risk for phase 2.
- Establishing that constraint first prevents building the wrong bootstrap around a browser package that does not actually exist in usable form on this host.

### What worked
- The stage-3 ticket workspace is created and ready.
- The host has a real Chromium snap payload available locally.
- The browser reports a usable version from the snap payload.

### What didn't work
- The apt repository does not provide a usable non-snap `chromium` package for this host environment.
- `chromium-browser` itself is not reusable as the browser payload because it only execs `/snap/bin/chromium`.

### What I learned
- Stage 3 is likely feasible without downloading a different browser build, because the installed snap already contains the real Chromium binary and asset tree.

### What was tricky to build
- The subtle point is distinguishing the visible launcher from the actual runtime. If I had treated `/usr/bin/chromium-browser` as “Chromium is installed normally,” I would have designed the guest packaging path around the wrong artifact.

### What warrants a second pair of eyes
- Whether copying the snap payload directly is the best long-term route, or just the fastest first working route.

### What should be done in the future
- Build the first phase-3 rootfs and init path around the snap payload.
- Boot far enough to determine whether Chromium renders under Weston at all.

### Code review instructions
- Review:
  - [design/01-stage-3-chromium-kiosk-guide.md](/home/manuel/code/wesen/2026-03-07--qemu-power-tick/ttmp/2026/03/07/QEMU-04-LAB-THREE--qemu-lab-three-chromium-kiosk-analysis-and-implementation/design/01-stage-3-chromium-kiosk-guide.md)
  - [sources/local/01-lab2.md](/home/manuel/code/wesen/2026-03-07--qemu-power-tick/ttmp/2026/03/07/QEMU-04-LAB-THREE--qemu-lab-three-chromium-kiosk-analysis-and-implementation/sources/local/01-lab2.md)
- Re-run:
  - `apt-cache policy chromium-browser chromium`
  - `snap list chromium`
  - `/snap/chromium/current/usr/lib/chromium-browser/chrome --version`

### Technical details
- Current host-side Chromium facts:
  - `/usr/bin/chromium-browser` is a shell script
  - `/snap/chromium/current/usr/lib/chromium-browser/chrome` is the real browser binary
  - current snap version: `145.0.7632.116`

## Step 2: Built The First Stage-3 Guest And Reached A Visible Chromium Window

The first real stage-3 implementation slice was about packaging and boot, not yet browser interaction. I built a phase-3 rootfs/initramfs on top of the working phase-2 Weston stack, added a dedicated Chromium launcher, and iterated until QEMU could boot far enough to run Chromium under Weston and produce a visible screenshot from the host.

This step had one useful failure before the success. My first phase-3 image was too large to unpack because I copied the entire host font tree and too much browser payload into the initramfs. The kernel failed with `Initramfs unpacking failed: write error` and then panicked because it never reached `/init`. After trimming the image to a minimal font subset and a smaller Chromium asset set, the image booted successfully. The first `about:blank` screenshot was visually ambiguous, so I reran with `chrome://version` to prove that Chromium was genuinely rendering and not just running behind a blank compositor surface.

### Prompt Context

**User prompt (verbatim):** (see Step 1)

**Assistant interpretation:** Move beyond ticket setup and get a real stage-3 browser build running in the guest.

**Inferred user intent:** Produce concrete forward progress on Chromium itself, not just planning.

**Commit (code):** `6c2b1c0` `Validate stage 3 chromium input`

### What I did
- Added:
  - [guest/build-phase3-rootfs.sh](/home/manuel/code/wesen/2026-03-07--qemu-power-tick/guest/build-phase3-rootfs.sh)
  - [guest/init-phase3](/home/manuel/code/wesen/2026-03-07--qemu-power-tick/guest/init-phase3)
  - [guest/chromium-wayland-launcher.sh](/home/manuel/code/wesen/2026-03-07--qemu-power-tick/guest/chromium-wayland-launcher.sh)
  - [guest/run-qemu-phase3.sh](/home/manuel/code/wesen/2026-03-07--qemu-power-tick/guest/run-qemu-phase3.sh)
- Mirrored those files into the stage-3 ticket `scripts/` directory.
- Built the first image with:
  - `guest/build-phase3-rootfs.sh build build/initramfs-phase3.cpio.gz`
- Booted the first attempt with:
  - `guest/run-qemu-phase3.sh --kernel build/vmlinuz --initramfs build/initramfs-phase3.cpio.gz --results-dir results-phase3-smoke1 --append-extra 'phase3_runtime_seconds=20 phase3_url=about:blank'`
- Diagnosed the failure from:
  - `results-phase3-smoke1/guest-serial.log`
- Measured the oversized payload and found the main offenders:
  - `usr/share/fonts = 543M`
  - `usr/lib/chromium-browser = 381M`
- Trimmed the builder to:
  - keep only DejaVu fonts,
  - keep only the Chromium files needed for first launch,
  - keep only English locale packs,
  - drop `chromedriver`.
- Rebuilt the image, reducing it to:
  - `build/initramfs-phase3.cpio.gz = 162M`
- Booted the trimmed image with:
  - `results-phase3-smoke2`
  - `results-phase3-smoke3`
- Captured screenshots through QMP and converted the PPM output to PNG for inspection.

### Why
- Stage 3 could not proceed until Chromium was actually running under Weston in the guest.
- Packaging was the highest-risk part of the phase, so the fastest path was a minimal first-working image rather than trying to solve all browser features at once.

### What worked
- The trimmed phase-3 image boots successfully.
- Weston still starts correctly under the phase-3 init path.
- Chromium launches and remains alive until the configured runtime limit.
- QMP screendump works during the Chromium run.
- `results-phase3-smoke3/01-stage3.png` shows a visible Chromium surface rendering under Weston.

### What didn't work
- The first phase-3 image failed during early boot:
  - `Initramfs unpacking failed: write error`
  - followed by a root-fs panic because the kernel never reached `/init`.
- The first successful `about:blank` screenshot in `results-phase3-smoke2` was visually ambiguous because a blank white page is expected for that URL.

### What I learned
- For Chromium in this lab, payload minimization matters immediately. The browser itself is large enough that careless “copy everything” packaging causes initramfs failure before Chromium is even relevant.
- `chrome://version` is a much better first visible validation target than `about:blank`.
- A large part of the snap payload can be treated as ordinary files in the guest once the real binary path is known.

### What was tricky to build
- The subtle failure was that the first oversized image looked like a rootfs or kernel bug at first glance, but the serial log made the real cause clear: the initramfs never finished unpacking. Measuring directory sizes before changing more code was the right move. The second subtle point was screenshot interpretation. The all-white `about:blank` result could have been either success or failure; switching to a clearly non-blank internal Chromium page removed that ambiguity.

### What warrants a second pair of eyes
- The current browser run still logs DBus, GBM, and GSettings complaints. They did not block first render, but they may matter for later stability, GPU behavior, or input/suspend behavior.

### What should be done in the future
- Commit this first stage-3 browser bring-up milestone.
- Validate keyboard and pointer injection into Chromium next.
- Then decide which of the current startup warnings actually need to be fixed before suspend/resume work.

### Code review instructions
- Start with:
  - [guest/build-phase3-rootfs.sh](/home/manuel/code/wesen/2026-03-07--qemu-power-tick/guest/build-phase3-rootfs.sh)
  - [guest/init-phase3](/home/manuel/code/wesen/2026-03-07--qemu-power-tick/guest/init-phase3)
  - [guest/chromium-wayland-launcher.sh](/home/manuel/code/wesen/2026-03-07--qemu-power-tick/guest/chromium-wayland-launcher.sh)
  - [guest/run-qemu-phase3.sh](/home/manuel/code/wesen/2026-03-07--qemu-power-tick/guest/run-qemu-phase3.sh)
- Then inspect:
  - `results-phase3-smoke1/guest-serial.log`
  - `results-phase3-smoke2/guest-serial.log`
  - `results-phase3-smoke3/guest-serial.log`
  - `results-phase3-smoke3/01-stage3.png`
- Reproduce with:
  - `guest/build-phase3-rootfs.sh build build/initramfs-phase3.cpio.gz`
  - `guest/run-qemu-phase3.sh --kernel build/vmlinuz --initramfs build/initramfs-phase3.cpio.gz --results-dir results-phase3-smoke3 --append-extra 'phase3_runtime_seconds=15 phase3_url=chrome://version'`
  - `python3 host/qmp_harness.py --socket results-phase3-smoke3/qmp.sock screendump --file results-phase3-smoke3/01-stage3.ppm`

### Technical details
- First failed image size:
  - `build/initramfs-phase3.cpio.gz = 442M`
- Trimmed image size:
  - `build/initramfs-phase3.cpio.gz = 162M`
- Important runtime warnings still present:
  - missing `/run/dbus/system_bus_socket`
  - `MESA-LOADER: failed to open dri ... dri_gbm.so`
  - `g_settings_schema_source_lookup: assertion 'source != NULL' failed`

## Step 3: Built A Deterministic Chromium Input Harness And Proved Keyboard Plus Pointer Delivery

After the first visible Chromium render, the next real risk was validation quality. I did not want to judge stage-3 input behavior from ambiguous pages like `about:blank`, Google search, or `chrome://version`, because those pages leave too much room for false confidence. Instead, I switched to a deterministic browser-side test page and wrapped the whole sequence in a small host-side checkpoint harness.

The deterministic page is a `data:` URL that renders three clear signals:
- an autofocus text input for keyboard validation,
- a large `CLICK` button for pointer validation,
- a status line that flips from `idle` to `clicked` and changes the page background to lime when the button activates.

That gave me a clean visual proof path. The first manual run showed the concept worked, but one repeated `l` key was dropped when the sequence was sent too aggressively. I treated that as a harness-quality problem rather than a Chromium/input failure and fixed it by adding per-key pacing in the new checkpoint helper. The final harness run produced the expected screenshots:
- `results-phase3-checkpoints1/01-after-keyboard.png` showing `hello` in the input field
- `results-phase3-checkpoints1/02-after-pointer.png` showing the green background and `clicked` status after the button press

### Prompt Context

**User prompt (verbatim):** "ok continue"

**Assistant interpretation:** Continue stage 3 with concrete engineering work and keep the diary current while doing it.

**Inferred user intent:** Push through the next browser milestone, but keep it reproducible enough that later review and reporting stay easy.

**Commit (code):** `1ce649c` `Add stage 3 suspend plumbing`

### What I did
- Generated a deterministic stage-3 browser page as a `data:` URL.
- Ran a manual validation pass in `results-phase3-input2` to verify that:
  - keyboard input reaches Chromium,
  - pointer movement reaches Chromium,
  - button clicks trigger visible DOM changes.
- Added:
  - [host/make_phase3_test_url.py](/home/manuel/code/wesen/2026-03-07--qemu-power-tick/host/make_phase3_test_url.py)
  - [host/capture_phase3_checkpoints.py](/home/manuel/code/wesen/2026-03-07--qemu-power-tick/host/capture_phase3_checkpoints.py)
- Mirrored those scripts into the stage-3 ticket `scripts/` directory.
- Ran a clean reproducible checkpoint pass in `results-phase3-checkpoints1` with:
  - `URL=$(python3 host/make_phase3_test_url.py)`
  - `guest/run-qemu-phase3.sh --kernel build/vmlinuz --initramfs build/initramfs-phase3.cpio.gz --results-dir results-phase3-checkpoints1 --append-extra "phase3_runtime_seconds=35 phase3_url=$URL"`
  - `python3 host/capture_phase3_checkpoints.py --socket results-phase3-checkpoints1/qmp.sock --results-dir results-phase3-checkpoints1 --text hello --settle-seconds 9`

### Why
- Browser input validation is too easy to overclaim if the page itself is ambiguous.
- A deterministic page plus a deterministic harness turns “I think input worked” into screenshot-backed evidence that can be rerun later.
- The same harness can become the front end of a later stage-3 checkpoint and suspend/resume workflow.

### What worked
- Chromium rendered the deterministic page correctly.
- Autofocus landed in the input field, so host key injection reached the expected element.
- With paced key delivery, Chromium showed the full text `hello`.
- Host pointer movement and click injection activated the button.
- The visual state change was large and unambiguous:
  - white background plus `idle` before click,
  - lime background plus `clicked` after click.
- The new checkpoint helper produced distinct image deltas:
  - `keyboard_ae=3275`
  - `pointer_ae=872308`

### What didn't work
- The first manual key burst in `results-phase3-input2` produced `helo` instead of `hello`.
- That failure mode strongly suggests that repeated characters sent too quickly via sequential `send-key` calls can be dropped by the current harness cadence.

### What I learned
- Stage-3 input does work end-to-end through QMP, QEMU USB input devices, Weston/libinput, and Chromium.
- The harder problem was not browser support but test quality and pacing.
- A deterministic browser page is much more useful than relying on third-party pages or internal browser pages for UI/input validation.

### What was tricky to build
- The page URL had to be safe to pass through the kernel command line, so base64 `data:` encoding was the cleanest route.
- The validation target needed to be visually obvious enough that a screenshot alone could prove success.
- Repeated-character reliability depended on adding small inter-key delays in the harness.

### What warrants a second pair of eyes
- Whether `send-key` pacing should be moved into the lower-level QMP harness rather than living in the stage-3 checkpoint helper.
- Whether the eventual stage-3 suspend workflow should reuse this exact deterministic page or switch to a real hosted kiosk page before resume testing.

### What should be done in the future
- Commit this stage-3 input-validation milestone.
- Reintroduce suspend/resume on top of the now-proven Chromium input path.
- Use the deterministic page again if suspend/resume debugging needs a minimal browser surface with obvious redraw state.

### Code review instructions
- Review:
  - [host/make_phase3_test_url.py](/home/manuel/code/wesen/2026-03-07--qemu-power-tick/host/make_phase3_test_url.py)
  - [host/capture_phase3_checkpoints.py](/home/manuel/code/wesen/2026-03-07--qemu-power-tick/host/capture_phase3_checkpoints.py)
  - [scripts/make_phase3_test_url.py](/home/manuel/code/wesen/2026-03-07--qemu-power-tick/ttmp/2026/03/07/QEMU-04-LAB-THREE--qemu-lab-three-chromium-kiosk-analysis-and-implementation/scripts/make_phase3_test_url.py)
  - [scripts/capture_phase3_checkpoints.py](/home/manuel/code/wesen/2026-03-07--qemu-power-tick/ttmp/2026/03/07/QEMU-04-LAB-THREE--qemu-lab-three-chromium-kiosk-analysis-and-implementation/scripts/capture_phase3_checkpoints.py)
- Inspect:
  - `/home/manuel/code/wesen/2026-03-07--qemu-power-tick/results-phase3-checkpoints1/00-before-input.png`
  - `/home/manuel/code/wesen/2026-03-07--qemu-power-tick/results-phase3-checkpoints1/01-after-keyboard.png`
  - `/home/manuel/code/wesen/2026-03-07--qemu-power-tick/results-phase3-checkpoints1/02-after-pointer.png`
- Re-run:
  - `URL=$(python3 host/make_phase3_test_url.py)`
  - `guest/run-qemu-phase3.sh --kernel build/vmlinuz --initramfs build/initramfs-phase3.cpio.gz --results-dir results-phase3-checkpoints1 --append-extra "phase3_runtime_seconds=35 phase3_url=$URL"`
  - `python3 host/capture_phase3_checkpoints.py --socket results-phase3-checkpoints1/qmp.sock --results-dir results-phase3-checkpoints1 --text hello --settle-seconds 9`

### Technical details
- Manual proof run:
  - `results-phase3-input2/01-before.png`
  - `results-phase3-input2/02-after.png`
- Reproducible harness proof run:
  - `results-phase3-checkpoints1/00-before-input.png`
  - `results-phase3-checkpoints1/01-after-keyboard.png`
  - `results-phase3-checkpoints1/02-after-pointer.png`
- Current QMP-reported active absolute mouse during the run:
  - `QEMU HID Tablet`

## Step 4: Reintroduced Suspend For Stage 3 And Found A Real Continuity Split Between `freezer` And `devices`

With input validation in place, I moved on to stage-3 suspend/resume. The main design choice here was to keep Chromium itself unmodified and move suspend orchestration into a separate guest helper. That keeps the phase-3 browser launcher thin and avoids dragging the phase-2 Wayland client state machine back into stage 3 where it no longer fits naturally.

I added a tiny standalone helper, `suspendctl`, that performs the same core suspend work as the earlier labs:
- optional `pm_test` programming,
- RTC wakealarm programming,
- writing `freeze` to `/sys/power/state`,
- logging `state=SUSPENDING`,
- logging `state=RESUMED`,
- emitting `sleep_duration` and `suspend_resume_gap` metrics.

`init-phase3` now parses stage-3 suspend-specific kernel command-line options and launches `suspendctl` in the background while Chromium is running. That gave me a working suspend pipeline quickly, but it also surfaced an immediate packaging regression: the first rebuild failed because `guest/build-suspendctl.sh` was not executable. After fixing that permission error, the rebuilt image booted correctly.

The more important result came from the validation matrix:
- `pm_test=devices` successfully enters and exits suspend and emits sensible metrics, but visible display continuity is broken after resume.
- `pm_test=freezer` preserves the Chromium-rendered surface across the suspend cycle.

That distinction matters. It means the stage-3 suspend plumbing itself is real and working, but the device-level resume path is not currently restoring the visible browser/compositor output correctly.

### Prompt Context

**User prompt (verbatim):** "ok continue"

**Assistant interpretation:** Keep moving through stage 3 and tackle the next uncompleted lab milestone, while keeping implementation notes current.

**Inferred user intent:** Continue from browser input into suspend/resume validation, not just planning.

**Commit (code):** Pending at this step

### What I did
- Added:
  - [guest/suspendctl.c](/home/manuel/code/wesen/2026-03-07--qemu-power-tick/guest/suspendctl.c)
  - [guest/build-suspendctl.sh](/home/manuel/code/wesen/2026-03-07--qemu-power-tick/guest/build-suspendctl.sh)
  - [host/capture_phase3_suspend_checkpoints.py](/home/manuel/code/wesen/2026-03-07--qemu-power-tick/host/capture_phase3_suspend_checkpoints.py)
- Updated:
  - [guest/build-phase3-rootfs.sh](/home/manuel/code/wesen/2026-03-07--qemu-power-tick/guest/build-phase3-rootfs.sh)
  - [guest/init-phase3](/home/manuel/code/wesen/2026-03-07--qemu-power-tick/guest/init-phase3)
- Mirrored the new helper sources/scripts into the stage-3 ticket `scripts/` directory.
- Rebuilt the phase-3 initramfs.
- Ran stage-3 suspend validation with:
  - `pm_test=devices` in:
    - `results-phase3-suspend1`
    - `results-phase3-suspend2`
    - `results-phase3-suspend3`
    - `results-phase3-suspend4`
  - `pm_test=freezer` in:
    - `results-phase3-suspend-freezer1`

### Why
- Stage 3 needs its own suspend path because the browser is now an external app, not a custom client where suspend can be embedded directly in the rendering loop.
- The host-side checkpoint logic also needed to be saved as a real script instead of staying in inline Python snippets.
- Testing both `freezer` and `devices` is important because they answer different questions:
  - `freezer` checks the higher-level orchestration path without a full device exercise,
  - `devices` checks whether the graphics/input stack survives the more realistic device resume path.

### What worked
- `suspendctl` successfully programmed wakealarm, entered suspend, resumed, and logged metrics.
- `pm_test=devices` produced stable measurements:
  - `results-phase3-suspend1`: `sleep_duration = 5778 ms`, `suspend_resume_gap = 5778 ms`
  - `results-phase3-suspend2`: `sleep_duration = 5784 ms`, `suspend_resume_gap = 5784 ms`
  - `results-phase3-suspend4`: `sleep_duration = 5780 ms`, `suspend_resume_gap = 5780 ms`
- `pm_test=freezer` preserved the visible Chromium surface:
  - `results-phase3-suspend-freezer1/00-pre.png`
  - `results-phase3-suspend-freezer1/01-post.png`
  - image delta `AE = 0`
- The rebuilt non-suspend smoke run still rendered Chromium correctly in:
  - `results-phase3-postrebuild-smoke1/00-smoke.png`

### What didn't work
- The first rebuild failed because `guest/build-suspendctl.sh` was missing execute permission.
- `pm_test=devices` broke visible display continuity after resume.
- Multiple `devices` attempts showed the same pattern:
  - suspend metrics were emitted,
  - the guest kept running,
  - but QMP screenshots after resume showed a bad display state instead of the Chromium page.
- Injecting post-resume keyboard and pointer input in `results-phase3-suspend4` did not recover the visible browser surface.

### What I learned
- There is now a clear stage-3 split:
  - suspend orchestration works,
  - `freezer` continuity works,
  - `devices` continuity does not.
- That points away from a general suspend helper bug and toward a lower-level graphics/compositor/device-resume issue.
- The failed `devices` continuity is not just “Chromium forgot to repaint after resume,” because post-resume input did not restore the visible output.
- The strongest log-level clue so far is in `results-phase3-suspend2/guest-serial.log`, where the `devices` path resumes with:
  - `virtio_gpu_dequeue_ctrl_func ... *ERROR* response 0x1203 (command 0x105)`
  - `virtio_gpu_dequeue_ctrl_func ... *ERROR* response 0x1203 (command 0x104)`
  while the `freezer` path does not show the same DRM error pattern.

### What was tricky to build
- Timing the screenshots too close to the suspend marker produced misleading captures, so the suspend checkpoint path needed explicit pre/post delay controls.
- The first `devices` runs were easy to misread because the metric logs looked healthy while the visible surface was not.
- Distinguishing “helper works, display continuity fails” required comparing `devices` against `freezer`.

### What warrants a second pair of eyes
- Whether the `devices` failure is a virtio-gpu or Weston DRM-backend resume issue rather than a Chromium issue.
- Whether a compositor restart, mode reprobe, or explicit redraw trigger exists that could recover the `devices` path without restarting Chromium itself.

### What should be done in the future
- Commit this suspend-plumbing milestone.
- Investigate the `pm_test=devices` continuity failure specifically as a graphics/compositor resume problem.
- Keep `pm_test=freezer` as the current known-good stage-3 continuity baseline while investigating the stronger `devices` path.

### Code review instructions
- Review:
  - [guest/suspendctl.c](/home/manuel/code/wesen/2026-03-07--qemu-power-tick/guest/suspendctl.c)
  - [guest/build-suspendctl.sh](/home/manuel/code/wesen/2026-03-07--qemu-power-tick/guest/build-suspendctl.sh)
  - [guest/build-phase3-rootfs.sh](/home/manuel/code/wesen/2026-03-07--qemu-power-tick/guest/build-phase3-rootfs.sh)
  - [guest/init-phase3](/home/manuel/code/wesen/2026-03-07--qemu-power-tick/guest/init-phase3)
  - [host/capture_phase3_suspend_checkpoints.py](/home/manuel/code/wesen/2026-03-07--qemu-power-tick/host/capture_phase3_suspend_checkpoints.py)
- Inspect:
  - `/home/manuel/code/wesen/2026-03-07--qemu-power-tick/results-phase3-postrebuild-smoke1/00-smoke.png`
  - `/home/manuel/code/wesen/2026-03-07--qemu-power-tick/results-phase3-suspend-freezer1/00-pre.png`
  - `/home/manuel/code/wesen/2026-03-07--qemu-power-tick/results-phase3-suspend-freezer1/01-post.png`
  - `/home/manuel/code/wesen/2026-03-07--qemu-power-tick/results-phase3-suspend4/01-post-resume-input.png`
- Re-run a passing freezer validation:
  - `URL=$(python3 host/make_phase3_test_url.py)`
  - `guest/run-qemu-phase3.sh --kernel build/vmlinuz --initramfs build/initramfs-phase3.cpio.gz --results-dir results-phase3-suspend-freezer1 --append-extra "phase3_runtime_seconds=40 phase3_url=$URL phase3_suspend_delay_seconds=10 phase3_wake_seconds=5 phase3_pm_test=freezer"`
  - `python3 host/capture_phase3_suspend_checkpoints.py --socket results-phase3-suspend-freezer1/qmp.sock --serial-log results-phase3-suspend-freezer1/guest-serial.log --results-dir results-phase3-suspend-freezer1 --pre-name 00-pre --post-name 01-post`

### Technical details
- `devices` metric samples:
  - `results-phase3-suspend1`: `5778 ms`
  - `results-phase3-suspend2`: `5784 ms`
  - `results-phase3-suspend4`: `5780 ms`
- `freezer` metric sample:
  - `results-phase3-suspend-freezer1`: `5031 ms`
- Current interpretation:
  - stage-3 suspend plumbing is implemented,
  - stage-3 continuity passes for `freezer`,
  - stage-3 continuity remains open for `devices`,
  - the best current root-cause lead is a virtio-gpu / DRM resume failure in the `devices` path.

## Step 5: Wrote A Stage-3 Postmortem And Review Guide Based On The Diary, Git History, And Current Code

After the latest stage-3 implementation slice, I stopped to assess the quality of the work itself instead of just continuing to push features. The goal of this step was to answer a different question: not “what changed?” but “how good is the result, where is it strong, where is it still weak, and what should the next engineer understand before taking over?”

I based the review on the existing stage-3 diary, the stage-3 commit history, the current guest and host helper files, and the result logs from the successful and failing runs. That matters because a postmortem written from memory would be much less useful than one anchored to the exact artifacts the next engineer will inspect.

### Prompt Context

**User prompt (verbatim):** "Ok, so far your little brother I think has been doing quite good. Look at his diaries and his process and progress (you can use git too), and write a postmortem analysis of the quality of the result (for all I know it could be complete garbage), and analyze where they struggled on their way, what they did well, what they could improve, what we could improv eein the lab.

Create a detailed analysis / and review guide that is very detailed for the brother, explaining all the parts of the system needed to understand what it is, with prose paragraphs and bullet point sand pseudocode and diagrams and api references and file
  references.
  It should be very clear and detailed. Store in the ticket and the nupload to remarkable."

**Assistant interpretation:** Write an evidence-based postmortem and review guide for the current stage-3 work, store it in the ticket, and deliver it through the documentation pipeline.

**Inferred user intent:** Assess the quality of the stage-3 result honestly and leave the next engineer with a clear mental model, a list of strengths and weaknesses, and a practical review/onboarding document.

**Commit (code):** Pending at this step

### What I did
- Reviewed:
  - the stage-3 diary
  - the stage-3 guide
  - the stage-3 tasks and changelog
  - the stage-3 commit history
  - the current guest/host implementation files
  - the result logs and screenshots from successful and failing runs
- Added:
  - [design/02-stage-3-postmortem-and-review-guide.md](/home/manuel/code/wesen/2026-03-07--qemu-power-tick/ttmp/2026/03/07/QEMU-04-LAB-THREE--qemu-lab-three-chromium-kiosk-analysis-and-implementation/design/02-stage-3-postmortem-and-review-guide.md)
- Updated:
  - [index.md](/home/manuel/code/wesen/2026-03-07--qemu-power-tick/ttmp/2026/03/07/QEMU-04-LAB-THREE--qemu-lab-three-chromium-kiosk-analysis-and-implementation/index.md)
  - [changelog.md](/home/manuel/code/wesen/2026-03-07--qemu-power-tick/ttmp/2026/03/07/QEMU-04-LAB-THREE--qemu-lab-three-chromium-kiosk-analysis-and-implementation/changelog.md)

### Why
- The engineering work had reached the point where another person could plausibly take over, so a quality review becomes almost as important as the next code change.
- The ticket already had strong evidence, which made it possible to produce a real postmortem instead of vague praise or generic criticism.

### What worked
- The existing diary and result artifacts were good enough to support a real postmortem.
- Git history was coherent enough to reconstruct the three main implementation milestones.
- The review could be anchored to concrete files and logs rather than general impressions.

### What didn't work
- The diary’s commit bookkeeping had drifted, which reduced traceability and became one of the explicit review findings.

### What I learned
- The stage-3 work is materially better than a superficial look might suggest because the strongest value is in the evidence and validation quality, not just the visible Chromium screenshot.
- The main unresolved weakness is clear and concrete rather than vague: device-level graphics continuity on resume.

### What was tricky to build
- The hardest part of this review was separating “incomplete” from “bad.” The work is not complete, but a lot of the underlying engineering choices were still good and worth preserving.

### What warrants a second pair of eyes
- Whether the review is too generous or too harsh on the shell-heavy orchestration layer.
- Whether the diary commit-drift should be corrected immediately or left as part of the historical record and fixed in a later cleanup pass.

### What should be done in the future
- Commit this review document and ticket update.
- Upload the stage-3 review bundle to reMarkable.

### Code review instructions
- Start with:
  - [design/02-stage-3-postmortem-and-review-guide.md](/home/manuel/code/wesen/2026-03-07--qemu-power-tick/ttmp/2026/03/07/QEMU-04-LAB-THREE--qemu-lab-three-chromium-kiosk-analysis-and-implementation/design/02-stage-3-postmortem-and-review-guide.md)
  - [reference/01-diary.md](/home/manuel/code/wesen/2026-03-07--qemu-power-tick/ttmp/2026/03/07/QEMU-04-LAB-THREE--qemu-lab-three-chromium-kiosk-analysis-and-implementation/reference/01-diary.md)
- Reconstruct milestones with:
  - `git log --oneline --max-count=10`
  - `nl -ba guest/init-phase3 | sed -n '1,240p'`
  - `nl -ba guest/build-phase3-rootfs.sh | sed -n '1,220p'`
  - `nl -ba results-phase3-suspend2/guest-serial.log | sed -n '612,623p'`

### Technical details
- Stage-3 commits reviewed:
  - `b88e9d9`
  - `6c2b1c0`
  - `1ce649c`
- Key evidence files reviewed:
  - `results-phase3-smoke3/01-stage3.png`
  - `results-phase3-checkpoints1/01-after-keyboard.png`
  - `results-phase3-checkpoints1/02-after-pointer.png`
  - `results-phase3-suspend2/guest-serial.log`
  - `results-phase3-suspend-freezer1/guest-serial.log`

## Step 6: Narrowed The `devices` Resume Failure Further By Comparing Stage 2 And Rejecting A Plain `virtio-gpu-pci` Swap

After the review document was written, I went back to the actual blocker: the stage-3 `pm_test=devices` continuity failure. This step was about getting a better negative result, not yet a final fix. I wanted to answer two questions cleanly:

1. Is the resume noise just generic QEMU USB churn that was already present in stage 2?
2. Could I sidestep the bad post-resume screen state by replacing `virtio-vga` with plain `virtio-gpu-pci`?

The answers were useful:

- Stage 2 does show the same xHCI/root-hub resume noise, but it does **not** show the stage-3-specific `virtio_gpu_dequeue_ctrl_func` DRM errors.
- A plain `virtio-gpu-pci` swap is not a good immediate fix, because it lost the visible Chromium surface even in a normal non-suspend smoke run.

So the strongest current interpretation is that the stage-3 regression is more likely tied to Chromium plus the stage-3 graphics stack on top of the shared QEMU resume noise, not to the shared xHCI behavior by itself.

### Prompt Context

**User prompt (verbatim):** "continue"

**Assistant interpretation:** Keep pushing on the main unresolved stage-3 blocker instead of stopping at the review document.

**Inferred user intent:** Continue the stage-3 technical investigation and keep the diary current while doing it.

**Commit (code):** Pending at this step

### What I did
- Compared the stage-2 `devices` resume log in `results-phase2-suspend6/guest-serial.log` against the stage-3 `devices` resume log in `results-phase3-suspend2/guest-serial.log`.
- Verified:
  - stage 2 has the xHCI/root-hub reset path too,
  - stage 2 does not show the stage-3 `virtio_gpu_dequeue_ctrl_func` DRM errors.
- Temporarily made the stage-3 QEMU GPU device selectable and tested `virtio-gpu-pci` in:
  - `results-phase3-gpu-alt-smoke1`
- Rejected that branch after the non-suspend smoke screenshot showed only the legacy firmware text screen instead of the Chromium page.
- Reverted the temporary `virtio-gpu-pci` experiment hook to avoid leaving low-value debug scaffolding in the main script.

### Why
- Shared resume noise can lead to the wrong diagnosis if it is not compared against a known-good earlier stage.
- A bad workaround is still useful if it rules out an attractive but incorrect direction quickly.

### What worked
- The stage-2 comparison gave a cleaner boundary:
  - xHCI/root-hub resets are not new,
  - stage-3-specific DRM errors are new.
- The failed `virtio-gpu-pci` smoke test ruled out one plausible “easy fix” path quickly.

### What didn't work
- `virtio-gpu-pci` did not preserve normal visible rendering in `results-phase3-gpu-alt-smoke1`, so it is not a viable drop-in replacement for the current stage-3 workflow.

### What I learned
- The `devices` resume failure is more likely related to stage-3 graphics/client state than to shared USB resume noise alone.
- The plain-GPU-device swap is not the right next fix path.

### What was tricky to build
- The temptation was to overread the shared xHCI errors as the root cause, but the stage-2 comparison showed that this would be too simplistic.
- The GPU-device experiment had to be reverted once it stopped increasing confidence and started adding clutter.

### What warrants a second pair of eyes
- Whether the next best experiment is:
  - gathering more post-resume Weston-side evidence,
  - or trying to reduce Chromium’s graphics-path complexity with different browser flags.

### What should be done in the future
- Keep debugging the stage-3 `devices` path specifically.
- Treat stage 2 as the “same QEMU base, simpler client” control case during further comparisons.

### Code review instructions
- Compare:
  - `/home/manuel/code/wesen/2026-03-07--qemu-power-tick/results-phase2-suspend6/guest-serial.log`
  - `/home/manuel/code/wesen/2026-03-07--qemu-power-tick/results-phase3-suspend2/guest-serial.log`
- Inspect the failed GPU-device experiment screenshot:
  - `/home/manuel/code/wesen/2026-03-07--qemu-power-tick/results-phase3-gpu-alt-smoke1/00-smoke.png`

### Technical details
- Stage-2 shared resume noise:
  - xHCI reinit
  - root hub reset
- Stage-3 new resume signal:
  - `virtio_gpu_dequeue_ctrl_func ... *ERROR* response 0x1203`

## Step 7: Switched To Smaller Isolation Experiments And Discovered That My Debug Console Writes Were Polluting The Resume Investigation

At this point the big “Chromium resume is broken” framing had become too coarse, so I switched to smaller experiments. The goal was to shrink the moving pieces until I could tell which failures were real and which ones I had accidentally introduced while gathering more logs. That produced a genuinely useful correction.

I first extended the phase-3 rootfs so the same initramfs could launch either Chromium or `weston-simple-shm`. That let me test the same Weston, suspend helper, QEMU device model, and `pm_test=devices` flow without the browser. The first isolate run still showed the old `virtio_gpu_dequeue_ctrl_func` invalid-resource errors, but the pre-suspend screenshot was already wrong: it showed the firmware text plane instead of a Wayland surface. That was the clue that the latest debug instrumentation itself might be interfering with the display path.

### Prompt Context

**User prompt (verbatim):** (same as Step 6)

**Assistant interpretation:** Keep debugging the unresolved stage-3 `pm_test=devices` path, but use more targeted experiments rather than repeatedly rerunning the full Chromium case.

**Inferred user intent:** Reduce uncertainty around the display-resume failure by isolating variables and preserving the investigation trail.

**Commit (code):** Pending at this step

### What I did
- Added a phase-3 client selector to:
  - [guest/init-phase3](/home/manuel/code/wesen/2026-03-07--qemu-power-tick/guest/init-phase3)
  - [guest/build-phase3-rootfs.sh](/home/manuel/code/wesen/2026-03-07--qemu-power-tick/guest/build-phase3-rootfs.sh)
- Mirrored the updated scripts into the stage-3 ticket script archive:
  - [scripts/init-phase3](/home/manuel/code/wesen/2026-03-07--qemu-power-tick/ttmp/2026/03/07/QEMU-04-LAB-THREE--qemu-lab-three-chromium-kiosk-analysis-and-implementation/scripts/init-phase3)
  - [scripts/build-phase3-rootfs.sh](/home/manuel/code/wesen/2026-03-07--qemu-power-tick/ttmp/2026/03/07/QEMU-04-LAB-THREE--qemu-lab-three-chromium-kiosk-analysis-and-implementation/scripts/build-phase3-rootfs.sh)
- Rebuilt the phase-3 initramfs and ran a `pm_test=devices` isolate with:
  - `phase3_client=weston-simple-shm`
  - results in `/home/manuel/code/wesen/2026-03-07--qemu-power-tick/results-phase3-westonshm-devices1`
- Observed in that first isolate:
  - the pre-suspend screenshot was already stuck on the boot-text plane,
  - the post-resume screenshot was also the boot-text plane,
  - the old `virtio_gpu_dequeue_ctrl_func ... response 0x1203` errors still appeared.
- Identified the most suspicious new variable: I had recently added `tail -f /var/log/weston.log >/dev/console`, which continuously wrote text to the DRM console while Weston owned the display.
- Removed that continuous Weston-log tail from [guest/init-phase3](/home/manuel/code/wesen/2026-03-07--qemu-power-tick/guest/init-phase3), rebuilt again, and reran the same isolate in:
  - `/home/manuel/code/wesen/2026-03-07--qemu-power-tick/results-phase3-westonshm-devices2`
- Observed in the cleaned isolate:
  - the pre-suspend screenshot became a quiet black framebuffer rather than the firmware text plane,
  - the post-resume screenshot still fell back to the firmware text plane,
  - the `virtio_gpu_dequeue_ctrl_func` invalid-resource errors disappeared from the serial log.
- Started a fresh Chromium rerun on the cleaned initramfs in `results-phase3-suspend6`, but the user intentionally interrupted the turn before that capture finished, so I did not treat it as valid evidence.

### Why
- The earlier experiments had too many changing variables at once:
  - Chromium,
  - Weston,
  - suspend timing,
  - additional debug logging,
  - and the framebuffer capture path.
- A smaller control experiment was necessary to separate a real resume bug from self-inflicted instrumentation noise.

### What worked
- The client-selector change made it possible to reuse the same phase-3 rootfs and suspend path for a minimal control client.
- The first `weston-simple-shm` isolate exposed that the display was already in a bad state before resume, which made the investigation much more honest.
- Removing the continuous Weston-log tail removed the `virtio_gpu` invalid-resource errors in the control run, which means those errors were at least partly caused by the debug instrumentation.

### What didn't work
- `weston-simple-shm` did not immediately give a nice known-good visible scene in the phase-3 capture harness, so it was not a perfect visual control.
- The cleaned `weston-simple-shm` run still did not preserve display continuity through `pm_test=devices`; it only removed the extra DRM error noise.
- The Chromium confirmation rerun was aborted by user interruption, so there is not yet a corresponding cleaned Chromium dataset.

### What I learned
- Yes, smaller experiments make sense here; this step produced more useful information than another blind full-stack rerun.
- The earlier `virtio_gpu_dequeue_ctrl_func ... response 0x1203` signal was not a pure app or compositor symptom. My own continuous `/dev/console` writes were contaminating the resume path.
- There is still a real continuity problem after `pm_test=devices`, but it is narrower than before:
  - one layer was instrumentation-induced,
  - another layer is still a genuine display restoration failure.
- This means the stage-3 investigation should now focus on the display handoff / console / scanout state after resume, not on Chromium first.

### What was tricky to build
- The hardest part was noticing that better logging had become worse logging. The `tail -f /var/log/weston.log` change looked harmless because it only touched observability, but it introduced a second actor writing to the same display stack being investigated.
- The first isolate run was easy to misread as “the same bug without Chromium,” when the pre-suspend screenshot was already telling a more important story: the capture path had been dirtied before the suspend cycle even began.

### What warrants a second pair of eyes
- Whether `weston-simple-shm` is the right long-term control client for stage-3 display debugging, or whether a purpose-built fullscreen shm test client would be better.
- Whether the remaining post-resume fallback to the firmware text plane is best explained by fbcon reasserting itself, Weston losing scanout ownership, or the display never being repainted after resume.

### What should be done in the future
- Re-run the cleaned Chromium `pm_test=devices` case and compare it directly to `results-phase3-westonshm-devices2`.
- Prefer smaller controls before adding more logging or more browser flags.
- Keep `/dev/console` logging minimal during graphics-resume experiments.

### Code review instructions
- Review the control-client hook in:
  - [guest/init-phase3](/home/manuel/code/wesen/2026-03-07--qemu-power-tick/guest/init-phase3)
  - [guest/build-phase3-rootfs.sh](/home/manuel/code/wesen/2026-03-07--qemu-power-tick/guest/build-phase3-rootfs.sh)
- Inspect the before/after isolate artifacts:
  - `/home/manuel/code/wesen/2026-03-07--qemu-power-tick/results-phase3-westonshm-devices1/00-pre.png`
  - `/home/manuel/code/wesen/2026-03-07--qemu-power-tick/results-phase3-westonshm-devices1/01-post.png`
  - `/home/manuel/code/wesen/2026-03-07--qemu-power-tick/results-phase3-westonshm-devices2/00-pre.png`
  - `/home/manuel/code/wesen/2026-03-07--qemu-power-tick/results-phase3-westonshm-devices2/01-post.png`
- Compare the serial logs:
  - `/home/manuel/code/wesen/2026-03-07--qemu-power-tick/results-phase3-westonshm-devices1/guest-serial.log`
  - `/home/manuel/code/wesen/2026-03-07--qemu-power-tick/results-phase3-westonshm-devices2/guest-serial.log`
- Reproduce the cleaned control run:
  - `bash guest/build-phase3-rootfs.sh build build/initramfs-phase3.img`
  - `bash guest/run-qemu-phase3.sh --kernel build/vmlinuz --initramfs build/initramfs-phase3.img --results-dir results-phase3-westonshm-devices2 --append-extra 'phase3_client=weston-simple-shm phase3_runtime_seconds=35 phase3_suspend_delay_seconds=10 phase3_wake_seconds=5 phase3_pm_test=devices'`
  - `python3 host/capture_phase3_suspend_checkpoints.py --socket results-phase3-westonshm-devices2/qmp.sock --serial-log results-phase3-westonshm-devices2/guest-serial.log --results-dir results-phase3-westonshm-devices2 --pre-name 00-pre --post-name 01-post`

### Technical details
- `results-phase3-westonshm-devices1`:
  - pre-suspend screenshot: firmware text plane
  - post-resume screenshot: firmware text plane
  - serial log includes:
    - `virtio_gpu_dequeue_ctrl_func ... *ERROR* response 0x1203 (command 0x105)`
    - `virtio_gpu_dequeue_ctrl_func ... *ERROR* response 0x1203 (command 0x104)`
- `results-phase3-westonshm-devices2`:
  - pre-suspend screenshot: black framebuffer
  - post-resume screenshot: firmware text plane
  - serial log does **not** show the earlier `virtio_gpu_dequeue_ctrl_func` errors
  - serial log still shows the xHCI resume/reset path
  - image difference metric from the checkpoint harness: `AE=3647`
- Interrupted follow-up:
  - a cleaned Chromium rerun was started in `results-phase3-suspend6`
  - the user intentionally interrupted the turn before the run completed, so that dataset needs to be treated as partial and re-run cleanly.

## Step 8: Ran A Real Stage-2 Control And Found The Same `devices` Visual Fallback Pattern In The Simpler Stack

After the stage-3 control isolate, I went back and did the control test the right way: rebuild the current phase-2 image with the actual suspend-driving client binary present, then run the same `pm_test=devices` cycle and capture pre/post screenshots. This was important because the first attempt accidentally booted only `weston-simple-shm` without the suspend-driving `wl_sleepdemo`, which meant the guest never actually entered the suspend path.

The corrected control run was much more informative. It showed the same overall display pattern as the cleaned stage-3 control run:
- pre-suspend screenshot: black framebuffer
- post-resume screenshot: firmware text plane
- xHCI resume/reset noise in the log
- no `virtio_gpu_dequeue_ctrl_func` invalid-resource errors

That means the remaining issue is probably not stage-3-specific anymore. Once the noisy debug instrumentation was removed, the surviving `devices` problem looks shared between the simpler phase-2 stack and the phase-3 stack.

### Prompt Context

**User prompt (verbatim):** "do the control test, then advise and continue. keep a diary sa you work"

**Assistant interpretation:** Run the proposed control experiment instead of reasoning abstractly, preserve the exact outcome in the diary, then use it to decide the next debugging direction.

**Inferred user intent:** Replace guesswork with a concrete control result and keep the investigation state explicit enough that later steps build on evidence instead of memory.

**Commit (code):** Pending at this step

### What I did
- Rebuilt the phase-2 initramfs once with the default settings and discovered the first control run was invalid because the rootfs did not contain `/bin/wl_sleepdemo`.
- Verified in [guest/init-phase2](/home/manuel/code/wesen/2026-03-07--qemu-power-tick/guest/init-phase2) that suspend only runs through `/bin/wl_sleepdemo` when present.
- Rebuilt the phase-2 rootfs correctly with:
  - `bash guest/build-wl-sleepdemo.sh build build/wl_sleepdemo`
  - `bash guest/build-phase2-rootfs.sh build build/initramfs-phase2.img build/wl_sleepdemo`
- Ran the corrected control in:
  - `/home/manuel/code/wesen/2026-03-07--qemu-power-tick/results-phase2-control-devices2`
- Captured:
  - `/home/manuel/code/wesen/2026-03-07--qemu-power-tick/results-phase2-control-devices2/00-pre.png`
  - `/home/manuel/code/wesen/2026-03-07--qemu-power-tick/results-phase2-control-devices2/01-post.png`
- Compared the phase-2 control directly against the cleaned stage-3 control:
  - `/home/manuel/code/wesen/2026-03-07--qemu-power-tick/results-phase3-westonshm-devices2`

### Why
- The stage-3 control result was pointing toward a shared base-layer issue, but without a current-tree phase-2 confirmation that was still an inference.
- A current phase-2 control removes a lot of ambiguity because it uses the simpler app stack on the same QEMU/Weston/suspend base.

### What worked
- The corrected phase-2 control entered suspend and resumed cleanly.
- The phase-2 serial log showed healthy suspend metrics:
  - `sleep_duration=5753 ms`
  - `suspend_resume_gap=5753 ms`
  - `resume_to_redraw=4 ms`
- The phase-2 control did **not** show the earlier stage-3-only-looking `virtio_gpu_dequeue_ctrl_func` errors.
- The visual result matched the cleaned stage-3 control closely enough to change the diagnosis.

### What didn't work
- The first phase-2 control attempt was invalid because the image did not include `wl_sleepdemo`, so the test never exercised suspend.
- The corrected phase-2 control still did not preserve visible framebuffer continuity in the screenshot harness, despite the app-level redraw metric saying the client repainted.

### What I learned
- The surviving `devices` display problem is likely a shared base issue, not a Chromium problem and not even obviously a phase-3 problem.
- The earlier `virtio_gpu` invalid-resource errors were a misleading extra signal caused by my debug logging, not the core issue.
- The fact that phase 2 reports `resume_to_redraw=4 ms` while the post-resume screenshot still shows the firmware text plane is strong evidence that the client/compositor may be alive while scanout or console ownership is wrong after resume.

### What was tricky to build
- The phase-2 control had a hidden footgun: the default init path only becomes the suspend-driving path if `/bin/wl_sleepdemo` exists in the rootfs. Without checking that branch explicitly, it was easy to think the first run was a valid control when it was not.
- The app-level metrics and the screenshot-level evidence now disagree in an important way, which is exactly the kind of partial-truth situation that can waste time if it is not written down carefully.

### What warrants a second pair of eyes
- Whether QEMU `screendump` after `pm_test=devices` is observing the wrong plane or a fallback fbcon plane rather than Weston’s intended scanout.
- Whether the next best proof is a cleaned stage-3 Chromium rerun or a lower-level scanout/DRM/fbcon ownership probe after resume.

### What should be done in the future
- Re-run the cleaned stage-3 Chromium `pm_test=devices` case now that the phase-2 control suggests a shared base issue.
- Shift the investigation away from “Chromium resume is broken” and toward “what owns visible scanout after `devices` resume?”
- If needed, add a more direct post-resume ownership probe rather than relying only on screenshots.

### Code review instructions
- Review the phase-2 control launch condition in:
  - [guest/init-phase2](/home/manuel/code/wesen/2026-03-07--qemu-power-tick/guest/init-phase2)
  - [guest/build-phase2-rootfs.sh](/home/manuel/code/wesen/2026-03-07--qemu-power-tick/guest/build-phase2-rootfs.sh)
- Inspect the corrected control artifacts:
  - `/home/manuel/code/wesen/2026-03-07--qemu-power-tick/results-phase2-control-devices2/00-pre.png`
  - `/home/manuel/code/wesen/2026-03-07--qemu-power-tick/results-phase2-control-devices2/01-post.png`
  - `/home/manuel/code/wesen/2026-03-07--qemu-power-tick/results-phase2-control-devices2/guest-serial.log`
- Compare against the cleaned stage-3 control:
  - `/home/manuel/code/wesen/2026-03-07--qemu-power-tick/results-phase3-westonshm-devices2/00-pre.png`
  - `/home/manuel/code/wesen/2026-03-07--qemu-power-tick/results-phase3-westonshm-devices2/01-post.png`
  - `/home/manuel/code/wesen/2026-03-07--qemu-power-tick/results-phase3-westonshm-devices2/guest-serial.log`

### Technical details
- Invalid first control:
  - `results-phase2-control-devices1`
  - rootfs lacked `/bin/wl_sleepdemo`
  - no suspend cycle occurred
- Corrected control:
  - `results-phase2-control-devices2`
  - metrics:
    - `sleep_duration=5753 ms`
    - `suspend_resume_gap=5753 ms`
    - `resume_to_redraw=4 ms`
  - visual diff metric:
    - `AE=3657`
- Shared surviving pattern between:
  - `results-phase2-control-devices2`
  - `results-phase3-westonshm-devices2`
  is:
  - pre-suspend black framebuffer
  - post-resume firmware text plane
  - xHCI resume/reset messages
  - no `virtio_gpu_dequeue_ctrl_func` invalid-resource errors

## Step 9: Re-Ran Cleaned Stage 3 With Chromium And Confirmed A Two-Layer Failure Instead Of One

The stage-2 control narrowed the problem, but it did not eliminate stage-3-specific behavior. So I ran one cleaned stage-3 Chromium `pm_test=devices` validation on the same initramfs that had already removed the self-inflicted Weston log tail. That was the right next step because it could tell me whether Chromium still brought back the extra DRM failure signal or whether the control and stage-3 paths had fully converged.

They did not fully converge. The cleaned Chromium run matched the controls at the screenshot level, but it also restored the `virtio_gpu_dequeue_ctrl_func` invalid-resource errors. So the current model is no longer “one mysterious stage-3 failure.” It is:
- one shared `devices`-resume visual fallback that also appears in the simpler phase-2 and phase-3 control runs
- plus one extra Chromium-associated DRM/resource failure signal on top of that

### Prompt Context

**User prompt (verbatim):** (see Step 8)

**Assistant interpretation:** After the control result, verify whether cleaned stage 3 still behaves differently from the controls, instead of assuming the control fully explained the stage-3 failure.

**Inferred user intent:** Use the control to reduce uncertainty, then keep going until the remaining difference is explicit.

**Commit (code):** Pending at this step

### What I did
- Re-ran cleaned stage 3 in:
  - `/home/manuel/code/wesen/2026-03-07--qemu-power-tick/results-phase3-suspend7`
- Used the same deterministic test page and the same suspend checkpoint harness shape as earlier stage-3 runs.
- Captured:
  - `/home/manuel/code/wesen/2026-03-07--qemu-power-tick/results-phase3-suspend7/00-pre.png`
  - `/home/manuel/code/wesen/2026-03-07--qemu-power-tick/results-phase3-suspend7/01-post.png`
- Compared the result against:
  - `results-phase2-control-devices2`
  - `results-phase3-westonshm-devices2`

### Why
- The phase-2 control strongly suggested a shared base-layer issue, but it did not answer whether Chromium still added its own failure mode.
- A cleaned Chromium rerun was required to avoid overcorrecting from “Chromium is the whole problem” to “Chromium is irrelevant.”

### What worked
- The run completed cleanly enough to provide a usable comparison dataset.
- The screenshot-level behavior matched the controls:
  - pre-suspend black framebuffer
  - post-resume firmware text plane
- The serial log still emitted valid suspend metrics:
  - `sleep_duration=5759 ms`
  - `suspend_resume_gap=5759 ms`

### What didn't work
- The cleaned Chromium run still restored the `virtio_gpu_dequeue_ctrl_func ... response 0x1203` errors:
  - command `0x105`
  - command `0x104`
- So removing the debug log tail was not sufficient to make stage 3 fully match the simpler controls.

### What I learned
- The phase-2 control was worth doing because it separated two problems that had been conflated:
  - shared base-layer visual fallback after `pm_test=devices`
  - Chromium-associated DRM resource errors on top of that base issue
- The remaining stage-3-specific question is now much narrower:
  - what is Chromium doing to the Weston/DRM path that causes the invalid-resource responses after resume?
- The right next experiments should now be smaller Chromium-focused controls, not more generic suspend reruns.

### What was tricky to build
- The danger after the phase-2 control was overgeneralizing from it. The cleaned Chromium rerun was necessary specifically to avoid collapsing two separate problems into one simplified narrative.
- The screenshot artifacts alone were not enough; the serial log remained necessary because the visual behavior matched the controls while the DRM error signature did not.

### What warrants a second pair of eyes
- Whether Chromium’s Wayland client path is causing a different class of buffer/resource reuse on resume than `wl_sleepdemo` or `weston-simple-shm`.
- Whether the next control should target Chromium’s rendering path specifically:
  - simpler browser content,
  - different launch flags,
  - or forcing a different client-side buffer behavior.

### What should be done in the future
- Keep the shared base-layer display fallback as one track of investigation.
- Start a second, smaller track for Chromium-specific post-resume DRM errors.
- Avoid treating either track as the full explanation by itself.

### Code review instructions
- Inspect the cleaned Chromium artifacts:
  - `/home/manuel/code/wesen/2026-03-07--qemu-power-tick/results-phase3-suspend7/00-pre.png`
  - `/home/manuel/code/wesen/2026-03-07--qemu-power-tick/results-phase3-suspend7/01-post.png`
  - `/home/manuel/code/wesen/2026-03-07--qemu-power-tick/results-phase3-suspend7/guest-serial.log`
- Compare those against the control runs:
  - `/home/manuel/code/wesen/2026-03-07--qemu-power-tick/results-phase2-control-devices2/guest-serial.log`
  - `/home/manuel/code/wesen/2026-03-07--qemu-power-tick/results-phase3-westonshm-devices2/guest-serial.log`

### Technical details
- `results-phase3-suspend7`:
  - visual diff metric: `AE=3647`
  - metrics:
    - `sleep_duration=5759 ms`
    - `suspend_resume_gap=5759 ms`
  - serial log includes:
    - `virtio_gpu_dequeue_ctrl_func ... *ERROR* response 0x1203 (command 0x105)`
    - `virtio_gpu_dequeue_ctrl_func ... *ERROR* response 0x1203 (command 0x104)`
- Comparison result:
  - screenshot behavior is shared with the controls
  - extra DRM invalid-resource errors are still Chromium-associated
