/* SPDX-License-Identifier: GPL-2.0-or-later */
#include "jit_cache.h"
#include <assert.h>
#include <stdio.h>
static struct revival_jit_cache c;
int main(void) {
    uint32_t live[8]={0x24010001,0x24210001,0x24210001,0x24210001,0x08000000,0,0,0};
    struct revival_jit_slot *s,*again;
    int fresh=-1;
    revival_jit_cache_reset(&c);
    s=revival_jit_lookup(&c,0x80010000,live,8,&fresh);
    assert(s && fresh && s->guest_count==4 && c.misses==1);
    s->verified=3;
    again=revival_jit_lookup(&c,0x80010000,live,8,&fresh);
    assert(again==s && !fresh && c.hits==1 && again->verified==3);
    live[2]=0x24210002;
    s=revival_jit_lookup(&c,0x80010000,live,8,&fresh);
    assert(s && fresh && s->guest[2]==live[2] && c.changed==1 && s->verified==0);
    live[2]=0x08000000;
    assert(!revival_jit_lookup(&c,0x80010000,live,8,&fresh) && !fresh && c.rejected==1);
    assert(!revival_jit_lookup(&c,0x80010000,live,8,&fresh) && c.hits==2);
    live[2]=0x24210001;
    assert(revival_jit_lookup(&c,0x80010000,live,8,&fresh) && fresh && c.changed==3);
    assert(!revival_jit_lookup(&c,0x80010000,live,3,&fresh));
    assert(!revival_jit_lookup(&c,0x80010001,live,8,&fresh));
    assert(revival_jit_lookup(&c,0xa0010000,live,8,&fresh) && fresh);
    revival_jit_cache_reset(&c);
    assert(c.hits==0 && c.misses==0 && c.changed==0);
    puts("JIT cache tests passed");return 0;
}
