/* SPDX-License-Identifier: GPL-2.0-or-later */
#include <pspkernel.h>
#include <psppower.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include "perf.h"
#include "boottrace.h"
#ifdef REVIVAL_PEOPS_AUDIO
#include "audio_backend.h"
#endif

int revival_perf_mode = REVIVAL_PERF_LEGACY;
int revival_perf_native;
int revival_perf_direct;
int revival_cd_ack_fix;
int revival_timer_fix;
int revival_fastcore;
#if REVIVAL_OPT_LEVEL == 2
#define REVIVAL_OPT_LABEL "-O2"
#elif REVIVAL_OPT_LEVEL == 1
#define REVIVAL_OPT_LABEL "-Os"
#else
#define REVIVAL_OPT_LABEL "unknown"
#endif
unsigned int revival_timer_events[3], revival_timer_irqs[3], revival_hw_exceptions;
struct revival_cd_trace revival_cd_trace;
static uint64_t stages[4];
static unsigned int native_hits;
static unsigned int present_requests, present_skipped, draw_phase;
static unsigned int sample_divisor = 1;
static uint64_t component_us[3];
static unsigned int component_calls[3];
static const char *selected_mode;
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
        else if (!strcmp(mode, "direct") || !strcmp(mode, "half") || !strcmp(mode, "ceiling") || !strcmp(mode, "boottrace") || !strcmp(mode, "bootfix") || !strcmp(mode, "irqfix") || !strcmp(mode, "fastcore")) {
            revival_perf_mode = REVIVAL_PERF_SINGLE;
            revival_perf_direct = 1;
            if (!strcmp(mode, "half")) { sample_divisor = 2; selected_mode = "half"; }
            else if (!strcmp(mode, "ceiling")) { sample_divisor = 0; selected_mode = "ceiling"; }
            else if (!strcmp(mode, "boottrace") || !strcmp(mode, "bootfix") || !strcmp(mode, "irqfix") || !strcmp(mode, "fastcore")) {
                sample_divisor = 2;
                revival_fastcore = !strcmp(mode, "fastcore");
                revival_timer_fix = revival_fastcore || !strcmp(mode, "irqfix");
                revival_cd_ack_fix = revival_timer_fix || !strcmp(mode, "bootfix");
                selected_mode = revival_fastcore ? "fastcore" : revival_timer_fix ? "irqfix" : revival_cd_ack_fix ? "bootfix" : "boottrace";
            }
        }
        else if (!strcmp(mode, "native")) { revival_perf_mode = REVIVAL_PERF_SINGLE; revival_perf_native = 1; }
    }
    fclose(f);
}

void revival_perf_begin(void) {
    FILE *f = fopen("PERF.CSV", "wb");
    if (!f) return;
    fprintf(f, "mode,cpu_mhz,bus_mhz,wall_us,laces,counter_vblanks,legacy_cycle_delta,cap_us,swap_us,convert_us,scale_us,sdl_blit_us,flip_us,native_hits,present_requests,present_skipped,gpu_bulk_us,gpu_bulk_calls,cd_us,cd_calls,log_us,log_calls,compiler_opt,audio_produced,audio_consumed,audio_silent,audio_dropped,audio_nonzero");
    revival_boot_header(f);
    fclose(f);
    samples = laces = vblanks = last_cycle = draw_phase = 0;
    memset(&revival_cd_trace, 0, sizeof(revival_cd_trace));
    cap_us = draw_us = 0;
    memset(stages, 0, sizeof(stages));
    native_hits = 0;
    present_requests = present_skipped = 0;
    memset(component_us, 0, sizeof(component_us));
    memset(component_calls, 0, sizeof(component_calls));
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
    int skip = sample_divisor == 0 || (sample_divisor > 1 && (draw_phase++ % sample_divisor) != 0);
    if (active) ++present_requests;
    if (skip) { if (active) ++present_skipped; return; }
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
    fprintf(f, "%s,%d,%d,%llu,%u,%u,%u,%llu,%llu,%llu,%llu,%llu,%llu,%u,%u,%u,%llu,%u,%llu,%u,%llu,%u",
            selected_mode ? selected_mode : revival_perf_direct ? "direct" : revival_perf_native ? "native" : names[revival_perf_mode], scePowerGetCpuClockFrequencyInt(),
            scePowerGetBusClockFrequencyInt(), (unsigned long long)(now - start),
            laces, vblanks, cycle - last_cycle,
            (unsigned long long)cap_us, (unsigned long long)draw_us,
            (unsigned long long)stages[0], (unsigned long long)stages[1],
            (unsigned long long)stages[2], (unsigned long long)stages[3], native_hits,
            present_requests, present_skipped,
            (unsigned long long)component_us[0], component_calls[0],
            (unsigned long long)component_us[1], component_calls[1],
            (unsigned long long)component_us[2], component_calls[2]);
    fprintf(f, ",%s", REVIVAL_OPT_LABEL);
#ifdef REVIVAL_PEOPS_AUDIO
    revival_audio_snapshot(f);
#else
    fprintf(f,",0,0,0,0,0");
#endif
    revival_boot_snapshot(f);
    fclose(f);
    last_cycle = cycle;
    laces = vblanks = 0;
    cap_us = draw_us = 0;
    memset(stages, 0, sizeof(stages));
    native_hits = 0;
    present_requests = present_skipped = 0;
    memset(component_us, 0, sizeof(component_us));
    memset(component_calls, 0, sizeof(component_calls));
    start = sceKernelGetSystemTimeWide();
    if (++samples >= 60) active = 0;
}

unsigned long long revival_perf_now(void) { return active ? sceKernelGetSystemTimeWide() : 0; }
void revival_perf_stage(unsigned int stage, unsigned long long t) { if (active && t && stage < 4) stages[stage] += sceKernelGetSystemTimeWide() - t; }
void revival_perf_native_hit(void) { if (active) ++native_hits; }

void revival_perf_component(unsigned int component, unsigned long long t) {
    if (active && t && component < 3) {
        component_us[component] += sceKernelGetSystemTimeWide() - t;
        ++component_calls[component];
    }
}
