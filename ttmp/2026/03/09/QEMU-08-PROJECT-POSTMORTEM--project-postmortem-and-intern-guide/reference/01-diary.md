---
Title: Diary
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
DocType: reference
Intent: long-term
Owners: []
RelatedFiles: []
ExternalSources: []
Summary: Chronological notes for producing the project-level postmortem and intern guide.
LastUpdated: 2026-03-09T23:16:00-04:00
WhatFor: Record how the project-level summary document was produced and what source tickets it drew from.
WhenToUse: Read this when reviewing the provenance of the final root-level report.
---

# Diary

## Step 1: Create A Dedicated Summary Ticket

The repository already had many narrow tickets, but none of them was a clean project-wide retrospective. I created `QEMU-08-PROJECT-POSTMORTEM` to hold the final root-level summary document, the minimal bookkeeping around it, and the upload metadata. That keeps the project-level report separate from the narrower runtime investigations.

## Step 2: Write The Root-Level End Report

The main deliverable for this ticket is not just a ticket-local design doc. It is the root-level file:

- [PROJECT-POSTMORTEM-AND-INTERN-GUIDE.md](../../../../../../../PROJECT-POSTMORTEM-AND-INTERN-GUIDE.md)

This was a deliberate choice because the user wanted the end result stored in the root of the repo rather than hidden only inside `ttmp/`.

The report was written by synthesizing evidence from:

- stage-1 implementation and report material in QEMU-01,
- stage-2 implementation and final report in QEMU-02,
- stage-3 Chromium kiosk work and review material in QEMU-04,
- QMP/display-resume investigation in QEMU-05,
- direct DRM/Ozone analysis in QEMU-06,
- controller-discovery investigation in QEMU-07.

## Step 3: Mirror The Report Into The Ticket

The same conceptual report was mirrored into:

- [design/01-project-postmortem-and-intern-guide.md](../design/01-project-postmortem-and-intern-guide.md)

This keeps the document reachable from both the repository root and the structured ticket workspace.

## Step 4: Preserve Cross-Repository State

One subtle but important thing to preserve was the fact that the latest direct-DRM investigation changed the external Chromium checkout, not just files inside this repository.

That state was stabilized before the final summary:

- Chromium checkout: `/home/manuel/chromium/src`
- Branch: `qemu-07-content-shell-controller-debug`
- Commit: `9f6c936991`

That means the final report can honestly point to a saved external-source state rather than only mirrored snapshots.

## Step 5: Make The Root Report The Main Entry Point And Ship It

After the report itself existed, I updated the top-level repository landing page so new readers would not stop at the original stage-1-only README framing.

### What I did
- Updated [README.md](../../../../../../../README.md) to link the root-level postmortem and intern guide.
- Related the main repo-level files into the ticket design doc.
- Ran `docmgr doctor --ticket QEMU-08-PROJECT-POSTMORTEM --stale-after 30`.
- Uploaded a reMarkable bundle containing:
  - `PROJECT-POSTMORTEM-AND-INTERN-GUIDE.md`
  - `README.md`

### Upload details
- Remote folder: `/ai/2026/03/09/QEMU-08-PROJECT-POSTMORTEM`
- Remote file: `QEMU Project Postmortem Bundle`

### Why
- The user explicitly wanted the end result stored in the repo root.
- Making the root report the README entry point keeps the repository from pretending it is still only a stage-1 lab.
- Uploading the bundle makes the final summary portable and reviewable outside the code checkout.
