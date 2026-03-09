---
Title: Project Postmortem And Intern Guide
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
DocType: design
Intent: long-term
Owners: []
RelatedFiles:
    - Path: PROJECT-POSTMORTEM-AND-INTERN-GUIDE.md
      Note: Canonical root-level version of the long-form project postmortem and intern guide
    - Path: README.md
      Note: Repository entry point linking new contributors to the canonical guide
    - Path: guest/build-phase2-rootfs.sh
      Note: Representative Weston-path packaging script discussed in the guide
    - Path: guest/init-phase4-drm
      Note: Representative direct-DRM bootstrap script discussed in the guide
ExternalSources: []
Summary: Ticket-local companion for the canonical root-level project postmortem, with a reading map and stable links into the supporting tickets, reports, diaries, and implementation files.
LastUpdated: 2026-03-09T23:59:00-04:00
WhatFor: Give ticket readers a structured path into the canonical root-level guide and the most important supporting material without duplicating and drifting a second full copy.
WhenToUse: Read this when onboarding from the ticket workspace or when reviewing the provenance and supporting evidence behind the root-level project postmortem.
---

# QEMU Sleep/Wake Lab
## Project Postmortem And Intern Guide Companion

## Canonical Version

The canonical long-form version of this document now lives at:

- [PROJECT-POSTMORTEM-AND-INTERN-GUIDE.md](../../../../../../../PROJECT-POSTMORTEM-AND-INTERN-GUIDE.md)

That root-level file is intentionally the primary reading experience. It was expanded into a much more detailed, blog-style narrative with architecture walkthroughs, ticket cross-links, pseudocode, system diagrams, and a clearer explanation of what was built and what remains open.

This ticket-local document is a companion, not a competing copy. Its purpose is to:

- preserve the structured ticket metadata,
- point readers to the canonical root document,
- provide a stable table of supporting evidence,
- explain how to navigate from the project-level narrative back into the phase-specific tickets and source files.

Keeping the long-form narrative canonical in the repository root avoids a common failure mode in `docmgr` tickets: a mirrored copy slowly drifts out of sync and eventually becomes less trustworthy than either the root version or the ticket diaries.

## What The Canonical Guide Covers

The root guide is intended to be read like an in-depth engineering blog post for a new intern. It covers:

- the overall system shape,
- the host/guest split,
- the boot model,
- why initramfs guests were used instead of disk images,
- the evolution from stage 1 to stage 4,
- the strongest working implementation path,
- the hardest unresolved problems,
- what was learned from the failed branches,
- and how to resume the work without starting from zero.

If you want the single best “read this first” artifact for the whole repository, read the root guide rather than this ticket companion.

## Reading Map

### First-pass reading order

1. [PROJECT-POSTMORTEM-AND-INTERN-GUIDE.md](../../../../../../../PROJECT-POSTMORTEM-AND-INTERN-GUIDE.md)
2. [README.md](../../../../../../../README.md)
3. [QEMU-01 final report](../../../../03/07/QEMU-01-LAB-ONE--qemu-lab-one-analysis-and-implementation-guide/design-doc/02-final-implementation-report.md)
4. [QEMU-02 phase-2 final report](../../../../03/07/QEMU-02-LAB-TWO--qemu-lab-two-wayland-client-analysis-and-implementation/design-doc/03-phase-2-final-implementation-report.md)
5. [QEMU-04 stage-3 postmortem and review](../../../../03/07/QEMU-04-LAB-THREE--qemu-lab-three-chromium-kiosk-analysis-and-implementation/design/02-stage-3-postmortem-and-review-guide.md)
6. [QEMU-05 QMP capture postmortem](../../../../03/07/QEMU-05-DEVICES-RESUME-INVESTIGATION--devices-resume-display-ownership-investigation/design-doc/02-qmp-capture-path-postmortem-report.md)
7. [QEMU-07 controller-mapping investigation guide](../../../../03/09/QEMU-07-CONTENT-SHELL-MODESET--content-shell-drm-modeset-and-controller-mapping-investigation/design/01-content-shell-modeset-mapping-investigation-guide.md)

### Diaries worth reading after the reports

- [QEMU-02 diary](../../../../03/07/QEMU-02-LAB-TWO--qemu-lab-two-wayland-client-analysis-and-implementation/reference/01-diary.md)
- [QEMU-04 diary](../../../../03/07/QEMU-04-LAB-THREE--qemu-lab-three-chromium-kiosk-analysis-and-implementation/reference/01-diary.md)
- [QEMU-05 investigation diary](../../../../03/07/QEMU-05-DEVICES-RESUME-INVESTIGATION--devices-resume-display-ownership-investigation/reference/01-investigation-diary.md)
- [QEMU-07 diary](../../../../03/09/QEMU-07-CONTENT-SHELL-MODESET--content-shell-drm-modeset-and-controller-mapping-investigation/reference/01-diary.md)

These diaries are especially important because the project only makes full sense if you see the dead ends, the corrected assumptions, and the experiments that narrowed vague failures into specific ones.

## Why The Root Guide Was Expanded

The original project-level summary did the minimum acceptable job:

- it identified the major phases,
- it linked the right tickets,
- it described the broad current state.

What it did not do well enough was make the repository pleasant to understand. It read more like a technical summary than an onboarding narrative.

The rewritten root guide now does three things better:

- it tells the story in sequence instead of presenting only a static summary,
- it explains why certain design choices were made and what tradeoffs they created,
- it ties the “what happened” narrative back to concrete implementation files and investigation tickets.

That is why the root version should now be treated as the canonical long-form artifact.

## Repository Entry Points

If you are browsing from the repository root, the main entry points are:

- [README.md](../../../../../../../README.md)
- [PROJECT-POSTMORTEM-AND-INTERN-GUIDE.md](../../../../../../../PROJECT-POSTMORTEM-AND-INTERN-GUIDE.md)

If you are browsing from the implementation side first, the most important source files to inspect are:

- [guest/sleepdemo.c](../../../../../../../guest/sleepdemo.c)
- [guest/init](../../../../../../../guest/init)
- [guest/init-phase2](../../../../../../../guest/init-phase2)
- [guest/build-phase2-rootfs.sh](../../../../../../../guest/build-phase2-rootfs.sh)
- [guest/wl_suspend.c](../../../../../../../guest/wl_suspend.c)
- [guest/init-phase3](../../../../../../../guest/init-phase3)
- [guest/init-phase4-drm](../../../../../../../guest/init-phase4-drm)
- [guest/content-shell-drm-launcher.sh](../../../../../../../guest/content-shell-drm-launcher.sh)
- [host/qmp_harness.py](../../../../../../../host/qmp_harness.py)
- [host/capture_phase4_smoke.py](../../../../../../../host/capture_phase4_smoke.py)

If you are trying to understand the direct-DRM investigation state specifically, add:

- [guest/kms_pattern.c](../../../../../../../guest/kms_pattern.c)
- [host/extract_drmstate_from_serial.py](../../../../../../../host/extract_drmstate_from_serial.py)
- [QEMU-05 analysis guide](../../../../03/07/QEMU-05-DEVICES-RESUME-INVESTIGATION--devices-resume-display-ownership-investigation/design-doc/01-devices-resume-analysis-guide.md)
- [QEMU-06 analysis and implementation guide](../../../../03/08/QEMU-06-CONTENT-SHELL-DRM-OZONE--chromium-content-shell-direct-drm-ozone-analysis-and-implementation/design-doc/01-content-shell-drm-ozone-analysis-and-implementation-guide.md)
- [QEMU-07 modeset investigation guide](../../../../03/09/QEMU-07-CONTENT-SHELL-MODESET--content-shell-drm-modeset-and-controller-mapping-investigation/design/01-content-shell-modeset-mapping-investigation-guide.md)

## Summary Of The Project-Level Conclusions

The conclusions preserved in the canonical guide are:

- Stage 1 is complete enough to trust and remains the cleanest end-to-end suspend/resume subsystem in the repository.
- Stage 2 is the strongest graphics implementation and the best practical baseline for future work.
- Stage 3 achieved real Chromium-on-Weston bring-up and usable host automation, but device-resume continuity remained incomplete.
- QEMU-05 showed that some of the “graphics resume is broken” story was actually a host-visible capture-path problem rather than a pure guest failure.
- Stage 4 and QEMU-07 narrowed the direct-DRM browser problem substantially, but that branch is still an investigation rather than a finished product.

For a new contributor, that means:

- do not start by “fixing everything,”
- do not assume the newest branch is the best branch,
- and do not ignore the diaries, because many strong conclusions came from correcting earlier mistaken explanations.

## Provenance

The root guide was produced by synthesizing:

- phase-level final reports,
- phase-level postmortems,
- implementation diaries,
- source files in `guest/`, `host/`, and `scripts/`,
- and the saved external Chromium branch state referenced from QEMU-07.

That provenance is recorded chronologically in:

- [reference/01-diary.md](../reference/01-diary.md)

## Maintenance Note

If the root guide is revised again, update this ticket companion only enough to:

- keep the pointer to the canonical root file accurate,
- preserve the reading map,
- and note the revision in the diary and changelog.

Do not create a second fully independent long-form copy here unless there is a strong reason to fork the narrative.
