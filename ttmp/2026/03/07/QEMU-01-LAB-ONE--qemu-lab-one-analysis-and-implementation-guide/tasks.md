# Tasks

## TODO

- [x] Create ticket `QEMU-01-LAB-ONE`
- [x] Import `/tmp/lab-assignment.md` into the ticket
- [x] Read the imported assignment and extract stage-1 requirements
- [x] Write a detailed analysis/design/implementation guide
- [x] Create and update the diary document

## Implementation Run

- [x] Scaffold `sleep-lab/` source tree in the repo root
- [x] Add `Makefile` targets for build, initramfs packaging, guest run, and measurement capture
- [x] Implement `host/drip_server.py` with interval, burst, pause, and log controls
- [x] Implement `guest/sleepdemo.c` with `epoll`, `timerfd`, `signalfd`, nonblocking TCP, and terminal rendering
- [x] Implement guest suspend/resume timing instrumentation and reconnect logic
- [x] Add guest `/init` workflow and initramfs builder script
- [x] Add `guest/run-qemu.sh` with serial logging, QMP socket, and deterministic wake support
- [x] Add README with exact host/guest workflow and measurement procedure

## Bring-Up

- [x] Build the host server and static guest binary successfully
- [x] Boot the initramfs guest in QEMU successfully
- [x] Verify guest networking to host-side server through QEMU user networking
- [x] Verify the app blocks in `epoll_wait()` while idle
- [x] Verify periodic redraw updates while awake
- [x] Verify clean shutdown through `signalfd`

## Suspend And Resume

- [x] Validate `pm_test=freezer`
- [x] Validate `pm_test=devices`
- [x] Validate real `freeze` suspend-to-idle entry
- [ ] Wake the guest deterministically through QEMU monitor/QMP
- [x] Verify post-resume redraw and state continuity
- [x] Verify reconnect behavior after resume when the socket breaks

## Measurements And Reporting

- [ ] Capture suspend-entry latency
- [x] Capture observed sleep duration inside the guest
- [x] Capture resume-to-redraw latency
- [x] Capture reconnect latency after resume
- [x] Record test commands, logs, and observed caveats in the diary
- [ ] Refresh ticket docs, changelog, and task completion state
- [x] Commit milestone changes at appropriate intervals
- [ ] Upload the refreshed ticket bundle to reMarkable

## Bonus Point

- [ ] Reduce the bootstrap dependency on `busybox` as far as practical
- [ ] Move more early-boot responsibilities into `sleepdemo` itself
- [ ] Evaluate replacing `udhcpc` with direct networking setup inside `sleepdemo`
- [ ] Evaluate replacing shell-driven shutdown with a `sleepdemo`-controlled exit path
- [ ] Document the tradeoffs between the current thin-init model and a near-single-process model
