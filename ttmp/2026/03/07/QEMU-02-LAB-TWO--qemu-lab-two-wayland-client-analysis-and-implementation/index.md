---
Title: QEMU Lab Two Wayland Client Analysis and Implementation
Ticket: QEMU-02-LAB-TWO
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
ExternalSources:
    - local:01-lab2.md
Summary: Phase-2 ticket for a Weston-based guest Wayland stack, a custom Wayland client, a QMP harness, measurements, and the report artifacts.
LastUpdated: 2026-03-07T21:00:00-05:00
WhatFor: Track the phase-2 Weston plus Wayland client implementation, supporting docs, tasks, diary, and final report.
WhenToUse: Use this ticket when working on the phase-2 guest GUI stack, host QMP harness, measurements, and report deliverables.
---

# QEMU Lab Two Wayland Client Analysis and Implementation

## Overview

This ticket covers phase 2 of the QEMU lab. The phase-2 target in the imported brief is a real guest Wayland stack built around Weston and one minimal custom Wayland client, with host-side screenshots and input injection. Chromium is intentionally deferred to the later stage.

The implementation goal is to preserve the stage-1 state machine and suspend/resume behavior while changing the guest output path from a serial/text renderer to a visible Wayland surface that can be observed and driven from the host.

## Key Links

- **Primary design doc**: [design-doc/01-phase-2-wayland-client-analysis-and-implementation-guide.md](./design-doc/01-phase-2-wayland-client-analysis-and-implementation-guide.md)
- **Postmortem and review guide**: [design-doc/02-postmortem-and-review-guide.md](./design-doc/02-postmortem-and-review-guide.md)
- **Final implementation report**: [design-doc/03-phase-2-final-implementation-report.md](./design-doc/03-phase-2-final-implementation-report.md)
- **Diary**: [reference/01-diary.md](./reference/01-diary.md)
- **Input playbook**: [reference/02-input-bring-up-playbook.md](./reference/02-input-bring-up-playbook.md)
- **Tasks**: [tasks.md](./tasks.md)
- **Imported source**: [sources/local/01-lab2.md](./sources/local/01-lab2.md)

## Status

Current status: **active**

Working status on 2026-03-07:

- ticket created,
- phase-2 brief imported,
- design guide and task plan in progress,
- implementation about to start with guest packaging/runtime validation.

## Topics

- qemu
- virtualization
- lab
- systems

## Changelog

See [changelog.md](./changelog.md) for recent changes and decisions.

## Structure

- `design-doc/` architecture and submission-oriented documents
- `reference/` chronological diary and quick-reference material
- `scripts/` ticket-local copies of helper scripts used during this phase
- `sources/` imported source material
