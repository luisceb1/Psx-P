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
        -DGPU_UNAI -Dspu_pcsxrearmed -DSPU_PCSXREARMED -DUSE_GPULIB \
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

# Preserve both independent CPU and GPU probes, build a third combined EBOOT.
core=$(dirname "$out")/core-relocatable.o
combined=$(dirname "$out")/integration-test
mkdir -p "$combined"
cp "$core" "$combined/core-relocatable.o"
cp "$out/gpu_core.o" "$out/gpu_unai.o" "$combined/"
out=$combined
compile src/psxdma.cpp dma_core
compile src/psxhw.cpp hw_core
# Compile the original stubs and vout with integration-specific definitions.
for pair in "guest-test/platform_stubs.cpp:integration_stubs" "gpu-test/gpu_platform.cpp:integration_vout" "integration-test/integration_main.cpp:integration_main"; do
    unit=${pair%:*}
    name=${pair#*:}
    psp-g++ -std=gnu++11 -O2 -G0 -fno-rtti -fno-strict-aliasing \
        -D__PSP__ -DPSXREC -DREVIVAL_PSP_PROBE -DREVIVAL_GPU_INTEGRATION \
        -D'INLINE=static __inline__' -D'asm=__asm__ __volatile__' \
        -I"$sdk/include" -Isrc -Isrc/port/sdl -Isrc/plugin_lib \
        -Isrc/gpu/gpu_unai -Isrc/spu/spu_pcsxrearmed \
        -DGPU_UNAI -Dspu_pcsxrearmed -DSPU_PCSXREARMED -DUSE_GPULIB \
        -c "/revival/revival/pcsx4all-probe/$unit" -o "$out/$name.o"
done
make -C "$out" -f /revival/revival/pcsx4all-probe/integration-test/Makefile
test -s "$out/EBOOT.PBP"
sha256sum "$out/EBOOT.PBP"

# Separate data-only CD image reader and compile-only controller units.
out=$(dirname "$out")/cd-test
mkdir -p "$out"
echo "=== CD image reader and controller compile probe ==="
compile src/cdriso.cpp cd_iso
compile src/ppf.cpp cd_ppf
compile src/cdrom.cpp cd_controller
compile src/decode_xa.cpp cd_xa
compile /revival/revival/pcsx4all-probe/cd-test/cd_main.cpp cd_main
make -C "$out" -f /revival/revival/pcsx4all-probe/cd-test/Makefile
test -s "$out/EBOOT.PBP"
sha256sum "$out/EBOOT.PBP"
