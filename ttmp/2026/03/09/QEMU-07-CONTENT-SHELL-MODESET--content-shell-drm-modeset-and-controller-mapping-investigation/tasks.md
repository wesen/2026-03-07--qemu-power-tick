# Tasks

## TODO

- [x] Import `ozone-answers.md` into the new ticket and review it against the current phase-4 evidence.
- [x] Write a focused design guide that explains the controller-mapping hypothesis and the next experiment ladder.
- [x] Start the new diary with the rationale for splitting this ticket out of QEMU-06.
- [ ] Mirror any new launcher/init helpers into the ticket `scripts/` folder as they change.
- [ ] Run the first `content_shell` no-fbdev control at `800x600` instead of `1280x800`.
- [ ] Capture:
  - serial log
  - DRM debugfs snapshots
  - shallow display probe output
  - QMP screenshot
- [ ] Compare the `800x600` run to:
  - `results-phase4-drm24`
  - `results-phase4-kms2`
- [ ] Tighten Chromium DRM/Ozone logging to the exact display-controller files if the `800x600` run still leaves the connector disabled.
- [ ] Determine which branch we are on:
  - no controller mapping
  - controller mapping but failed modeset
- [ ] Only if still ambiguous, add one guest-trusted visible-frame proof for the no-fbdev configuration.
