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
- Checked the official Chromium dependency installer:
  - quick check identified the expected missing Linux build packages
  - full install attempt stopped at the `sudo` password boundary
- Added `host/build_phase4_chromium_targets.sh`, which scripts `gclient runhooks`, `configure_phase4_chromium_build.sh`, and the initial `autoninja` target set.
- The Chromium checkout completed far enough for a real source-tree build: `gclient sync --nohooks --no-history` succeeded and `gclient runhooks` reached `113/113`.
- Updated `host/configure_phase4_chromium_build.sh` to match Chromium's documented DRM path:
  - `target_os = "chromeos"`
  - no forced `toolkit_views = false`
  - direct fallback to `src/buildtools/linux64/gn` when depot_tools `gn` is only a thin wrapper
- Generated `/home/manuel/chromium/src/out/Phase4DRM/build.ninja` successfully.
- Started the first real `autoninja` build for `content_shell`, `chrome_sandbox`, and `chrome_crashpad_handler`.
- Added `host/stage_phase4_chromium_payload.sh` to copy the first Chromium build artifacts into `build/phase4/chromium-direct` and immediately probe the staged payload.
- Stopped the first long-running `autoninja` build at the user's request and regenerated `out/Phase4DRM` with `ozone_platform_headless = true` in addition to `ozone_platform_drm = true` so the same `content_shell` build can support both DRM and headless backend testing.

## 2026-03-09

- The first local Chromium build finished successfully and produced a stageable `content_shell`, `chrome_sandbox`, and `chrome_crashpad_handler` payload.
- Added and mirrored `host/run_phase4_headless_smoke.sh`, then validated the staged payload in host-side `--ozone-platform=headless` mode before returning to QEMU DRM.
- Restaged the phase-4 Chromium payload so the guest rootfs builder now carries `content_shell.pak`, `snapshot_blob.bin`, ANGLE frontend libs, and the helper binaries that the first DRM boots actually needed.
- Corrected the payload/runtime probe to distinguish base payload files from optional `content_shell` assets and to require the native Mesa/glvnd pieces that the DRM path needs.
- Added `/dev/shm` bring-up and better launcher diagnostics, which removed the earlier shared-memory startup failures and made the remaining DRM failures much easier to read.
- Identified and fixed the missing Ozone `PostCreateMainMessageLoop()` call in the local Chromium `content_shell` tree (outside this repo), which removed the `EventFactoryEvdev` / `user_input_task_runner_` crash and allowed the DRM startup path to reach real GPU initialization.
- Switched the DRM launcher to `--use-gl=angle --use-angle=default`, then followed the resulting dependency chain from missing `libEGL.so.1` to the native Mesa/glvnd stack that ANGLE expects on Linux.
- Expanded the phase-4 rootfs builder to stage host-native `libEGL.so.1`, `libGLdispatch.so.0`, `libEGL_mesa.so.0`, `libGLESv2.so.2`, `libgbm.so.1`, a minimal DRI driver set, and the `egl_vendor.d` / `drirc.d` data Chromium needed to get past the previous EGL loader failures.
- Fixed a bad symlink experiment that had pointed `libEGL.so.1` back at Chromium's own bundled `libEGL.so` and triggered the ANGLE `GlobalMutex.cpp` recursion assert.
- Fixed the phase-4 font layout and runtime directories:
  - `/etc/fonts` contents now land at the correct path
  - `HOME`, `XDG_CACHE_HOME`, and the user-data directory are now explicit under `/var/lib/content-shell`
  - the launcher now requests a `1280x800` fullscreen window
- Added and mirrored a phase-4 guest display probe path based on `display_probe.sh`, then used it to capture stable `fb0`/vtconsole/connector state while `content_shell` was running.
- Proved the current direct-DRM state boundary:
  - `content_shell` authenticates `/dev/dri/card0`
  - the GPU process reaches `init_success:1`
  - Ozone DRM discovers `renderD128`
  - guest-side `@@DISPLAY` remains stable with `card0-Virtual-1 status=connected enabled=enabled dpms=On`
  - host-side QMP screenshots remain completely black even with later capture delays
- Exposed a second phase-4 harness issue while packaging the larger Mesa stack: several runs accidentally used a partially written initramfs. The stable lesson is that the current Mesa-heavy initramfs must be treated as a completed artifact before boot, and in practice the direct DRM runs have been most reliable with larger guest RAM while this rootfs stays initramfs-based.
- Committed the repo-side runtime checkpoint as `8b88ab1` (`Advance phase 4 DRM runtime bring-up`).
