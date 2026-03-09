---
Title: Chromium content_shell Direct DRM Ozone Analysis and Implementation
Ticket: QEMU-06-CONTENT-SHELL-DRM-OZONE
Status: active
Topics:
    - qemu
    - linux
    - drm
    - chromium
    - graphics
DocType: index
Intent: long-term
Owners: []
RelatedFiles: []
ExternalSources:
    - local:01-drm-ozone.md
    - local:02-chromium-build-and-ozone-reference.md
    - local:ozone-answers.md
Summary: ""
LastUpdated: 2026-03-09T18:13:42.500167368-04:00
WhatFor: ""
WhenToUse: ""
---




# Chromium content_shell Direct DRM Ozone Analysis and Implementation

## Overview

This ticket starts a fresh phase after the Weston/Wayland kiosk work. The goal is to run Chromium `content_shell` directly on Linux DRM/KMS through Chromium's Ozone DRM backend, without Weston in front of it. That should simplify the graphics stack and separate "browser owns KMS directly" from "browser is a Wayland client inside a compositor."

The current repo already has a working phase-3 path for `kernel -> initramfs -> udev/seatd -> Weston DRM -> Chromium Ozone/Wayland`, but it is intentionally software-rendered and still mixes compositor behavior with browser behavior. The new ticket is about building a second branch:
- QEMU display device
- Linux DRM/KMS
- Chromium `content_shell`
- Ozone DRM/GBM
- QMP capture and suspend instrumentation

Current status:
- imported upstream/analysis note reviewed
- initial intern-facing implementation guide written
- implementation tasks defined
- diary started
- guide uploaded to reMarkable
- Chromium bootstrap helper added and mirrored into the ticket scripts archive
- first live Chromium fetch started; solution config exists and `gclient sync` is in progress
- phase-4 kms-only initramfs and runner created
- first no-Weston phase-4 QMP smoke screenshot captured successfully at `1280x800`
- phase-4 payload/runtime probe added and corrected for the real DRM payload/runtime boundary
- Chromium `src/` tree is materialized and the first local `content_shell` build completed
- verified target set is `content_shell`, `chrome_sandbox`, and `chrome_crashpad_handler`
- `out/Phase4DRM` now builds both Ozone DRM and Ozone headless backends
- staged payload validated in host-side `--ozone-platform=headless` mode
- local Chromium `content_shell` was patched outside this repo to call Ozone `PostCreateMainMessageLoop()`, which removed the earlier evdev/input crash on the DRM path
- direct DRM/Ozone QEMU boots now get through:
  - DRM authentication on `/dev/dri/card0`
  - GPU process `init_success:1`
  - Ozone discovery of `/dev/dri/renderD128`
- native Mesa/glvnd runtime packaging is now present in the phase-4 guest
- guest-side display probe stays stable while Chromium runs, but host-visible QMP screenshots are still fully black
- deeper DRM debugfs snapshots now show why the situation is still stuck:
  - Chromium creates `DrmThread` framebuffers
  - the active plane remains bound to the fbcon-allocated framebuffer
  - the result is unchanged even in the `phase4_unbind_fbcon=1` control
- removing the forced `EGL_PLATFORM=surfaceless` default did not change that outcome
- the stronger `drm_kms_helper.fbdev_emulation=0` control changed the state substantially:
  - `fb0` disappears
  - the connector becomes `connected enabled=disabled`
  - `plane-0` no longer points at an fbcon-owned framebuffer
  - Chromium still creates `DrmThread` framebuffers but does not activate the CRTC/connector
  - the simple `kms_pattern` control can still re-enable the connector under the same kernel setting, which means the guest can still modeset without fbdev emulation
- the current open problem is no longer "Chromium won't start"; it is "Chromium starts on direct DRM, but it still does not activate a visible scanout path, and QMP becomes ambiguous once fbdev emulation is disabled"

## Key Links

- Analysis guide: [design-doc/01-direct-drm-ozone-content-shell-analysis-and-implementation-guide.md](./design-doc/01-direct-drm-ozone-content-shell-analysis-and-implementation-guide.md)
- Diary: [reference/01-diary.md](./reference/01-diary.md)
- Imported note: [sources/local/01-drm-ozone.md](./sources/local/01-drm-ozone.md)
- Imported Chromium refs: [sources/local/02-chromium-build-and-ozone-reference.md](./sources/local/02-chromium-build-and-ozone-reference.md)
- Prior phase-3 ticket: [../QEMU-04-LAB-THREE--qemu-lab-three-chromium-kiosk-analysis-and-implementation/index.md](../QEMU-04-LAB-THREE--qemu-lab-three-chromium-kiosk-analysis-and-implementation/index.md)
- Prior QEMU capture investigation: [../QEMU-05-DEVICES-RESUME-INVESTIGATION--devices-resume-display-ownership-investigation/index.md](../QEMU-05-DEVICES-RESUME-INVESTIGATION--devices-resume-display-ownership-investigation/index.md)

## Status

Current status: **active**

## Topics

- qemu
- linux
- drm
- chromium
- graphics

## Tasks

See [tasks.md](./tasks.md) for the current task list.

## Changelog

See [changelog.md](./changelog.md) for recent changes and decisions.

## Structure

- design-doc/ - Architecture and design documents
- reference/ - Prompt packs, API contracts, context summaries
- scripts/ - Temporary code and tooling
- sources/ - Imported reference material
