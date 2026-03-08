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
LastUpdated: 2026-03-07T22:20:00-05:00
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

**Commit (code):** Pending at this step

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

**Commit (code):** Pending at this step

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
