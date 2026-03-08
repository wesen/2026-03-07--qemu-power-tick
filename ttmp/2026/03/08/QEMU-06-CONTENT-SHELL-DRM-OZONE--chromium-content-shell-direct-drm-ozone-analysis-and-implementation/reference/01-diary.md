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
    - Path: guest/build-phase3-rootfs.sh
      Note: Phase-3 baseline reviewed during the initial design step
    - Path: guest/chromium-wayland-launcher.sh
      Note: Current Wayland launcher contrasted against the planned direct DRM launcher
    - Path: guest/init-phase3
      Note: Phase-3 baseline reviewed during the initial design step
    - Path: host/bootstrap_chromium_checkout.sh
      Note: Reusable Chromium checkout bootstrap helper created in Step 2
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

### What I learned
- The machine setup risk is now much narrower than before: local disk and `depot_tools` are fine.
- The first real external bottleneck is Chromium remote checkout latency, not local packaging or missing helper tooling.
- Creating the helper before running the fetch was the right move because it turned the environment work into a reviewable artifact immediately.

### What was tricky to build
- The tricky part was distinguishing "scriptable setup" from "long-running remote sync." Those are different failure domains, and the ticket needs to preserve that difference. The helper had to be idempotent so later reruns do not turn into a second undocumented workflow once `/home/manuel/chromium` exists.
- Another subtlety is that the first `fetch chromium` path is quiet for long stretches. Without checking the live process tree and the appearance of `.gclient`, it would have been too easy to misclassify the command as hung.

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
