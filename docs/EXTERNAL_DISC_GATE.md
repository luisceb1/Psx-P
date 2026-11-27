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

## Standalone inspector V8

External inspection is now available without initializing any emulator subsystem. The V7 full diagnostic is unchanged. The inspector reads a single fixed GAME.CUE and its validated local BIN basename from a supplied directory, opens both read-only and reports phase/status, boot name, sector count, executable extent and initial register metadata on a match. It does not read all sectors of a disc or certify execution safety. A rejected profile is not proof of a damaged disc. No external media are included in tests.

- Host: `make -C revival inspect` then `./revival/disc_inspect /path/to/directory`. Put a copied CUE named GAME.CUE alongside its original referenced BIN. Return 0 matches the narrow profile, 1 not admitted, 2 usage.
- PSP: build inspect-test/EBOOT.PBP alongside the other probes. Copy to a new directory, with DISC/GAME.CUE and referenced BIN in DISC/. The PSP frontend derives its own app location using the existing storage helper and fails closed if unavailable; it does not create directories. It writes only PSXPRV_DISC_INSPECT.TXT beside its own EBOOT and does not modify the image. Square or HOME Exit; no Pause Game test here.
- The filesystem path is constrained by the caller-selected directory and validated CUE basename; no support for multiple tracks, subdirectories, compression or other media formats. Host symlinks are not resolved/contained by this tool; inspect only your own local copy. No image execution, BIOS, RAM mutation or saves. Host wrapper tests generate a private temporary directory and cover missing files and the matching fixture. Hardware operation pending.

Next admission gate: load verified synthetic payload transactionally into emulated RAM using a new controlled reader, compare against old HLE loader, then audit emulator-side runtime behavior before allowing private images to run.

## Transactional loader V9

New disc_load.h stages the validated executable payload in an independently allocated buffer (bounded by 2 MiB guest RAM), reads all payload sectors and only then commits the checked contiguous interval to caller-provided emulated RAM and publishes metadata. The caller closes the media handle before commit; stage failure or rejected destination never modifies RAM or published metadata. Stage ownership is explicit; discard frees and clears the buffer. No BSS or relocation, and no execution of external input is enabled.

Portable tests use a generated two-sector executable, confirm exact payload, unchanged adjacent and other RAM, insufficient-capacity rejection without RAM/metadata changes, and truncated image rejection. The full PSP diagnostic compares the new staging result and initial PC/GP/SP against the original HLE loader on the private generated disc, commits only on parity, clears the relevant dynarec cache and restores the same register values before the trusted bounded native loop. Three additional physical checks; if preconditions fail the original trusted fixture may still run, but the new loader is not marked as passed. No changes to the standalone inspector/pause probe.

## Direct synthetic interpreter boot V10

The new diagnostic runs only the original generated fixture, not the external inspector's input. It stages and verifies the exact fixture payload, resets the full core with HLE initialization (without executing the legacy HLE bootstrap opcode), explicitly initializes the 256x240 progressive display, loads text through the transactional reader into sentinel-filled guest RAM, and sets PC/GP/SP. The same 20,000-iteration program then runs via the actual interpreter in bounded slices, with a checkpoint before the guest GPU-writing code. Hardware interrupts are masked for this fixture because it has no handler; scheduled events can still run. Total cooperative time limit eight seconds, bounded dispatch count, cancellation START+SELECT. No hard watchdog for an internal call that fails to return.

Eight new physical checks: staging, RAM load with guards, entry registers, loop, completion, VRAM, fresh GU submission, selected final result parity with previous native fixture execution. Cycle deltas and dispatch counts recorded separately; different initialization and slicing mean these are not a real game benchmark or an exact cycle equivalence claim. No commercial media opened or executed. Remaining gates: physical results, real media and runtime audit including HLE/DMA/MDEC/SIO and state lifetime, then controlled external interpreter test; no dynarec expansion or publication yet.

## V14 single-pass transactional payload staging

Previously the staging loader scanned every payload sector during read-only preflight and read them all again into the private buffer. The shared metadata pass is now separate from the full inspection pass. The standalone inspector still returns success only after reading every declared sector; the transactional loader reads each payload sector once into its private buffer, checking the same Form1 and I/O requirements, and publishes the staged result only when all reads succeed. No change to the admitted CUE/ISO9660/EXE profile or guest execution scope. The metadata-only internal helper must never be treated as full admission.

The portable synthetic two-sector test additionally verifies that an ordinary sector format/read failure after the first payload sector discards the partial stage, produces no successful inspector result, and leaves guest RAM and previously published metadata untouched. Reduced payload I/O is an implementation change, not a measured PSP load-time improvement. Hardware and private-media execution gates remain pending.
