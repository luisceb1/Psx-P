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

## Independent ABI and ISA gate

Add a standalone PSP EBOOT build for synthetic host blocks only: indirect RA return, stack return slot and rewriting code at the same address after cache synchronization. It checks returned PC/cycles and guest-context sentinels. This is not a guest CPU test and cannot prove callee-saved preservation exhaustively or calls from generated code into C. No EBOOT uploaded. A separate assembler probe compares ten relevant host instructions and NOP with upstream emitter words; differences are recorded as porting work, not silently accepted as valid opcodes. Three-operand MUL and optional MIPS32r2 instructions are not covered by this first corpus. Never execute upstream-generated code until ISA mismatches are addressed and a bounded guest test exists.

## First complete compile/link and ISA results

CI 45989391917 passed both jobs: nine core units, bridge, relocatable link (4,676,647 bytes aggregate text/data/bss), ABI EBOOT packaging and GU compile. ISA comparison: 10/11 words match; CLZ upstream 71284020 differs from Allegrex 01204016. Adapt the CLZ encoder to SPECIAL fn 0x16, disable MIPS32 SPECIAL2 MUL and unverified r2 options, make the first ISA corpus a strict gate. Add a minimal guest EBOOT candidate with real interpreter and dynarec, original five-instruction ALU + jump/delay-slot program, bounded interpreter dispatch and a single test-specific dynarec dispatch. Peripheral stubs fail closed; no BIOS boot, actual event queue, RAM-op semantics or full compatibility proven. The one-block emitter itself is not a general bounded executor and must only be invoked on this trusted program. Do not execute physically until the full link and further ABI/ISA review pass. No binary upload.

CI 45989621794: corrected ISA corpus 11/11, ABI EBOOT packaging passed, all nine core modules and relocatable link passed. Guest harness compile failed on missing psxevents.h (PSXINT_NEXT_EVENT). Add header and second trusted single-block test for SW/LW and explicit load-delay NOP; verify RAM word and GPR/PC against literal expected values as well as interpreter. Still no execution or performance evidence; memory calls from generated code remain a runtime gate.

CI 45989980801: both jobs passed; first synthetic guest EBOOT fully linked (SHA256 6071a5e2c4edc057962050e156ab7fe94a519e7244e320163be9dab697a1a09a). Extend trusted PC allowlist for a 20,000-iteration loop; bounded dispatch, compare expected registers and PC, record cold-start wall times (includes initial compilation) and guest cycles. Add code rewrite via psxMemWrite32 at block entry, not proof of arbitrary SMC. Log to local PCSX4ALL_PROBE.TXT, flush per test, request 333/166 and record actual clocks. No game/BIOS, no uploaded binaries. This is not a game performance benchmark; peripherals and event dispatch remain outside the test.

## First physical PSP Go validation — 2026-11-27

User-reported local EBOOT SHA256 matches CI 45990141530: 5eeabdfbb9036dbd3eec9ab350821cde79880a35030bd2c43873b21415e84a31. Probe v2 reports CPU 333 MHz, bus 166 MHz. Original user log:

```text
PCSX4ALL guest probe v2 cpu=333 bus=166
ALU + branch,PASS,PASS,PASS,10,10
RAM store/load,PASS,PASS,PASS,14,14
loop,PASS,69567,23661,160006,160006,20000
rewrite,PASS
```

Interpreter 69,567 us; dynarec 23,661 us including cold compilation; speedup 2.9402x; wall-time reduction 65.99%. Both engines report identical cycles in these workloads. Explicit block-entry rewrite passed. This is one physical run on synthetic code, not a game/FPS benchmark; independent ABI executable not physically tested. Actual event queue, broader load delays, MMIO, exceptions, arbitrary SMC, GPU/SPU/CD and game compatibility remain unvalidated. No direct comparison with legacy fastcore. Next priority: minimal platform integration of real peripheral modules, retain interpreter fallback and bounded synthetic regression; no arbitrary timing changes or public binary release.

## GPU integration — first headless candidate

After successful physical CPU validation, compile pinned upstream GPU Unai (gpulib_if.cpp, GPL-2.0-or-later notices) and GPULib (gpu.cpp, GPL-2.0-or-later or LGPL-2.1-or-later) without the original SDL presentation frontend. Use upstream software rasterizer and real GP0/GP1 commands, allocate upstream guarded VRAM (about 2 MiB), keep frameskip disabled. Independent GPU EBOOT tests fill, CPU upload, CPU readback, VRAM copy, flat rectangle, and headless presentation callback; verify literal 15-bit pixel values. No CPU MMIO integration yet, no GU rendering, no actual display presentation, no BIOS or disc, no public binary. Preserve CPU physical baseline and its separate EBOOT. Later integrate actual psxHw and DMA with real GPU, then PSP display adapter and measure presentation cost. Per-file license audit still required before distribution.

CI 45991525768: ABI/ISA passed; GPU renderer compilation failed because -Dgpu_unai collides with the renderer state variable of the same name, and unsigned int* command list differs from uint32_t* on PSP newlib. Remove lowercase selector for GPU probe (keep GPU_UNAI), normalize command list pointers to the declared GPULib uint32_t interface. No permissive conversion flags. GPU runtime remains untested.
