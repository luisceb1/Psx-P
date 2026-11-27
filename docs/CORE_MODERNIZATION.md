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
