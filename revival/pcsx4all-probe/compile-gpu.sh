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

# Fourth subsystem: link actual CD controller into a separate combined candidate.
cdout=$out
out=$(dirname "$out")/integration-test
cp "$cdout"/cd_iso.o "$cdout"/cd_ppf.o "$cdout"/cd_controller.o "$cdout"/cd_xa.o "$out/"
for pair in "guest-test/platform_stubs.cpp:integration_cd_stubs" "integration-test/integration_main.cpp:integration_cd_main"; do
    unit=${pair%:*}
    name=${pair#*:}
    psp-g++ -std=gnu++11 -O2 -G0 -fno-rtti -fno-strict-aliasing \
        -D__PSP__ -DPSXREC -DREVIVAL_PSP_PROBE -DREVIVAL_GPU_INTEGRATION -DREVIVAL_CD_INTEGRATION \
        -D'INLINE=static __inline__' -D'asm=__asm__ __volatile__' \
        -I"$sdk/include" -Isrc -Isrc/port/sdl -Isrc/plugin_lib \
        -Isrc/gpu/gpu_unai -Isrc/spu/spu_pcsxrearmed \
        -DGPU_UNAI -Dspu_pcsxrearmed -DSPU_PCSXREARMED -DUSE_GPULIB \
        -c "/revival/revival/pcsx4all-probe/$unit" -o "$out/$name.o"
done
cdcombined=$(dirname "$out")/integration-cd-test
mkdir -p "$cdcombined"
for obj in core-relocatable gpu_core gpu_unai integration_vout dma_core hw_core cd_iso cd_ppf cd_controller cd_xa integration_cd_stubs integration_cd_main; do
    cp "$out/$obj.o" "$cdcombined/"
done
make -C "$cdcombined" -f /revival/revival/pcsx4all-probe/integration-test/Makefile CD_INTEGRATION=1
test -s "$cdcombined/EBOOT.PBP"
sha256sum "$cdcombined/EBOOT.PBP"

# Keep prior CD integration EBOOT intact; link loader in a new candidate.
out=$(dirname "$out")/exe-test
mkdir -p "$out"
for obj in core-relocatable gpu_core gpu_unai integration_vout dma_core hw_core cd_iso cd_ppf cd_controller cd_xa; do
    cp "$cdcombined/$obj.o" "$out/"
done
compile src/misc.cpp exe_loader
for pair in "guest-test/platform_stubs.cpp:integration_cd_stubs" "integration-test/integration_main.cpp:integration_cd_main"; do
    unit=${pair%:*}
    name=${pair#*:}
    psp-g++ -std=gnu++11 -O2 -G0 -fno-rtti -fno-strict-aliasing \
        -D__PSP__ -DPSXREC -DREVIVAL_PSP_PROBE -DREVIVAL_GPU_INTEGRATION -DREVIVAL_CD_INTEGRATION -DREVIVAL_EXE_INTEGRATION \
        -D'INLINE=static __inline__' -D'asm=__asm__ __volatile__' \
        -I"$sdk/include" -Isrc -Isrc/port/sdl -Isrc/plugin_lib \
        -Isrc/gpu/gpu_unai -Isrc/spu/spu_pcsxrearmed \
        -DGPU_UNAI -Dspu_pcsxrearmed -DSPU_PCSXREARMED -DUSE_GPULIB \
        -c "/revival/revival/pcsx4all-probe/$unit" -o "$out/$name.o"
done
make -C "$out" -f /revival/revival/pcsx4all-probe/integration-test/Makefile CD_INTEGRATION=1 EXE_INTEGRATION=1
test -s "$out/EBOOT.PBP"
sha256sum "$out/EBOOT.PBP"

# Preserve EXE candidate, link the remaining HLE/MDEC/SIO/PAD units separately.
exeout=$out
out=$(dirname "$out")/hle-test
mkdir -p "$out"
for obj in core-relocatable gpu_core gpu_unai integration_vout dma_core hw_core cd_iso cd_ppf cd_controller cd_xa exe_loader; do
    cp "$exeout/$obj.o" "$out/"
done
compile src/psxbios.cpp bios_core
compile src/psxhle.cpp hle_core
compile src/mdec.cpp mdec_core
compile src/sio.cpp sio_core
compile src/pad.cpp pad_core
for pair in "guest-test/platform_stubs.cpp:integration_cd_stubs" "integration-test/integration_main.cpp:integration_cd_main"; do
    unit=${pair%:*}
    name=${pair#*:}
    psp-g++ -std=gnu++11 -O2 -G0 -fno-rtti -fno-strict-aliasing \
        -D__PSP__ -DPSXREC -DREVIVAL_PSP_PROBE -DREVIVAL_GPU_INTEGRATION -DREVIVAL_CD_INTEGRATION -DREVIVAL_EXE_INTEGRATION -DREVIVAL_HLE_INTEGRATION \
        -D'INLINE=static __inline__' -D'asm=__asm__ __volatile__' \
        -I"$sdk/include" -Isrc -Isrc/port/sdl -Isrc/plugin_lib \
        -Isrc/gpu/gpu_unai -Isrc/spu/spu_pcsxrearmed \
        -DGPU_UNAI -Dspu_pcsxrearmed -DSPU_PCSXREARMED -DUSE_GPULIB \
        -c "/revival/revival/pcsx4all-probe/$unit" -o "$out/$name.o"
done
make -C "$out" -f /revival/revival/pcsx4all-probe/integration-test/Makefile CD_INTEGRATION=1 EXE_INTEGRATION=1 HLE_INTEGRATION=1
test -s "$out/EBOOT.PBP"
sha256sum "$out/EBOOT.PBP"

# Compile the original single-thread SPU and null output backend separately.
out=$(dirname "$out")/spu-objects
mkdir -p "$out"
echo "=== SPU portable C compile probe ==="
for unit in spu dma freeze out nullsnd registers; do
    psp-gcc -std=gnu99 -O2 -G0 -fno-strict-aliasing \
        -D__PSP__ -DPSXREC -DREVIVAL_PSP_PROBE \
        -D'INLINE=static __inline__' \
        -I"$sdk/include" -Isrc -Isrc/port/sdl -Isrc/spu/spu_pcsxrearmed \
        -DSPU_PCSXREARMED -c "src/spu/spu_pcsxrearmed/$unit.c" -o "$out/$unit.o"
    echo "PASS SPU $unit"
done
psp-gcc -r -nostdlib "$out"/*.o -o "$out/spu-relocatable.o"
psp-nm -u "$out/spu-relocatable.o"
psp-size "$out/spu-relocatable.o"

# Link the complete diagnostic with original SPU, preserving HLE-only candidate.
spuout=$out
if psp-nm -u "$spuout/spu-relocatable.o" | grep -Eq 'pthread_|sem_(init|wait|post|destroy)'; then
    echo "Unexpected SPU POSIX worker dependency" >&2; exit 1
fi
hleout=$(dirname "$out")/hle-test
out=$(dirname "$out")/full-test
mkdir -p "$out"
for obj in core-relocatable gpu_core gpu_unai integration_vout dma_core hw_core cd_iso cd_ppf cd_controller cd_xa exe_loader bios_core hle_core mdec_core sio_core pad_core; do
    cp "$hleout/$obj.o" "$out/"
done
cp "$spuout/spu-relocatable.o" "$out/"
for pair in "guest-test/platform_stubs.cpp:integration_cd_stubs" "integration-test/integration_main.cpp:integration_cd_main"; do
    unit=${pair%:*}
    name=${pair#*:}
    psp-g++ -std=gnu++11 -O2 -G0 -fno-rtti -fno-strict-aliasing \
        -D__PSP__ -DPSXREC -DREVIVAL_PSP_PROBE -DREVIVAL_GPU_INTEGRATION -DREVIVAL_CD_INTEGRATION -DREVIVAL_EXE_INTEGRATION -DREVIVAL_HLE_INTEGRATION -DREVIVAL_SPU_INTEGRATION \
        -D'INLINE=static __inline__' -D'asm=__asm__ __volatile__' \
        -I"$sdk/include" -Isrc -Isrc/port/sdl -Isrc/plugin_lib \
        -Isrc/gpu/gpu_unai -Isrc/spu/spu_pcsxrearmed \
        -DGPU_UNAI -Dspu_pcsxrearmed -DSPU_PCSXREARMED -DUSE_GPULIB \
        -c "/revival/revival/pcsx4all-probe/$unit" -o "$out/$name.o"
done
make -C "$out" -f /revival/revival/pcsx4all-probe/integration-test/Makefile CD_INTEGRATION=1 EXE_INTEGRATION=1 HLE_INTEGRATION=1 SPU_INTEGRATION=1
test -s "$out/EBOOT.PBP"
sha256sum "$out/EBOOT.PBP"

# Independent PSP Go pause/suspend probe: no emulator core, no SDL, no GU.
out=$(dirname "$out")/pause-test
mkdir -p "$out"
psp-gcc -std=gnu99 -O2 -G0 -I"$sdk/include" -c /revival/revival/pcsx4all-probe/pause-test/pause_main.c -o "$out/pause_main.o"
make -C "$out" -f /revival/revival/pcsx4all-probe/pause-test/Makefile
test -s "$out/EBOOT.PBP"
sha256sum "$out/EBOOT.PBP"

# Standalone read-only disc inspector, no emulation objects.
out=$(dirname "$out")/inspect-test
mkdir -p "$out"
psp-gcc -std=gnu99 -O2 -G0 -I"$sdk/include" -c /revival/revival/pcsx4all-probe/inspect-test/inspect_main.c -o "$out/inspect_main.o"
psp-gcc -std=gnu99 -O2 -G0 -I"$sdk/include" -c /revival/revival/storage.c -o "$out/storage.o"
make -C "$out" -f /revival/revival/pcsx4all-probe/inspect-test/Makefile
test -s "$out/EBOOT.PBP"
sha256sum "$out/EBOOT.PBP"

# Independent native output + genuine SPU mixer probe. Full null-SPU EBOOT unchanged.
out=$(dirname "$out")/audio-test
mkdir -p "$out"
cp "$spuout/spu-relocatable.o" "$out/"
for unit in audio_main audio_psp; do
    psp-gcc -std=gnu99 -O2 -G0 -I"$sdk/include" -Isrc/spu/spu_pcsxrearmed \
        -c "/revival/revival/pcsx4all-probe/audio-test/$unit.c" -o "$out/$unit.o"
done
make -C "$out" -f /revival/revival/pcsx4all-probe/audio-test/Makefile
test -s "$out/EBOOT.PBP"
sha256sum "$out/EBOOT.PBP"
