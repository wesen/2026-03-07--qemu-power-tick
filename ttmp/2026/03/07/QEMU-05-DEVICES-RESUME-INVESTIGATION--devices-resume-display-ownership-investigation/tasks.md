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
- [x] Refresh the intern guide and bundle after the corrected phase-3 reruns.
- [x] Run a corrected stage-3 `weston-simple-shm` `display_unbind_fbcon=1` control and compare it to the corrected baseline.
- [x] Add one guest-visible screenshot or readback experiment to compare guest-side output against QMP `screendump` after resume.
- [x] Add a lower-level guest DRM/KMS state experiment, because `/dev/fb0` does not match the compositor-visible plane even before suspend in the current stage-3 stack.
- [x] Investigate QEMU `screendump` / `virtio-vga` plane selection now that guest DRM state and guest compositor screenshots both remain healthy after resume.
- [x] Add launcher support to swap the QEMU display device so `virtio-vga` and `virtio-gpu-pci` can be compared without ad hoc command edits.
- [x] Query the active QEMU build for `screendump` capabilities and schema details, including whether explicit device/head selection is supported.
- [x] Run a stage-3 `weston-simple-shm` `pm_test=devices` control on `virtio-vga` with the new probe path and capture the post-resume screenshots.
- [x] Run the same stage-3 `weston-simple-shm` `pm_test=devices` control on `virtio-gpu-pci` with default VGA disabled and compare the post-resume screenshots.
- [x] Add a guest-side KMS dumb-buffer pattern helper that can take over scanout without Weston or Chromium.
- [x] Run a minimal post-resume KMS pattern test and compare guest KMS state to QMP `screendump`.
- [ ] Assign a stable QEMU display-device id and test explicit `screendump --device/--head` targeting after resume, because this QEMU build advertises those args and the default `virtio-gpu-pci` post-resume capture is black.
- [ ] Decide whether stage 3 should switch from `virtio-vga` to `virtio-gpu-pci`, or whether the host-capture divergence should be documented as a QEMU limitation instead.
- [x] Update the QEMU-05 guide, diary, and changelog with the device-variant, QMP-schema, and KMS-pattern results.
