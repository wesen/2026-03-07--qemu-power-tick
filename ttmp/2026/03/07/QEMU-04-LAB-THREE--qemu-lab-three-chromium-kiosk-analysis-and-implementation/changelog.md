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
- Added stage-3 suspend plumbing with:
  - `guest/suspendctl.c`
  - `guest/build-suspendctl.sh`
  - `host/capture_phase3_suspend_checkpoints.py`
- Verified stage-3 suspend metrics and continuity split:
  - `pm_test=freezer` preserves the Chromium surface (`results-phase3-suspend-freezer1`)
  - `pm_test=devices` records good suspend metrics but loses visible display continuity after resume (`results-phase3-suspend2`, `results-phase3-suspend4`)
- Added a detailed stage-3 postmortem review doc assessing the result quality, process quality, struggles, and recommended next steps.
