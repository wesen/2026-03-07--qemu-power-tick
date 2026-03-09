---
Title: Diary
Ticket: QEMU-07-CONTENT-SHELL-MODESET
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
LastUpdated: 2026-03-09T18:16:01.080065982-04:00
WhatFor: ""
WhenToUse: ""
---

# Diary

## Goal

This diary records the focused follow-up investigation into why `content_shell` on Ozone DRM allocates buffers but still fails to activate a visible display controller in QEMU.

## Step 1: Split The Modeset-Mapping Investigation Into Its Own Ticket

The previous ticket had already done the hard bring-up work. Chromium built, headless worked, direct DRM startup reached real GPU initialization, and the no-fbdev controls showed that the guest could still modeset through `kms_pattern`. That meant the remaining failure no longer belonged in the general phase-4 ticket. It belonged in a narrower ticket about Chromium's own display-controller logic.

This new ticket starts from a specific hypothesis derived from the imported `ozone-answers.md` note: `content_shell` may be creating buffers without ever getting mapped to an enabled display controller because its window bounds do not match the active mode rectangle the way Ozone DRM expects. The reason to write that down first is simple: if the hypothesis is wrong, the next experiments should disprove it quickly instead of wandering back into generic QEMU/DRM debugging.

### Prompt Context

**User prompt (verbatim):** "alright, create a new ticket to do that, add a detailed guide on what you are going to do and why, then create individual tasks and work on them one by one, comitting at appropriate intervals, keeping a diary"

**Assistant interpretation:** Create a fresh, narrowly scoped ticket for the `content_shell` controller-mapping hypothesis, document the plan clearly, then execute the new experiments with normal commit and diary discipline.

**Inferred user intent:** Keep the next debugging slice small and reviewable, rather than continuing to pile mode-setting experiments into the broader phase-4 ticket.

### What I did
- Created the ticket `QEMU-07-CONTENT-SHELL-MODESET`.
- Imported `/tmp/ozone-answers.md` into the new ticket.
- Wrote the focused design guide in:
  - `design/01-content-shell-modeset-mapping-investigation-guide.md`
- Replaced the placeholder task list with a narrow experiment ladder.
- Started this diary.

### Why
- The remaining bug is now much narrower than "Chromium on DRM doesn't work."
- The previous ticket already established a good baseline.
- A fresh ticket makes it easier to reason about the next controls and write a clean report later.

### What worked
- The imported note matched the existing evidence well.
- The new ticket now starts with a real hypothesis and a real decision tree instead of an empty scaffold.

### What didn't work
- N/A yet. This step was setup and reframing, not a runtime control.

### What I learned
- The cleanest next experiment is not another generic fullscreen run.
- The strongest next probe is a no-fbdev `800x600` run aimed directly at the controller-mapping theory.

### What was tricky to build
- The main challenge here was scope discipline. It would have been easy to keep using the phase-4 ticket as a catch-all, but that would have made the next conclusion harder to trust and harder to explain.

### What warrants a second pair of eyes
- The exact framing of the hypothesis should be checked against the Chromium DRM code once the first `800x600` result lands.

### What should be done in the future
- Run the first `800x600` no-fbdev control.
- Compare it directly to the previous `drm24` and `kms2` controls.
- Tighten Chromium logging only if the `800x600` run still leaves the connector disabled.

### Code review instructions
- Start with:
  - `design/01-content-shell-modeset-mapping-investigation-guide.md`
  - `tasks.md`
  - `sources/local/01-ozone-answers.md`

### Technical details
- Ticket path:
```text
ttmp/2026/03/09/QEMU-07-CONTENT-SHELL-MODESET--content-shell-drm-modeset-and-controller-mapping-investigation
```
