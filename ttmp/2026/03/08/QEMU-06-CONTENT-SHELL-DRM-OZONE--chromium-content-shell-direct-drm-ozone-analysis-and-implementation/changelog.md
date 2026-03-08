# Changelog

## 2026-03-08

- Created the `QEMU-06-CONTENT-SHELL-DRM-OZONE` ticket as a fresh phase for direct DRM/KMS Chromium bring-up without Weston.
- Imported the upstream/direct-research note from `/tmp/drm-ozone.md`.
- Wrote the initial intern-facing guide describing the direct DRM/Ozone architecture, implementation plan, risks, and test matrix.
- Started the diary and task list for the new phase.
- Added a reusable Chromium checkout bootstrap helper at `host/bootstrap_chromium_checkout.sh` and mirrored it into the ticket `scripts/` folder.
- Confirmed local disk headroom and successful access to the `depot_tools` remote.
- Started the first live Chromium checkout via `fetch --nohooks chromium`; current state progressed through `.gclient` creation and into `gclient sync`.
