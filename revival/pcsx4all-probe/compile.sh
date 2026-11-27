#!/bin/sh
# SPDX-License-Identifier: GPL-2.0-or-later
# Compile independently to expose all failures, never link or execute guest code.
set -eu
root="$1"
out="$2"
mkdir -p "$out"
cd "$root"
psp-g++ --version
psp-config --pspsdk-path
failed=0
for unit in r3000a psxinterpreter psxevents psxcounters psxmem gte recompiler/mips/mips_codegen recompiler/mips/mem_mapping recompiler/mips/recompiler; do
    name=$(echo "$unit" | tr / _)
    echo "=== $unit ==="
    if psp-g++ -std=gnu++11 -O2 -G0 -fno-rtti -fno-strict-aliasing \
        -D__PSP__ -DPSXREC -DREVIVAL_PSP_PROBE \
        -D'INLINE=static __inline__' -D'asm=__asm__ __volatile__' \
        -Isrc -Isrc/port/sdl -Isrc/plugin_lib \
        -Isrc/gpu/gpu_unai -Isrc/spu/spu_pcsxrearmed \
        -Dgpu_unai -DGPU_UNAI -Dspu_pcsxrearmed -DSPU_PCSXREARMED -DUSE_GPULIB \
        -c "src/$unit.cpp" -o "$out/$name.o"; then
        echo "PASS $unit"
        psp-size "$out/$name.o"
    else
        echo "FAIL $unit"
        failed=1
    fi
done
exit "$failed"
