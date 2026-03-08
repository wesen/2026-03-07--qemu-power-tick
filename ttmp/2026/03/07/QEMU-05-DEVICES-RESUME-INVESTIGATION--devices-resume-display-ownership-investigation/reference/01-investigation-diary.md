---
Title: Investigation Diary
Ticket: QEMU-05-DEVICES-RESUME-INVESTIGATION
Status: active
Topics:
    - qemu
    - linux
    - power-management
    - wayland
    - chromium
DocType: reference
Intent: long-term
Owners: []
RelatedFiles:
    - Path: guest/build-phase2-rootfs.sh
      Note: Phase-2 rootfs rebuild step that added the display probe
    - Path: guest/build-phase3-rootfs.sh
      Note: Phase-3 rootfs rebuild step that added the display probe
    - Path: guest/init-phase2
      Note: Diary evidence for the corrected fbcon-unbind experiment
    - Path: guest/init-phase3
      Note: Diary evidence for the stage-3 probe and log-only unbind attempt
ExternalSources: []
Summary: ""
LastUpdated: 2026-03-07T22:12:20.785209247-05:00
WhatFor: ""
WhenToUse: ""
---


# Investigation Diary

## Goal

This diary records the first narrow follow-up ticket for the `pm_test=devices` resume failure. Its purpose is to preserve the exact experiments, commands, failures, and conclusions that separate the shared stage-2/stage-3 display fallback from the extra Chromium-specific DRM errors.

## Step 1: Split the Resume Investigation and Add a Display Probe

The stage-2 and stage-3 tickets had already converged on a useful but awkward result: both stacks showed the same post-resume screenshot fallback, while Chromium sometimes added extra `virtio_gpu_dequeue_ctrl_func ... response 0x1203` noise. That was enough evidence to stop piling more work into the older tickets and move the next debug loop into a smaller, purpose-built investigation workspace.

The first concrete goal in this new ticket was to instrument the guest without contaminating the graphics path. Earlier debug techniques that continuously tailed logs to `/dev/console` had already proven they could interfere with the very problem being studied. The probe therefore writes to a file during runtime and only gets dumped to serial after the client exits.

### Prompt Context

**User prompt (verbatim):** "ok, make a new investigation ticket to continue, to avoid having one overblown ticket. Write the tasks for the next tests in there and execute them. Then write a detailed analysis in the new ticket fr our intern to follow along: Create a detailed analysis guide that is very detailed for a new intern, explaining all the parts of the system needed to understand what it is, with prose paragraphs and bullet point sand pseudocode and diagrams and api references and file
  references.
  It should be very clear and detailed. Store in the ticket and the nupload to remarkable."

**Assistant interpretation:** Create a fresh ticket dedicated to the `pm_test=devices` resume investigation, execute the next targeted experiments there, and produce a self-contained guide that an intern can use without reconstructing prior chat context.

**Inferred user intent:** Reduce ticket sprawl, preserve evidence, and turn the current understanding into a usable investigation package rather than another ad hoc debugging thread.

### What I did
- Created the dedicated ticket workspace `QEMU-05-DEVICES-RESUME-INVESTIGATION`.
- Wrote the initial task list in `tasks.md`.
- Added [guest/display_probe.sh](../../../../../../guest/display_probe.sh), a guest-side sampler for:
  - `/sys/class/vtconsole/vtcon*/name`
  - `/sys/class/vtconsole/vtcon*/bind`
  - `/sys/class/graphics/fb0/name`
  - `/proc/fb`
  - `/sys/class/drm/card*-*/status`
  - `/sys/class/drm/card*-*/enabled`
  - `/sys/class/drm/card*-*/dpms`
- Updated [guest/build-phase2-rootfs.sh](../../../../../../guest/build-phase2-rootfs.sh) and [guest/build-phase3-rootfs.sh](../../../../../../guest/build-phase3-rootfs.sh) to copy `display_probe.sh` into the initramfs.
- Updated [guest/init-phase2](../../../../../../guest/init-phase2) and [guest/init-phase3](../../../../../../guest/init-phase3) to:
  - parse `display_probe=1`
  - parse `display_unbind_fbcon=1`
  - start the probe in the background
  - dump `/var/log/display_probe.log` to serial on shutdown

### Why
- The earlier tickets had grown too broad.
- We needed a probe that could answer a specific question:
  is the visible post-resume fallback caused by fbcon ownership, by a lower DRM scanout problem, or by a client/compositor layer above that?

### What worked
- The new probe produced stable, parseable `@@DISPLAY` lines in the phase-2 control runs.
- The probe did not require continuous writes to `/dev/console` during runtime.
- The ticket-local `scripts/` directory now contains the relevant probe and init/build scripts so the investigation is reproducible.

### What didn't work
- The stage-3 probe runs did not emit visible `@@DISPLAY` lines into the serial log even though the same runtime hook exists in [guest/init-phase3](../../../../../../guest/init-phase3). That makes the stage-3 probe evidence weaker than the phase-2 evidence in this first pass.

### What I learned
- Runtime instrumentation itself is a variable. If the debug method writes into the active display/console path, it can distort the resume bug being measured.
- A file-backed probe plus end-of-run dump is a safer pattern for this class of graphics investigation.

### What was tricky to build
- The `display_unbind_fbcon=1` hook looked trivial at first, but the correct `vtconsole` is not guaranteed to be `vtcon1`. The first version unbound the wrong console.
- I fixed that by matching the `vtconsole` whose `name` contains `frame buffer`, instead of hardcoding an index.

### What warrants a second pair of eyes
- The absence of `@@DISPLAY` output in the phase-3 serial logs may be a logic gap, a logging-order issue, or a real path difference.
- The init scripts now have more debug behavior; they are small enough to review directly and should be checked for sequencing mistakes.

### What should be done in the future
- Validate why the phase-3 probe branch does not visibly dump `@@DISPLAY` lines.
- Consider redirecting the probe to a file copied out through another channel if serial dumping remains unreliable in phase 3.

### Code review instructions
- Start with [guest/display_probe.sh](../../../../../../guest/display_probe.sh), then inspect the probe wiring in [guest/init-phase2](../../../../../../guest/init-phase2) and [guest/init-phase3](../../../../../../guest/init-phase3).
- Validate by rebuilding both rootfs images and running one phase-2 probe control and one phase-3 probe control with `display_probe=1`.

### Technical details
- Rebuild commands used:
```bash
bash guest/build-phase2-rootfs.sh build build/initramfs-phase2.img
bash guest/build-phase3-rootfs.sh build build/initramfs-phase3.img
```
- Probe output format:
```text
@@DISPLAY sample=N uptime_s=... fb0=... proc_fb=... vt=... drm=...
```

## Step 2: Run the Control Matrix and Compare Probe Output to Screenshots

With the probe in place, I ran the smallest useful control set instead of jumping back into full Chromium debugging. The point of the matrix was to separate shared behavior from layer-specific behavior: phase 2 as the shared Wayland control, phase 3 with `weston-simple-shm` as a compositor-only stage-3 control, and phase 3 with Chromium as the full stack.

The strongest result here came from the phase-2 control, because it gave both screenshots and probe samples. The visible screenshot fell back from a `1280x800` graphical plane before suspend to a `720x400` firmware-looking text plane after resume, yet the guest-side probe kept reporting stable `fb0`, `vtconsole`, and DRM connector state across the same interval.

### Prompt Context

**User prompt (verbatim):** (same as Step 1)

**Assistant interpretation:** Execute the next narrow tests inside the new ticket and preserve the evidence, not just the conclusion.

**Inferred user intent:** Produce a credible, replayable investigation baseline before any further design claims are made.

### What I did
- Rebuilt phase-2 and phase-3 initramfs images with the probe included.
- Ran the phase-2 control:
```bash
bash guest/run-qemu-phase2.sh \
  --kernel build/vmlinuz \
  --initramfs build/initramfs-phase2.img \
  --results-dir results-phase2-probe1 \
  --append-extra 'display_probe=1 phase2_idle_seconds=5 phase2_max_suspend_cycles=1 phase2_wake_seconds=5 phase2_pm_test=devices phase2_runtime_seconds=20'
```
- Captured phase-2 pre/post screenshots:
```bash
python3 host/capture_phase3_suspend_checkpoints.py \
  --socket results-phase2-probe1/qmp.sock \
  --serial-log results-phase2-probe1/guest-serial.log \
  --results-dir results-phase2-probe1 \
  --pre-delay-seconds 4.0 \
  --post-resume-delay-seconds 1.0 \
  --pre-name 00-pre \
  --post-name 01-post
```
- Ran the phase-3 `weston-simple-shm` control:
```bash
bash guest/run-qemu-phase3.sh \
  --kernel build/vmlinuz \
  --initramfs build/initramfs-phase3.img \
  --results-dir results-phase3-probe-shm1 \
  --append-extra 'display_probe=1 phase3_client=weston-simple-shm phase3_runtime_seconds=35 phase3_suspend_delay_seconds=10 phase3_wake_seconds=5 phase3_pm_test=devices'
```
- Ran the phase-3 Chromium control:
```bash
bash guest/run-qemu-phase3.sh \
  --kernel build/vmlinuz \
  --initramfs build/initramfs-phase3.img \
  --results-dir results-phase3-probe-chromium1 \
  --append-extra 'display_probe=1 phase3_runtime_seconds=40 phase3_url=data:text/html;base64,... phase3_suspend_delay_seconds=10 phase3_wake_seconds=5 phase3_pm_test=devices'
```

### Why
- We needed one shared-control run with strong guest-side evidence.
- We also needed to know whether the `virtio_gpu_dequeue_ctrl_func ... 0x1203` errors were unique to Chromium or already present in a simpler phase-3 client.

### What worked
- The phase-2 control produced the cleanest evidence:
  - `00-pre.png` was a `1280x800` graphical frame.
  - `01-post.png` was a `720x400` firmware-looking text frame.
  - `@@DISPLAY` stayed stable before and after resume:
    - `fb0=virtio_gpudrmfb`
    - `proc_fb=0 virtio_gpudrmfb`
    - `vtcon0 name=frame buffer device bind=1`
    - `vtcon1 name=dummy device bind=0`
    - `card0-Virtual-1 status=connected enabled=enabled dpms=On`
- The phase-3 `weston-simple-shm` control reproduced the same screenshot pattern and also showed:
  - `virtio_gpu_dequeue_ctrl_func ... response 0x1203 (command 0x105)`
  - `virtio_gpu_dequeue_ctrl_func ... response 0x1203 (command 0x104)`
- The phase-3 Chromium control reproduced the same screenshot pattern but, in this specific probe run, did not visibly print the same `virtio_gpu_dequeue_ctrl_func` lines in the truncated inspection pass.

### What didn't work
- The stage-3 probe runs did not yield visible `@@DISPLAY` lines in the serial log.
- That means the strongest probe-backed conclusion is still phase-2-based.

### What I learned
- The shared screenshot fallback is not enough to prove fbcon rebinding by itself.
- At least in phase 2, the guest-side ownership indicators stay stable while QMP `screendump` still switches to a firmware-looking plane.
- That points toward a deeper scanout/plane restoration problem, or a QMP-visible-plane problem, not just a `vtconsole` ownership toggle.

### What was tricky to build
- The phase-3 runs are harder to interpret than phase 2 because the probe dump is missing there, so it is easier to overfit to screenshot behavior alone.
- I had to keep the control claims asymmetric: phase 2 gives strong probe evidence, phase 3 gives weaker screenshot-and-error evidence.

### What warrants a second pair of eyes
- Review whether the stage-3 `display_probe` branch actually runs and flushes the log before poweroff.
- Review whether the `virtio_gpu` error lines in `results-phase3-probe-shm1` are enough to collapse the earlier “Chromium-only” hypothesis.

### What should be done in the future
- Add one direct stage-3 probe validation run whose only goal is to prove `@@DISPLAY` dumping works.
- Compare the final framebuffer dimensions captured by QMP against what the DRM backend thinks is active.

### Code review instructions
- Start with the control artifacts:
  - `results-phase2-probe1/00-pre.png`
  - `results-phase2-probe1/01-post.png`
  - `results-phase2-probe1/guest-serial.log`
  - `results-phase3-probe-shm1/guest-serial.log`
  - `results-phase3-probe-chromium1/guest-serial.log`
- Then cross-check the capture helper [host/capture_phase3_suspend_checkpoints.py](../../../../../../host/capture_phase3_suspend_checkpoints.py).

### Technical details
- Phase-2 metrics:
  - `sleep_duration=5759 ms`
  - `suspend_resume_gap=5759 ms`
  - `resume_to_redraw=6 ms`
- Key phase-2 probe lines:
```text
@@DISPLAY sample=1 uptime_s=0.76 fb0=virtio_gpudrmfb ... vt=vtcon0{name=frame_buffer_device,bind=1} vtcon1{name=dummy_device,bind=0} drm=card0-Virtual-1{status=connected,enabled=enabled,dpms=On}
@@DISPLAY sample=6 uptime_s=11.54 fb0=virtio_gpudrmfb ... vt=vtcon0{name=frame_buffer_device,bind=1} vtcon1{name=dummy_device,bind=0} drm=card0-Virtual-1{status=connected,enabled=enabled,dpms=On}
```

## Step 3: Correct the fbcon Unbind Experiment and Learn What It Actually Changes

The `display_unbind_fbcon=1` experiment was meant to test a very specific theory: maybe the firmware-looking post-resume screenshot is simply the framebuffer console winning ownership again. The first attempt failed because I unbound the wrong `vtconsole`, which would have produced a false negative if I had not checked the probe output carefully.

The corrected run turned into the most useful single experiment of the ticket so far. It changed the pre-suspend visible plane dramatically, but it still did not preserve the graphical frame after resume. That narrows the problem more than either a pure screenshot diff or a pure log diff by itself.

### Prompt Context

**User prompt (verbatim):** (same as Step 1)

**Assistant interpretation:** Keep the next tests small, verify each hypothesis directly, and record failures instead of smoothing them over.

**Inferred user intent:** Make the investigation trustworthy enough that a new contributor can continue it without repeating avoidable mistakes.

### What I did
- Ran an initial unbind experiment:
```bash
bash guest/run-qemu-phase2.sh \
  --kernel build/vmlinuz \
  --initramfs build/initramfs-phase2.img \
  --results-dir results-phase2-probe-unbind1 \
  --append-extra 'display_probe=1 display_unbind_fbcon=1 phase2_idle_seconds=5 phase2_max_suspend_cycles=1 phase2_wake_seconds=5 phase2_pm_test=devices phase2_runtime_seconds=20'
```
- Inspected the log and found:
  - `[init-phase2] unbound vtcon1 framebuffer console`
  - but the probe still showed `vtcon0 name=frame buffer device bind=1`
- Fixed the init logic to unbind whichever `vtconsole` name matched `frame buffer`.
- Rebuilt the phase-2 initramfs.
- Reran the corrected experiment:
```bash
bash guest/run-qemu-phase2.sh \
  --kernel build/vmlinuz \
  --initramfs build/initramfs-phase2.img \
  --results-dir results-phase2-probe-unbind2 \
  --append-extra 'display_probe=1 display_unbind_fbcon=1 phase2_idle_seconds=5 phase2_max_suspend_cycles=1 phase2_wake_seconds=5 phase2_pm_test=devices phase2_runtime_seconds=20'
```
- Captured the corrected pre/post screenshots with the same helper.
- Also ran one stage-3 log-only unbind attempt:
  - `results-phase3-probe-chromium-unbind1`
  - this finished before the post-hoc screenshot helper could attach

### Why
- If fbcon ownership alone explains the fallback, unbinding the framebuffer console should preserve the graphical plane after resume.
- If unbinding changes the pre-suspend screenshot but not the post-resume result, then the theory is only partially right.

### What worked
- The corrected phase-2 unbind run showed:
  - `[init-phase2] unbound vtcon0 framebuffer console`
  - probe state changed to:
    - `vtcon0 bind=0`
    - `vtcon1 bind=1`
  - `00-pre.png` changed from a nearly black `1280x800` frame to a clearly visible dashboard-like graphical frame
  - `01-post.png` still fell back to the `720x400` firmware-looking text plane
- The QMP image difference was very large:
  - baseline `resume_ae=3657`
  - corrected unbind `resume_ae≈1.024e+06`

### What didn't work
- The first unbind attempt targeted the wrong `vtconsole`.
- The stage-3 unbind attempt did not produce a valid screenshot pair because the helper started after QEMU had already exited.
- Exact error from the failed capture attempt:
```text
timed out waiting for qmp socket: results-phase3-probe-chromium-unbind1/qmp.sock
```

### What I learned
- fbcon ownership definitely affects what QMP `screendump` sees before suspend.
- fbcon ownership alone does not explain the post-resume fallback.
- The remaining bug is likely lower-level:
  - DRM plane/scanout restoration
  - virtio-gpu resource state after resume
  - or QEMU `screendump` observing a fallback plane rather than Weston’s intended scanout

### What was tricky to build
- A wrong unbind target can easily masquerade as a meaningful result unless the probe validates the new ownership state.
- Post-hoc capture is fragile if the VM exits quickly; for future stage-3 unbind work, the capture helper needs to run concurrently with QEMU.

### What warrants a second pair of eyes
- The corrected unbind result should be reviewed by comparing:
  - the screenshots
  - the probe lines
  - the dimensions reported by `file`
- The stage-3 unbind path should be rerun with a concurrent capture harness before stronger claims are made.

### What should be done in the future
- Repeat the stage-3 unbind experiment with capture running concurrently.
- Add a smaller probe that records whether fbcon or DRM state changes immediately after resume and before the client redraws.

### Code review instructions
- Review [guest/init-phase2](../../../../../../guest/init-phase2) and [guest/init-phase3](../../../../../../guest/init-phase3) around the `display_unbind_fbcon` block.
- Validate the corrected behavior by checking `results-phase2-probe-unbind2/guest-serial.log` and the paired screenshots.

### Technical details
- Screenshot file metadata:
```text
results-phase2-probe1/00-pre.png:        1280 x 800
results-phase2-probe1/01-post.png:        720 x 400
results-phase2-probe-unbind2/00-pre.png: 1280 x 800
results-phase2-probe-unbind2/01-post.png: 720 x 400
```
- Corrected unbind probe lines:
```text
@@DISPLAY sample=1 ... vt=vtcon0{name=frame_buffer_device,bind=0} vtcon1{name=dummy_device,bind=1} ...
@@DISPLAY sample=6 ... vt=vtcon0{name=frame_buffer_device,bind=0} vtcon1{name=dummy_device,bind=1} ...
```

## Step 4: Validate the Ticket and Upload the Bundle

Once the guide, diary, and script archive were in place, the last step was to make the ticket usable as a handoff artifact instead of a local scratchpad. That meant fixing the `docmgr` vocabulary warnings rather than deleting topics, validating the ticket cleanly, committing the work, and uploading a bundle that includes the overview, guide, diary, tasks, and changelog.

This step is boring compared to the probe experiments, but it matters. A ticket that cannot pass `doctor`, cannot be located on reMarkable, or cannot be replayed from its own scripts is not a good handoff, no matter how interesting the debugging was.

### Prompt Context

**User prompt (verbatim):** (same as Step 1)

**Assistant interpretation:** Finish the ticket as a usable package, not just a local branch of experiments.

**Inferred user intent:** Make the investigation durable for another person to pick up.

**Commit (code):** `634eddb` — `Add devices resume probe investigation`

### What I did
- Related the key files to the guide and diary with `docmgr doc relate`.
- Added missing vocabulary topics:
  - `chromium`
  - `linux`
  - `power-management`
  - `wayland`
- Ran:
```bash
docmgr doctor --ticket QEMU-05-DEVICES-RESUME-INVESTIGATION --stale-after 30
```
- Committed the probe wiring and ticket docs.
- Ran a dry-run bundle upload, then the real upload:
```bash
remarquee upload bundle --dry-run ... --name 'QEMU-05 Devices Resume Investigation Bundle' --remote-dir '/ai/2026/03/07/QEMU-05-DEVICES-RESUME-INVESTIGATION' --toc-depth 2
remarquee upload bundle ... --name 'QEMU-05 Devices Resume Investigation Bundle' --remote-dir '/ai/2026/03/07/QEMU-05-DEVICES-RESUME-INVESTIGATION' --toc-depth 2
remarquee cloud ls /ai/2026/03/07/QEMU-05-DEVICES-RESUME-INVESTIGATION --long --non-interactive
```

### Why
- The ticket needed to be valid and portable.
- The user explicitly asked for the bundle to be uploaded to reMarkable.

### What worked
- `docmgr doctor` passed cleanly after the vocabulary fix.
- The bundle upload succeeded.
- The remote listing shows:
```text
[f]    QEMU-05 Devices Resume Investigation Bundle
```

### What didn't work
- I raced the first remote listing call against the in-flight upload and got:
```text
Error: no matches for 'QEMU-05-DEVICES-RESUME-INVESTIGATION'
```
- Rerunning the listing after the upload completed fixed that.

### What I learned
- `docmgr` vocabulary drift is worth fixing directly when the terms are legitimate, instead of flattening the metadata to whatever already exists.
- The upload/listing sequence needs to respect timing; checking the remote path before the upload lands can produce a misleading false failure.

### What was tricky to build
- The interesting part here was not the commands themselves but the discipline of turning a live investigation into a reliable handoff package.
- The ticket now has the necessary minimum:
  - overview
  - guide
  - diary
  - tasks
  - changelog
  - mirrored scripts

### What warrants a second pair of eyes
- The remaining technical gap is still the unresolved stage-3 concurrent-capture unbind run.
- The documentation itself is now in good enough shape for review and continuation.

### What should be done in the future
- Do the pending stage-3 concurrent-capture unbind experiment as the next focused step.

### Code review instructions
- Run `docmgr doctor` again and confirm it passes.
- Check the remote listing under `/ai/2026/03/07/QEMU-05-DEVICES-RESUME-INVESTIGATION`.
- Review commit `634eddb` for the exact code and ticket payload.

### Technical details
- Validation result:
```text
## Doctor Report (1 findings)

### QEMU-05-DEVICES-RESUME-INVESTIGATION

- ✅ All checks passed
```
- Upload artifact name:
```text
QEMU-05 Devices Resume Investigation Bundle
```
