# Changelog

## 2026-03-07

- Initial workspace created
- Added guest-side `display_probe.sh` instrumentation and wired it into phase-2 and phase-3 initramfs builds.
- Ran a phase-2 probe-backed `pm_test=devices` control and confirmed that `fb0`, `vtconsole`, and DRM connector state stayed stable while QMP screenshots still fell back to a firmware-looking text plane.
- Ran phase-3 `weston-simple-shm` and Chromium probe validations and confirmed the same visual fallback pattern, with `weston-simple-shm` also surfacing `virtio_gpu_dequeue_ctrl_func ... response 0x1203`.
- Corrected the `display_unbind_fbcon=1` experiment to target the actual framebuffer `vtconsole`, proving that fbcon ownership changes the visible pre-suspend plane but does not fix the post-resume fallback.
- Wrote the intern-facing analysis guide and investigation diary for follow-on debugging.
