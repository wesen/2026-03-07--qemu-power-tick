---
Title: content_shell modeset mapping investigation guide
Ticket: QEMU-07-CONTENT-SHELL-MODESET
Status: active
Topics:
    - qemu
    - linux
    - drm
    - chromium
    - graphics
DocType: design
Intent: long-term
Owners: []
RelatedFiles: []
ExternalSources: []
Summary: ""
LastUpdated: 2026-03-09T18:16:00.991739543-04:00
WhatFor: ""
WhenToUse: ""
---

# content_shell Modeset Mapping Investigation Guide

## Goal

This ticket isolates one narrow question from phase 4: why does Chromium `content_shell` on Ozone DRM create `DrmThread` framebuffers but fail to activate a visible scanout path in QEMU? The working hypothesis is that this is primarily a window-to-controller mapping problem inside Chromium's Ozone DRM path, not a generic rendering failure and not a generic guest DRM failure.

## Why A New Ticket Exists

The previous phase-4 ticket already answered the large questions:
- Chromium builds from source.
- Headless mode works.
- Direct DRM startup reaches GPU initialization successfully.
- With `drm_kms_helper.fbdev_emulation=0`, the guest can still modeset through the direct `kms_pattern` helper.

What remains is a narrower subsystem bug hunt:
- `content_shell` allocates buffers.
- the connector/CRTC never become active under the Chromium path.
- the allocated framebuffer size is suspiciously `814x669` rather than `1280x800`.

That deserves its own ticket because the experiments are now about Chromium's display-binding logic, not about initial bring-up.

## Current Evidence

The important inputs from the previous ticket are:
- `results-phase4-drm23/`
  - removing forced `EGL_PLATFORM=surfaceless` did not help
  - active plane still stayed on fbcon when fbdev emulation was enabled
- `results-phase4-drm24/`
  - with `drm_kms_helper.fbdev_emulation=0`, `fb0` disappears
  - Chromium still creates `DrmThread` framebuffers
  - `crtc[35] enable=0 active=0`
  - `connector[36] crtc=(null)`
- `results-phase4-kms2/`
  - on the same guest image and same no-fbdev kernel setting, `kms_pattern` enables the connector

That means the guest DRM stack is still capable of modesetting in the no-fbdev configuration. The remaining failure is therefore more likely to be specific to Chromium's window/display-controller mapping or modeset path.

## Imported Guidance From `ozone-answers.md`

The imported note points at Chromium Ozone DRM internals rather than host capture quirks:
- `ScreenManager::UpdateControllerToWindowMapping()` only maps enabled controllers to windows whose bounds exactly match the display mode rectangle.
- `DrmWindow::SchedulePageFlip()` can acknowledge swaps without a real flip if no display controller is attached.
- `content_shell` is a poor fullscreen probe because it is historically closer to an `800x600` shell window than a true kiosk shell.

This fits the current evidence unusually well:
- buffers exist
- controller stays disabled
- buffer size is not the mode size

## System Model

The relevant runtime path is:

```text
host
  -> build initramfs
  -> boot qemu with virtio-gpu-pci
guest /init
  -> mount filesystems
  -> load virtio-gpu/input modules
  -> start udev
  -> exec content_shell --ozone-platform=drm
Chromium Ozone DRM
  -> discover display devices
  -> create DRM window
  -> allocate GBM/DRM buffers
  -> map window to display controller
  -> test modeset
  -> real modeset / page flip
```

The bug appears to live in the last three lines, not the first ones.

## Important Files

Repo files that matter most in this ticket:
- `guest/init-phase4-drm`
- `guest/content-shell-drm-launcher.sh`
- `guest/build-phase4-rootfs.sh`
- `host/capture_phase4_smoke.py`
- `host/extract_drmstate_from_serial.py`

Chromium files that matter most in this ticket:
- `/home/manuel/chromium/src/ui/ozone/platform/drm/gpu/screen_manager.cc`
- `/home/manuel/chromium/src/ui/ozone/platform/drm/gpu/drm_window.cc`
- `/home/manuel/chromium/src/ui/ozone/platform/drm/gpu/drm_thread.cc`
- `/home/manuel/chromium/src/ui/ozone/platform/drm/gpu/hardware_display_controller.cc`
- `/home/manuel/chromium/src/ui/ozone/platform/drm/gpu/crtc_controller.cc`
- `/home/manuel/chromium/src/ui/ozone/platform/drm/gpu/drm_gpu_display_manager.cc`
- `/home/manuel/chromium/src/ui/ozone/platform/drm/gpu/hardware_display_plane_manager_atomic.cc`

## Main Hypothesis

Primary hypothesis:
- `content_shell` is not creating a window whose bounds match the display controller's mode rectangle.
- Because of that, Ozone DRM never binds the window to an enabled controller.
- Buffer allocation still succeeds, which is why `DrmThread` framebuffers exist.
- Real scanout never begins, which is why the connector remains disabled.

Secondary hypothesis:
- the failure is still in Chromium's `TestModeset()` / `Modeset()` path even after controller mapping is attempted.

Less likely hypothesis:
- QEMU/virtio-gpu is fundamentally incompatible here.

That last one is weaker because `kms_pattern` still enables the connector under the same no-fbdev setting.

## Investigation Strategy

The next work should be deliberately narrow.

### Step 1: Test The Window-Size Theory Directly

Run `content_shell` in a configuration closer to its documented/default window shape instead of forcing `1280x800`.

First probe:
- use `800x600`
- keep `drm_kms_helper.fbdev_emulation=0`
- keep the deep guest probes on

Expected outcomes:
- If the connector becomes active, the mapping hypothesis is strongly supported.
- If not, move to Chromium's modeset logic itself.

What the first two runs already established:
- `results-phase4-drm26` was not a valid size-control test because `content_shell` does not honor Chrome's generic `--window-size` switch.
- `results-phase4-drm27` used Chromium's real `--content-shell-host-window-size=800x600` switch and did change Blink/content geometry to about `800x595`.
- Even after that correction, the scanout-capable `DrmThread` buffers still stayed at `814x669`, the connector remained disabled, and the host-visible QMP frame remained the same `640x480` fallback image.

That means content area sizing alone is not enough. The next branch is to remove shell chrome so the outer host window can get closer to the mode rectangle Chromium's controller-mapping logic expects.

### Step 2: Remove Shell Chrome And Re-run

The imported note and Chromium source both point at `content_shell` itself being a poor kiosk probe. The launcher should grow an explicit control for:
- `--content-shell-hide-toolbar`

The next control should keep:
- `phase4_content_shell_window_size=800,600`
- `phase4_content_shell_fullscreen=0`
- `drm_kms_helper.fbdev_emulation=0`

and add:
- `phase4_content_shell_hide_toolbar=1`

Expected outcomes:
- If connector activation appears, the remaining mismatch was likely the shell frame/toolbar rather than content size.
- If not, the investigation moves down into modeset/controller activation itself.

### Step 3: Tighten Chromium DRM Logging

Do not use broad wildcard VLOG patterns. Focus on:
- `screen_manager`
- `drm_window`
- `drm_thread`
- `hardware_display_controller`
- `crtc_controller`
- `drm_gpu_display_manager`
- `hardware_display_plane_manager`

Goal:
- capture whether a controller is selected
- capture whether `TestModeset()` or `Modeset()` is called
- capture whether the controller-window mapping is rejected

### Step 4: Add A Guest-Trusted Visual Proof

Because QMP becomes ambiguous in the no-fbdev configuration, add one guest-trusted display proof if needed:
- guest-side DRM state
- guest-side framebuffer/plane readback
- or a controlled KMS readback helper

This should only be added if the `800x600` and tighter-log runs still do not explain the modeset failure clearly.

## Decision Tree

```text
Run corrected 800x600 no-fbdev content_shell
  -> connector active?
     yes -> content_shell sizing/fullscreen mismatch was the main problem
     no  -> hide toolbar / shell chrome and rerun
            -> connector active?
               yes -> outer shell window bounds were the blocking mismatch
               no  -> inspect focused Chromium DRM logs
            -> modeset attempted and failed?
               yes -> debug TestModeset/Modeset path
               no  -> debug window/controller mapping path
```

## Pseudocode

The internal Chromium question we are really asking is closer to:

```text
create_window()
discover_enabled_controllers()
if window.bounds != controller.mode_rect:
    no_controller_mapping
    swaps_ack_without_real_flip
else:
    test_modeset()
    if test_modeset_fails:
        no_real_scanout
    else:
        modeset()
        page_flip()
```

The ticket is about finding which branch is actually happening.

## Success Criteria

Short-term success:
- one run that clearly distinguishes "no controller mapping" from "modeset failed after mapping"

Medium-term success:
- one direct DRM Chromium run with:
  - connector active
  - CRTC active
  - guest and host capture no longer contradictory enough to block interpretation

## Review Guidance

If someone new joins this ticket, they should review in this order:
1. the imported `ozone-answers.md`
2. `results-phase4-drm24/drmstate/drmstate-phase4-early.txt`
3. `results-phase4-kms2/drmstate/drmstate-phase4-early.txt`
4. `guest/init-phase4-drm`
5. `guest/content-shell-drm-launcher.sh`
6. Chromium's `screen_manager.cc` and `drm_window.cc`

That order explains the state transition from "generic DRM bring-up" to "Chromium display-controller mapping bug hunt."
