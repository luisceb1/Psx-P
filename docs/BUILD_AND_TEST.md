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
