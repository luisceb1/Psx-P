#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
"""Adapt only a disposable checkout of pinned PCSX4ALL for compile probing."""
import pathlib
import subprocess
import sys

PIN = "c7faf12b893eba94faced88baa5e7dd059be58b7"
root = pathlib.Path(sys.argv[1]).resolve()
actual = subprocess.check_output(["git", "-C", str(root), "rev-parse", "HEAD"], text=True).strip()
if actual != PIN:
    raise SystemExit("Unexpected upstream revision: " + actual)
p = root / "src/recompiler/mips/recompiler.cpp"
s = p.read_text()
def replace_once(a, b):
    global s
    if s.count(a) != 1:
        raise SystemExit("Unexpected upstream source: " + a)
    s = s.replace(a, b, 1)
# Use the conservative non-virtual, C dispatcher; not a performance build.
for macro in ("ASM_EXECUTE_LOOP", "USE_DIRECT_BLOCK_RETURN_JUMPS",
              "USE_DIRECT_FASTPATH_BLOCK_RETURN_JUMPS", "USE_DIRECT_MEM_ACCESS"):
    replace_once("#define " + macro + "\n", "/* PSP probe disables " + macro + " */\n")
replace_once("#ifndef HAVE_MIPS32R2_CACHE_OPS\n#include <sys/cachectl.h>\n#endif",
             "#include <pspkernel.h>")
start = s.index("static inline void clear_insn_cache(")
end = s.index("/* Set default recompilation options", start)
s = s[:start] + """static inline void clear_insn_cache(void *start, void *end, int flags)
{
    (void)flags;
    unsigned int bytes = (unsigned int)((char *)end - (char *)start);
    sceKernelDcacheWritebackInvalidateRange(start, bytes);
    sceKernelIcacheInvalidateRange(start, bytes);
}

""" + s[end:]
# Smaller code arena for first port exploration; upstream safety reserve retained.
replace_once("#define RECMEM_SIZE         (12 * 1024 * 1024)",
             "#define RECMEM_SIZE         (4 * 1024 * 1024)")
# Replace the C inline-asm dispatcher bridge with an ordinary external call.
# The separate assembly routine has an explicit save frame and compiler-visible ABI.
start = s.index("__attribute__((noinline)) static void recFunc(void *fn)")
end = s.index("\n#endif", start)
s = s[:start] + """extern "C" void revival_pcsx4all_call(void *fn, void *regs, u32 pc, u32 result[2]);
static void recFunc(void *fn)
{
    u32 result[2];
    revival_pcsx4all_call(fn, &psxRegs, psxRegs.pc, result);
    psxRegs.pc = result[0];
    psxRegs.cycle += result[1];
}
""" + s[end:]
p.write_text(s)
# Upstream signed constant division uses unsigned values (including abs).
# Avoid the entire unverified shortcut, not just the GCC overload error.
# Signed constant multiplication also needs INT_MIN / negative tests.
mdu = root / "src/recompiler/mips/rec_mdu.cpp.h"
m = mdu.read_text()
for macro in ("USE_CONST_DIV_OPTIMIZATIONS", "USE_CONST_MULT_OPTIMIZATIONS",
              "OMIT_DIV_BY_ZERO_FIXUP_IF_EXCEPTION_SEQUENCE_FOUND"):
    token = "#define " + macro + "\n"
    if m.count(token) != 1:
        raise SystemExit("Unexpected MDU source: " + macro)
    m = m.replace(token, "/* PSP probe disables " + macro + " */\n", 1)
mdu.write_text(m)
print("Pinned source prepared; compile-only, not a runnable emulator")

# Allegrex CLZ uses SPECIAL function 0x16; MIPS32 SPECIAL2 differs.
h = root / "src/recompiler/mips/mips_codegen.h"
header = h.read_text()
old = "write32(0x70000020 | ((rs) << 21) | ((rd) << 16) | ((rd) << 11))"
new = "write32(0x00000016 | ((rs) << 21) | ((rd) << 11))"
if header.count(old) != 1:
    raise SystemExit("Unexpected CLZ emitter")
header = header.replace(old, new, 1)
header = header.replace("#define HAVE_MIPS32_3OP_MUL\n", "/* No MIPS32 SPECIAL2 MUL on this candidate. */\n", 1)
# Do not assume optional MIPS32r2 instruction encodings.
for name in ("HAVE_MIPS32R2_EXT_INS", "HAVE_MIPS32R2_SEB_SEH", "HAVE_MIPS32R2_CACHE_OPS"):
    token = "#define " + name
    header = header.replace(token, "/* disabled " + name + " */", 1)
h.write_text(header)
# One bounded dispatcher entry for trusted synthetic blocks only.
# No use of the unbounded production ExecuteBlock loop.
with p.open("a") as out:
    out.write("""
extern "C" int revival_pcsx4all_probe_one(void)
{
    if ((psxRegs.pc != 0x80010000u && psxRegs.pc != 0x80010004u &&
         psxRegs.pc != 0x80010014u) ||
        psxRegs.cycle >= psxRegs.io_cycle_counter)
        return 0;
    block_ret_addr = block_fast_ret_addr = 0;
    u32 *entry = (u32*)PC_REC(psxRegs.pc);
    if (!*entry) recRecompile();
    if ((uptr)*entry < (uptr)recMemBase ||
        (uptr)*entry >= (uptr)recMem || (*entry & 3u))
        return 0;
    recFunc((void*)(uptr)*entry);
    return 1;
}
""")

# GPULib's public command list uses uint32_t. PSP newlib makes it unsigned long.
gpu_if = root / "src/gpu/gpu_unai/gpulib_if.cpp"
g = gpu_if.read_text()
for old, new in (("unsigned int *list,", "uint32_t *list,"),
                 ("unsigned int *list_start", "uint32_t *list_start"),
                 ("unsigned int *list_end", "uint32_t *list_end")):
    if g.count(old) != 1:
        raise SystemExit("Unexpected GPU command declaration: " + old)
    g = g.replace(old, new, 1)
gpu_if.write_text(g)

# First CD data-only probe: no POSIX audio thread or CDDA playback.
iso = root / "src/cdriso.cpp"
s = iso.read_text()
s = s.replace("#include <pthread.h>", "/* CDDA disabled for PSP data-only probe */", 1)
s = s.replace("static pthread_t threadid;", "/* no CDDA thread */", 1)
a = s.index("#ifdef _WIN32\nstatic void playthread(")
b = s.index("// this function tries to get the .toc", a)
s = s[:a] + """static void stopCDDA() { playing = FALSE; }
static void startCDDA() { playing = FALSE; }
\n""" + s[b:]
iso.write_text(s)

# MSF addresses are nonnegative sector counts; subtract in signed wide type.
cd = root / "src/cdrom.cpp"
s = cd.read_text()
old = "i = abs(i - msf2sec(set_loc));"
new = "i = (int)((s64)i >= (s64)msf2sec(set_loc) ? (s64)i - (s64)msf2sec(set_loc) : (s64)msf2sec(set_loc) - (s64)i);"
if s.count(old) != 1:
    raise SystemExit("Unexpected CD seek distance expression")
cd.write_text(s.replace(old, new, 1))

# Retain original executable and ISO9660 loaders, exclude savestates for now.
misc = root / "src/misc.cpp"
s = misc.read_text()
marker = "// Savestate file handling //"
if s.count(marker) != 1:
    raise SystemExit("Unexpected misc savestate boundary")
s = s[:s.index(marker)]
# ISO directory extents are frequently unaligned on MIPS.
old = "int block = *((int*)b);"
new = "int block = (int)((u32)(u8)b[0] | ((u32)(u8)b[1]<<8) | ((u32)(u8)b[2]<<16) | ((u32)(u8)b[3]<<24));"
if s.count(old) != 1:
    raise SystemExit("Unexpected ISO extent read")
s = s.replace(old,new,1)
misc.write_text(s)

# BIOS heap descriptors are guest 32-bit words, not host unsigned int.
bios = root / "src/psxbios.cpp"
s = bios.read_text()
old = "unsigned int *chunk, *newchunk = NULL;"
if s.count(old) != 1:
    raise SystemExit("Unexpected BIOS heap descriptor declaration")
s = s.replace(old, "u32 *chunk, *newchunk = NULL;", 1)
# Avoid dereferencing the uninitialized heap before the original null check.
old = """chunk = heap_addr;
\tcsize = ((u32)*chunk) & 0xfffffffc;
\tcstat = ((u32)*chunk) & 1;
\tdsize = (a0 + 3) & 0xfffffffc;

\t// exit on uninitialized heap"""
new = """chunk = heap_addr;
\tdsize = (a0 + 3) & 0xfffffffc;

\t// exit on uninitialized heap"""
if s.count(old) != 1:
    raise SystemExit("Unexpected BIOS heap null check")
s = s.replace(old,new,1)
old = "\t// search an unused chunk that is big enough until the end of the heap"
new = "\tcsize = ((u32)*chunk) & 0xfffffffc;\n\tcstat = ((u32)*chunk) & 1;\n\n" + old
if s.count(old) != 1:
    raise SystemExit("Unexpected BIOS heap search")
bios.write_text(s.replace(old,new,1))

# XA/CDDA ring pointers are unsigned int throughout the original SPU.
# PSP newlib uint32_t is unsigned long; allocate with the actual element type.
spu = root / "src/spu/spu_pcsxrearmed/spu.c"
s = spu.read_text()
for old,new in (("(uint32_t *)malloc(44100 * sizeof(uint32_t))", "(unsigned int *)malloc(44100 * sizeof(*spu.XAStart))"), ("(uint32_t *)malloc(CDDA_BUFFER_SIZE)", "(unsigned int *)malloc(CDDA_BUFFER_SIZE)")):
    if s.count(old) != 1:
        raise SystemExit("Unexpected SPU ring allocation")
    s = s.replace(old,new,1)
spu.write_text(s)

# Keep gettimeofday for XA, disable only POSIX worker on PSP.
s = spu.read_text()
old = "#define THREAD_ENABLED 1"
if s.count(old) != 1:
    raise SystemExit("Unexpected SPU thread define")
spu.write_text(s.replace(old, "#ifndef __PSP__\n#define THREAD_ENABLED 1\n#endif",1))
