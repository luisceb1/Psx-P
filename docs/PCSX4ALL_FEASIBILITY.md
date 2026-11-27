# PCSX4ALL feasibility — 2026-11-27

This is a separate investigation based on main 04796217abf41224492ebb26d346c038602d7add. The old CPU and PRs 10/11 are untouched. No runnable emulator is produced, no third-party source is committed, no game/BIOS/saves or binary artifacts.

## Evidence and provenance

Upstream https://github.com/dmitrysmagin/pcsx4all/tree/c7faf12b893eba94faced88baa5e7dd059be58b7 (2019-02-19), PCSX4ALL 2.4. Root gpl.txt, GPL-2.0-or-later notices in r3000a.cpp, psxevents.cpp, psxcounters.cpp; MIT-style notice in recompiler.cpp and mem_mapping.cpp; GPL-2.0-or-later in host_asm.S; mips_codegen.cpp includes explicitly GPLv2-derived functions. Per-file audit of GPU/SPU/assets/ports still required. Preserve attribution and original history for any eventual vendoring; no release clearance.

- psxcommon.h defines PSXCLK 33868800, psxinterpreter.cpp advances cycles per instruction, psxevents.cpp schedules events and rebases timestamps; psxcounters.cpp derives VBlank and root counters from cycle deadlines. This is a more coherent starting point, not verified cycle accuracy. Do not mix with the old branch-count cycle model.
- recompiler.cpp supports register allocation, constants, code invalidation, block returns and non-virtual pointer LUT; mem_mapping.cpp contains existing non-mmap stubs. PSP can initially use these, not Linux SHMEM/TMPFS or mapping at address zero.
- Host cache maintenance uses Linux cacheflush or MIPS32r2 synci/rdhwr; PSP adaptation uses sceKernelDcacheWritebackInvalidateRange then sceKernelIcacheInvalidateRange.
- Original code arena 12 MiB plus 2.5 MiB block-pointer arrays and 256 KiB LUT for 32-bit pointers; probe reduces arena to 4 MiB, preserves 512 KiB reserve. No proof of runtime fit or safe emission yet.
- Probe intentionally disables inline assembly dispatcher, direct-return jumps, fastpath jumps and direct memory access initially. Do not benchmark this conservative configuration as a finished port. Audit remaining generated opcodes, ABI, address region reachability, load delay, exception semantics, SMC and DMA invalidation before executing generated code.
- GPU Unai + GPULib and ReARMed SPU are upstream components. GU probe is only a textured presentation API compilation check; exact PS1 rasterization requires tests for mask bits, texture windows, dithering, semi-transparency, ordering, VRAM readback, format and synchronization. A host GPU is not automatically a correct PS1 GPU.

## Gates

1. Compile isolated CPU, scheduler, memory, GTE and MIPS recompiler units with PSPSDK. Record exact source SHA and SDK image digest. Inspect individual failures. No linking, runtime or speed claim.
2. Build independent synthetic test executable with no BIOS/game. Validate guest ALU, branch/load delay, RAM mirrors, event delivery and cache/ABI; compare interpreter and recompiler with the same core.
3. Link a minimal PSP platform port; measure memory, use isolated blank storage and retain fallback interpreter. Only then consider physical testing.
4. Add disc and frontend adapters, GPU and SPU. Benchmark fixed workloads with guest frame/event counts and elapsed wall time. Compare against original fastcore separately.
5. Consider GU rasterization or VFPU only after identifying bottleneck. Do not change original BIAS/VSync or merge experimental code prematurely.

## First cross-compile result — 2026-11-27

CI 45988677116: GCC 15.2.0 compiled 8/9 core translation units: r3000a, psxinterpreter, psxevents, psxcounters, psxmem, gte, mips_codegen and mem_mapping. The full recompiler and GU probe failed because standalone compiler calls lacked the SDK include directory (pspkernel.h / pspgu.h not found). This is a build configuration failure, not evidence that the full backend is compatible. Add explicit SDK include path from psp-config and header existence checks. On successful compilation, combine the objects using a relocatable link and report unresolved external symbols to scope the next platform/core integration. Relocatable output is not an executable and is not uploaded. No hardware test.

## Second cross-compile result — 2026-11-27

CI 45988854776: same eight core objects compile; GU probe now compiles (168 bytes text). Full recompiler failed at rec_mdu.cpp.h:354/360 due to abs(u32) overload ambiguity in signed constant DIV. Inspection also found unsigned division/remainder in the signed constant path and sign-dependent shortcuts that need semantic tests. Disable constant multiply/divide shortcuts and the exception-sequence-based omission of division-by-zero fixup for the first candidate; do not treat this as fixing runtime MDU semantics. Runtime DIV overflow and zero behavior on Allegrex still need verification. The upstream C dispatcher itself uses inline asm recFunc with incomplete caller-clobber declaration and manual stack manipulation; a dedicated ABI-audited assembly trampoline or equivalent is a runtime gate even if compilation succeeds. No physical test or performance claim. GU result is compile-only.

## Third cross-compile result — 2026-11-27

CI 45989046932 compiled all nine core translation units including full recompiler and GU. Job failed after compilation because a malformed quoted grep expression broke the post-compile shell script. Fix script and add original external ABI bridge (call_block.S) to replace inline C asm. Bridge preserves s0-s7, fp, gp, ra, maintains 8-byte stack alignment, provides 16-byte outgoing argument area and return slot at 16(sp), passes guest context in fp and PC in v0, returns next PC and cycle delta through result[2]. This is a compile/link-only candidate, not runtime validated. Remaining ABI assumptions: generated calls to C, stack use inside emitted blocks, same J/JAL 256 MiB segment, instruction encodings (especially MIPS32 SPECIAL2 MUL/CLZ and MOVN/MOVZ), and cache coherency require audit and synthetic execution before hardware use. No speed claim. GU remains compile-only.
