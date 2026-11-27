/* SPDX-License-Identifier: GPL-2.0-or-later */
#include "jit_policy.h"
#include <assert.h>
#include <stdio.h>
int main(void) {
    unsigned int v=0;
    assert(!revival_jit_can_run(v,0));
    v=revival_jit_verified_next(v); assert(!revival_jit_can_run(v,0));
    v=revival_jit_verified_next(v); assert(!revival_jit_can_run(v,0));
    v=revival_jit_verified_next(v); assert(revival_jit_can_run(v,0));
    assert(revival_jit_verified_next(v)==v);
    assert(revival_jit_can_run(v,254));assert(!revival_jit_can_run(v,255));
    assert(revival_jit_can_run(v,256));
    puts("JIT policy tests passed");return 0;
}
