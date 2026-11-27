/* SPDX-License-Identifier: GPL-2.0-or-later */
#include <pspkernel.h>
#include <psppower.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include "perf.h"

int revival_perf_mode = REVIVAL_PERF_LEGACY;
int revival_perf_native;
int revival_perf_direct;
static uint64_t stages[4];
static unsigned int native_hits;
static int active;
static unsigned int samples, laces, vblanks, last_cycle;
static uint64_t start, cap_us, draw_us;
static const char *names[] = {"legacy", "single", "uncapped"};

void revival_perf_init(void) {
    char mode[24];
    FILE *f = fopen("PERF_MODE.TXT", "rb");
    if (!f) return;
    if (fscanf(f, "%23s", mode) == 1) {
        if (!strcmp(mode, "single")) revival_perf_mode = REVIVAL_PERF_SINGLE;
        else if (!strcmp(mode, "uncapped")) revival_perf_mode = REVIVAL_PERF_UNCAPPED;
        else if (!strcmp(mode, "direct")) { revival_perf_mode = REVIVAL_PERF_SINGLE; revival_perf_direct = 1; }
        else if (!strcmp(mode, "native")) { revival_perf_mode = REVIVAL_PERF_SINGLE; revival_perf_native = 1; }
    }
    fclose(f);
}

void revival_perf_begin(void) {
    FILE *f = fopen("PERF.CSV", "wb");
    if (!f) return;
    fprintf(f, "mode,cpu_mhz,bus_mhz,wall_us,laces,counter_vblanks,legacy_cycle_delta,cap_us,swap_us,convert_us,scale_us,sdl_blit_us,flip_us,native_hits\n");
    fclose(f);
    samples = laces = vblanks = last_cycle = 0;
    cap_us = draw_us = 0;
    memset(stages, 0, sizeof(stages));
    native_hits = 0;
    start = sceKernelGetSystemTimeWide();
    active = 1;
}

void revival_perf_vblank(void) { if (active) ++vblanks; }

void revival_perf_cap(void (*fn)(void)) {
    uint64_t t;
    if (!active) { fn(); return; }
    t = sceKernelGetSystemTimeWide();
    fn();
    cap_us += sceKernelGetSystemTimeWide() - t;
}

void revival_perf_draw(void (*fn)(void)) {
    uint64_t t;
    if (!active) { fn(); return; }
    t = sceKernelGetSystemTimeWide();
    fn();
    draw_us += sceKernelGetSystemTimeWide() - t;
}

void revival_perf_lace(unsigned int cycle) {
    uint64_t now;
    FILE *f;
    if (!active) return;
    ++laces;
    now = sceKernelGetSystemTimeWide();
    if (now - start < 2000000ULL) return;
    f = fopen("PERF.CSV", "ab");
    if (!f) { active = 0; return; }
    fprintf(f, "%s,%d,%d,%llu,%u,%u,%u,%llu,%llu,%llu,%llu,%llu,%llu,%u\n",
            revival_perf_direct ? "direct" : revival_perf_native ? "native" : names[revival_perf_mode], scePowerGetCpuClockFrequencyInt(),
            scePowerGetBusClockFrequencyInt(), (unsigned long long)(now - start),
            laces, vblanks, cycle - last_cycle,
            (unsigned long long)cap_us, (unsigned long long)draw_us,
            (unsigned long long)stages[0], (unsigned long long)stages[1],
            (unsigned long long)stages[2], (unsigned long long)stages[3], native_hits);
    fclose(f);
    last_cycle = cycle;
    laces = vblanks = 0;
    cap_us = draw_us = 0;
    memset(stages, 0, sizeof(stages));
    native_hits = 0;
    start = sceKernelGetSystemTimeWide();
    if (++samples >= 60) active = 0;
}

unsigned long long revival_perf_now(void) { return active ? sceKernelGetSystemTimeWide() : 0; }
void revival_perf_stage(unsigned int stage, unsigned long long t) { if (active && t && stage < 4) stages[stage] += sceKernelGetSystemTimeWide() - t; }
void revival_perf_native_hit(void) { if (active) ++native_hits; }
