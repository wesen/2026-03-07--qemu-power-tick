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

## Step 6: Rewrite The Report Into A Real Long-Form Onboarding Narrative

The first version of the project-level report was structurally correct but still read too much like a technical summary. The user then asked for something much more detailed and pleasant to read: a document that felt like an in-depth engineering blog post for a new intern, not just a checklist of outcomes.

That rewrite happened in the root-level canonical file:

- [PROJECT-POSTMORTEM-AND-INTERN-GUIDE.md](../../../../../../../PROJECT-POSTMORTEM-AND-INTERN-GUIDE.md)

The rewrite expanded several things substantially:

- the framing section explaining why the repository is confusing at first glance,
- the reading order and document map,
- the architectural explanation of the three system shapes,
- the explanation of the initramfs shipping model,
- the story of each stage, including what was learned from the failures,
- and the practical “what should a new contributor do next?” guidance.

## Step 7: Keep The Ticket In Sync Without Creating A Bad Mirror

After the rewrite, it was no longer a good idea to maintain a second full copy inside the ticket. The root-level version now contains many repository-relative links that are correct in the root file and would be fragile to duplicate again inside the ticket.

So the ticket design doc was deliberately changed into a companion document:

- [design/01-project-postmortem-and-intern-guide.md](../design/01-project-postmortem-and-intern-guide.md)

That companion now:

- identifies the root guide as canonical,
- preserves the supporting reading map,
- records the main conclusions,
- and explains why the ticket should point to the root version instead of drifting into a stale copy.

This keeps the project readable without creating two competing “final” reports.

## Step 8: Refresh Validation And The External Bundle

Once the rewritten root guide and the ticket companion were aligned, the ticket needed to be refreshed rather than left in a half-updated state.

That meant:

- updating `index.md`, `tasks.md`, and `changelog.md`,
- rerunning `docmgr doctor`,
- re-uploading the root guide bundle to reMarkable so the portable copy matched the repository.

This step matters because a project-level summary that exists in three slightly different versions is worse than having only one. The rewrite only counts as finished once the root file, the ticket metadata, and the uploaded bundle all describe the same thing.

### Refreshed upload details
- Remote folder: `/ai/2026/03/09/QEMU-08-PROJECT-POSTMORTEM`
- Remote file: `QEMU Project Postmortem Bundle Update`
