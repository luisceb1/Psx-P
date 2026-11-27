#!/bin/sh
# SPDX-License-Identifier: GPL-2.0-or-later
set -eu
root="$1"
out="$2"
mkdir -p "$out"
cd "$root"
sdk=$(psp-config --pspsdk-path)
compile() {
    psp-g++ -std=gnu++11 -O2 -G0 -fno-rtti -fno-strict-aliasing \
        -D__PSP__ -DPSXREC -DREVIVAL_PSP_PROBE \
        -D'INLINE=static __inline__' -D'asm=__asm__ __volatile__' \
        -I"$sdk/include" -Isrc -Isrc/port/sdl -Isrc/plugin_lib \
        -Isrc/gpu/gpu_unai -Isrc/spu/spu_pcsxrearmed \
        -Dgpu_unai -DGPU_UNAI -Dspu_pcsxrearmed -DSPU_PCSXREARMED -DUSE_GPULIB \
        -c "$1" -o "$out/$2.o"
}
echo "=== GPU Unai + GPULib synthetic probe ==="
compile src/gpu/gpulib/gpu.cpp gpu_core
compile src/gpu/gpu_unai/gpulib_if.cpp gpu_unai
compile /revival/revival/pcsx4all-probe/gpu-test/gpu_main.cpp gpu_main
compile /revival/revival/pcsx4all-probe/gpu-test/gpu_platform.cpp gpu_platform
make -C "$out" -f /revival/revival/pcsx4all-probe/gpu-test/Makefile
test -s "$out/EBOOT.PBP"
sha256sum "$out/EBOOT.PBP"
