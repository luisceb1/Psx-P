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
