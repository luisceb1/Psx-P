# Build and test v0.0.1 locally

Prerequisites: Git and working Docker (e.g. Docker Desktop with WSL integration). Run these commands in a Linux/WSL shell. The recommended local build uses SDL 1.2.15 from pinned source, not the historical SDL archives; distribution is not cleared. Do not upload the PBP, BIOS, games or saves.

```sh
git clone https://github.com/luisceb1/Psx-P.git
cd Psx-P
git checkout 31002f292dd7376e278b1633a02bc82d5531c492
docker pull pspdev/pspdev:latest
docker image inspect --format '{{index .RepoDigests 0}}' pspdev/pspdev:latest
docker run --rm -v "$PWD:/src" -w /src pspdev/pspdev:latest sh -c 'set -e; apk add --no-cache autoconf automake libtool build-base; sh revival/build-source-sdl.sh; make -f makefile clean; make -f makefile SDL_PREFIX=/tmp/psxprv-sdl SDL_MIXER=0; test -s EBOOT.PBP; sha256sum EBOOT.PBP'
```

Record image digest and commit; the latest tag is mutable. Copy EBOOT.PBP into a new directory PSP/GAME/PSXPRV/ on the PSP Go internal storage first. Do not replace an old emulator or import existing saves. Launch from XMB with no BIOS or disc and check menu, directory creation, square to exit, and PSXP_OUT.TXT. Do not launch BIOS or a game until the menu smoke test passes. Copy the log and note any crash, but redact personal paths before posting publicly.

Then test separately on removable storage. Further tests require lawful user-supplied files and a separate blank memory card. This is an experimental emulator and may freeze; no claim of game compatibility, speed or sound. The default SPU is still a null plugin; performance measurement is a subsequent milestone.

## Windows PowerShell checkout

The repository enforces LF for text files in .gitattributes. Existing Windows clones may still contain CRLF. If git status --short is empty (no local changes to preserve), run:

```powershell
git pull --ff-only
git checkout-index --force --all
git ls-files --eol revival/build-source-sdl.sh revival/patches/sdl-pspsdk-types.patch
```

Both files must show w/lf before compiling. From the repository directory in PowerShell, with Docker Desktop running:

```powershell
docker run --rm -v "${PWD}:/src" -w /src pspdev/pspdev:latest sh -c 'set -e; apk add --no-cache autoconf automake libtool build-base; sh revival/build-source-sdl.sh; make -f makefile clean; make -f makefile SDL_PREFIX=/tmp/psxprv-sdl SDL_MIXER=0; test -s EBOOT.PBP; sha256sum EBOOT.PBP'
```

## HOME exit regression (0.0.2 candidate)

User observed the 0.0.1 main menu on a real PSP Go, but HOME -> Exit and Pause froze. The original callback set a flag not consumed by the main menu. Candidate uses sceKernelExitGame from the registered callback thread, without cross-thread plugin or stdio cleanup. Test with no disc, BIOS or valuable saves: first Square exit, then relaunch and HOME -> Exit. Repeat three times. Do not select system Pause Game until ordinary exit is verified; PSP Go suspension and snapshot support are not validated. HOME exit is immediate; do not use during memory card writes. This change is not a hardware-verified fix.

### Hardware result — 2026-11-27

User confirms the v0.0.2 test build (92644c6f481795f6b835743a2d7d1874531f22f3) returns to XMB via both Square and HOME Exit on PSP Go. CI run 45968390212 passed all three jobs. Pause Game remains untested after the fix; no game execution or save integrity confirmed. Next: isolated single-track disc test and inspect PSXP_OUT.TXT.

## 0.0.3 timing probe (experimental)

User reports Resident Evil 1 boots with a real BIOS on PSP Go in v0.0.2, but the BIOS logo and execution are extremely slow. No quantitative speed or gameplay completion verified. This probe does not modify CPU cycles, counter rates or rendering algorithms.

Use a new isolated directory PSXPRV3. Copy the same lawful BIOS and disc, not existing saves. Set PERF_MODE.TXT to legacy, single or uncapped (ASCII, one word). Default: legacy. Single removes the redundant PCFrameCap in standard non-frameskip mode. Uncapped is a short diagnostic only, never a gameplay recommendation. Each launch overwrites PERF.CSV when emulation begins. Test the same BIOS-logo launch with O for 30–60 seconds per mode; return to XMB via HOME and copy PERF.CSV before the next test. Keep region, files, storage and clock unchanged. CSV reports wall time, legacy GPU lace calls, emulated counter VBlank starts, legacy cycle delta, time in cap routines (including CheckFrameRate/FrameSkip bookkeeping), and buffer swap time; these are not game FPS or proof of timing accuracy. Clock values and mode recorded. If no rows appear, report that and PSXP_OUT.TXT. Do not select Pause Game or write valuable saves during tests.

## 0.0.4 presentation probe

The user's single-mode CSV contains 41 intervals over 85.018118 s at CPU 333 MHz and bus 166 MHz. For the 17 intervals with swap/wall > 0.4 and laces/s < 20, the weighted rates are 10.128 laces/s, 59.253% swap time and 0.029% cap time. The 17 intervals with laces/s > 50 average 58.740 laces/s. These are legacy update counts, not display FPS or accurate PS1 timing.

New columns split the stretched presentation path into VRAM conversion, scaling, SDL blit and flip; other paths can contribute to swap_us without appearing in these four columns. New PERF_MODE.TXT value native uses single pacing and a guarded direct 32-bit RGB-to-BGR copy for the full-screen software surface only. It falls back to SDL when format or locking conditions differ. native_hits counts successful fast-path copies. Compare single versus native using the identical BIOS and disc, check colors and log, and do not assume speedup until measured.

## 0.0.5 fused presentation experiment

Native-mode user CSV: 32 intervals, 66.977094 s, 333/166 MHz, zero native_hits. 16 slow intervals (swap/wall > 0.4, laces/s < 20) comprise 32.816905 s and 9.964 laces/s. Conversion 24.274%, scale 8.430%, SDL blit 25.541%, flip 0.023%, cap 0.031% of wall time. Previous native-copy hypothesis was not exercised; no speedup claim.

New direct mode fuses source sampling, 15/24-bit RGB conversion and scaling into the actual SDL screen surface, honoring destination pitch and RGB masks; no intermediate copy. Conservative source bounds and unsupported mode fallback. Old rasterizer, pacing and SDL flip unchanged. direct uses single pacing; native_hits counts direct frames, convert_us includes fused work. Direct skips legacy intermediate gun cursor, preview and scanline cases by falling back. Compare same disc/BIOS and check colors, image geometry, CSV and PSXP_OUT.TXT (surface format). Host tests cover RGB packing, 24-bit input, destination pitch, borders and rejection without writes. Real hardware still required.

## 0.0.6 core ceiling experiment

0.0.5 direct test on real PSP Go: 43 intervals, 89.654556 s, 397 successful direct frames, 333/166 MHz. For identical selection criteria across CSVs (laces/s <25 and swap/wall >0.15), 0.0.3 single: 10.923 laces/s and 58.514% swap; 0.0.4 native: 11.332 and 56.483%; 0.0.5 direct: 17.451 and 25.932%. Separate launches, non-aligned windows; not proof of guest real-time speed. Direct conversion averages 14.86ms per successful frame in selected intervals. In fast windows legacy laces ~60/s but guest counter VBlanks ~116/s. Do not equate either with actual FPS or alter timing blindly.

New modes half and ceiling retain direct mode pacing and GPU command processing but suppress every other presentation request or all instrumented presentation requests, respectively. Ceiling is a short diagnostic only: picture can be frozen or absent, HOME Exit after ~20–30 seconds; no valuable saves, never Pause Game. Test direct and half on same BIOS and game, save CSV before relaunch. Ceiling is optional to estimate remaining non-presentation cost. Bulk GPU timings (>=16 words) are inclusive of any nested rendering, CD timings are inclusive of cache lookup, log timings include format and output; do not sum overlapping durations. BIOS putchar diagnostics are batched without changing guest call hooks. Exact instruction rate and timing accuracy are still unknown.

## 0.0.7 boot compatibility probe — Resident Evil USA

0.0.6 half-mode hardware log identifies SLUS00170, reads SYSTEM.CNF, loads SLUS_001.70 (PC 8005f8a0, text 80010000, size 0009f000), prints Execute!, ResetGraph, PAD driver, CD_init and addr=800acc5c. No visible game menu reported. 46 CSV intervals total 92.748651s; final 21 intervals total 42.262367s with 120 legacy lace calls and cycle delta 1466640 each, no draw requests, CD read calls, bulk GPU or log calls. CPU not stopped; boot hang cause not established. Correct earlier descriptions of game boot: guest executable begins, gameplay not verified.

New modes boottrace (same half presentation, original CD semantics) and bootfix (same plus accept CD IRQ acknowledge writes whose low three bits are set, including 0x1f). Legacy CD code only accepts exact 0x07. PSX-SPX CDROM controller documentation describes 0x07 and 0x1f acknowledgements: https://psx-spx.consoledev.net/cdromdrive/ . The optional change does not implement full bank/flag semantics, and is not proven to resolve this hang. New CSV columns snapshot PC, return address, stack, opcode, COP0, interrupt and CD state and cumulative CD events. Sampling is at the existing two-second lace reporting interval and may alias guest loops; no full instruction trace. Compare same game, BIOS, O launch, storage, for ~90s; copy CSV and TXT before each launch. Also test X launch separately to distinguish BIOS loader path from direct loading. If bootfix worsens behavior, revert to boottrace. No save writes or Pause Game during tests.
