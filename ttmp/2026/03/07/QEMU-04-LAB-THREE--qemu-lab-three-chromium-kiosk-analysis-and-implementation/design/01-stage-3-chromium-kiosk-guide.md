---
Title: Stage 3 Chromium Kiosk Guide
Ticket: QEMU-04-LAB-THREE
Status: active
Topics:
    - qemu
    - virtualization
    - lab
    - systems
DocType: design
Intent: long-term
Owners: []
RelatedFiles: []
ExternalSources:
    - local:01-lab2.md
Summary: Architecture and implementation guide for the Chromium-on-Wayland kiosk stage, including packaging strategy, runtime model, validation plan, and likely risks.
LastUpdated: 2026-03-07T21:25:00-05:00
WhatFor: Define the stage-3 architecture and implementation plan before and during Chromium kiosk bring-up.
WhenToUse: Read this when implementing or reviewing the stage-3 Chromium kiosk work.
---

# Stage 3 Chromium Kiosk Guide

## Goal

Replace the phase-2 custom demo client with Chromium running on Weston `kiosk-shell`, while preserving:

- host-side screenshot capture,
- host-side keyboard and pointer injection,
- suspend/resume continuity,
- the ability to study whether Chromium or its hosted page introduces unwanted wake activity.

## System Model

The stage-3 stack keeps most of phase 2 intact:

- guest bootstrap still mounts the minimal initramfs root,
- `systemd-udevd` and `seatd` still prepare input/device access,
- Weston still provides the Wayland compositor,
- the host still drives screenshots and input via QMP,
- the network/server harness still shapes behavior from the host,
- only the guest foreground app changes from `wl_sleepdemo` to Chromium.

## Immediate Constraint

On this host, `/usr/bin/chromium-browser` is only a shell launcher for the installed snap:

- it requires `/snap/bin/chromium`,
- it is not itself the browser payload,
- the actual browser binary is available at `/snap/chromium/current/usr/lib/chromium-browser/chrome`.

That means the first implementation path is:

1. copy the installed Chromium snap payload into the guest rootfs,
2. copy the shared-library dependencies Chromium still resolves from `/lib/x86_64-linux-gnu`,
3. launch Chromium directly with a thin custom wrapper instead of reproducing full snap semantics inside the guest.

## Planned Runtime Model

The first stage-3 runtime target is intentionally simple:

- Weston DRM backend
- `kiosk-shell`
- Chromium launched with:
  - `--enable-features=UseOzonePlatform`
  - `--ozone-platform=wayland`
  - `--no-sandbox`
  - `--disable-dev-shm-usage`
  - `--user-data-dir=/var/lib/chromium`
  - `--no-first-run`
  - `--no-default-browser-check`
  - one startup URL such as `about:blank`

This is the debug bring-up target, not yet the full wake-study target.

## Planned Validation Ladder

1. Boot Chromium under Weston and verify it reaches a visible surface.
2. Capture a QMP screenshot showing Chromium rendered.
3. Inject keyboard input and verify focus/input works.
4. Inject pointer input and verify pointer interaction works.
5. Reintroduce suspend/resume and confirm Chromium survives it.
6. Add URL and kiosk-path validation.
7. Add wake-study instrumentation and traffic-shaping experiments.

## Pseudocode

```text
build phase-3 rootfs:
  copy phase-2 userspace pieces
  copy Chromium snap payload
  copy Chromium shared-library dependencies
  add phase-3 init + launcher

boot guest:
  mount proc/sys/dev
  configure network
  start udev + seatd + weston
  wait for WAYLAND_DISPLAY
  launch Chromium with Wayland kiosk flags

validate:
  capture screenshot
  inject key
  inject pointer
  inspect serial log + weston log + browser log
```

## First-Round Risks

- Chromium may depend on more snap-specific runtime expectations than the raw binary/ldd view suggests.
- `--no-sandbox` is probably required in this guest model.
- Chromium may need additional assets beyond ELF libraries:
  - locales
  - `.pak` files
  - `icudtl.dat`
  - crashpad helper
  - fontconfig data
  - X11 support files
- Browser startup may expose DBus, GPU, or font assumptions not exercised by the phase-2 demo client.

## Deliverables

- phase-3 guest rootfs builder
- phase-3 init / browser launcher
- run script
- diary of packaging/runtime failures
- first successful Chromium screenshot
- follow-up task list for input, suspend, and wake-study work
