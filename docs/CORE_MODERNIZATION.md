# CPU and timing modernization — 2026-11-27

## Evidence and architectural decision

Resident Evil USA SLUS00170 boots on real PSP Go. User reports extreme slowdown. Direct presentation improved selected slow intervals from 11.33 to 17.45 legacy GPU lace updates/s (not FPS); only 25.9% of wall time remains in buffer presentation. No claim of real-time accuracy. BIOS logging originally opened the storage file for every character, now batched in 0.0.6. Disc reads and bulk GPU dispatch need measurement.

The original MIPS_JIT_R4000 directory contains only TODO!!!!!!!!.txt, not an implementation. R3000A.c selects interpreter on PSP; the existing x86 and SH4 recompilers are incompatible. PsxCommon.h defines BIAS=60 and PSXCLK=(33868800*2), and GPU lace is triggered after a fixed number of branch tests rather than actual emulated VBlank. Do not simply toggle Config.Cpu or change BIAS: guest instruction timing, interrupts and delay slots require tests.

## Candidate CPU approaches

* PCSX4ALL MIPS-to-MIPS recompiler (https://github.com/dmitrysmagin/pcsx4all/tree/master/src/recompiler/mips), derived from Ulrich Hecht's psx4all-dingoo. Candidate for feasibility audit, not a PSP-ready implementation. Inspect host MIPS ISA, ABI, Linux mmap/memory assumptions, executable buffers, Allegrex cache synchronization and instruction support; verify exact source commit and compatible license before reuse.
* Lightrec (https://github.com/pcercuei/lightrec), GNU Lightning based; audit Allegrex support, memory mapping and footprint before choosing. No PSP performance claim.
* Native small-block interpreter optimization, maintaining fallback and differential tests. Avoid new JIT until CPU semantics baseline and cache coherency are validated.

## Acceptance gates

1. Same boot workload in direct/half/ceiling with measured bulk GPU, CD and logging times.
2. Synthetic CPU tests: branch delay, load delay, arithmetic overflow, unaligned accesses, COP0 exceptions, self-modifying code and RAM aliases. Capture interpreter behavior; compare against documented PS1 behavior, not blindly legacy output.
3. Audit candidate recompiler and build isolated PSP test with lawful synthetic guest code; verify D/I-cache synchronization, stack/ABI, no unsafe host register clobbering and bounds.
4. Only then integrate an opt-in CPU backend; preserve interpreter baseline and compare game boots. No public binary until license/relink audit complete.

## First isolated Allegrex code generator

Separate revival/allegrex-jit-probe branch, based on the hardware-tested 0.0.9 reference. Original small emitter, no imported third-party recompiler. Accepts at most 32 non-trapping integer ALU instructions; emits a leaf Allegrex routine operating on an explicit array of 32 guest GPRs, with only a0, t0, t1, ra and no guest-controlled host addresses. Rejects entire blocks containing unsupported instructions or insufficient output capacity without modifying output. Guest r0 reads as zero and ignores writes. Host tests use a small independent host-word simulator plus expected guest register results, rejection and capacity checks. Standalone PSP probe runs only hardcoded synthetic guest operations, flushes data and instruction caches, and displays pass/fail. No commercial content, no emulator integration, no claims of acceleration or PSP hardware success until tested. Memory ops, branch/load delay slots, exceptions, GTE, timing, invalidation of self-modifying code, block dispatch and interpreter differential tests remain outstanding. Keep audio work independent.

## Cached differential execution (experimental, before native integration)

128 direct-mapped slots with exact live guest-word validation on every reuse, including negative entries. A block never crosses a legacy 64KiB memory mapping page; only ordinary RAM and its KSEG0/KSEG1 aliases accepted. No dependence on complete write invalidation hooks (including DMA and aliases); a same-PC changed source is recompiled, a collision evicts. Host tests exercise hits, changed instructions, rejection, alias and page bounds. The jitshadow PERF_MODE.TXT mode retains fastcore, runs an occasional native ALU block on scratch registers and then executes the authoritative interpreter block; checks GPRs, final PC/opcode and legacy cycle delta, logs and disables on mismatch. The ordinary mode never executes generated code. Do NOT activate jitshadow until standalone hardware probe passes; it can still crash on real hardware and is not intended to speed up execution. Only main interpreter loop is instrumented, no branch-delay or ExecuteBlock changes. Native execution replacing interpreter remains gated on real-hardware differential tests, and current legacy cycle semantics must be preserved before timing redesign. No audio in this independent branch yet.

Standalone probe now also validates cache reuse and rewriting a generated block at the same host address after source modification (D/I cache flush). Four on-screen PASS results are required before trying jitshadow. Version label 0.0.12 JIT test is independent of audio 0.0.11. No binary distribution.

## 0.0.13 gated native ALU dispatch

Implement experimental native execution only for existing non-trapping, straight-line RAM ALU blocks. Requires both build flag REVIVAL_JIT_NATIVE=1 and runtime PERF_MODE.TXT=jitnative; normal builds resolve that mode to fastcore without generated code. Each cached block must pass three differential executions before native replacement. Verification state resets on eviction or any source modification. After 255 native blocks, require another successful differential audit before native dispatch resumes. The audit count is separate from the native execution count. Interpreter still handles all branches, delay slots, loads/stores, COP0/GTE, exceptions, BIOS and unsupported opcodes. Preserve legacy PC, opcode and cycle effects; no timing correction. CSV adds cumulative native blocks and words and current enabled flag. Native mode can still crash or diverge, and the per-block warmup does not prove equivalence for all register inputs. Do not activate until standalone probe passes on real hardware and jitshadow logs show zero mismatches; no audio integration on this branch. Host tests check cache trust reset and activation policy, CI builds gated native variant. No speed claim.

## Physical hardware milestone — 2026-11-27

User reports all four on-screen PASS results on PSP Go using standalone probe built locally at f606f62593ddfc4647cdf7dc0388418dcf124e22 (first execution, cache hit, rewritten code, unsupported opcode). This validates the tested synthetic native code and cache synchronization path on that console, not full guest CPU equivalence or performance. CI run 45979892383 for 0.0.13 (5bdc5ce1bc3671a565af1c5fde7a11b6e0a86193) also completed successfully. Next physical gate: use the 0.0.13 silent build, with REVIVAL_JIT_NATIVE=1 at compile time but PERF_MODE.TXT=jitshadow at runtime, in an isolated install. Same Resident Evil disc and BIOS, at least two minutes through gameplay if possible; retain PERF.CSV and PSXP_OUT.TXT. Require nonzero jit_blocks and jit_words, zero jit_mismatch, and no new visible regression before trying jitnative in a separate run. Native mode should then be compared against fastcore with same build and scene; no speedup presumed. Preserve the known working 0.0.9 EBOOT and memory cards. Audio branch remains separate.
