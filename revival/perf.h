/* SPDX-License-Identifier: GPL-2.0-or-later */
#ifndef REVIVAL_PERF_H
#define REVIVAL_PERF_H
enum { REVIVAL_PERF_LEGACY, REVIVAL_PERF_SINGLE, REVIVAL_PERF_UNCAPPED };
extern int revival_perf_mode;
extern int revival_perf_native;
extern int revival_perf_direct;
unsigned long long revival_perf_now(void);
void revival_perf_stage(unsigned int stage, unsigned long long start);
void revival_perf_native_hit(void);
/* Inclusive timings: GPU bulk dispatch, CD track read, diagnostic log. */
void revival_perf_component(unsigned int component, unsigned long long start);
void revival_perf_init(void);
void revival_perf_begin(void);
void revival_perf_lace(unsigned int cycle);
void revival_perf_vblank(void);
void revival_perf_cap(void (*fn)(void));
void revival_perf_draw(void (*fn)(void));
#endif
