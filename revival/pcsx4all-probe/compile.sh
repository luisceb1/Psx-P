#!/bin/sh
# SPDX-License-Identifier: GPL-2.0-or-later
# Compile and link synthetic probes, never execute guest code in CI.
set -eu
root="$1"
out="$2"
mkdir -p "$out"
cd "$root"
psp-g++ --version
sdk=$(psp-config --pspsdk-path)
printf "PSPSDK=%s\n" "$sdk"
test -f "$sdk/include/pspkernel.h"
failed=0
for unit in r3000a psxinterpreter psxevents psxcounters psxmem gte recompiler/mips/mips_codegen recompiler/mips/mem_mapping recompiler/mips/recompiler; do
    name=$(echo "$unit" | tr / _)
    echo "=== $unit ==="
    if psp-g++ -std=gnu++11 -O2 -G0 -fno-rtti -fno-strict-aliasing \
        -D__PSP__ -DPSXREC -DREVIVAL_PSP_PROBE \
        -D'INLINE=static __inline__' -D'asm=__asm__ __volatile__' \
        -I"$sdk/include" -Isrc -Isrc/port/sdl -Isrc/plugin_lib \
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
if [ "$failed" -eq 0 ]; then
    echo "=== Compile ABI bridge ==="
    psp-gcc -G0 -c /revival/revival/pcsx4all-probe/call_block.S -o "$out/call_block.o"
    psp-objdump -dr "$out/call_block.o"
    echo "=== Relocatable core link (not executable) ==="
    psp-g++ -r -nostdlib "$out"/*.o -o "$out/core-relocatable.o"
    psp-size "$out/core-relocatable.o"
    echo "=== Remaining external symbols ==="
    psp-nm -u -C "$out/core-relocatable.o"
    echo "=== Defined CPU backend symbols ==="
    psp-nm -C "$out/core-relocatable.o" > "$out/symbols.txt"
    for symbol in psxInt psxRec psxCpu revival_pcsx4all_call; do
        grep -F " $symbol" "$out/symbols.txt"
    done
    echo "=== Host ABI / ISA attributes ==="
    psp-readelf -h -A "$out/core-relocatable.o"
    echo "=== Link isolated synthetic guest EBOOT ==="
    for unit in guest_main platform_stubs; do
        psp-g++ -std=gnu++11 -O2 -G0 -fno-rtti -fno-strict-aliasing \
            -D__PSP__ -DPSXREC -DREVIVAL_PSP_PROBE \
            -D'INLINE=static __inline__' -D'asm=__asm__ __volatile__' \
            -I"$sdk/include" -Isrc -Isrc/port/sdl -Isrc/plugin_lib \
            -Isrc/gpu/gpu_unai -Isrc/spu/spu_pcsxrearmed \
            -Dgpu_unai -DGPU_UNAI -Dspu_pcsxrearmed -DSPU_PCSXREARMED -DUSE_GPULIB \
            -c "/revival/revival/pcsx4all-probe/guest-test/$unit.cpp" -o "$out/$unit.o"
    done
    make -C "$out" -f /revival/revival/pcsx4all-probe/guest-test/Makefile
    test -s "$out/EBOOT.PBP"
    sha256sum "$out/EBOOT.PBP"
    sh /revival/revival/pcsx4all-probe/compile-gpu.sh "$root" "$out/gpu-test"
fi
exit "$failed"
