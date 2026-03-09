---
Title: Diary
Ticket: QEMU-06-CONTENT-SHELL-DRM-OZONE
Status: active
Topics:
    - qemu
    - linux
    - drm
    - chromium
    - graphics
DocType: reference
Intent: long-term
Owners: []
RelatedFiles:
    - Path: ../../../../../../../../../chromium/src/components/crash/core/app/BUILD.gn
      Note: Verified chrome_crashpad_handler target location for Step 6
    - Path: ../../../../../../../../../chromium/src/content/shell/BUILD.gn
      Note: Verified content_shell target location for Step 6
    - Path: ../../../../../../../../../chromium/src/sandbox/linux/BUILD.gn
      Note: Verified chrome_sandbox target location for Step 6
    - Path: guest/build-phase3-rootfs.sh
      Note: Phase-3 baseline reviewed during the initial design step
    - Path: guest/build-phase4-rootfs.sh
      Note: Phase-4 initramfs builder added in Step 3
    - Path: guest/chromium-wayland-launcher.sh
      Note: Current Wayland launcher contrasted against the planned direct DRM launcher
    - Path: guest/init-phase3
      Note: Phase-3 baseline reviewed during the initial design step
    - Path: guest/init-phase4-drm
      Note: Phase-4 PID 1 implementation added in Step 3
    - Path: guest/run-qemu-phase4.sh
      Note: Phase-4 QEMU runner added in Step 3
    - Path: host/bootstrap_chromium_checkout.sh
      Note: Reusable Chromium checkout bootstrap helper created in Step 2
    - Path: host/capture_phase4_smoke.py
      Note: Phase-4 QMP smoke capture helper added in Step 3
    - Path: host/configure_phase4_chromium_build.sh
      Note: Phase-4 GN args and target helper added in Step 6
    - Path: host/probe_phase4_chromium_payload.py
      Note: Phase-4 payload/runtime boundary probe added in Step 4
    - Path: ttmp/2026/03/08/QEMU-06-CONTENT-SHELL-DRM-OZONE--chromium-content-shell-direct-drm-ozone-analysis-and-implementation/scripts/bootstrap_chromium_checkout.sh
      Note: Ticket-local mirror of the bootstrap helper for review and reproduction
    - Path: ttmp/2026/03/08/QEMU-06-CONTENT-SHELL-DRM-OZONE--chromium-content-shell-direct-drm-ozone-analysis-and-implementation/sources/local/01-drm-ozone.md
      Note: Imported direct DRM/Ozone research note that shaped the first diary step
ExternalSources: []
Summary: ""
LastUpdated: 2026-03-08T12:13:43.22501215-04:00
WhatFor: ""
WhenToUse: ""
---






# Diary

## Goal

This diary records the implementation of the new direct DRM/Ozone `content_shell` phase. Its job is to preserve exactly how the ticket was set up, how the design was derived from the existing phase-3 tree, and how each implementation slice is validated and committed.

## Step 1: Create the New Ticket, Import the Research Notes, and Freeze the Design

This ticket started with a deliberate reset in scope. The Weston/Wayland phase already exists and works well enough to prove Chromium-on-Wayland bring-up, so the new ticket is not trying to "fix phase 3." It is starting a new branch that removes Weston from the success path and asks a narrower question: can Chromium `content_shell` own DRM/KMS directly through Ozone DRM in QEMU?

The most important thing in this first step was to avoid writing a generic browser note. I read the imported direct-DRM note against the real repo files and wrote a guide that makes the architectural break explicit: phase 4 is a new branch with a new init, new rootfs builder, new launcher, and a staged validation plan that starts with direct KMS before it blames Chromium.

### Prompt Context

**User prompt (verbatim):** "Import /tmp/qemu-debug.md with docmgr import file --file ... --ticket ... and read it, just fyi, to close up the ticket. \n\nThen, we want to start on a new fresh ticket running chromium content_shell straight against DRM, which will require quite a bit more work, but also simplify the stack.\n\nImport /tmp/drm-ozone.md into the new ticket, read it, then write a detailed analysis document of how you are going to buildl and test this, just to verify we got it right. Then upload to remarkable.\n\nCreate a detailed analysis / design / implementation guide that is very detailed for a new intern, explaining all the parts of the system needed to understand what it is, with prose paragraphs and bullet point sand pseudocode and diagrams and api references and file\n  references.\n  It should be very clear and detailed. Store in the ticket and the nupload to remarkable.\n\nThen create tasks for in the ticket, and work on them one by one, committing as you go, and keeping a detailed diary."

**Assistant interpretation:** Close the current investigation ticket with an imported note, create a fresh direct-DRM Chromium ticket, import the new research note, turn that note plus the repo state into a detailed intern-facing design guide, upload the initial bundle, and then begin implementation under that new plan.

**Inferred user intent:** Split the direct DRM/Ozone work into a new, cleaner track so the next implementation phase has a solid design record, task list, and diary before code changes begin.

### What I did
- Imported `/tmp/qemu-debug.md` into the QEMU-05 ticket for close-out context.
- Created the new ticket `QEMU-06-CONTENT-SHELL-DRM-OZONE`.
- Imported `/tmp/drm-ozone.md` into the new ticket.
- Read the imported direct-DRM note against the current repo files:
  - `/home/manuel/code/wesen/2026-03-07--qemu-power-tick/guest/build-phase3-rootfs.sh`
  - `/home/manuel/code/wesen/2026-03-07--qemu-power-tick/guest/init-phase3`
  - `/home/manuel/code/wesen/2026-03-07--qemu-power-tick/guest/chromium-wayland-launcher.sh`
  - `/home/manuel/code/wesen/2026-03-07--qemu-power-tick/guest/kms_pattern.c`
- Wrote the initial design guide in:
  - `/home/manuel/code/wesen/2026-03-07--qemu-power-tick/ttmp/2026/03/08/QEMU-06-CONTENT-SHELL-DRM-OZONE--chromium-content-shell-direct-drm-ozone-analysis-and-implementation/design-doc/01-direct-drm-ozone-content-shell-analysis-and-implementation-guide.md`
- Replaced the placeholder `index.md`, `tasks.md`, and `changelog.md` content with ticket-specific content.
- Started this diary.

### Why
- The direct DRM/Ozone work is a new branch with different runtime assumptions.
- The ticket needed to be explicit that phase 3 is a valid baseline, not a failed prototype.
- A detailed guide is necessary before coding because the highest risk is not a single bug; it is mixing the compositor-based phase with the new browser-owns-DRM phase.

### What worked
- The imported note matched the repo reality well enough to produce a concrete phase-4 plan.
- The repo already contains useful reusable pieces:
  - QMP harnesses
  - direct KMS helper
  - initramfs build pattern
  - module/udev boot logic
- The new guide now clearly defines:
  - the new phase boundary
  - proposed files
  - a low-risk milestone order
  - the test matrix
- The prerequisite probe immediately clarified the first hard blocker:
  - `gn` is not installed
  - `gclient` is not installed
  - there is no existing Chromium checkout on this machine

### What didn't work
- The first `docmgr import file` into the new ticket failed immediately after ticket creation because the ticket lookup did not resolve on the first attempt:
```text
Error: failed to find ticket directory: ticket not found: QEMU-06-CONTENT-SHELL-DRM-OZONE
```
- Rerunning after verifying the ticket path worked cleanly.

### What I learned
- The repo is in a better place for this than it first appears: phase 3 already tells us exactly what must be removed from the critical path.
- The cleanest first milestone is not Chromium itself; it is "phase-4 boot assumptions still support direct KMS."
- Treating phase 4 as a new branch is a design requirement, not just an organizational preference.

### What was tricky to build
- The main challenge was scope control. The imported note could easily turn into an aspirational Chromium write-up, but the actual ticket needs to be grounded in the current repo and the specific runtime assets we already have.
- The other subtle part was identifying what is genuinely reusable from phase 3. The right answer is "boot plumbing and validation harnesses," not "Weston launch logic."

### What warrants a second pair of eyes
- The guide assumes a direct-DRM `content_shell` payload will be obtainable or buildable in this environment; that remains the biggest unresolved implementation risk.
- The exact runtime dependency set for GBM/EGL/DRI may still need one round of correction once the first phase-4 rootfs exists.

### What should be done in the future
- Upload the initial guide bundle to reMarkable.
- Start the first code task by establishing the Chromium checkout/build prerequisites.
- Then create the phase-4 file skeletons.
- Use the first code slice to validate phase-4 boot assumptions before attempting Chromium bring-up.

### Code review instructions
- Start with:
  - `/home/manuel/code/wesen/2026-03-07--qemu-power-tick/ttmp/2026/03/08/QEMU-06-CONTENT-SHELL-DRM-OZONE--chromium-content-shell-direct-drm-ozone-analysis-and-implementation/design-doc/01-direct-drm-ozone-content-shell-analysis-and-implementation-guide.md`
  - `/home/manuel/code/wesen/2026-03-07--qemu-power-tick/ttmp/2026/03/08/QEMU-06-CONTENT-SHELL-DRM-OZONE--chromium-content-shell-direct-drm-ozone-analysis-and-implementation/tasks.md`
- Then compare the new guide to the current phase-3 files:
  - `/home/manuel/code/wesen/2026-03-07--qemu-power-tick/guest/build-phase3-rootfs.sh`
  - `/home/manuel/code/wesen/2026-03-07--qemu-power-tick/guest/init-phase3`
  - `/home/manuel/code/wesen/2026-03-07--qemu-power-tick/guest/chromium-wayland-launcher.sh`

### Technical details
- New ticket path:
```text
ttmp/2026/03/08/QEMU-06-CONTENT-SHELL-DRM-OZONE--chromium-content-shell-direct-drm-ozone-analysis-and-implementation
```
- Local prerequisite check:
```text
gn not found
gclient not found
no ~/chromium checkout present
```
- Proposed new runtime files:
```text
guest/init-phase4-drm
guest/build-phase4-rootfs.sh
guest/content-shell-drm-launcher.sh
guest/run-qemu-phase4.sh
guest/phase4-smoke.html
```

## Step 2: Bootstrap the Chromium Checkout Path and Start the Real Fetch

The first hard blocker for this ticket is not a kernel or QEMU problem. It is build availability. The machine did not have `depot_tools`, `gclient`, `gn`, or any Chromium checkout, so the first implementation slice had to turn that absence into a repeatable bootstrap path instead of a one-off shell session.

I treated this as a real implementation step rather than "just environment setup" because phase 4 depends on a custom Chromium payload, and if the checkout process is not scripted early then the diary and the ticket scripts stop being trustworthy. The result of this step is a reusable bootstrap helper, mirrored into the ticket, plus the first live Chromium fetch attempt.

### Prompt Context

**User prompt (verbatim):** "we will need to clone and build."

**Assistant interpretation:** The user is confirming that the ticket should assume a real Chromium source checkout and build, not a distro or snap-provided browser payload.

**Inferred user intent:** Make the Chromium source/bootstrap path an explicit early task and start executing it now instead of deferring it as a later surprise.

**Commit (code):** `3ce4f8b` - `Bootstrap Chromium checkout path`

### What I did
- Added a reusable bootstrap helper at:
  - `/home/manuel/code/wesen/2026-03-07--qemu-power-tick/host/bootstrap_chromium_checkout.sh`
- Mirrored the helper into the ticket archive at:
  - `/home/manuel/code/wesen/2026-03-07--qemu-power-tick/ttmp/2026/03/08/QEMU-06-CONTENT-SHELL-DRM-OZONE--chromium-content-shell-direct-drm-ozone-analysis-and-implementation/scripts/bootstrap_chromium_checkout.sh`
- Confirmed enough local disk exists for a full Chromium tree:
```text
Filesystem                         Size  Used Avail Use% Mounted on
/dev/mapper/ubuntu--vg-ubuntu--lv  1.8T  1.2T  578G  67% /
```
- Confirmed access to the `depot_tools` remote:
```text
4ce8ba39a3488397a2d1494f167020f21de502f3	HEAD
```
- Ran the bootstrap helper from the repo root:
```text
./host/bootstrap_chromium_checkout.sh
```
- Observed the bootstrap path progress through:
  - `git clone https://chromium.googlesource.com/chromium/tools/depot_tools.git /home/manuel/depot_tools`
  - `fetch.py --nohooks chromium`
  - `gclient config`
  - `gclient sync --nohooks`
- Verified that the first stable Chromium checkout state exists:
```text
/home/manuel/chromium/.gclient
```
- Continued watching the live sync until it advanced into the real Chromium source clone:
```text
Running: gclient sync --nohooks
________ running 'git -c core.deltaBaseCacheLimit=2g clone --no-checkout --progress https://chromium.googlesource.com/chromium/src.git /home/manuel/chromium/_gclient_src_pf2wv8k_' in '/home/manuel/chromium'
remote: Sending approximately 61.14 GiB ...
```

### Why
- Phase 4 needs a custom Chromium payload that the current repo does not have.
- A scripted bootstrap path is much easier to review and repeat than ad hoc `fetch` and `gclient` commands hidden in shell history.
- The ticket needed a concrete answer to "how are we actually going to get Chromium onto this machine?"

### What worked
- `depot_tools` cloned cleanly into `/home/manuel/depot_tools`.
- The bootstrap helper is idempotent and can update `depot_tools` or resume an existing checkout later.
- `fetch chromium` progressed far enough to create the Chromium solution config, so the machine is past the "missing toolchain" stage and into the "real checkout latency" stage.

### What didn't work
- The first Chromium fetch did not materialize `/home/manuel/chromium/src` immediately. The current bottleneck is the remote probe/sync stage against `https://chromium.googlesource.com/chromium/src.git`, which is much slower than the `depot_tools` clone.
- While waiting, the active process tree looked like:
```text
git -c color.ui=never ls-remote --symref https://chromium.googlesource.com/chromium/src.git HEAD
/usr/lib/git-core/git remote-https https://chromium.googlesource.com/chromium/src.git https://chromium.googlesource.com/chromium/src.git
/usr/lib/git-core/git-remote-https https://chromium.googlesource.com/chromium/src.git https://chromium.googlesource.com/chromium/src.git
```
- So this step ended with a valid bootstrap path and an in-flight Chromium fetch, not a fully populated `src/` checkout yet.
- The size of the first clone is substantial even before hooks:
```text
remote: Sending approximately 61.14 GiB ...
```

### What I learned
- The machine setup risk is now much narrower than before: local disk and `depot_tools` are fine.
- The first real external bottleneck is Chromium remote checkout latency, not local packaging or missing helper tooling.
- Creating the helper before running the fetch was the right move because it turned the environment work into a reviewable artifact immediately.

### What was tricky to build
- The tricky part was distinguishing "scriptable setup" from "long-running remote sync." Those are different failure domains, and the ticket needs to preserve that difference. The helper had to be idempotent so later reruns do not turn into a second undocumented workflow once `/home/manuel/chromium` exists.
- Another subtlety is that the first `fetch chromium` path is quiet for long stretches. Without checking the live process tree and the appearance of `.gclient`, it would have been too easy to misclassify the command as hung.
- After the remote probe completed, the next quiet interval was the transition into the full `src.git` clone. Watching for the temporary `_gclient_src_*` directory was the easiest way to confirm that the sync had actually crossed into bulk transfer.

### What warrants a second pair of eyes
- The helper currently uses `fetch --nohooks chromium`, which is the conservative path. If the full checkout becomes too slow or too large for the workflow we want, it may be worth reviewing whether `--no-history` or a more constrained sync policy is acceptable for this ticket.
- The next reviewer should check whether the intended direct-DRM targets really require the full Chrome tree, or whether the ticket should bias earlier toward a narrower `content_shell`-only development path once `src/` exists.

### What should be done in the future
- Let the first Chromium sync reach a real `src/` tree or rerun it with adjusted flags if the current remote probe remains the dominant blocker.
- Once `src/` exists, verify the exact initial target set for phase 4.
- Then create the phase-4 runtime file skeletons.

### Code review instructions
- Start with:
  - `/home/manuel/code/wesen/2026-03-07--qemu-power-tick/host/bootstrap_chromium_checkout.sh`
  - `/home/manuel/code/wesen/2026-03-07--qemu-power-tick/ttmp/2026/03/08/QEMU-06-CONTENT-SHELL-DRM-OZONE--chromium-content-shell-direct-drm-ozone-analysis-and-implementation/scripts/bootstrap_chromium_checkout.sh`
- Then review this step against the live checkout paths:
  - `/home/manuel/depot_tools`
  - `/home/manuel/chromium`
- Validation commands:
```text
git ls-remote https://chromium.googlesource.com/chromium/tools/depot_tools.git HEAD
ls -la /home/manuel/chromium
ps -ef | rg 'fetch.py --nohooks chromium|gclient sync|chromium/src.git'
```

### Technical details
- Bootstrap helper contract:
```text
DEPOT_TOOLS_DIR   default: $HOME/depot_tools
CHECKOUT_DIR      default: $HOME/chromium
FETCH_TARGET      default: chromium
FETCH_FLAGS       default: --nohooks
SYNC_FLAGS        default: --nohooks --with_branch_heads --with_tags
```
- First stable checkout marker from this step:
```text
/home/manuel/chromium/.gclient
```
- Current in-flight clone directory:
```text
/home/manuel/chromium/_gclient_src_pf2wv8k_
```

## Step 3: Build the Phase-4 Skeleton and Validate the No-Weston KMS Control

Once the Chromium checkout moved into a long-running bulk transfer, the next productive move was to stop waiting on the network and build the local phase-4 branch itself. The goal of this step was not "fake progress while Chromium downloads." The goal was to prove that the new direct-DRM phase can already boot and render without Weston, using the existing `kms_pattern` witness as the first control.

This step matters because it removes one future ambiguity. If direct DRM Chromium fails later, we now have a repo-local answer to "is the new phase-4 initramfs and QEMU shape valid at all?" The answer is yes.

### Prompt Context

**User prompt (verbatim):** "continue"

**Assistant interpretation:** Keep executing the ticket tasks rather than stopping after the design/write-up checkpoint.

**Inferred user intent:** Maintain momentum by turning the design into real implementation slices with diary entries, scripts, and commits.

**Commit (code):** `879f500` - `Add phase 4 DRM skeleton and smoke harness`

### What I did
- Added the phase-4 runtime files:
  - `/home/manuel/code/wesen/2026-03-07--qemu-power-tick/guest/init-phase4-drm`
  - `/home/manuel/code/wesen/2026-03-07--qemu-power-tick/guest/build-phase4-rootfs.sh`
  - `/home/manuel/code/wesen/2026-03-07--qemu-power-tick/guest/content-shell-drm-launcher.sh`
  - `/home/manuel/code/wesen/2026-03-07--qemu-power-tick/guest/run-qemu-phase4.sh`
  - `/home/manuel/code/wesen/2026-03-07--qemu-power-tick/guest/phase4-smoke.html`
- Designed the new init so that it supports:
  - `phase4_mode=kms-pattern`
  - `phase4_mode=content-shell`
- Added a reusable phase-4 capture helper:
  - `/home/manuel/code/wesen/2026-03-07--qemu-power-tick/host/capture_phase4_smoke.py`
- Mirrored all of those files into the ticket script archive:
  - `/home/manuel/code/wesen/2026-03-07--qemu-power-tick/ttmp/2026/03/08/QEMU-06-CONTENT-SHELL-DRM-OZONE--chromium-content-shell-direct-drm-ozone-analysis-and-implementation/scripts/`
- Built the first phase-4 initramfs without a Chromium payload:
```text
./guest/build-phase4-rootfs.sh build/phase4 build/initramfs-phase4.cpio.gz
warning: CHROMIUM_PAYLOAD_DIR is unset; building a kms-only phase-4 initramfs
```
- Booted the new phase under QEMU and captured the first screenshot:
```text
./host/capture_phase4_smoke.py \
  --kernel build/vmlinuz \
  --initramfs build/initramfs-phase4.cpio.gz \
  --results-dir results-phase4-kms1 \
  --append-extra 'phase4_runtime_seconds=12 phase4_mode=kms-pattern' \
  --capture-delay 4 \
  --timeout 40
```
- Verified the captured image size:
```text
{'size': (1280, 800), 'mode': 'RGB'}
```
- Verified the guest-side KMS witness:
```text
@@KMSPATTERN device=/dev/dri/card0 connector_id=36 crtc_id=35 fb_id=42 width=1280 height=800 pattern=pre
```

### Why
- The direct-DRM ticket needed a local success path that does not depend on the Chromium build finishing first.
- `kms_pattern` is already the cleanest low-level witness in this repo, so phase 4 should use it before blaming Chromium or Ozone.
- A reusable capture helper makes the first no-Weston validation reviewable and repeatable.

### What worked
- The new phase-4 rootfs builder successfully produced `build/initramfs-phase4.cpio.gz`.
- The phase-4 init loaded DRM/input modules, started `udevd`, and saw `/dev/dri/card0` and `/dev/input/event*`.
- The `virtio-gpu-pci` + `-vga none` runner worked as intended for the no-Weston control.
- QMP screendump captured a real `1280x800` frame in `results-phase4-kms1/00-smoke.png`.

### What didn't work
- This step did not attempt a real Chromium boot yet because the Chromium checkout is still in progress.
- The rootfs builder warns when `CHROMIUM_PAYLOAD_DIR` is unset:
```text
warning: CHROMIUM_PAYLOAD_DIR is unset; building a kms-only phase-4 initramfs
```
- That warning is expected for this step, but it means `phase4_mode=content-shell` would still fail until the Chromium payload exists.

### What I learned
- The phase-4 branch is already valid as a KMS appliance even before Chromium arrives.
- The split between `kms-pattern` and `content-shell` was the right design because it gives us a trustworthy control path.
- `virtio-gpu-pci` with `-vga none` works cleanly for the first no-Weston validation and produces a host-visible `1280x800` capture.

### What was tricky to build
- The main tricky part was keeping the phase-4 files useful before Chromium exists. That required `build-phase4-rootfs.sh` and `init-phase4-drm` to degrade intentionally into a kms-only mode instead of pretending the browser payload is already there.
- Another subtle point was choosing where to put the first screenshot automation. A small dedicated `capture_phase4_smoke.py` helper is better than burying one more QMP sequence in shell history because the ticket explicitly wants every helper preserved.

### What warrants a second pair of eyes
- The `content-shell-drm-launcher.sh` flags are an informed first pass, but they still need a real Chromium payload before they can be trusted.
- The phase-4 rootfs builder currently copies the broad font/XKB/udev base but does not yet probe for all GBM/EGL/DRI runtime pieces; that still needs a dedicated dependency pass once the Chromium build outputs exist.

### What should be done in the future
- Finish the Chromium source checkout.
- Verify the initial Chromium target set in the checkout.
- Add the runtime dependency probe for the direct DRM payload.
- Attempt the first `phase4_mode=content-shell` boot.

### Code review instructions
- Start with:
  - `/home/manuel/code/wesen/2026-03-07--qemu-power-tick/guest/init-phase4-drm`
  - `/home/manuel/code/wesen/2026-03-07--qemu-power-tick/guest/build-phase4-rootfs.sh`
  - `/home/manuel/code/wesen/2026-03-07--qemu-power-tick/guest/run-qemu-phase4.sh`
  - `/home/manuel/code/wesen/2026-03-07--qemu-power-tick/host/capture_phase4_smoke.py`
- Then validate with:
```text
./guest/build-phase4-rootfs.sh build/phase4 build/initramfs-phase4.cpio.gz
./host/capture_phase4_smoke.py --kernel build/vmlinuz --initramfs build/initramfs-phase4.cpio.gz --results-dir results-phase4-kms1 --append-extra 'phase4_runtime_seconds=12 phase4_mode=kms-pattern' --capture-delay 4 --timeout 40
tail -n 80 results-phase4-kms1/guest-serial.log
```

### Technical details
- Current no-Weston phase-4 witness files:
```text
results-phase4-kms1/00-smoke.ppm
results-phase4-kms1/00-smoke.png
results-phase4-kms1/guest-serial.log
```
- Mode switch currently implemented in the new init:
```text
phase4_mode=kms-pattern
phase4_mode=content-shell
```

## Step 4: Add a Payload Probe So the Chromium Failure Boundary Is Obvious

After the kms-only phase-4 control succeeded, the next useful local task was not another boot attempt. It was a boundary tool: something that can tell us whether a future direct-DRM failure is because the Chromium payload is incomplete or because the system runtime is missing DRM/GBM prerequisites. Without that split, the first `content_shell` failure would be noisier than it needs to be.

This step therefore adds a dedicated payload/runtime probe. Its first run is intentionally against an empty payload directory, because that baseline is still informative: it tells us which prerequisites already exist on the host and which artifacts still depend on the Chromium build finishing.

### Prompt Context

**User prompt (verbatim):** (same as Step 3)

**Assistant interpretation:** Continue working through the next useful implementation tasks while the Chromium checkout is still running.

**Inferred user intent:** Use the waiting time to eliminate local ambiguity and leave the next direct-DRM Chromium failure mode easier to diagnose.

**Commit (code):** `b4ef3e8` - `Add phase 4 payload probe`

### What I did
- Added the probe helper:
  - `/home/manuel/code/wesen/2026-03-07--qemu-power-tick/host/probe_phase4_chromium_payload.py`
- Mirrored it into the ticket archive:
  - `/home/manuel/code/wesen/2026-03-07--qemu-power-tick/ttmp/2026/03/08/QEMU-06-CONTENT-SHELL-DRM-OZONE--chromium-content-shell-direct-drm-ozone-analysis-and-implementation/scripts/probe_phase4_chromium_payload.py`
- Ran the baseline probe against the intended payload location, allowing the payload to be absent for now:
```text
mkdir -p results-phase4-runtime-probe1
./host/probe_phase4_chromium_payload.py \
  --payload-dir build/phase4/chromium-direct \
  --allow-missing-payload \
  --output results-phase4-runtime-probe1/probe.json
```
- Recorded the baseline JSON result at:
  - `/home/manuel/code/wesen/2026-03-07--qemu-power-tick/results-phase4-runtime-probe1/probe.json`

### Why
- The ticket needs a fast answer to "is the problem the Chromium payload or the runtime base?"
- The rootfs builder will eventually need the exact Chromium artifact set, and a dedicated probe makes that contract explicit.
- Running the probe before Chromium exists is still useful because it establishes the host graphics/runtime baseline now.

### What worked
- The probe correctly identified that the payload directory does not exist yet.
- It also confirmed that the expected host-side runtime libraries already exist:
```text
/lib/x86_64-linux-gnu/libdrm.so.2
/lib/x86_64-linux-gnu/libgbm.so.1
/lib/x86_64-linux-gnu/libEGL.so.1
/lib/x86_64-linux-gnu/libGLESv2.so.2
/lib/x86_64-linux-gnu/libxkbcommon.so.0
```
- The probe also found populated DRI driver directories including `virtio_gpu_dri.so` and `swrast_dri.so`.

### What didn't work
- There is still no Chromium payload to validate, so every required browser artifact is currently missing:
```text
content_shell
chrome_sandbox
icudtl.dat
resources.pak
v8_context_snapshot.bin
chrome_100_percent.pak
chrome_200_percent.pak
```
- That is expected for now, but it means the next meaningful run of this tool depends on the Chromium checkout/build completing.

### What I learned
- The build gate is becoming cleaner: the host DRM runtime is not the first suspect anymore.
- The first future `content_shell` failure should now be interpreted against this baseline. If the payload is present and the probe still passes on runtime libs, the investigation should move to launch flags or browser runtime behavior, not missing host graphics packages.

### What was tricky to build
- The tricky part was making the tool useful before the payload exists. That required an explicit `--allow-missing-payload` mode so the baseline can still be captured without turning the current state into a hard failure.
- I also caught one small bug immediately: the first version serialized file-presence booleans as strings, which would have made downstream automation more awkward. That was fixed before the baseline run.

### What warrants a second pair of eyes
- The list of "required" payload files is a first-pass contract based on the imported note and the existing phase-3 packaging assumptions. Once the real Chromium build exists, this list should be checked against the actual `content_shell` output directory.
- The probe currently checks common host runtime libraries and DRI directories, but it does not yet inspect a built rootfs image directly. That may be worth adding later if rootfs-only failures appear.

### What should be done in the future
- Re-run this probe against the real Chromium output directory once `content_shell` is built.
- Fold its results into the phase-4 rootfs copy step so missing payload artifacts fail early and readably.

### Code review instructions
- Start with:
  - `/home/manuel/code/wesen/2026-03-07--qemu-power-tick/host/probe_phase4_chromium_payload.py`
- Then validate with:
```text
./host/probe_phase4_chromium_payload.py --payload-dir build/phase4/chromium-direct --allow-missing-payload --output results-phase4-runtime-probe1/probe.json
cat results-phase4-runtime-probe1/probe.json
```

### Technical details
- Current baseline conclusion:
```text
host DRM/GBM/EGL runtime looks present
Chromium payload directory is still absent
```
- Probe output file:
```text
results-phase4-runtime-probe1/probe.json
```

## Step 5: Correct the Chromium Bootstrap Strategy Before It Burns Hours

The first Chromium fetch path succeeded in one important sense: it proved the bootstrap helper worked and that the machine could start a real Chromium sync. But it also revealed two process bugs that were worth fixing immediately instead of waiting for a very expensive mistake to finish. The first was that the initial helper defaulted to a full-history fetch. The second was that `depot_tools` landed in detached HEAD, which broke the helper's original `git pull --ff-only` update logic.

This was a good example of why the diary matters. Without writing down the transfer size and the detached-HEAD behavior, the next rerun could easily have repeated the same wasteful path and looked "normal" just because Chromium is large.

### Prompt Context

**User prompt (verbatim):** (same as Step 3)

**Assistant interpretation:** Keep iterating pragmatically while the Chromium checkout is the current bottleneck.

**Inferred user intent:** Improve the actual workflow when a better path becomes obvious, instead of stubbornly continuing with an inefficient first attempt.

**Commit (code):** `69a8e05` - `Optimize Chromium bootstrap fetch path`

### What I did
- Checked `fetch --help` and confirmed that the Chromium bootstrap supports reduced history:
```text
usage: fetch.py [-h] [-n] [--nohooks] [--nohistory] [--force]
  --nohistory, --no-history
                        history.
```
- Measured the cost of the first in-flight full-history attempt:
```text
2.4G  /home/manuel/chromium/_gclient_src_pf2wv8k_
```
- Stopped the full-history transfer after it had already proven the wrong thing:
```text
remote: Sending approximately 61.14 GiB ...
```
- Updated the bootstrap helper defaults in:
  - `/home/manuel/code/wesen/2026-03-07--qemu-power-tick/host/bootstrap_chromium_checkout.sh`
  so that `FETCH_FLAGS` now defaults to:
```text
--nohooks --no-history
```
- Fixed the `depot_tools` refresh logic so it no longer assumes an attached local branch:
```text
git -C "$DEPOT_TOOLS_DIR" fetch origin main
git -C "$DEPOT_TOOLS_DIR" checkout -B main origin/main
```
- Moved the aborted full-history checkout aside:
```text
/home/manuel/chromium.fullhistory-aborted
```
- Restarted the Chromium bootstrap on the cheaper path:
```text
./host/bootstrap_chromium_checkout.sh
Running: gclient sync --nohooks --no-history
```

### Why
- A full-history Chromium fetch is not necessary for this ticket and would waste hours.
- The helper should be safe to rerun even if `depot_tools` is detached, because that state is what the actual clone produced here.
- Fixing the bootstrap path early is better than accepting a bad default and "documenting around it."

### What worked
- `fetch` explicitly supports the reduced-history mode we want.
- The detached-HEAD problem in `depot_tools` is now understood and handled.
- The no-history retry is running with the corrected helper and a fresh checkout root.

### What didn't work
- The first Chromium sync strategy was too expensive for the task:
```text
remote: Sending approximately 61.14 GiB ...
```
- The original `depot_tools` update block failed when rerun:
```text
You are not currently on a branch.
Please specify which branch you want to merge with.
```
- That failure exposed a real bug in the helper rather than just a transient environment problem.

### What I learned
- For this ticket, "a working bootstrap" is not enough. It also needs to be cheap enough to use repeatedly.
- The actual `depot_tools` clone shape on this machine matters more than my assumption that `git pull --ff-only` would always work.
- The safest path is to treat bootstrap scripts like real code under investigation, not just disposable setup snippets.

### What was tricky to build
- The tricky part here was deciding to stop a command that was technically making progress. The full-history transfer was not "broken," but it was the wrong optimization target for this ticket. The evidence that justified stopping it was the observed `61.14 GiB` history transfer plus the fact that `fetch` already supports `--no-history`.
- Another subtle point was cleanup. A direct recursive delete was blocked by policy, so I moved the aborted checkout aside instead. That preserved the evidence and still let the no-history retry start cleanly.

### What warrants a second pair of eyes
- Once the no-history checkout finishes enough to inspect the tree, it is still worth confirming that this reduced-history mode is sufficient for the GN/ninja workflow we need.
- The renamed aborted checkout can be deleted later if disk pressure matters, but it should stay out of the documented critical path.

### What should be done in the future
- Let the no-history Chromium sync materialize a real `src/` tree.
- Then verify the initial target set in that checkout.

### Code review instructions
- Start with:
  - `/home/manuel/code/wesen/2026-03-07--qemu-power-tick/host/bootstrap_chromium_checkout.sh`
  - `/home/manuel/code/wesen/2026-03-07--qemu-power-tick/ttmp/2026/03/08/QEMU-06-CONTENT-SHELL-DRM-OZONE--chromium-content-shell-direct-drm-ozone-analysis-and-implementation/scripts/bootstrap_chromium_checkout.sh`
- Validate with:
```text
~/depot_tools/fetch --help | rg 'nohistory|no-history'
git -C /home/manuel/depot_tools status --short --branch
```

### Technical details
- Aborted full-history checkout archive:
```text
/home/manuel/chromium.fullhistory-aborted
```
- Current intended fetch mode:
```text
gclient sync --nohooks --no-history
```

## Step 6: Lock the First Chromium Targets and Write the Initial GN Args Baseline

Once the no-history checkout exposed a real `src/` tree, the next high-value step was to stop speaking about Chromium targets abstractly and verify them directly from Chromium's own BUILD files. That matters because phase 4 only needs a small first build set, and the difference between "I think content_shell is enough" and "the tree actually defines these targets" is exactly the kind of detail that prevents wasted build experiments.

I used that source visibility to add a small build helper rather than just another diary paragraph. The helper writes a first-pass `args.gn` into `~/chromium/src/out/Phase4DRM` and prints the initial target list so the next build command is obvious once `gclient sync` settles.

### Prompt Context

**User prompt (verbatim):** (same as Step 3)

**Assistant interpretation:** Continue converting the design into concrete, reviewable build steps as soon as the source tree makes them possible.

**Inferred user intent:** Reduce future guesswork by turning the first Chromium build configuration into an explicit ticket artifact.

**Commit (code):** `ffa15e3` - `Document phase 4 Chromium targets and args`

### What I did
- Confirmed that the no-history checkout had created a usable Chromium source tree:
```text
/home/manuel/chromium/src
/home/manuel/chromium/src/.git
```
- Verified from Chromium BUILD files that the first relevant targets are real:
  - `//content/shell:content_shell`
  - `//sandbox/linux:chrome_sandbox`
  - `//components/crash/core/app:chrome_crashpad_handler`
- Evidence came from:
  - `/home/manuel/chromium/src/content/shell/BUILD.gn`
  - `/home/manuel/chromium/src/sandbox/linux/BUILD.gn`
  - `/home/manuel/chromium/src/components/crash/core/app/BUILD.gn`
- Checked the Ozone build flags in:
  - `/home/manuel/chromium/src/build/config/ozone.gni`
- Added the new helper:
  - `/home/manuel/code/wesen/2026-03-07--qemu-power-tick/host/configure_phase4_chromium_build.sh`
- Mirrored it into the ticket archive:
  - `/home/manuel/code/wesen/2026-03-07--qemu-power-tick/ttmp/2026/03/08/QEMU-06-CONTENT-SHELL-DRM-OZONE--chromium-content-shell-direct-drm-ozone-analysis-and-implementation/scripts/configure_phase4_chromium_build.sh`
- Ran it in write-only mode:
```text
./host/configure_phase4_chromium_build.sh --write-only
```
- Confirmed it wrote:
```text
/home/manuel/chromium/src/out/Phase4DRM/args.gn
```
- The helper currently writes a first-pass direct DRM/Ozone baseline:
```text
use_ozone = true
ozone_auto_platforms = false
ozone_platform_drm = true
ozone_platform_headless = false
ozone_platform_wayland = false
ozone_platform_x11 = false
ozone_platform = "drm"
toolkit_views = false
is_debug = false
is_component_build = false
symbol_level = 0
```

### Why
- The ticket needed a verified initial build set before the first expensive GN/ninja attempt.
- Writing `args.gn` through a helper is better than burying the first configuration in shell history.
- The Ozone source/config files now available in `src/` are enough to justify a real first-pass build configuration.

### What worked
- BUILD-file inspection confirmed the intended target set instead of leaving it speculative.
- The helper wrote the output config successfully.
- The source tree already contains the Ozone build config we need to reason about `ozone_platform_drm`.

### What didn't work
- One probe used an old guessed path:
```text
rg: /home/manuel/chromium/src/components/crash/content/app/BUILD.gn: No such file or directory
```
- The correct crash target file is:
```text
/home/manuel/chromium/src/components/crash/core/app/BUILD.gn
```
- I corrected the path and kept going rather than treating that as a blocker.

### What I learned
- The tree confirms the exact target names we care about, which means future build failures can be discussed precisely.
- `build/config/ozone.gni` shows why our helper needs to disable auto-platform selection and force DRM explicitly on Linux.
- The first phase-4 Chromium build should stay narrowly focused on `content_shell` plus the Linux sandbox/crash helper targets, not a broader Chrome build.

### What was tricky to build
- The main tricky part was timing: enough of the source tree existed to inspect BUILD files, but the entire `gclient sync` was not done yet. That means the right move was a write-only configuration step, not a premature `gn gen` that might fail for reasons unrelated to the phase-4 configuration itself.
- Another subtle point was Chromium path drift. The crash helper target lives under `components/crash/core/app`, not the path I first guessed. The source tree itself was the correction mechanism.

### What warrants a second pair of eyes
- The GN args in the helper are still a first pass. They are grounded in Chromium docs and source, but they have not yet survived a real `gn gen` + `autoninja` run.
- Once sync finishes, the first reviewer should check whether `toolkit_views = false` is still the best choice for this direct DRM `content_shell` path on the current tree.

### What should be done in the future
- Let `gclient sync` complete enough that a real `gn gen` is meaningful.
- Then run the helper without `--write-only`.
- Build the initial target set and inspect the resulting payload directory with the phase-4 probe.

### Code review instructions
- Start with:
  - `/home/manuel/code/wesen/2026-03-07--qemu-power-tick/host/configure_phase4_chromium_build.sh`
  - `/home/manuel/chromium/src/content/shell/BUILD.gn`
  - `/home/manuel/chromium/src/sandbox/linux/BUILD.gn`
  - `/home/manuel/chromium/src/components/crash/core/app/BUILD.gn`
- Validation commands:
```text
./host/configure_phase4_chromium_build.sh --write-only
sed -n '1,120p' /home/manuel/chromium/src/out/Phase4DRM/args.gn
rg -n 'executable\\(\"content_shell\"|chrome_sandbox|chrome_crashpad_handler' /home/manuel/chromium/src/content/shell/BUILD.gn /home/manuel/chromium/src/sandbox/linux/BUILD.gn /home/manuel/chromium/src/components/crash/core/app/BUILD.gn
```

### Technical details
- First verified target set:
```text
//content/shell:content_shell
//sandbox/linux:chrome_sandbox
//components/crash/core/app:chrome_crashpad_handler
```
- Current generated config path:
```text
/home/manuel/chromium/src/out/Phase4DRM/args.gn
```

## Step 7: Check the Official Build-Dependency Path and Script the First Real Build

After the target set and GN args baseline were explicit, the next question was simple: what is still missing between "source tree exists" and "we can ask Chromium to compile"? There are two answers. One is external and expected: Chromium's Linux dependency installer wants `sudo`. The other is internal and fixable now: the repo needed one script that turns the verified target plan into an actual build invocation.

So this step did both. I ran the official dependency checks to see whether the host is close enough, and I added a build driver helper that scripts the first real phase-4 build path once the sync and package prerequisites are satisfied.

### Prompt Context

**User prompt (verbatim):** (same as Step 3)

**Assistant interpretation:** Keep moving the ticket toward a real build attempt rather than stopping at static analysis.

**Inferred user intent:** Make the path from "Chromium checkout exists" to "first direct DRM build command" as explicit and repeatable as the earlier phase-4 boot work.

**Commit (code):** `1e915bc` - `Script phase 4 Chromium build path`

### What I did
- Ran Chromium's quick dependency check:
```text
PATH=/home/manuel/depot_tools:$PATH /home/manuel/chromium/src/build/install-build-deps.sh --quick-check
```
- It reported the missing package set, including:
```text
libevdev-dev
libinput-dev
libnspr4-dev
libnss3-dev
libspeechd-dev
libsystemd-dev
libva-dev
libxshmfence-dev
libxslt1-dev
openbox
xcompmgr
xserver-xorg-video-dummy
```
- Attempted the real package install with conservative flags:
```text
PATH=/home/manuel/depot_tools:$PATH /home/manuel/chromium/src/build/install-build-deps.sh --no-prompt --no-arm --no-chromeos-fonts
```
- That stopped at the expected privilege boundary:
```text
sudo: a password is required
```
- Added the new build driver:
  - `/home/manuel/code/wesen/2026-03-07--qemu-power-tick/host/build_phase4_chromium_targets.sh`
- Mirrored it into the ticket archive:
  - `/home/manuel/code/wesen/2026-03-07--qemu-power-tick/ttmp/2026/03/08/QEMU-06-CONTENT-SHELL-DRM-OZONE--chromium-content-shell-direct-drm-ozone-analysis-and-implementation/scripts/build_phase4_chromium_targets.sh`
- The helper now scripts:
```text
gclient runhooks
configure_phase4_chromium_build.sh
autoninja -C out/Phase4DRM content_shell chrome_sandbox chrome_crashpad_handler
```

### Why
- The quick check tells us whether the host is missing obvious Chromium build prerequisites before we waste time on later build failures.
- The dependency installer result needed to be recorded as a real blocker, not left implicit.
- A single build driver script is the natural next artifact after the target-verification helper.

### What worked
- The quick dependency check ran cleanly and gave a concrete missing-package list.
- The build driver script passes shell syntax checks and is mirrored into the ticket.
- The Chromium sync is still progressing in parallel, so this step did not block on waiting.

### What didn't work
- The real dependency install cannot proceed unattended in this environment because it needs `sudo`:
```text
install-build-deps.py [INFO]: You might have to enter your password one or more times for 'sudo'.
sudo: a password is required
```
- That means the Chromium build is now blocked on either:
  - you entering the password interactively, or
  - you installing the listed packages yourself out of band

### What I learned
- The remaining pre-build blockers are now very clean:
  1. finish `gclient sync`
  2. satisfy the official Linux build packages
- The repo-side build path itself is no longer vague. The next unprivileged build command is already scripted.

### What was tricky to build
- The tricky part here was not technical complexity; it was separating "real blocker" from "not actually blocked." The missing packages are real, but the rest of the build flow can still be scripted and validated now. That is why it was worth adding the build driver even before the package install is complete.
- Another subtlety is that Chromium's dependency list includes some packages that look kiosk- or desktop-oriented rather than obviously direct-DRM-specific. That is Chromium's build ecosystem showing through; the script is still the right thing to trust for the baseline.

### What warrants a second pair of eyes
- Once the build-deps install is allowed to complete, the first `gclient runhooks` + `gn gen` run should be reviewed closely because that is where the first source-tree-specific configuration breakage is likely to appear.
- The missing package list from `--quick-check` is worth re-running after installation to confirm nothing was skipped inadvertently.

### What should be done in the future
- Finish the package install with `sudo`.
- Let `gclient sync` finish.
- Run the build driver helper.

### Code review instructions
- Start with:
  - `/home/manuel/code/wesen/2026-03-07--qemu-power-tick/host/build_phase4_chromium_targets.sh`
  - `/home/manuel/code/wesen/2026-03-07--qemu-power-tick/host/configure_phase4_chromium_build.sh`
- Validation commands:
```text
PATH=/home/manuel/depot_tools:$PATH /home/manuel/chromium/src/build/install-build-deps.sh --quick-check
bash -n /home/manuel/code/wesen/2026-03-07--qemu-power-tick/host/build_phase4_chromium_targets.sh
```

### Technical details
- Current build blockers:
```text
1. gclient sync still running
2. build/install-build-deps.sh needs sudo/password
```
- First scripted build entry point:
```text
/home/manuel/code/wesen/2026-03-07--qemu-power-tick/host/build_phase4_chromium_targets.sh
```

## Step 8: Finish the Chromium Sync and Run Hooks on the Real Source Tree

At this point the ticket stopped being hypothetical. The Chromium checkout was no longer just a bootstrap helper and a long-running fetch; it became a usable source tree. That matters because the rest of phase 4 depends on real Chromium build metadata rather than guesses from old blog posts or half-remembered flags.

The practical result of this step is simple: the reduced-history checkout path worked, the source tree under `/home/manuel/chromium/src` is now real enough to drive `gclient runhooks`, and the remaining build-path work moved from "checkout plumbing" into "Chromium configuration."

### Prompt Context

**User prompt (verbatim):** "ok it' sdone"

**Assistant interpretation:** The user finished the privileged `install-build-deps.sh` step, so the local blocker moved from package installation back to the unprivileged Chromium build path.

**Inferred user intent:** Resume immediately from the first point where unprivileged work can continue: verify the checkout state, finish sync/hooks, and push into the real build.

### What I did
- Confirmed the reduced-history sync completed successfully:
```text
PATH=/home/manuel/depot_tools:$PATH gclient sync --nohooks --no-history
```
- Ran the Chromium hooks successfully:
```text
PATH=/home/manuel/depot_tools:$PATH gclient runhooks
```
- Verified the hooks finished cleanly at:
```text
Running hooks: 113/113 (100%) done
```
- Checked the real tree and tool locations:
  - `/home/manuel/chromium/src/.gn`
  - `/home/manuel/chromium/src/buildtools/linux64/gn`
- Confirmed that the depot_tools `gn` wrapper was still not usable directly in this environment even though the source-tree `gn` binary exists and works.

### Why
- The ticket needed to move from source acquisition to source configuration.
- `gclient runhooks` is the line between "there is a checkout on disk" and "the checkout is ready to generate build files."
- I wanted to prove the official Chromium path itself was viable before editing more repo-side phase-4 code.

### What worked
- `gclient sync --nohooks --no-history` completed.
- `gclient runhooks` completed.
- The source tree now contains the expected build metadata and tool binaries.

### What didn't work
- The depot_tools `gn` wrapper still reported:
```text
gn.py: Unable to find gn in your $PATH
```
- That was not a real Chromium checkout failure, but it did mean the phase-4 helper needed to be less naive about where `gn` comes from.

### What I learned
- The reduced-history checkout path was the right call for this ticket; it got us to a buildable tree without dragging around the full history.
- The right source of truth for `gn` here is the checked-out Chromium tool binary, not the environment wrapper.

### What should be done in the future
- Update the phase-4 GN helper to use the real `src/buildtools/linux64/gn` binary when the wrapper is unreliable.
- Generate the first `out/Phase4DRM` build directory.
- Start the first `autoninja` compile.

### Technical details
- Working Chromium root:
```text
/home/manuel/chromium/src
```
- Verified local `gn` binary:
```text
/home/manuel/chromium/src/buildtools/linux64/gn
```

## Step 9: Correct the GN Args for Ozone DRM and Start the First Real Build

This was the first Chromium-specific configuration step that actually taught us something about the direct DRM path. The original helper was too optimistic: it assumed generic Linux Ozone settings would be enough and it assumed the depot_tools `gn` wrapper would be the only entry point we needed. Both assumptions were wrong.

The Chromium tree itself was explicit about the two fixes. First, the DRM platform configuration under this checkout wants the ChromeOS target path rather than a generic Linux build. Second, the source tree already contains a working `gn` binary, so the helper should use it directly instead of treating the wrapper as mandatory. Once those were fixed, `gn gen` succeeded and the first `autoninja` build became real.

### Prompt Context

**User prompt (verbatim):** "did you look on instructions on the web on how to build chrome and all that? what is depot_tools?" and then "save the instructions in the ticket with docmgr import file --file ... --ticket ... as well, for future reference. then continue, ocntinue keeping the diary"

**Assistant interpretation:** Use the official Chromium docs as the primary source, save them into the ticket, and then keep moving through the real build path instead of stalling at general explanations.

**Inferred user intent:** Validate the build process against upstream documentation and capture the exact local adjustments we needed so the next person does not have to rediscover them.

### What I did
- Read the official Chromium sources that matter for this path:
  - `/home/manuel/chromium/src/docs/linux/build_instructions.md`
  - `/home/manuel/chromium/src/docs/ozone_overview.md`
  - `/home/manuel/chromium/src/docs/linux/ozone_drm.md`
- Confirmed the source-tree `gn` binary works:
```text
/home/manuel/chromium/src/buildtools/linux64/gn --version
2341 (d8c2f07d6535)
```
- Updated `/home/manuel/code/wesen/2026-03-07--qemu-power-tick/host/configure_phase4_chromium_build.sh` so it:
  - writes `target_os = "chromeos"`
  - removes the bad `toolkit_views = false` assumption
  - uses `src/buildtools/linux64/gn` when the depot_tools wrapper is not usable
- Mirrored the updated helper into the ticket `scripts/` archive.
- Generated the first build directory successfully:
```text
PATH=/home/manuel/depot_tools:$PATH ./host/configure_phase4_chromium_build.sh --src /home/manuel/chromium/src --out /home/manuel/chromium/src/out/Phase4DRM
```
- Verified the generated build output exists:
```text
/home/manuel/chromium/src/out/Phase4DRM/build.ninja
```
- Started the first real build:
```text
PATH=/home/manuel/depot_tools:$PATH ./host/build_phase4_chromium_targets.sh --src /home/manuel/chromium/src --out /home/manuel/chromium/src/out/Phase4DRM --skip-hooks
```

### Why
- The phase-4 ticket needed to be tied to Chromium's actual documented DRM path, not a guessed Linux desktop configuration.
- `gn gen` is the first real proof that the build configuration is coherent.
- Starting `autoninja` is the clean boundary between "config path exists" and "payload may actually be buildable."

### What worked
- The source-tree `gn` binary worked reliably.
- The corrected helper produced `args.gn` and `build.ninja`.
- The first `autoninja` build is now genuinely running against the target set:
```text
content_shell
chrome_sandbox
chrome_crashpad_handler
```

### What didn't work
- The initial GN args were wrong in two ways:
  - forcing `toolkit_views = false` triggered a grit/toolkit assertion
  - omitting `target_os = "chromeos"` tripped:
```text
assert(is_chromeos, "Ozone DRM platform is ChromeOS-only")
```
- That means the old helper would have sent the next person down the wrong path if it had not been corrected now.

### What I learned
- For this checkout, "direct DRM/Ozone" is not just "Linux with a DRM flag." Chromium's own build graph wants the ChromeOS target path for the DRM backend.
- The upstream source tree is the best guide here. The local fixes matched the checked-in docs and the checked-in `BUILD.gn` behavior.

### What warrants a second pair of eyes
- Once the first build finishes, the actual runtime payload composition should be checked carefully:
  - `content_shell`
  - `chrome_sandbox`
  - `chrome_crashpad_handler`
  - `icudtl.dat`
  - `resources.pak`
  - `v8_context_snapshot.bin`
  - locale assets
- The eventual guest launcher flags should also be reviewed against the built binary's available switches, not just old assumptions from the Wayland phase.

### What should be done in the future
- Let the first `autoninja` run finish or fail with a concrete compiler error.
- Probe the output directory once artifacts appear.
- Wire that payload into `guest/build-phase4-rootfs.sh`.
- Attempt the first direct DRM/Ozone `content_shell` boot.

### Code review instructions
- Start with:
  - `/home/manuel/code/wesen/2026-03-07--qemu-power-tick/host/configure_phase4_chromium_build.sh`
  - `/home/manuel/code/wesen/2026-03-07--qemu-power-tick/host/build_phase4_chromium_targets.sh`
  - `/home/manuel/chromium/src/docs/linux/ozone_drm.md`
- Validation commands:
```text
/home/manuel/chromium/src/buildtools/linux64/gn --version
PATH=/home/manuel/depot_tools:$PATH ./host/configure_phase4_chromium_build.sh --src /home/manuel/chromium/src --out /home/manuel/chromium/src/out/Phase4DRM
PATH=/home/manuel/depot_tools:$PATH ./host/build_phase4_chromium_targets.sh --src /home/manuel/chromium/src --out /home/manuel/chromium/src/out/Phase4DRM --skip-hooks
```

### Technical details
- Corrected key args:
```text
target_os = "chromeos"
use_ozone = true
ozone_platform_drm = true
ozone_platform = "drm"
use_system_minigbm = false
```
- Generated build directory:
```text
/home/manuel/chromium/src/out/Phase4DRM
```

## Step 10: Script the Chromium Payload Staging Path Before the Build Finishes

By the time the first `autoninja` run started, the next bottleneck was already visible. Even if the compile succeeded, phase 4 would still need a clean way to move the Chromium runtime payload out of `out/Phase4DRM` and into the repo-local directory that `guest/build-phase4-rootfs.sh` already knows how to consume. That is the sort of step that easily turns into undocumented shell history if it is not scripted early.

So I used the build time to add the staging helper before the build produced artifacts. That keeps the workflow deterministic: build output directory in, staged phase-4 payload directory out, then probe the result immediately.

### Prompt Context

**User prompt (verbatim):** "alright, let's go then."

**Assistant interpretation:** Keep making forward progress instead of idling on the long Chromium compile.

**Inferred user intent:** Use the waiting time productively so the next post-build step is already automated when the compile finishes.

### What I did
- Added:
  - `/home/manuel/code/wesen/2026-03-07--qemu-power-tick/host/stage_phase4_chromium_payload.sh`
- Mirrored it into the ticket archive:
  - `/home/manuel/code/wesen/2026-03-07--qemu-power-tick/ttmp/2026/03/08/QEMU-06-CONTENT-SHELL-DRM-OZONE--chromium-content-shell-direct-drm-ozone-analysis-and-implementation/scripts/stage_phase4_chromium_payload.sh`
- The helper now:
  - reads from `~/chromium/src/out/Phase4DRM` by default
  - stages into `build/phase4/chromium-direct` by default
  - copies the first-pass required runtime set:
    - `content_shell`
    - `chrome_sandbox`
    - `icudtl.dat`
    - `resources.pak`
    - `v8_context_snapshot.bin`
    - `chrome_100_percent.pak`
    - `chrome_200_percent.pak`
    - `locales/`
  - copies optional helpers if present:
    - `chrome`
    - `chrome_crashpad_handler`
    - `nacl_helper`
  - immediately runs the existing payload probe on the staged directory

### Why
- The phase-4 rootfs builder is already designed around `CHROMIUM_PAYLOAD_DIR`; the only missing piece was turning Chromium's output directory into that payload consistently.
- This is a mechanical boundary, so it should be scripted once rather than improvised after the first successful build.

### What worked
- The packaging assumptions in `guest/build-phase4-rootfs.sh` already matched the staging helper well.
- The helper reuses `host/probe_phase4_chromium_payload.py` instead of inventing a second validation path.

### What didn't work
- I caught one bug immediately while writing it: the initial shell expansion would have passed `--allow-missing-payload` even when the flag was logically false because the variable was non-empty. I fixed that before using the script by switching to an explicit `probe_args` array.

### What I learned
- The phase-4 packaging path is in good shape already. The rootfs builder does not need a redesign; it just needs the Chromium output copied into the right shape.
- That means the next real post-build steps are straightforward:
  1. stage the payload
  2. probe it
  3. rebuild the phase-4 initramfs with `CHROMIUM_PAYLOAD_DIR`
  4. attempt the first direct DRM `content_shell` boot

### What should be done in the future
- Let `autoninja` finish.
- Run the new staging helper against `out/Phase4DRM`.
- Use the staged payload to build the first real Chromium-backed phase-4 initramfs.

### Code review instructions
- Start with:
  - `/home/manuel/code/wesen/2026-03-07--qemu-power-tick/host/stage_phase4_chromium_payload.sh`
  - `/home/manuel/code/wesen/2026-03-07--qemu-power-tick/guest/build-phase4-rootfs.sh`
  - `/home/manuel/code/wesen/2026-03-07--qemu-power-tick/host/probe_phase4_chromium_payload.py`

### Technical details
- Default staging destination:
```text
/home/manuel/code/wesen/2026-03-07--qemu-power-tick/build/phase4/chromium-direct
```
- Default source build directory:
```text
/home/manuel/chromium/src/out/Phase4DRM
```

## Step 11: Add Ozone Headless to the Same Build Before Spending More Compile Time

This was a good intervention point. The first `autoninja` run had already proven that the Chromium tree, hooks, and GN configuration were fundamentally viable, but it was still far from producing a finished browser payload. Since the user wants a fallback path for non-graphical validation too, it was worth stopping that build and broadening the compiled backend set before we sink more hours into the wrong configuration.

The key point from Chromium's own build files is that the runtime default backend and the compiled backend set are separate concerns. `ozone_platform = "drm"` controls the default runtime choice, while `ozone_platform_drm = true` and `ozone_platform_headless = true` decide which backends are built into the binary. That means we can keep DRM as the default path and still compile headless support into the same `content_shell` build.

### Prompt Context

**User prompt (verbatim):** "can we add headless too? so we have support for both? stop the current build and add it before it's too late. That way we can do headless tests as well if we struggle with the graphics."

**Assistant interpretation:** Stop the in-progress Chromium build, confirm whether Chromium can compile both Ozone DRM and Ozone headless support at once, and if so update the build config before restarting.

**Inferred user intent:** Avoid wasting more build time on a one-backend binary when a dual-backend binary would give us a safer debugging path later.

### What I did
- Stopped the long-running `autoninja` session with `Ctrl-C`.
- Read Chromium's checked-in Ozone configuration logic:
  - `/home/manuel/chromium/src/build/config/ozone.gni`
  - `/home/manuel/chromium/src/build/args/headless.gn`
- Confirmed from `ozone.gni` that:
  - `ozone_platform` is the default runtime backend
  - `ozone_platform_drm` controls whether DRM is compiled
  - `ozone_platform_headless` controls whether headless is compiled
- Updated:
  - `/home/manuel/code/wesen/2026-03-07--qemu-power-tick/host/configure_phase4_chromium_build.sh`
  - ticket mirror of the same script
- Changed the generated GN args from:
```text
ozone_platform_headless = false
```
to:
```text
ozone_platform_headless = true
```
- Re-generated the build directory:
```text
PATH=/home/manuel/depot_tools:$PATH ./host/configure_phase4_chromium_build.sh --src /home/manuel/chromium/src --out /home/manuel/chromium/src/out/Phase4DRM
```
- Verified the output config now contains:
```text
ozone_platform_headless = true
```

### Why
- The user is right that headless is a useful fallback lane if DRM/KMS bring-up becomes the bottleneck.
- The cheapest time to add that support is before the first full Chromium build completes.
- Chromium's own build graph allows both backends to be compiled into one binary, so there is no need to maintain two separate output trees yet.

### What worked
- The checked-in Chromium files were enough to answer the question without guessing.
- The `out/Phase4DRM/args.gn` regeneration succeeded cleanly.
- The final configuration keeps DRM as the default runtime backend while adding headless as a selectable backend.

### What didn't work
- The stopped build had already run much longer than the CLI progress samples suggested. When I finally interrupted it, the output showed it had continued far deeper into the graph than the earlier short polls made obvious.
- That was not a correctness problem, but it is a reminder that long-running `autoninja` sessions need occasional hard checkpoints rather than assuming the last visible progress line is current.

### What I learned
- For Chromium Ozone, "build both backends" and "default to one backend" are separate knobs.
- That is exactly what we want here: one binary that can run `--ozone-platform=drm` for the real QEMU target and `--ozone-platform=headless` for fallback validation.

### What was tricky to build
- The tricky part was not the GN edit itself; it was validating the semantics from Chromium's own files instead of assuming that `ozone_platform = "drm"` would exclude headless entirely. `build/config/ozone.gni` made the distinction explicit and prevented a bad guess.
- The other sharp edge was stopping the build cleanly. The session had much more buffered progress than the earlier short polls showed, so the interrupt only became visible after a large delayed flush.

### What warrants a second pair of eyes
- Once the restarted build finishes, we should confirm the resulting `content_shell` really accepts both runtime flags:
  - `--ozone-platform=drm`
  - `--ozone-platform=headless`
- The eventual payload probe should also compare the runtime asset set for both modes, especially if headless ends up wanting fewer external files than the DRM path.

### What should be done in the future
- Restart `autoninja` from the regenerated `out/Phase4DRM`.
- When artifacts exist, probe and stage the payload.
- Use the same built binary for:
  - direct DRM/Ozone testing in QEMU
  - headless fallback testing if graphics debugging blocks progress

### Code review instructions
- Start with:
  - `/home/manuel/chromium/src/build/config/ozone.gni`
  - `/home/manuel/chromium/src/build/args/headless.gn`
  - `/home/manuel/code/wesen/2026-03-07--qemu-power-tick/host/configure_phase4_chromium_build.sh`
- Validation commands:
```text
grep -n "ozone_platform_headless" /home/manuel/chromium/src/out/Phase4DRM/args.gn
sed -n '1,120p' /home/manuel/chromium/src/build/config/ozone.gni
```

### Technical details
- Current dual-backend Ozone settings:
```text
ozone_auto_platforms = false
ozone_platform_drm = true
ozone_platform_headless = true
ozone_platform_wayland = false
ozone_platform_x11 = false
ozone_platform = "drm"
```

## Step 12: Validate the Built Payload in Headless Mode and Break the First Real DRM Startup Barrier

This step is where phase 4 stopped being theoretical. The Chromium build finished, the first staged payload existed in-repo, and the first useful question became "is the browser payload itself valid?" rather than "can we compile Chromium at all?" I answered that by proving the staged `content_shell` works in host-side headless mode before returning to the much noisier QEMU DRM path.

The second important part of this step happened outside the repo: the first direct DRM runs showed that `content_shell` itself was missing a necessary Ozone `PostCreateMainMessageLoop()` handoff. I patched the local Chromium tree under `/home/manuel/chromium/src` so the browser gets far enough into Ozone DRM initialization for real GPU/runtime failures to surface.

### Prompt Context

**User prompt (verbatim):** (same ticket continuation; see Step 1)

**Assistant interpretation:** Continue phase 4, keep the diary current, and push through the first real direct-DRM bring-up instead of stopping at build/bootstrap work.

**Inferred user intent:** Turn the Chromium build into evidence: prove the payload is usable, then narrow the first direct DRM failure to a concrete layer instead of a vague "browser doesn't work" state.

### What I did
- Staged the built Chromium payload into:
  - `/home/manuel/code/wesen/2026-03-07--qemu-power-tick/build/phase4/chromium-direct`
- Added and mirrored:
  - `/home/manuel/code/wesen/2026-03-07--qemu-power-tick/host/run_phase4_headless_smoke.sh`
  - ticket-local mirror of the same script
- Validated the staged payload in host-side Ozone headless mode:
  - `results-phase4-headless-staged4/`
- Drove the first direct DRM/Ozone guest boots and recorded the initial failure boundary in:
  - `results-phase4-drm4/`
  - `results-phase4-drm7/`
  - `results-phase4-drm8/`
- Patched the local Chromium source tree outside this repo:
  - `/home/manuel/chromium/src/content/shell/browser/shell_browser_main_parts.cc`
- Rebuilt the local Chromium `content_shell` target after that patch and restaged the payload.

### Why
- Headless is the cheapest way to prove the built payload is not fundamentally broken.
- The direct DRM boot had to get past startup-order failures before runtime packaging and rendering issues could be debugged sanely.
- A missing Ozone hook in `content_shell` would have invalidated every later DRM runtime conclusion if it stayed unfixed.

### What worked
- The staged payload works in host-side headless mode, so the first browser binary/artifact boundary is now validated.
- The local Chromium patch removed the earlier Ozone/evdev startup crash and made the DRM logs materially better:
  - the browser authenticates `/dev/dri/card0`
  - the startup path reaches GPU initialization

### What didn't work
- The first direct DRM runs failed for multiple reasons before reaching stable GPU startup:
  - missing `/dev/shm`
  - incomplete staged payload entries
  - the missing Ozone `PostCreateMainMessageLoop()` handoff in `content_shell`
- Those failures showed up across the early result directories:
  - `results-phase4-drm4/`
  - `results-phase4-drm7/`
  - `results-phase4-drm8/`

### What I learned
- The repo-side phase-4 work was not enough by itself; the local Chromium source tree needed a small but real `content_shell` fix to make the direct DRM path meaningful.
- Headless validation is worth doing first because it cleanly separates "payload is bad" from "DRM runtime is bad."

### What was tricky to build
- The sharp edge here was provenance: the critical `content_shell` fix lives in the local Chromium checkout, not in this repo. That makes it easy to forget unless the diary records it explicitly.
- The other tricky part was resisting the temptation to treat every early DRM crash as a graphics issue. Several of them were actually startup-order or payload-layout bugs.

### What warrants a second pair of eyes
- The local Chromium patch is outside the repo, so it still needs to be preserved more formally than "it exists in `~/chromium/src`."
- It would be useful to capture the exact Chromium diff into the ticket or a reproducible patch file later.

### What should be done in the future
- Preserve the local Chromium `content_shell` Ozone patch in a tracked artifact.
- Keep using headless as the quickest "is the payload still valid?" control whenever phase-4 packaging changes.

### Code review instructions
- Start with:
  - `/home/manuel/code/wesen/2026-03-07--qemu-power-tick/host/run_phase4_headless_smoke.sh`
  - `/home/manuel/code/wesen/2026-03-07--qemu-power-tick/host/stage_phase4_chromium_payload.sh`
  - `/home/manuel/chromium/src/content/shell/browser/shell_browser_main_parts.cc`
- Validation artifacts:
  - `/home/manuel/code/wesen/2026-03-07--qemu-power-tick/results-phase4-headless-staged4/`
  - `/home/manuel/code/wesen/2026-03-07--qemu-power-tick/results-phase4-drm7/guest-serial.log`

### Technical details
- Useful output landmarks from this step:
```text
Succeeded authenticating /dev/dri/card0
content_shell ozone PostCreateMainMessageLoop runner=1
```

## Step 13: Package the Native Mesa/glvnd DRM Runtime and Untangle the Initramfs Size Trap

Once the startup hook issue was fixed, the next barrier was the Linux native graphics runtime itself. ANGLE on Linux did not just want Chromium's bundled `libEGL.so`; it wanted the native `libEGL.so.1` / `libGLdispatch.so.0` / Mesa vendor path as well. I followed that chain until the guest could finally reach `gpu initialization completed init_success:1`.

This step also exposed a separate harness problem: the Mesa-heavy guest became large enough that a half-written initramfs or an unlucky initrd placement looked like random Chromium regressions. That was not a browser bug. It was a guest-artifact / boot-transport problem, and writing it down matters because it changes how later runs have to be launched.

### Prompt Context

**User prompt (verbatim):** (same ticket continuation; see Step 1)

**Assistant interpretation:** Keep moving forward from the direct DRM failures, preserve the debugging trail, and fix the actual runtime dependencies instead of stopping at the first opaque EGL error.

**Inferred user intent:** Turn the first DRM bring-up into a disciplined dependency analysis so the ticket records exactly what Chromium needs in userspace and where the setup is still fragile.

**Commit (code):** `8b88ab1` — `Advance phase 4 DRM runtime bring-up`

### What I did
- Updated the payload/runtime probe:
  - `/home/manuel/code/wesen/2026-03-07--qemu-power-tick/host/probe_phase4_chromium_payload.py`
- Updated the payload staging helper:
  - `/home/manuel/code/wesen/2026-03-07--qemu-power-tick/host/stage_phase4_chromium_payload.sh`
- Expanded the phase-4 rootfs builder:
  - `/home/manuel/code/wesen/2026-03-07--qemu-power-tick/guest/build-phase4-rootfs.sh`
- Added host-side headless validation and mirrored it into the ticket scripts archive.
- Followed the native graphics dependency chain through these key failure signatures:
  - missing `libEGL.so.1`
  - missing `libGLdispatch.so.0`
  - bad recursion when `libEGL.so.1` pointed back at Chromium's own bundled `libEGL.so`
- Copied and staged:
  - native `libEGL.so.1`
  - native `libGLdispatch.so.0`
  - native `libEGL_mesa.so.0`
  - native `libGLESv2.so.2`
  - native `libgbm.so.1`
  - `egl_vendor.d/50_mesa.json`
  - a minimal DRI subset (`virtio_gpu_dri.so`, `kms_swrast_dri.so`, `swrast_dri.so`, `libdril_dri.so`)
- Recorded and then corrected multiple invalid runs caused by booting a partially written or oversized initramfs.

### Why
- ANGLE on Linux uses both Chromium's bundled frontend libs and the host-native Mesa/glvnd path.
- Without the native vendor side, "Chromium on DRM" never gets far enough to say anything meaningful about rendering.
- The initramfs-size issue needed to be recorded separately because otherwise it looked like a random Chromium regression.

### What worked
- The guest now gets past the earlier EGL loader failures and reaches:
```text
gpu initialization completed init_success:1
```
- The working evidence for that transition is in:
  - `/home/manuel/code/wesen/2026-03-07--qemu-power-tick/results-phase4-drm16/guest-serial.log`
  - `/home/manuel/code/wesen/2026-03-07--qemu-power-tick/results-phase4-drm18/guest-serial.log`
- The payload/runtime probe now reflects the real DRM runtime boundary instead of just the Chromium artifact boundary.

### What didn't work
- One bad experiment pointed `libEGL.so.1` back at Chromium's bundled `libEGL.so`, which caused the ANGLE recursion/assert path rather than fixing the native runtime.
- Several later runs were invalid because I reused an initramfs before the rebuild had actually finished. That produced "Initramfs unpacking failed: read error" failures that were easy to misread as browser regressions.

### What I learned
- Chromium DRM bring-up in this configuration is a combined payload problem and native graphics-runtime problem.
- The current initramfs-based transport is fragile once the Mesa runtime gets large; future direct DRM work should either slim the image or stop treating initramfs as the long-term rootfs.

### What was tricky to build
- The dependency split is counterintuitive: Chromium bundles `libEGL.so`, but ANGLE still expects the Linux native `libEGL.so.1` / `libGLdispatch.so.0` / Mesa vendor side.
- The other sharp edge was tool control: I initially treated the build-session ID like a process ID and convinced myself the initramfs rebuild had finished when it had not. That led directly to invalid boot results.

### What warrants a second pair of eyes
- The current Mesa-heavy initramfs is usable for debugging but not robust. A second review should challenge whether phase 4 should stay initramfs-based at all.
- The runtime packaging is now broad enough that it deserves one more pass for unnecessary bloat.

### What should be done in the future
- Either reduce the guest artifact size or move to a non-initramfs rootfs transport.
- Preserve the exact "working enough to get `init_success:1`" runtime boundary in the guide and scripts.

### Code review instructions
- Start with:
  - `/home/manuel/code/wesen/2026-03-07--qemu-power-tick/guest/build-phase4-rootfs.sh`
  - `/home/manuel/code/wesen/2026-03-07--qemu-power-tick/host/probe_phase4_chromium_payload.py`
  - `/home/manuel/code/wesen/2026-03-07--qemu-power-tick/host/stage_phase4_chromium_payload.sh`
- Validation artifacts:
  - `/home/manuel/code/wesen/2026-03-07--qemu-power-tick/results-phase4-drm11/guest-serial.log`
  - `/home/manuel/code/wesen/2026-03-07--qemu-power-tick/results-phase4-drm16/guest-serial.log`

### Technical details
- Current direct-DRM runtime boundary includes both:
```text
/usr/lib/chromium-direct/libEGL.so
/usr/lib/libEGL.so.1
/usr/share/glvnd/egl_vendor.d/50_mesa.json
/usr/lib/x86_64-linux-gnu/dri/virtio_gpu_dri.so
```

## Step 14: Prove That Chromium Starts on DRM While the Host-visible Frame Stays Black

This is the current phase-4 state. The browser does start. The GPU process does initialize. Ozone DRM finds the render node. The guest-side connector state remains stable for the whole run. And despite all of that, the host-visible QMP screenshot is still entirely black. That is a much better problem than "Chromium crashes on startup," because it narrows the investigation to presentation/scanout visibility instead of generic bring-up.

I also fixed a few runtime-hygiene problems in this step: fonts were being copied into the wrong path, Chromium was trying to use `/root/.cache`, and the content-shell log tail could race file creation and hide the only useful error output. Those are now cleaned up, so the remaining black-frame result is harder to dismiss as a trivial setup bug.

### Prompt Context

**User prompt (verbatim):** (same ticket continuation; see Step 1)

**Assistant interpretation:** Continue iterating on the direct DRM path, keep the diary detailed, and produce a trustworthy checkpoint instead of a hand-wavy "it still doesn't render."

**Inferred user intent:** Distinguish between "browser/GPU startup works" and "browser content is actually visible," then document the exact current state so the next debugging step starts from facts instead of guesses.

**Commit (code):** `8b88ab1` — `Advance phase 4 DRM runtime bring-up`

### What I did
- Fixed phase-4 font layout and runtime dirs in:
  - `/home/manuel/code/wesen/2026-03-07--qemu-power-tick/guest/build-phase4-rootfs.sh`
  - `/home/manuel/code/wesen/2026-03-07--qemu-power-tick/guest/content-shell-drm-launcher.sh`
  - `/home/manuel/code/wesen/2026-03-07--qemu-power-tick/guest/init-phase4-drm`
- Added the guest display probe path using:
  - `/home/manuel/code/wesen/2026-03-07--qemu-power-tick/guest/display_probe.sh`
  - mirrored ticket-local script
- Ran the key validation/control runs:
  - `results-phase4-drm18/`
  - `results-phase4-drm19/`
  - `results-phase4-drm20/`
- Compared early and late captures and confirmed they are identical black frames.
- Used the display probe to record:
  - `fb0=virtio_gpudrmfb`
  - `proc_fb=0 virtio_gpudrmfb`
  - `card0-Virtual-1 status=connected enabled=enabled dpms=On`
  - `vtcon1` frame-buffer console still bound

### Why
- The biggest risk at this point was false confidence. "GPU init succeeded" does not mean "the browser is visibly on screen."
- The display probe gives the next intern a guest-side baseline even if the host screenshot continues to disagree with what Chromium claims to be doing.

### What worked
- Fontconfig and cache path noise were reduced:
  - the previous `Fontconfig error: Cannot load default config file` message disappeared after the `/etc/fonts` copy fix
  - the previous `/root/.cache` permission failure disappeared after the launcher got explicit state/cache dirs
- The browser/GPU path now looks stable:
```text
Succeeded authenticating /dev/dri/card0
gpu initialization completed init_success:1
Found Gbm Device at /dev/dri/renderD128
```
- The guest-side display probe stayed stable for the full run in `results-phase4-drm20/guest-serial.log`.

### What didn't work
- The host-visible frame is still black, even with a later capture delay.
- `results-phase4-drm18/00-smoke.png` and `results-phase4-drm19/00-smoke.png` are byte-for-byte equivalent and effectively black.
- The browser still logs:
```text
libEGL warning: NEEDS EXTENSION: falling back to kms_swrast
components/leveldb_proto/public/proto_database_provider.h:101 In memory database cannot use the given database directory
```

### What I learned
- Phase 4 is no longer blocked on browser startup.
- The remaining bug is presentation visibility: either Chromium never binds a visible plane, or QEMU is still not showing the plane Chromium is using.
- The display probe is useful but not sufficient by itself because it shows only `fb0` / connector / vtconsole state, not DRM plane ownership or `FB_ID`.

### What was tricky to build
- The subtle bug here was the log-tail race in `init-phase4-drm`. Without pre-creating the log file and switching to `tail -F`, the browser could fail silently and leave the run looking healthier than it really was.
- The other tricky part was interpreting black screenshots after the startup path had become healthy. That requires resisting the instinct to keep "fixing startup" and instead moving to deeper display-state inspection.

### What warrants a second pair of eyes
- The current display probe may still be too shallow. A second pair of eyes should look at whether the next probe should dump `/sys/kernel/debug/dri/0/state` or another plane/FB-oriented source of truth.
- The remaining `leveldb_proto` warning may or may not matter; it should not be assumed benign without one more look.

### What should be done in the future
- Add a deeper DRM plane/FB debugfs probe.
- Decide whether the black frame is a Chromium presentation bug or another host-visible capture/scanout mismatch.
- Get the first unmistakably non-black frame on the host path before touching suspend work.

### Code review instructions
- Start with:
  - `/home/manuel/code/wesen/2026-03-07--qemu-power-tick/guest/content-shell-drm-launcher.sh`
  - `/home/manuel/code/wesen/2026-03-07--qemu-power-tick/guest/init-phase4-drm`
  - `/home/manuel/code/wesen/2026-03-07--qemu-power-tick/guest/display_probe.sh`
- Validation artifacts:
  - `/home/manuel/code/wesen/2026-03-07--qemu-power-tick/results-phase4-drm18/`
  - `/home/manuel/code/wesen/2026-03-07--qemu-power-tick/results-phase4-drm19/`
  - `/home/manuel/code/wesen/2026-03-07--qemu-power-tick/results-phase4-drm20/guest-serial.log`

### Technical details
- Stable probe line shape from `results-phase4-drm20/guest-serial.log`:
```text
@@DISPLAY ... fb0=virtio_gpudrmfb ... drm=card0-Virtual-1 status=connected enabled=enabled dpms=On
```
- Current host-visible frame result:
```text
00-smoke.png: bbox None, nonblack 0
```
