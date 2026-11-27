/* SPDX-License-Identifier: GPL-2.0-or-later */
#include "jit_cache.h"
#include <string.h>
void revival_jit_cache_reset(struct revival_jit_cache *c) { memset(c,0,sizeof(*c)); }
const struct revival_jit_slot *revival_jit_lookup(struct revival_jit_cache *c,
    uint32_t pc,const uint32_t *live,size_t avail,int *fresh) {
    struct revival_jit_slot *s;
    size_t n,k;
    if(fresh)*fresh=0;
    if(!c||!live||!fresh||(pc&3u)||avail<REVIVAL_JIT_MIN_GUEST) return NULL;
    if(avail>REVIVAL_JIT_MAX_GUEST)avail=REVIVAL_JIT_MAX_GUEST;
    s=&c->slots[((pc>>2)^(pc>>11))&(REVIVAL_JIT_CACHE_SLOTS-1u)];
    if(s->valid && s->pc==pc) {
        if(s->guest_count<=avail && !memcmp(s->guest,live,s->guest_count*sizeof(uint32_t))) {
            ++c->hits;
            return s->code_count?s:NULL;
        }
        ++c->changed;
    }
    ++c->misses;s->valid=0;
    /* Find the longest supported prefix. Each candidate emitter is bounded. */
    for(n=0;n<avail && revival_jit_supported(live[n]);++n) {}
    s->pc=pc;
    if(n<REVIVAL_JIT_MIN_GUEST) {
        s->guest_count=REVIVAL_JIT_MIN_GUEST;s->code_count=0;
        memcpy(s->guest,live,s->guest_count*sizeof(uint32_t));
        s->valid=1;++c->rejected;return NULL;
    }
    k=revival_jit_emit(live,n,s->code,REVIVAL_JIT_MAX_WORDS);
    if(!k) { ++c->rejected;return NULL; }
    memcpy(s->guest,live,n*sizeof(uint32_t));
    s->guest_count=(unsigned int)n;s->code_count=(unsigned int)k;
    s->valid=1;*fresh=1;return s;
}
