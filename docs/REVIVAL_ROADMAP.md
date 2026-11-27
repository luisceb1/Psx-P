# Roadmap

## M0 — Establish a baseline
- [x] Record exact upstream commit: 6f7f300223294859215c42b618fe853d0f847222.
- [ ] Inventory licenses and assets.
- [ ] Compile the unmodified legacy target with a modern PSPDEV SDK; preserve full logs.
- [ ] Replace or rebuild obsolete bundled SDL libraries from corresponding source if required.
- [ ] Resolve modern compiler and linker errors in isolated changes.
- [ ] Boot on PSP Go in a separate install directory; confirm clean return to XMB.

## M1 — Usability and diagnostics
- [ ] Integrate storage-root selection into the legacy frontend (portable helper included but not integrated).
- [ ] Use absolute paths based on the running app; support ef0:/ and ms0:/ explicitly.
- [ ] Validate BIOS size and presence; produce clear errors, never distribute BIOS.
- [ ] Add persistent logs, FPS, emulation speed and frame timing diagnostics.
- [ ] Support read-only single-track BIN/ISO and a strict CUE parser; reject unsupported layouts explicitly. Add multi-track/CDDA later.
- [ ] Protect memory cards and save states; keep old installs untouched.

## M2 — Timing and sound
- [ ] Benchmark interpreter, GPU and I/O separately. Establish PAL/NTSC timing correctness before tuning.
- [ ] Audit the archived MIPS dynarec (not enabled in upstream default build). Validate against interpreter with synthetic instruction tests before enabling.
- [ ] Integrate and benchmark a real SPU backend; historical default links a null SPU.
- [ ] Add frame pacing and audio buffering; compare performance at stock and optional clock settings.

## M3 — Interface and releases
- [ ] Minimal game browser, remappable controls, storage selector and configuration.
- [ ] Test PSP Go internal and removable storage, then PSP-1000/2000/3000.
- [ ] Consider CHD only after measuring decoder CPU/RAM costs and license compatibility.
- [ ] Publish experimental releases only after hardware tests and license/asset audit.
