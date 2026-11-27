# External disc and executable admission gate — 2026-11-27

Source reviewed at PCSX4ALL c7faf12b893eba94faced88baa5e7dd059be58b7: misc.cpp Load, LoadCdrom, LoadCdromFile, CheckCdrom, GetCdromFile; cdriso.cpp and associated media opening remain separate audit work.

**External media are not enabled.** Existing boot/EXE routines were validated only using a fixed self-generated fixture. A valid-looking disc header alone does not establish safe guest execution.

## Required next implementation
- Define a deliberately narrow initial supported profile (single-track Mode2/2352 BIN/CUE, ISO9660, PS-X EXE, no patch sidecars or archives). Reject unsupported input cleanly.
- Use explicit bounded parsing of CUE, sector geometry and ISO9660 descriptors/directory entries, with exact I/O success checks. Canonicalize selected files within user-selected storage; no automatic traversal or implicit writes.
- Validate PS-X EXE identification, declared text length and available bytes, contiguous guest RAM destination, alignment, entry PC and stack handling before loading. Use endian-independent parsing. Reject unsupported executable types; check all arithmetic and reads before committing state.
- Make loader failure transactional: no guest execution or partially initialized session on a failed admission, close opened files, keep existing saves untouched.
- Review guest DMA, HLE services, CPU block emission and peripheral paths before broadening the trusted synthetic dispatcher. A successful preflight is not an execution-safety proof.
- Add portable positive/boundary/error tests with generated fixtures only, then a bounded interpreter-first hardware launch from lawful privately supplied media. Keep dynarec opt-in until equivalence is established.

No game images, BIOS or external patches in Git/CI. No claims of external image support or release clearance.

## Read-only preflight V7

Portable C header revival/pcsx4all-probe/disc_preflight.h and host tests in revival/tests/disc_preflight_test.c. Initial conservative profile: exactly one quoted basename BIN in CUE, one Mode2/2352 track at index zero, raw image geometry, primary descriptor at sector 16 and terminator at 17, volume size matches file, root directory at most 64 KiB, root-only SYSTEM.CNF boot name or PSX.EXE fallback, single extent, PS-X EXE with sector-aligned payload in canonical cached guest RAM and aligned entry inside text. Reads every declared payload sector; no EDC/ECC verification, no subdirectory/multitrack/Joliet/patch support, no BSS or relocation handling. Unsupported layout is not equivalent to a corrupt image.

No file is opened by the parser itself, no guest RAM or CPU register writes, result is committed only on success. On PSP the full diagnostic preflights only its own freshly generated BIN/CUE before calling the original loader. No user path is accepted and no arbitrary guest code is executed; this gate is not sufficient for broader external-media execution. Host tests cover supported generated fixture and ordinary unsupported/truncated/read-only failure cases. Physical checks pending.
