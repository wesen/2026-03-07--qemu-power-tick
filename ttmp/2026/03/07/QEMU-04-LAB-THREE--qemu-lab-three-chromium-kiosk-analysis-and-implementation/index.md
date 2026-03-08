---
Title: QEMU Lab Three Chromium Kiosk Analysis and Implementation
Ticket: QEMU-04-LAB-THREE
Status: active
Topics:
    - qemu
    - virtualization
    - lab
    - systems
DocType: index
Intent: long-term
Owners: []
RelatedFiles: []
ExternalSources: []
Summary: Stage-3 ticket for Chromium on Weston kiosk mode, including packaging, host-driven validation, suspend/resume continuity, and the wake-study track.
LastUpdated: 2026-03-07T22:20:00-05:00
WhatFor: Track the Chromium kiosk stage that builds on the completed phase-2 Weston and Wayland-client stack.
WhenToUse: Use this when implementing, validating, or documenting stage 3 of the lab.
---

# QEMU Lab Three Chromium Kiosk Analysis and Implementation

## Overview

This ticket covers stage 3 of the lab: replacing the custom phase-2 Wayland client with Chromium running on Wayland under Weston `kiosk-shell`, while preserving host-side screenshot/input automation and the suspend/resume model.

The first active engineering risk is packaging. On this host, `chromium-browser` is only a shell launcher for the installed snap, so the initial stage-3 path is to package the snap payload itself into the guest rootfs rather than depend on a non-existent normal distro browser package.

## Key Links

- **Primary guide**: [design/01-stage-3-chromium-kiosk-guide.md](./design/01-stage-3-chromium-kiosk-guide.md)
- **Diary**: [reference/01-diary.md](./reference/01-diary.md)
- **Tasks**: [tasks.md](./tasks.md)
- **Imported source**: [sources/local/01-lab2.md](./sources/local/01-lab2.md)
- **Ticket-local scripts**: [scripts/](./scripts/)

## Status

Current status: **active**

Working status on 2026-03-07:

- ticket created,
- stage-3 source imported,
- Chromium packaging investigation completed for the first working path,
- Chromium booted visibly under Weston,
- host keyboard and pointer injection validated against a deterministic browser page,
- reproducible checkpoint helpers added to the repo and mirrored into the ticket.

## Topics

- qemu
- virtualization
- lab
- systems

## Tasks

See [tasks.md](./tasks.md) for the current task list.

## Changelog

See [changelog.md](./changelog.md) for recent changes and decisions.

## Structure

- design/ - Architecture and design documents
- reference/ - Diary and supporting notes
- playbooks/ - Command sequences and test procedures
- scripts/ - Ticket-local copies of helper scripts used during stage 3
- various/ - Working notes and research
- archive/ - Deprecated or reference-only artifacts
