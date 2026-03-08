---
Title: Devices Resume Display Ownership Investigation
Ticket: QEMU-05-DEVICES-RESUME-INVESTIGATION
Status: active
Topics:
    - qemu
    - linux
    - power-management
    - wayland
    - chromium
DocType: index
Intent: long-term
Owners: []
RelatedFiles:
    - Path: ttmp/2026/03/07/QEMU-05-DEVICES-RESUME-INVESTIGATION--devices-resume-display-ownership-investigation/scripts/display_probe.sh
      Note: Ticket-local copy of the guest display ownership probe
    - Path: ttmp/2026/03/07/QEMU-05-DEVICES-RESUME-INVESTIGATION--devices-resume-display-ownership-investigation/scripts/init-phase2
      Note: Ticket-local copy of phase-2 init used in the experiments
    - Path: ttmp/2026/03/07/QEMU-05-DEVICES-RESUME-INVESTIGATION--devices-resume-display-ownership-investigation/scripts/init-phase3
      Note: Ticket-local copy of phase-3 init used in the experiments
ExternalSources:
    - local:01-chat-gpt-research-kms.md
Summary: ""
LastUpdated: 2026-03-07T23:46:37.961789847-05:00
WhatFor: ""
WhenToUse: ""
---



# Devices Resume Display Ownership Investigation

## Overview

This ticket continues the `pm_test=devices` resume investigation after the earlier stage-2 and stage-3 tickets became too broad. The narrowed problem started as a shared post-resume visual fallback in QMP screenshots, with Chromium sometimes adding extra `virtio_gpu_dequeue_ctrl_func ... response 0x1203` DRM noise on top.

The purpose of this ticket is to keep the next investigation steps small and evidence-driven. Instead of broad rewrites, it focuses on display ownership and scanout restoration around resume:
- which layer owns the visible framebuffer after `pm_test=devices`,
- whether fbcon is reasserting itself after resume,
- whether the QMP screenshot path is observing a fallback plane,
- and what extra resource failure Chromium introduces beyond the shared base issue.

Recent correction:
- `weston-screenshooter` is not blocked by `kiosk-shell.so` in this setup.
- The decisive gate is Weston compositor debug/capture policy: with the same kiosk shell, adding `--debug` makes `weston-screenshooter` work.

Current strongest conclusion:
- guest DRM debugfs state remains healthy after resume,
- guest compositor screenshots also remain healthy after resume when explicitly captured,
- the host-side QMP `screendump` path is therefore the leading suspect for the `720x400` fallback view,
- and the newer device-variant controls now show that removing legacy VGA changes the failure shape but does not make the default post-resume capture correct.

## Key Links

- Analysis guide: [design-doc/01-devices-resume-analysis-guide.md](./design-doc/01-devices-resume-analysis-guide.md)
- Postmortem report: [design-doc/02-qmp-capture-path-postmortem-report.md](./design-doc/02-qmp-capture-path-postmortem-report.md)
- Diary: [reference/01-investigation-diary.md](./reference/01-investigation-diary.md)
- Prior stage-3 ticket: [../QEMU-04-LAB-THREE--qemu-lab-three-chromium-kiosk-analysis-and-implementation/index.md](../QEMU-04-LAB-THREE--qemu-lab-three-chromium-kiosk-analysis-and-implementation/index.md)
- Prior stage-2 ticket: [../QEMU-02-LAB-TWO--qemu-lab-two-wayland-client-analysis-and-implementation/index.md](../QEMU-02-LAB-TWO--qemu-lab-two-wayland-client-analysis-and-implementation/index.md)

## Status

Current status: **active**

## Topics

- qemu
- linux
- power-management
- wayland
- chromium

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
