# Changelog

## 2026-03-08

- Created the `QEMU-06-CONTENT-SHELL-DRM-OZONE` ticket as a fresh phase for direct DRM/KMS Chromium bring-up without Weston.
- Imported the upstream/direct-research note from `/tmp/drm-ozone.md`.
- Wrote the initial intern-facing guide describing the direct DRM/Ozone architecture, implementation plan, risks, and test matrix.
- Started the diary and task list for the new phase.
- Added a reusable Chromium checkout bootstrap helper at `host/bootstrap_chromium_checkout.sh` and mirrored it into the ticket `scripts/` folder.
- Confirmed local disk headroom and successful access to the `depot_tools` remote.
- Started the first live Chromium checkout via `fetch --nohooks chromium`; current state progressed through `.gclient` creation and into `gclient sync`.
- Committed the bootstrap checkpoint as `3ce4f8b` (`Bootstrap Chromium checkout path`).
- Added the first phase-4 runtime skeleton files: `init-phase4-drm`, `build-phase4-rootfs.sh`, `content-shell-drm-launcher.sh`, `run-qemu-phase4.sh`, and `phase4-smoke.html`.
- Added `host/capture_phase4_smoke.py` and mirrored all new phase-4 helpers into the ticket `scripts/` folder.
- Built a kms-only phase-4 initramfs and validated the no-Weston boot path with a successful `1280x800` QMP screenshot in `results-phase4-kms1/`.
- Added `host/probe_phase4_chromium_payload.py` and mirrored it into the ticket `scripts/` folder.
- Captured a baseline runtime probe in `results-phase4-runtime-probe1/probe.json`; current result is that host DRM/GBM/EGL prerequisites are present and the Chromium payload directory is still absent.
- Updated `host/bootstrap_chromium_checkout.sh` to default to `--nohooks --no-history` and to refresh `depot_tools` safely from `origin/main` even when the local checkout is detached.
- Aborted the wasteful full-history Chromium fetch after proving it was transferring the full 61 GiB history, then restarted the checkout on the reduced-history path.
- Imported the official Chromium Linux build/Ozone references into the ticket for future reference.
- Verified from Chromium BUILD files that the initial phase-4 target set is `content_shell`, `chrome_sandbox`, and `chrome_crashpad_handler`.
- Added `host/configure_phase4_chromium_build.sh`, which writes `~/chromium/src/out/Phase4DRM/args.gn` with a direct DRM/Ozone baseline and prints the initial target list.
