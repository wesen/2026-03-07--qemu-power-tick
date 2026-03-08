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
Summary: ""
LastUpdated: 2026-03-08T12:53:17.280619465-04:00
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
- phase-4 payload/runtime probe added; current baseline says host DRM runtime looks present and Chromium payload artifacts are still the missing piece
- Chromium `src/` tree is now materialized enough to inspect BUILD files
- initial verified target set is `content_shell`, `chrome_sandbox`, and `chrome_crashpad_handler`
- a first-pass phase-4 `args.gn` writer now creates `~/chromium/src/out/Phase4DRM/args.gn`
- a phase-4 build driver now scripts `gclient runhooks -> gn gen -> autoninja`
- official Chromium build-deps installer has now been run with sudo help from the user
- `gclient sync --nohooks --no-history` and `gclient runhooks` are complete
- `out/Phase4DRM/build.ninja` exists and the first `autoninja` build is in flight

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

- design/ - Architecture and design documents
- reference/ - Prompt packs, API contracts, context summaries
- playbooks/ - Command sequences and test procedures
- scripts/ - Temporary code and tooling
- various/ - Working notes and research
- archive/ - Deprecated or reference-only artifacts
