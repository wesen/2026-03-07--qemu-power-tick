# Changelog

## 2026-03-07

- Initial workspace created
- Added guest-side `display_probe.sh` instrumentation and wired it into phase-2 and phase-3 initramfs builds.
- Ran a phase-2 probe-backed `pm_test=devices` control and confirmed that `fb0`, `vtconsole`, and DRM connector state stayed stable while QMP screenshots still fell back to a firmware-looking text plane.
- Ran phase-3 `weston-simple-shm` and Chromium probe validations and confirmed the same visual fallback pattern, with `weston-simple-shm` also surfacing `virtio_gpu_dequeue_ctrl_func ... response 0x1203`.
- Corrected the `display_unbind_fbcon=1` experiment to target the actual framebuffer `vtconsole`, proving that fbcon ownership changes the visible pre-suspend plane but does not fix the post-resume fallback.
- Wrote the intern-facing analysis guide and investigation diary for follow-on debugging.
- Validated the ticket with `docmgr doctor` and uploaded `QEMU-05 Devices Resume Investigation Bundle` to reMarkable under `/ai/2026/03/07/QEMU-05-DEVICES-RESUME-INVESTIGATION`.
- Fixed a phase-3 `init-phase3` subshell bug that had been preventing `display_probe=1` and `display_unbind_fbcon=1` from taking effect.
- Added a concurrent stage-3 capture wrapper and reran corrected phase-3 `weston-simple-shm`, Chromium baseline, and Chromium unbind controls.
- Confirmed that corrected phase-3 runs now emit `@@DISPLAY`, no longer reproduce the old `0x1203` DRM errors, and still show the same `1280x800 -> 720x400` post-resume fallback.
- Backfilled the ticket-local `scripts/` archive with the new temporary helpers and uploaded `QEMU-05 Devices Resume Investigation Bundle Update` to reMarkable.
- Ran corrected stage-3 `weston-simple-shm` with `display_unbind_fbcon=1` and showed that stage-3 pre-suspend sensitivity to fbcon unbinding is client-dependent, while the post-resume fallback remains shared.
- Added a guest-side `/dev/fb0` readback path plus host extractor and proved that `fb0` stays a mostly black `1280x800` buffer that does not match the host-visible QMP pre-suspend frame and remains byte-identical across the `pm_test=devices` suspend/resume cycle.
- Validated that the earlier `weston-screenshooter` `unauthorized` failure was not caused by `kiosk-shell.so`: with the same kiosk shell, enabling Weston `--debug` exposed the screenshot interface and made guest screenshots succeed.
- Added a guest-side DRM debugfs state dump and showed that after `pm_test=devices` resume the active plane still points at a Weston-owned `1280x800` XR24 framebuffer, with only a weston buffer flip between pre and post captures.
- Added a suspend/resume guest screenshot control under explicit Weston `--debug` and showed that guest-visible output remains graphical at `1280x800` even while host-side QMP `screendump` still falls back to `720x400`.
- Added a postmortem report synthesizing the imported KMS research and the latest local evidence into the current working conclusion: guest display state is healthy after resume, and QMP/`virtio-vga` capture is now the main investigation target.
