/* SPDX-License-Identifier: GPL-2.0-or-later */
#ifndef REVIVAL_JIT_SHADOW_H
#define REVIVAL_JIT_SHADOW_H
#include <stdio.h>
extern int revival_jit_shadow_enabled;
extern int revival_jit_native_enabled;
extern int revival_jit_hot_enabled;
extern int revival_jit_regs_enabled;
void revival_jit_shadow_snapshot(FILE *f);
#endif
