# Tasks

## TODO

- [x] Import `/tmp/qemu-debug.md` into `QEMU-05-DEVICES-RESUME-INVESTIGATION` and review it for close-out context.
- [x] Create `QEMU-06-CONTENT-SHELL-DRM-OZONE` and import `/tmp/drm-ozone.md`.
- [x] Write the initial analysis/design/implementation guide for the direct DRM/Ozone `content_shell` phase.
- [x] Start the new ticket diary and capture the initial design/setup step.
- [x] Upload the initial QEMU-06 documentation bundle to reMarkable.
- [x] Add a reproducible Chromium bootstrap helper and mirror it into the ticket `scripts/` folder before running the checkout.
- [x] Clone `depot_tools` and a Chromium source checkout because this machine does not currently have `gn`, `gclient`, or a usable Chromium tree.
- [x] Verify which Chromium target set we need first for this phase (`content_shell` plus required runtime assets and helpers).
- [x] Add an initial phase-4 GN args helper that writes a direct DRM/Ozone `args.gn` baseline and expected target list.
- [x] Add an end-to-end phase-4 Chromium build driver helper for `runhooks -> gn gen -> autoninja`.
- [x] Run `gclient runhooks` and generate the first `out/Phase4DRM` build directory.
- [ ] Finish the first `autoninja` build for `content_shell`, `chrome_sandbox`, and `chrome_crashpad_handler`.
- [ ] Probe the built Chromium output directory and wire it into the phase-4 rootfs builder.
- [x] Add phase-4 runtime files:
  - `guest/init-phase4-drm`
  - `guest/build-phase4-rootfs.sh`
  - `guest/content-shell-drm-launcher.sh`
  - `guest/run-qemu-phase4.sh`
- [x] Reuse the phase-3 rootfs logic to build a phase-4 initramfs without Weston or seatd in the critical path.
- [x] Reuse the existing direct KMS controls to validate the new phase-4 QEMU/device assumptions before adding Chromium.
- [x] Add a phase-4 local HTML smoke page so the first `content_shell` test is file-backed, not network-backed.
- [x] Add a phase-4 QMP smoke capture helper and mirror it into the ticket `scripts/` folder.
- [x] Add a phase-4 rootfs/runtime dependency probe for Chromium DRM assets (`content_shell`, `icudtl.dat`, `resources.pak`, locales, GBM/EGL/DRI pieces).
- [ ] Attempt the first direct DRM/Ozone `content_shell` boot with `virtio-gpu-pci` and `-vga none`.
- [ ] Record the first failure mode or the first successful frame with screenshots, serial logs, and a diary step.
- [ ] Add a minimal phase-4 suspend harness only after no-suspend rendering works.
- [ ] Decide whether phase 4 will continue with `content_shell` only, or branch into full Chrome/ash-style DRM boot later.
