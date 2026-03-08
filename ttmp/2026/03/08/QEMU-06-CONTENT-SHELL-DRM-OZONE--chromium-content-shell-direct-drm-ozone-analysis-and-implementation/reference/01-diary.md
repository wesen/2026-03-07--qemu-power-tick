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
