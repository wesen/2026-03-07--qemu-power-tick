# Changelog

## 2026-03-07

- Initial workspace created
- Wrote the phase-2 guide, task plan, and diary baseline for the Weston plus Wayland-client stage.
- Added a phase-2 guest rootfs builder, dependency copier, Weston config, init script, and QEMU launcher.
- Validated guest graphics bring-up with `virtio-gpu`, reached a working Weston socket, launched `weston-simple-shm`, and captured the first QMP framebuffer screenshot.
- Added the first custom `wl_sleepdemo` client, generated `xdg-shell` bindings, embedded the client into the phase-2 initramfs, and validated reconnect plus screenshot capture with the custom surface running under Weston.
- Added a dedicated input bring-up playbook that documents the working keyboard/mouse path, required modules and userspace pieces, failure modes, and validation commands for future lab runs.
- Reached an input-complete milestone: host screenshots, pointer injection, and keyboard injection now all work end-to-end with the custom Wayland client under Weston.
