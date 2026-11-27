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
print("Pinned source prepared; compile-only, not a runnable emulator")
