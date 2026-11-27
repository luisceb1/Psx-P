# PSX-P Revival

Experimental modernization of PSX-P Alpha 2, the PlayStation emulator for Sony PSP originally developed by Yoshihiro and based on PCSX. Initial target: PSP Go on firmware 6.61 with ARK-5. Independent of Sony and ARK-5.

**Status: v0.0.1 diagnostic candidate.** CI successfully builds EBOOT.PBP with modern PSPSDK and pinned source-built SDL 1.2.15; no hardware validation or public binary release yet. The storage helper is integrated in the frontend startup.

Upstream: https://github.com/PSP-Archive/Psx-P. Baseline: `6f7f300223294859215c42b618fe853d0f847222`. Preserve its history and all existing notices. The original PCSX-derived code contains GPL-2.0-or-later notices. Audit per-file notices, bundled SDL binaries, icons and audio before publishing new binaries. New C files are GPL-2.0-or-later. Do not distribute BIOS, commercial games or assets without permission.

## Development

- Run `make -C revival test` on a host PC.
- Enable Actions if disabled on this fork and run **Legacy PSP baseline**. Inspect the compiler log. Both the historical comparison and source-built SDL jobs pass; neither is hardware validation.
- The homebrew program itself will still be packaged as EBOOT.PBP; the goal is that games remain in disc-image formats.

Build locally using the [hardware test guide](docs/BUILD_AND_TEST.md). Public binary distribution remains blocked by the [dependency audit](docs/DEPENDENCY_AUDIT.md).

See [roadmap](docs/REVIVAL_ROADMAP.md) and [test protocol](docs/REVIVAL_TESTS.md).
