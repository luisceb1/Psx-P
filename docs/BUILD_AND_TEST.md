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
