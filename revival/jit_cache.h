/* SPDX-License-Identifier: GPL-2.0-or-later */
#ifndef REVIVAL_JIT_CACHE_H
#define REVIVAL_JIT_CACHE_H
#include "jit_emit.h"
#define REVIVAL_JIT_CACHE_SLOTS 128u
#define REVIVAL_JIT_MIN_GUEST 4u
#define REVIVAL_JIT_HOT_MIN_GUEST 6u
struct revival_jit_slot {
    uint32_t pc;
    unsigned int valid, guest_count, code_count, verified;
    uint32_t guest[REVIVAL_JIT_MAX_GUEST];
    uint32_t code[REVIVAL_JIT_MAX_WORDS] __attribute__((aligned(64)));
};
struct revival_jit_negative { uint32_t pc; unsigned int remaining; };
struct revival_jit_cache {
    struct revival_jit_slot slots[REVIVAL_JIT_CACHE_SLOTS];
    unsigned int hits, misses, changed, rejected;
    struct revival_jit_negative negative[REVIVAL_JIT_CACHE_SLOTS];
    unsigned int suppressed, reg_guest, reg_host;
};
/* live points to readable current instruction words; avail excludes page boundary.
 * Cache hit requires exact guest-word match, not only PC or a hash.
 * fresh is set for newly generated code requiring host cache synchronization. */
struct revival_jit_slot *revival_jit_lookup(struct revival_jit_cache *c,
    uint32_t pc,const uint32_t *live,size_t avail,int *fresh);
/* Positive hits are checked at every use; cold addresses are sampled.
 * A negative decision only chooses interpreter fallback, never native code. */
static inline unsigned int revival_jit_index(uint32_t pc) {
    return ((pc>>2)^(pc>>11))&(REVIVAL_JIT_CACHE_SLOTS-1u);
}
static inline int revival_jit_hot_probe(const struct revival_jit_cache *c,
    uint32_t pc,unsigned int *cold) {
    const struct revival_jit_slot *s=&c->slots[revival_jit_index(pc)];
    if(s->valid && s->code_count && s->pc==pc) return 1;
    return (++*cold & 15u)==0;
}
struct revival_jit_slot *revival_jit_lookup_hot(struct revival_jit_cache *c,
    uint32_t pc,const uint32_t *live,size_t avail,int *fresh);
/* Positive blocks always take priority. Negative entries only delay retries,
 * never execute code; source changes can at worst delay optimization. */
static inline int revival_jit_reg_probe(struct revival_jit_cache *c,
    uint32_t pc,unsigned int *cold) {
    unsigned int idx=revival_jit_index(pc);
    const struct revival_jit_slot *s=&c->slots[idx];
    struct revival_jit_negative *neg=&c->negative[idx];
    if(s->valid && s->code_count && s->pc==pc) return 1;
    if((++*cold & 15u)!=0) return 0;
    if(neg->remaining && neg->pc==pc) {
        --neg->remaining;++c->suppressed;return 0;
    }
    return 1;
}
struct revival_jit_slot *revival_jit_lookup_regs(struct revival_jit_cache *c,
    uint32_t pc,const uint32_t *live,size_t avail,int *fresh);
void revival_jit_cache_reset(struct revival_jit_cache *c);
#endif
