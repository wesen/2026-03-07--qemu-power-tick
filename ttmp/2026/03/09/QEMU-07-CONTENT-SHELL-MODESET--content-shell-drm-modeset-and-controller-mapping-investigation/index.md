---
Title: content_shell DRM modeset and controller mapping investigation
Ticket: QEMU-07-CONTENT-SHELL-MODESET
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
    - local:01-ozone-answers.md
Summary: ""
LastUpdated: 2026-03-09T18:16:00.970638605-04:00
WhatFor: ""
WhenToUse: ""
---


# content_shell DRM modeset and controller mapping investigation

## Overview

This ticket isolates a narrow follow-up from phase 4. The main question is whether `content_shell` fails to light up direct DRM scanout because its window/controller mapping never becomes valid inside Chromium Ozone DRM.

Starting state:
- the direct DRM stack boots
- Chromium starts and allocates `DrmThread` framebuffers
- under `drm_kms_helper.fbdev_emulation=0`, the connector stays disabled in the Chromium path
- under the same no-fbdev setting, `kms_pattern` can still enable the connector

Current hypothesis:
- `content_shell` window sizing and controller matching are the most suspicious layer
- the next highest-value control is an `800x600` no-fbdev run with focused Chromium DRM logs

## Key Links

- Design guide: [design/01-content-shell-modeset-mapping-investigation-guide.md](./design/01-content-shell-modeset-mapping-investigation-guide.md)
- Diary: [reference/01-diary.md](./reference/01-diary.md)
- Imported note: [sources/local/01-ozone-answers.md](./sources/local/01-ozone-answers.md)
- Prior ticket: [../QEMU-06-CONTENT-SHELL-DRM-OZONE--chromium-content-shell-direct-drm-ozone-analysis-and-implementation/index.md](../QEMU-06-CONTENT-SHELL-DRM-OZONE--chromium-content-shell-direct-drm-ozone-analysis-and-implementation/index.md)

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
