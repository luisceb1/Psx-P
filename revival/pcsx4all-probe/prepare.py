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
