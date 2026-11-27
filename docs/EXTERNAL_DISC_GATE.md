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

## V15 CUE formatting compatibility

The bounded single-track CUE parser accepts an optional UTF-8 BOM, REM metadata lines, horizontal whitespace between FILE/TRACK/INDEX tokens, and conventional punctuation in quoted local BIN basenames (parentheses, brackets, apostrophe, comma, exclamation, ampersand and plus). It preserves the actual filename case for filesystem access. The admitted disc geometry remains exactly one MODE2/2352 BINARY track, INDEX 01 at 00:00:00; no extra tracks, pregaps, alternate modes, subdirectories, or archives. CUE input size and filename bounds remain unchanged. All executable metadata, sector checks and transactional staging behavior are unchanged. The inspector remains read-only and never executes input.

Portable regression covers four accepted formatting/name combinations and six unsupported ordinary layouts, with unchanged filename output on failure; the filesystem-backed inspector fixture also uses a BOM, REM and mixed whitespace. No commercial disc image tested, no hardware compatibility claim.

## V16 ISO9660 boot metadata compatibility

SYSTEM.CNF may specify a root boot filename without a version suffix; the inspector normalizes it to the explicit ISO9660 version ;1, which must exist as a single root extent. Case, root separator, whitespace and additional non-BOOT lines follow the prior policy. An explicit non-1 version, nested path, or missing named file remains unsupported; PSX.EXE fallback is used only when SYSTEM.CNF is absent. 

The logical volume size may be smaller than the physical single-track BIN size (e.g. trailing padding), but must contain the descriptors and every checked extent. The result and V16 inspector report both physical sectors and logical volume_sectors. Both the full preflight and transactional staging bound payload reads to the logical volume. No additional descriptor layouts, multitrack, subdirectory or guest execution support is implied. The earlier exact-size restriction is superseded by this bounded logical-volume policy.

Host fixtures cover four BOOT spellings selecting a distinct executable in the second root directory sector, fallback, unsupported boot names/layouts, and a 30-sector logical volume in a 32-sector image with successful staging and rejection when the logical volume exceeds the file. No commercial image or hardware result.

## V17 disc session ownership

The pinned CD backend's CDR_shutdown calls CDR_close and releases its patch cache. The full synthetic diagnostic now uses one shared single-threaded ownership state for the two legacy CD opens (boot fixture and DMA3 fixture reopen). Successful initialization followed by a failed open triggers shutdown; a second begin while owned is rejected; shutdown is idempotent at the wrapper level and a reported cleanup failure blocks a subsequent begin until cleanup succeeds. No additional implicit CDR_close call. Only the fixed generated fixture can be selected. No changes to event timing or runtime external media admission.

Host regression executes callback-based mock initialization/open/shutdown in order, three repeated cycles, init/open failure and failed-cleanup retry. The two new physical checks verify successful shutdown and backend read rejection after each real fixture session. Expected cumulative 72 PASS if every prerequisite succeeds; core/SPU/GU full restart and firmware pause are separate unvalidated work. No actual PSP execution or arbitrary media test yet.

## V18 ordered core teardown

The full diagnostic now tracks CPU, SPU and GPU acquisition explicitly; on failure it unwinds successfully acquired stages in reverse order. The CD session is closed first. Shutdown failure retains ownership and prevents lower-level teardown. CPU shutdown restores the engine that originally initialized the core (the diagnostics switch the active interpreter/dynarec pointer), invokes upstream psxShutdown and clears the diagnostic register/pointer state. SPUshutdown already closes sound, so a separate SPUclose is unnecessary. GPU shutdown releases its own GU/VRAM resources.

The pinned upstream PSP adaptation releases malloc-backed non-virtual recompiler block-pointer tables on shutdown and clears their lookup entries, checks table allocations before reset, and unwinds CPU and memory after a failed RAM initialization. Static code arena unchanged; generated instruction semantics unchanged. Upstream SPU allocation handling and fatal GPU initialization paths remain separate audit work; the wrapper cannot recover from a nonreturning or fatal upstream call.

Portable callback regression covers successful startup and teardown, three consecutive mock lifecycles, each stage's startup failure and cleanup failure/retry. Full integration adds one final ordered-shutdown check (including closed GPU and guest RAM pointers). Expected cumulative 73 PASS when prerequisites pass. Actual complete shutdown/restart on PSP, audio playback, Pause Game and external media execution remain unvalidated; a second real core initialization is deliberately not attempted yet.

## V19 bounded full restart diagnostic

At the end of the existing full diagnostic, seed and verify selected guest RAM, GPR, far-end VRAM and SPU RAM markers. After CD cleanup and successful ordered core teardown, initialize CPU/SPU/GPU a second time, reset the core with HLE selected, check that those markers are absent, and use the same fixed short interpreter GPU-fill fixture to verify guest output and a fresh GU submission. Shut down the second session. Skip dependent checks if seeding, teardown or startup fails. No arbitrary external executables, dynarec expansion or timing change.

The disposable PSP source adaptation now initializes the synchronous SPU's main state and working buffers afresh and checks its owned allocations before use, releasing partial allocations on a reported failure. GPU VRAM allocation failure returns an error instead of terminating the process. Native GU presentation counters and staged dimensions start fresh per initialization. This is not proof that all upstream static state (e.g. XA interpolation history), audio playback or all failure paths are reset. No worker threads, real audio or BIOS/game files in the restart test.

Nine added physical checks (seed, three startup stages, RAM/GPR reset, VRAM/SPU reset, interpreter output, GU submission, final shutdown), expected total 82 PASS if prerequisites succeed. Existing host lifecycle, parser, display and queue tests remain; CI cross-compilation does not execute the PSP restart. Physical validation of the combined diagnostic is the next gate before widening guest execution.
