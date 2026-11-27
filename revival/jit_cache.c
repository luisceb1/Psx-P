/* SPDX-License-Identifier: GPL-2.0-or-later */
#include "jit_cache.h"
#include <string.h>
void revival_jit_cache_reset(struct revival_jit_cache *c) { memset(c,0,sizeof(*c)); }
static struct revival_jit_slot *lookup(struct revival_jit_cache *c,
    uint32_t pc,const uint32_t *live,size_t avail,int *fresh,int hot) {
    struct revival_jit_slot *s;
    size_t n,k,min=hot?REVIVAL_JIT_HOT_MIN_GUEST:REVIVAL_JIT_MIN_GUEST;
    if(fresh)*fresh=0;
    if(!c||!live||!fresh||(pc&3u)||avail<min) return NULL;
    if(avail>REVIVAL_JIT_MAX_GUEST)avail=REVIVAL_JIT_MAX_GUEST;
    s=&c->slots[revival_jit_index(pc)];
    if(s->valid && s->pc==pc) {
        if((!hot || (s->code_count && s->guest_count>=min)) && s->guest_count<=avail && !memcmp(s->guest,live,s->guest_count*sizeof(uint32_t))) {
            ++c->hits;
            return s->code_count?s:NULL;
        }
        ++c->changed;
        s->valid=0;s->verified=0;
    }
    ++c->misses;
    /* Find the longest supported prefix. Each candidate emitter is bounded. */
    for(n=0;n<avail && revival_jit_supported(live[n]);++n) {}
    if(n<min && hot) {
        if(hot==2) {
            struct revival_jit_negative *neg=&c->negative[revival_jit_index(pc)];
            neg->pc=pc;neg->remaining=8;
        }
        ++c->rejected;return NULL;
    }
    s->valid=0;s->verified=0;s->pc=pc;
    if(n<min) {
        s->guest_count=REVIVAL_JIT_MIN_GUEST;s->code_count=0;
        memcpy(s->guest,live,s->guest_count*sizeof(uint32_t));
        s->valid=1;++c->rejected;return NULL;
    }
    k=hot==2 ? revival_jit_emit_regs(live,n,s->code,REVIVAL_JIT_MAX_WORDS) : revival_jit_emit(live,n,s->code,REVIVAL_JIT_MAX_WORDS);
    if(!k) { ++c->rejected;return NULL; }
    memcpy(s->guest,live,n*sizeof(uint32_t));
    s->guest_count=(unsigned int)n;s->code_count=(unsigned int)k;
    if(hot==2) { c->reg_guest+=(unsigned int)n;c->reg_host+=(unsigned int)k; }
    s->valid=1;*fresh=1;return s;
}

struct revival_jit_slot *revival_jit_lookup(struct revival_jit_cache *c,uint32_t pc,const uint32_t *live,size_t avail,int *fresh) { return lookup(c,pc,live,avail,fresh,0); }
struct revival_jit_slot *revival_jit_lookup_hot(struct revival_jit_cache *c,uint32_t pc,const uint32_t *live,size_t avail,int *fresh) { return lookup(c,pc,live,avail,fresh,1); }

struct revival_jit_slot *revival_jit_lookup_regs(struct revival_jit_cache *c,uint32_t pc,const uint32_t *live,size_t avail,int *fresh) { return lookup(c,pc,live,avail,fresh,2); }
