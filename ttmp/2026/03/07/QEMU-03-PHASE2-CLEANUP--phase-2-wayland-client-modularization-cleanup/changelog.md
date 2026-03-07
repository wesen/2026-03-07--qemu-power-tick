# Changelog

## 2026-03-07

- Initial workspace created
- Scoped this ticket as a behavior-preserving modularization pass for the phase-2 guest code, separate from the active feature-delivery work in `QEMU-02-LAB-TWO`.
- Extracted the phase-2 Wayland client into `wl_app_core`, `wl_render`, `wl_net`, and `wl_wayland` modules, kept `wl_sleepdemo.c` as the orchestration layer, and validated boot, screenshot, reconnect, pointer, and keyboard behavior after the refactor (commit `91076d0`).
