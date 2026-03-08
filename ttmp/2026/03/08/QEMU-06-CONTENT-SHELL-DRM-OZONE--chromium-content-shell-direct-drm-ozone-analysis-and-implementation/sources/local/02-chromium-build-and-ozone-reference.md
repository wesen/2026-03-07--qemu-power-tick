---
Title: Imported Chromium Build And Ozone Reference
Ticket: QEMU-06-CONTENT-SHELL-DRM-OZONE
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
LastUpdated: 2026-03-08T12:53:17-04:00
WhatFor: ""
WhenToUse: ""
---

# Chromium Build And Ozone DRM Reference

This note captures the official upstream references that shape the QEMU-06 direct DRM/Ozone plan.

## Primary Sources

- Chromium Linux build instructions:
  - https://chromium.googlesource.com/chromium/src/+/main/docs/linux/build_instructions.md
- Chromium Ozone overview:
  - https://chromium.googlesource.com/chromium/src/+/main/docs/ozone_overview.md

## Linux Build Instructions: Key Points

The Linux build instructions say:
- install `depot_tools`
- put `depot_tools` at the front of `PATH`
- create a checkout directory such as `~/chromium`
- run:

```sh
fetch --nohooks chromium
```

- if full repo history is not needed, add:

```sh
--no-history
```

- after `fetch` completes:
  - `.gclient` exists in the checkout root
  - `src/` exists underneath it
- then:

```sh
cd src
./build/install-build-deps.sh
gclient runhooks
gn gen out/Default
```

The doc also says Chromium uses:
- `depot_tools`
- `gclient`
- `gn`
- `Siso`/`ninja`

and expects a large machine:
- x86-64
- at least 8 GB RAM, 16 GB+ recommended
- at least 100 GB free disk

## Ozone Overview: Key Points

The Ozone overview explains that Ozone is Chromium's platform abstraction for graphics/input/window-system integration.

Important implications for QEMU-06:
- Wayland Ozone and DRM Ozone are different runtime paths.
- The current phase-3 repo path is Chromium as a Wayland client.
- QEMU-06 is specifically about the direct DRM/KMS path.
- `content_shell` is the smallest Chromium-family binary that still exercises the Ozone stack and is therefore the right first milestone.

## Why This Matters For QEMU-06

These upstream docs justify the current ticket decisions:
- use `depot_tools`, not ad hoc git + build guesses
- use `fetch chromium`
- prefer `--no-history` for this lab because full history is unnecessary
- start with `content_shell`
- keep direct DRM/Ozone as a separate phase from the already-working Weston/Wayland phase

## Local Interpretation

For this repo, the practical build sequence is:

```text
depot_tools
  -> fetch chromium
    -> chromium/src
      -> gn gen out/<dir>
        -> ninja/autoninja content_shell
          -> phase-4 rootfs packaging
            -> QEMU direct DRM/Ozone boot
```

The note is not a substitute for the upstream docs. It exists so the ticket keeps the exact references that justify the current build/bootstrap strategy.
