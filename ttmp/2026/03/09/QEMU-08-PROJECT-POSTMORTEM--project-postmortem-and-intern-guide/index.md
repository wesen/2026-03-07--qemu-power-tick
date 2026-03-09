---
Title: Project Postmortem and Intern Guide
Ticket: QEMU-08-PROJECT-POSTMORTEM
Status: active
Topics:
    - qemu
    - virtualization
    - lab
    - systems
    - chromium
    - wayland
    - drm
DocType: index
Intent: long-term
Owners: []
RelatedFiles: []
ExternalSources: []
Summary: ""
LastUpdated: 2026-03-10T00:01:00-04:00
WhatFor: ""
WhenToUse: ""
---

# Project Postmortem and Intern Guide

## Overview

This ticket exists to hold the project-level retrospective and intern guide for the repository. Unlike the narrower phase tickets, it summarizes the full arc of the work:

- what was built,
- which approaches were most successful,
- which investigations mattered most,
- what remains unfinished,
- and how a new contributor should approach the codebase.

## Key Links

- **Canonical root-level report**: [PROJECT-POSTMORTEM-AND-INTERN-GUIDE.md](../../../../../../../PROJECT-POSTMORTEM-AND-INTERN-GUIDE.md)
- **Ticket companion**: [design/01-project-postmortem-and-intern-guide.md](./design/01-project-postmortem-and-intern-guide.md)
- **Diary**: [reference/01-diary.md](./reference/01-diary.md)
- **Stage-2 final report**: [../QEMU-02-LAB-TWO--qemu-lab-two-wayland-client-analysis-and-implementation/design-doc/03-phase-2-final-implementation-report.md](../QEMU-02-LAB-TWO--qemu-lab-two-wayland-client-analysis-and-implementation/design-doc/03-phase-2-final-implementation-report.md)
- **Stage-3 review**: [../QEMU-04-LAB-THREE--qemu-lab-three-chromium-kiosk-analysis-and-implementation/design/02-stage-3-postmortem-and-review-guide.md](../QEMU-04-LAB-THREE--qemu-lab-three-chromium-kiosk-analysis-and-implementation/design/02-stage-3-postmortem-and-review-guide.md)
- **Direct-DRM investigation**: [../QEMU-07-CONTENT-SHELL-MODESET--content-shell-drm-modeset-and-controller-mapping-investigation/design/01-content-shell-modeset-mapping-investigation-guide.md](../QEMU-07-CONTENT-SHELL-MODESET--content-shell-drm-modeset-and-controller-mapping-investigation/design/01-content-shell-modeset-mapping-investigation-guide.md)

## Status

Current status: **active**

Current working state:

- root-level report expanded into a much more detailed blog-style onboarding guide,
- ticket companion updated to point at the canonical root version and preserve the supporting reading map,
- diary started,
- README link updated,
- ticket validated,
- reMarkable bundle uploaded and refreshed after the rewrite.

## Topics

- qemu
- virtualization
- lab
- systems
- chromium
- wayland
- drm

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
