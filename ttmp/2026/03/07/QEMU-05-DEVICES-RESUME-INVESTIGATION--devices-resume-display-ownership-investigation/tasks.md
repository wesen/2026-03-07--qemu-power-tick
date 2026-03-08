# Tasks

## TODO

- [x] Split the `pm_test=devices` resume work into a dedicated investigation ticket with its own docs and script archive.
- [x] Write an intern-facing analysis guide that explains the full stage-1/stage-2/stage-3 system and the narrowed resume hypotheses.
- [x] Add a guest-side display ownership probe that logs `vtconsole`, `fb0`, and DRM connector state without writing to `/dev/console` during runtime.
- [x] Mirror the new investigation scripts into the ticket-local `scripts/` directory.
- [x] Rebuild the phase-2 and phase-3 initramfs images with the display probe available.
- [x] Run a phase-2 `pm_test=devices` control with `display_probe=1`.
- [x] Run a phase-3 `weston-simple-shm` `pm_test=devices` control with `display_probe=1`.
- [x] Run a phase-3 Chromium `pm_test=devices` validation with `display_probe=1`.
- [x] Run one `display_unbind_fbcon=1` experiment against the shared-control path.
- [x] Compare the probe outputs against the QMP screenshot outcomes and decide whether the next layer to debug is fbcon ownership, Weston scanout restoration, or Chromium buffer reuse.
- [x] Update the diary and changelog with the control results and probe evidence.
- [x] Run a stage-3 `display_unbind_fbcon=1` experiment with concurrent screenshot capture, not just a log-only run.
- [x] Validate the new ticket with `docmgr doctor`.
- [x] Upload the investigation bundle to reMarkable.
- [ ] Refresh the intern guide and bundle after the corrected phase-3 reruns.
