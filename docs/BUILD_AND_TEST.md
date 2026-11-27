# Build and test v0.0.1 locally

Prerequisites: Git and working Docker (e.g. Docker Desktop with WSL integration). Run these commands in a Linux/WSL shell. A local build uses the inherited legacy libraries; distribution is not cleared. Do not upload the PBP, BIOS, games or saves.

```sh
git clone https://github.com/luisceb1/Psx-P.git
cd Psx-P
git checkout 45e9c49891e1f8332bac64256f10ec146c473940
docker pull pspdev/pspdev:latest
docker image inspect --format '{{index .RepoDigests 0}}' pspdev/pspdev:latest
docker run --rm -v "$PWD:/src" -w /src pspdev/pspdev:latest sh -c 'set -e; psp-gcc --version; make -f makefile; test -s EBOOT.PBP'
```

Record image digest and commit; the latest tag is mutable. Copy EBOOT.PBP into a new directory PSP/GAME/PSXPRV/ on the PSP Go internal storage first. Do not replace an old emulator or import existing saves. Launch from XMB with no BIOS or disc and check menu, directory creation, square to exit, and PSXP_OUT.TXT. Do not launch BIOS or a game until the menu smoke test passes. Copy the log and note any crash, but redact personal paths before posting publicly.

Then test separately on removable storage. Further tests require lawful user-supplied files and a separate blank memory card. This is an experimental emulator and may freeze; no claim of game compatibility, speed or sound. The default SPU is still a null plugin; performance measurement is a subsequent milestone.
