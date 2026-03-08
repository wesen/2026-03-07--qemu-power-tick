# Changelog

## 2026-03-07

- Initial workspace created.
- Copied the imported `lab2.md` brief into the stage-3 ticket.
- Identified the local Chromium packaging constraint: `chromium-browser` is only a snap launcher, while the actual browser payload is available under `/snap/chromium/current/usr/lib/chromium-browser/`.
- Added the first phase-3 guest builder, init path, browser launcher, and QEMU run script using the installed Chromium snap payload as the guest browser source.
- Hit and fixed an oversized initramfs failure by trimming the browser asset set and reducing fonts to a minimal DejaVu subset.
- Booted Chromium visibly under Weston and captured the first successful stage-3 screenshot in `results-phase3-smoke3/01-stage3.png`.
- Added deterministic stage-3 input-validation helpers:
  - `host/make_phase3_test_url.py`
  - `host/capture_phase3_checkpoints.py`
- Validated host keyboard injection into Chromium with `results-phase3-checkpoints1/01-after-keyboard.png`.
- Validated host pointer injection into Chromium with `results-phase3-checkpoints1/02-after-pointer.png`.
