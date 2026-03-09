---
Title: Diary
Ticket: QEMU-07-CONTENT-SHELL-MODESET
Status: active
Topics:
    - qemu
    - linux
    - drm
    - chromium
    - graphics
DocType: reference
Intent: long-term
Owners: []
RelatedFiles: []
ExternalSources: []
Summary: ""
LastUpdated: 2026-03-09T18:16:01.080065982-04:00
WhatFor: ""
WhenToUse: ""
---

# Diary

## Goal

This diary records the focused follow-up investigation into why `content_shell` on Ozone DRM allocates buffers but still fails to activate a visible display controller in QEMU.

## Step 1: Split The Modeset-Mapping Investigation Into Its Own Ticket

The previous ticket had already done the hard bring-up work. Chromium built, headless worked, direct DRM startup reached real GPU initialization, and the no-fbdev controls showed that the guest could still modeset through `kms_pattern`. That meant the remaining failure no longer belonged in the general phase-4 ticket. It belonged in a narrower ticket about Chromium's own display-controller logic.

This new ticket starts from a specific hypothesis derived from the imported `ozone-answers.md` note: `content_shell` may be creating buffers without ever getting mapped to an enabled display controller because its window bounds do not match the active mode rectangle the way Ozone DRM expects. The reason to write that down first is simple: if the hypothesis is wrong, the next experiments should disprove it quickly instead of wandering back into generic QEMU/DRM debugging.

### Prompt Context

**User prompt (verbatim):** "alright, create a new ticket to do that, add a detailed guide on what you are going to do and why, then create individual tasks and work on them one by one, comitting at appropriate intervals, keeping a diary"

**Assistant interpretation:** Create a fresh, narrowly scoped ticket for the `content_shell` controller-mapping hypothesis, document the plan clearly, then execute the new experiments with normal commit and diary discipline.

**Inferred user intent:** Keep the next debugging slice small and reviewable, rather than continuing to pile mode-setting experiments into the broader phase-4 ticket.

### What I did
- Created the ticket `QEMU-07-CONTENT-SHELL-MODESET`.
- Imported `/tmp/ozone-answers.md` into the new ticket.
- Wrote the focused design guide in:
  - `design/01-content-shell-modeset-mapping-investigation-guide.md`
- Replaced the placeholder task list with a narrow experiment ladder.
- Started this diary.

### Why
- The remaining bug is now much narrower than "Chromium on DRM doesn't work."
- The previous ticket already established a good baseline.
- A fresh ticket makes it easier to reason about the next controls and write a clean report later.

### What worked
- The imported note matched the existing evidence well.
- The new ticket now starts with a real hypothesis and a real decision tree instead of an empty scaffold.

### What didn't work
- N/A yet. This step was setup and reframing, not a runtime control.

### What I learned
- The cleanest next experiment is not another generic fullscreen run.
- The strongest next probe is a no-fbdev `800x600` run aimed directly at the controller-mapping theory.

### What was tricky to build
- The main challenge here was scope discipline. It would have been easy to keep using the phase-4 ticket as a catch-all, but that would have made the next conclusion harder to trust and harder to explain.

### What warrants a second pair of eyes
- The exact framing of the hypothesis should be checked against the Chromium DRM code once the first `800x600` result lands.

### What should be done in the future
- Run the first `800x600` no-fbdev control.
- Compare it directly to the previous `drm24` and `kms2` controls.
- Tighten Chromium logging only if the `800x600` run still leaves the connector disabled.

### Code review instructions
- Start with:
  - `design/01-content-shell-modeset-mapping-investigation-guide.md`
  - `tasks.md`
  - `sources/local/01-ozone-answers.md`

### Technical details
- Ticket path:
```text
ttmp/2026/03/09/QEMU-07-CONTENT-SHELL-MODESET--content-shell-drm-modeset-and-controller-mapping-investigation
```

## Step 2: Parameterize content_shell Window Controls And Run The First 800x600 Probe

The first runtime change in this ticket was to make the phase-4 guest pass `content_shell` window controls through the kernel cmdline instead of hardcoding one launch shape. The reason was simple: if the mapping hypothesis is real, the experiment needs to vary window geometry without changing unrelated boot logic.

### What I did
- Updated `guest/init-phase4-drm` to parse:
  - `phase4_content_shell_window_size=*`
  - `phase4_content_shell_fullscreen=0`
- Exported those values to `guest/content-shell-drm-launcher.sh`.
- Updated the launcher to:
  - log `WINDOW_SIZE` and `FULLSCREEN`
  - optionally omit `--start-fullscreen`
  - initially pass a generic `--window-size=$WINDOW_SIZE` switch
- Mirrored the updated scripts into the ticket `scripts/` folder.
- Rebuilt the phase-4 initramfs.
- Ran the first no-fbdev `800x600` control as `results-phase4-drm26`.

### Why
- The imported `ozone-answers.md` note made window/controller mapping the main suspect.
- The quickest way to test that was a non-fullscreen `800x600` run under `drm_kms_helper.fbdev_emulation=0`.

### What worked
- The new kernel-cmdline plumbing worked.
- The run captured the usual evidence set:
  - serial log
  - DRM state snapshots
  - display probe output
  - QMP screenshot

### What didn't work
- `results-phase4-drm26` turned out not to be a valid size-control test.
- The host-visible result stayed the same:
  - `00-smoke.png` stayed `640x480`
  - nonblack pixel count stayed `684`
- The guest still showed scanout-capable `DrmThread` shared images at `814x669`.

### What I learned
- `content_shell` does not use Chrome's generic `--window-size` switch for its host window.
- So `drm26` could not actually confirm or reject the mapping hypothesis.

### What warrants a second pair of eyes
- The right `content_shell` switch needed to be confirmed from Chromium source, not guessed from Chrome runtime habits.

## Step 3: Correct The content_shell Size Switch And Re-run The Probe

After `drm26`, I checked Chromium source directly instead of trying another flag guess.

### What I did
- Searched Chromium source and found:
  - `content/shell/common/shell_switches.h`
  - `content/shell/browser/shell.cc`
- Confirmed that `content_shell` uses:
  - `--content-shell-host-window-size=WxH`
  - not Chrome's generic `--window-size=...`
- Patched `guest/content-shell-drm-launcher.sh` to pass:
  - `--content-shell-host-window-size=${WINDOW_SIZE/,/x}`
- Mirrored the corrected launcher into the ticket `scripts/` folder.
- Rebuilt the phase-4 initramfs again.
- Reran the same no-fbdev `800x600` control as `results-phase4-drm27`.

### Why
- `drm26` was invalid as a real size-control experiment.
- The only responsible next step was to use the real `content_shell` host-window-size API and rerun the exact same control.

### What worked
- The corrected switch did reach the browser runtime.
- `results-phase4-drm27/guest-serial.log` now shows:
  - `WINDOW_SIZE=800,600 FULLSCREEN=0`
  - Blink/content geometry around `800x595`
- This proves the experiment now changes the actual shell window/content sizing layer rather than just the launcher arguments.

### What didn't work
- Even with the corrected size switch:
  - the connector stayed disabled
  - the CRTC stayed inactive
  - the active plane stayed unbound
  - the scanout-capable `DrmThread` shared images still stayed `814x669`
  - the host-visible frame stayed the same `640x480` fallback image with `684` nonblack pixels

### What I learned
- Content area sizing alone is not enough to get `content_shell` onto a visible controller in this setup.
- The next obvious suspect is shell chrome itself, especially the toolbar/frame that wraps the content area.
- Chromium source confirms that `content_shell` has an explicit switch for that too:
  - `--content-shell-hide-toolbar`

### What was tricky to build
- The tricky part here was not code. It was not letting a bad first control pollute the conclusion.
- `drm26` had to be written down as invalid for the original hypothesis even though it still produced logs and screenshots.

### What should be done in the future
- Add a guest-cmdline path for `content-shell-hide-toolbar`.
- Rebuild.
- Rerun the same no-fbdev `800x600` control with toolbar hidden.

### Technical details
- `results-phase4-drm26/00-smoke.png`:
  - size `640x480`
  - nonblack pixels `684`
- `results-phase4-drm27/00-smoke.png`:
  - size `640x480`
  - nonblack pixels `684`
- `results-phase4-drm27/guest-serial.log` shows:
  - launcher env: `WINDOW_SIZE=800,600 FULLSCREEN=0`
  - content sizing around `800x595`
  - but late DRM state still leaves the connector/CRTC inactive

## Step 4: Hide The Toolbar And Re-test The No-fbdev 800x600 Control

After `drm27`, the next smallest experiment was to remove shell chrome itself instead of changing content size again.

### What I did
- Added `phase4_content_shell_hide_toolbar=1` parsing to `guest/init-phase4-drm`.
- Exported `CONTENT_SHELL_HIDE_TOOLBAR`.
- Updated `guest/content-shell-drm-launcher.sh` to pass `--content-shell-hide-toolbar`.
- Mirrored the updated scripts into the ticket `scripts/` folder.
- Rebuilt the phase-4 initramfs.
- Ran `results-phase4-drm28` with:
  - `phase4_content_shell_window_size=800,600`
  - `phase4_content_shell_fullscreen=0`
  - `phase4_content_shell_hide_toolbar=1`
  - `drm_kms_helper.fbdev_emulation=0`

### Why
- `drm27` proved that content sizing changes were real but insufficient.
- The next plausible mismatch was the shell frame/toolbar around the content area.

### What worked
- The new flag reached the browser runtime.
- Content geometry moved to `800x600`.
- The scanout-capable `DrmThread` buffers changed from `814x669` to `810x628`.

### What didn't work
- The connector still stayed disabled.
- The CRTC still stayed inactive.
- `00-smoke.png` still stayed `640x480` with `684` nonblack pixels.

### What I learned
- Shell chrome affects geometry, but it still does not explain the absence of visible scanout by itself.

## Step 5: Instrument Chromium's Mapping Path

Once geometry changed again without activating scanout, the highest-value move was source-level logging inside Chromium itself.

### What I did
- Patched Chromium source in:
  - `ui/ozone/platform/drm/gpu/drm_window.cc`
  - `ui/ozone/platform/drm/gpu/screen_manager.cc`
- Added logs for:
  - `DrmWindow::SetBounds`
  - `DrmWindow::SetController`
  - `DrmWindow::SchedulePageFlip` when it ACKs with `controller=null`
  - `ScreenManager::AddWindow`
  - `ScreenManager::UpdateControllerToWindowMapping`
- Rebuilt `content_shell` incrementally.
- Restaged the payload.
- Rebuilt the phase-4 initramfs.
- Ran `results-phase4-drm29` and `results-phase4-drm30`.
- Mirrored the patched Chromium files into the ticket `scripts/` folder:
  - `chromium-drm_window.cc`
  - `chromium-screen_manager.cc`

### Why
- The open question was no longer "did geometry change?"
- The real question was whether Chromium had a controller at first-flip time at all.

### What worked
- The instrumentation answered that directly.
- `results-phase4-drm29` and `results-phase4-drm30` show:
  - `ScreenManager::AddWindow ... controller_count=0`
  - `UpdateControllerToWindowMapping begin controllers=0 windows=1`
  - `DrmWindow::SchedulePageFlip ... controller=null ... -> ack without real flip`

### What didn't work
- The connector still stayed disabled.
- The host-visible image still stayed the same fallback frame.

### What I learned
- This closes the original branch of the investigation.
- The failure is not "controller discovered, then exact-bounds mismatch prevented mapping."
- The failure is earlier: `content_shell` starts page-flipping while `ScreenManager` still has zero controllers.

## Step 6: Start Probing Display Discovery

Once the mapping logs showed `controllers=0`, the next probe had to move earlier into the display-discovery path.

### What I did
- Added first-pass `VLOG(1)` instrumentation to:
  - `ui/ozone/platform/drm/gpu/drm_gpu_display_manager.cc`
  - `GetDisplays()`
  - `NotifyScreenManager()`
- Rebuilt Chromium incrementally.
- Restaged the payload.
- Rebuilt the phase-4 initramfs.
- Ran `results-phase4-drm31` with:
  - `phase4_content_shell_vmodule=screen_manager=1,drm_window=1,drm_gpu_display_manager=1`
- Mirrored the patched Chromium file into the ticket `scripts/` folder:
  - `chromium-drm_gpu_display_manager.cc`

### Why
- `AddDisplayController` never appeared in the instrumented mapping runs.
- The next plausible missing piece was controller discovery / notification itself.

### What worked
- `drm31` reproduced the same null-controller behavior cleanly:
  - `ScreenManager::AddWindow ... controller_count=0`
  - `UpdateControllerToWindowMapping begin controllers=0 windows=1`
  - `DrmWindow::SchedulePageFlip ... controller=null`

### What didn't work
- The first-pass `drm_gpu_display_manager` `VLOG` lines did not appear.
- So this run did not yet explain whether discovery never ran, ran before logging was active, or needs stronger instrumentation.

### What I learned
- The strongest current conclusion is still the one from `drm29`/`drm30`: the bug is earlier than controller matching.
- The next probe should probably use stronger logging than `VLOG(1)` in the discovery/configuration path.

### External source state
- Chromium checkout: `/home/manuel/chromium/src`
- Local branch: `qemu-07-content-shell-controller-debug`
- Commit: `9f6c936991`
- Purpose: save the DRM/Ozone instrumentation in:
  - `ui/ozone/platform/drm/gpu/drm_window.cc`
  - `ui/ozone/platform/drm/gpu/screen_manager.cc`
  - `ui/ozone/platform/drm/gpu/drm_gpu_display_manager.cc`
