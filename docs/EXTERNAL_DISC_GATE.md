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
