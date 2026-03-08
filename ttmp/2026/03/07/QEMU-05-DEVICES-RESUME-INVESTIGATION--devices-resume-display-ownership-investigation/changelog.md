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
