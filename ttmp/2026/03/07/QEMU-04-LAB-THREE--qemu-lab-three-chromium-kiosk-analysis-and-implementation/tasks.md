# Tasks

## Stage 3 Plan

- [x] Create the stage-3 ticket, seed the guide/tasks/diary, and copy the imported source into the new ticket.
- [x] Investigate the local Chromium packaging situation and choose the first browser payload path.
- [x] Build a phase-3 guest rootfs/initramfs that packages Chromium on top of the working phase-2 Weston stack.
- [x] Add a phase-3 init / launcher path that starts Chromium on Wayland under Weston `kiosk-shell`.
- [x] Boot Chromium visibly under Weston and capture the first screenshot.
- [x] Validate host keyboard injection into Chromium.
- [x] Validate host pointer injection into Chromium.
- [x] Add a reproducible stage-3 checkpoint harness.
- [x] Reintroduce suspend/resume plumbing for stage 3.
- [x] Verify Chromium continuity after `pm_test=freezer` resume.
- [ ] Verify Chromium continuity after `pm_test=devices` resume.
- [ ] Validate startup-URL mode and typed-URL mode.
- [ ] Record Chromium-specific wake/idle observations and likely wake sources for the later study track.
- [ ] Keep the diary updated after each meaningful step, including exact commands, failures, and lessons learned.
- [ ] Commit implementation changes at coherent milestones and record commit hashes in the diary and changelog.
- [ ] Write the stage-3 final report and upload the bundle to reMarkable.

## Bonus Path

- [ ] Thin the bootstrap further so browser launch policy lives in a dedicated wrapper rather than being spread across `/init`.
- [ ] Factor shared host harness/QMP code so phase 2 and stage 3 do not diverge unnecessarily.
