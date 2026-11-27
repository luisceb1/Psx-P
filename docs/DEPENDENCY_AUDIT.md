# Dependency provenance — 2026-11-27

Status: preliminary inventory, not release clearance.

- PCSX core: original GPL-2.0-or-later notices; GPLv2 text at PCSX_CORE/Docs/License.txt.
- GPU and SPU: original notices and license texts retained at GPU_SOFT_SDL/license.txt and PSX_PEOPS_SPU/License.txt.
- SDL headers identify SDL 1.2.9 and SDL_mixer 1.2.6 under LGPL-2.0-or-later. The archived static libraries were built against GCC 4.0.2; their exact corresponding source and PSP-specific modifications are not identified. Header version is not proof of binary provenance. Do not publish an emulator binary until corresponding source and static-link compliance are resolved (or replace the dependency).
- Historical XMB artwork and audio excluded from diagnostic builds; rights not independently established.
- PSP/log.c retains Kymaera attribution; trace the referenced license before distribution.
- Modern SDK and libpng/zlib build inputs also require a recorded source/version manifest before a public binary release.
- No Sony BIOS or game images may be distributed.

The successful CI baseline 45964999775 generated a PBP but uploaded only diagnostic logs. Keep emulator binary artifacts disabled for now. A local source build is available for personal hardware testing; it is not a public release.

## Candidate replacement
The PSP-specific SDL 1.2 source is available at https://github.com/pspdev/SDL-legacy (commit a1f2392fd46137815ada13ff9fdbae21219f49e5). Its README.PSP documents an autoconf-based cross-build. This is a candidate replacement, not evidence that the archived 2006 binary corresponds to this commit. First measure whether SDL_mixer is actually linked, then build SDL from pinned source and record relink instructions and licensing obligations. Consider a native GU backend as an alternative.

## Source-built replacement verified — 2026-11-27

CI run https://github.com/luisceb1/Psx-P/actions/runs/45966094173 passed all three jobs. The source-built link command uses /tmp/psxprv-sdl/lib/libSDL.a and no archived SDL or SDL_mixer library. SDL 1.2.15 source headers declare LGPL-2.1-or-later; individual notices remain applicable. Source revision a1f2392fd46137815ada13ff9fdbae21219f49e5, compatibility patch revival/patches/sdl-pspsdk-types.patch, build script revival/build-source-sdl.sh. Observed SDL archive SHA-256: 660c1165e6e55fafff897752ae437dc58b38dd9e8851093196a723729f72723b. Observed PBP SHA-256: 7edd5c4db5f9d627e79e56d3832009b43dc8834eb327f96d7cfc8dce72cd1e17. Hashes describe that run, not a bit-for-bit reproducibility guarantee. The historical comparison build still uses the archived SDL. It is not the recommended hardware test build.

Remaining gates: investigate SDL_iconv.c realloc pointer warning; record SDK and other linked dependency versions and corresponding sources; trace Kymaera-derived log file notices; prepare source and static relinking materials with applicable licenses; hardware smoke test. Do not publish PBP until clearance. Build logs and symbols only are uploaded.
