# Tasks

## TODO

- [x] Import `ozone-answers.md` into the new ticket and review it against the current phase-4 evidence.
- [x] Write a focused design guide that explains the controller-mapping hypothesis and the next experiment ladder.
- [x] Start the new diary with the rationale for splitting this ticket out of QEMU-06.
- [x] Mirror the current `init-phase4-drm` and `content-shell-drm-launcher.sh` into the ticket `scripts/` folder.
- [x] Run the first `content_shell` no-fbdev control at `800x600` instead of `1280x800`.
- [x] Capture:
  - serial log
  - DRM debugfs snapshots
  - shallow display probe output
  - QMP screenshot
- [x] Compare the first `800x600` run to:
  - `results-phase4-drm24`
  - `results-phase4-kms2`
- [x] Tighten Chromium DRM/Ozone logging for the first `800x600` run.
- [x] Determine whether the generic `--window-size` flag actually controls `content_shell`.
- [x] Re-run the `800x600` no-fbdev control with the real `--content-shell-host-window-size=WxH` switch.
- [x] Determine which branch we are on after the corrected size-control run:
  - content area resizes, but controller still never binds
  - the remaining suspect is shell chrome / toolbar bounds rather than content size alone
- [ ] Add an explicit `content-shell-hide-toolbar` control to the launcher/init path.
- [ ] Rebuild the phase-4 initramfs with the new toolbar control.
- [ ] Run the next no-fbdev control with:
  - `phase4_content_shell_window_size=800,600`
  - `phase4_content_shell_fullscreen=0`
  - `phase4_content_shell_hide_toolbar=1`
- [ ] Compare `drm28` to:
  - `results-phase4-drm27`
  - `results-phase4-kms2`
- [ ] Determine whether removing shell chrome changes:
  - `DrmThread` scanout buffer size
  - connector/CRTC activation
  - host-visible QMP output
- [ ] Only if still ambiguous, add one guest-trusted visible-frame proof for the no-fbdev configuration.
