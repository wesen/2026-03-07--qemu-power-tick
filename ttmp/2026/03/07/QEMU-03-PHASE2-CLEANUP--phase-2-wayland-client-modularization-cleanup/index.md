---
Title: Phase 2 Wayland Client Modularization Cleanup
Ticket: QEMU-03-PHASE2-CLEANUP
Status: active
Topics:
    - qemu
    - virtualization
    - lab
    - systems
    - refactor
DocType: index
Intent: long-term
Owners: []
RelatedFiles: []
ExternalSources: []
Summary: Refactor the phase-2 Wayland client and related guest code into smaller modules so later suspend, metrics, and Chromium-adjacent work can move faster without changing current behavior.
LastUpdated: 2026-03-07T17:55:51.522961216-05:00
WhatFor: Track the dedicated cleanup pass that splits the current phase-2 monolith into reusable modules while preserving runtime behavior.
WhenToUse: Use this ticket when working on code structure, extraction, shared helpers, and cleanup work that should stay separate from the phase-2 feature-delivery ticket.
---

# Phase 2 Wayland Client Modularization Cleanup

## Overview

This ticket isolates the structural cleanup needed after the first working phase-2 implementation. The immediate trigger is that [`guest/wl_sleepdemo.c`](/home/manuel/code/wesen/2026-03-07--qemu-power-tick/guest/wl_sleepdemo.c) has reached a size where further feature work will be slower and riskier than it needs to be.

The goal here is not to add new lab features. The goal is to split the current guest code into cleaner modules, preserve behavior, and leave obvious seams for later suspend, timing, and stage-3 work.

## Key Links

- **Design guide**: [design/01-phase-2-modularization-guide.md](./design/01-phase-2-modularization-guide.md)
- **Diary**: [reference/01-diary.md](./reference/01-diary.md)
- **Tasks**: [tasks.md](./tasks.md)

## Status

Current status: **active**

Working status on 2026-03-07:

- cleanup ticket created,
- modularization plan written,
- phase-2 client refactor completed and committed,
- post-refactor validation completed for boot, reconnect, pointer, and keyboard behavior.

## Topics

- qemu
- virtualization
- lab
- systems
- refactor

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
