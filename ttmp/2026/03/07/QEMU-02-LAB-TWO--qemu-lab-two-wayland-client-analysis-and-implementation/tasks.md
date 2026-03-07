# Tasks

## Phase 2 Plan

- [ ] Normalize the imported `lab2.md` source, update ticket metadata, and relate the source brief to the phase-2 design doc.
- [x] Write the phase-2 analysis and implementation guide with architecture, APIs, pseudocode, validation plan, and a bonus path for making the client own more of early boot responsibilities later.
- [x] Create a ticket-local `scripts/` directory and copy every new helper script there as phase-2 work proceeds.
- [x] Decide the guest packaging strategy for Weston and its runtime dependencies, document the tradeoffs, and implement the chosen path.
- [x] Build a phase-2 guest rootfs/initramfs that boots reliably, configures devices, and can launch Weston plus one custom Wayland client.
- [x] Add a minimal Wayland client that renders state, tracks packet/suspend/input events, and redraws only when state changes.
- [x] Add host-side QMP automation for screendump, keyboard injection, pointer injection, wake attempts, and scripted checkpoints.
- [ ] Integrate the existing host drip server with the Wayland client state machine and verify reconnect behavior after resume.
- [ ] Capture screenshots and logs for boot, first frame, first network event, post-keyboard event, and post-pointer event.
- [ ] Extend the checkpoint capture set with pre-suspend and post-resume artifacts once phase-2 suspend is wired into the Wayland client.
- [ ] Measure redraw latency, reconnect latency, sleep interval, and any additional phase-2 GUI timing metrics from the resulting logs.
- [ ] Commit the input-complete milestone before changing suspend/resume behavior so the current interactive stack has a stable rollback point.
- [ ] Keep the diary updated after each major step, including exact commands, failures, and lessons learned.
- [ ] Commit implementation changes at coherent milestones and record commit hashes in the diary and changelog.
- [ ] Write a dedicated phase-2 final report after implementation and upload the resulting document bundle to reMarkable.

## Bonus Path

- [ ] Minimize bootstrap shell work so the Wayland client eventually owns more initialization directly instead of relying on busybox helpers.
- [ ] Evaluate whether networking, wake logic, or shutdown can be moved from `/init` into the client or a thinner bootstrap wrapper without making debugging materially worse.
