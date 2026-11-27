/* SPDX-License-Identifier: GPL-2.0-or-later */
#ifndef REVIVAL_JIT_CACHE_H
#define REVIVAL_JIT_CACHE_H
#include "jit_emit.h"
#define REVIVAL_JIT_CACHE_SLOTS 128u
#define REVIVAL_JIT_MIN_GUEST 4u
struct revival_jit_slot {
    uint32_t pc;
    unsigned int valid, guest_count, code_count, verified;
    uint32_t guest[REVIVAL_JIT_MAX_GUEST];
    uint32_t code[REVIVAL_JIT_MAX_WORDS] __attribute__((aligned(64)));
};
struct revival_jit_cache {
    struct revival_jit_slot slots[REVIVAL_JIT_CACHE_SLOTS];
    unsigned int hits, misses, changed, rejected;
};
/* live points to readable current instruction words; avail excludes page boundary.
 * Cache hit requires exact guest-word match, not only PC or a hash.
 * fresh is set for newly generated code requiring host cache synchronization. */
struct revival_jit_slot *revival_jit_lookup(struct revival_jit_cache *c,
    uint32_t pc,const uint32_t *live,size_t avail,int *fresh);
void revival_jit_cache_reset(struct revival_jit_cache *c);
#endif
