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
    /* A short colliding block must not evict a useful compiled block. */
    {
        uint32_t longblock[8]={0x24010001,0x24210001,0x24210001,0x24210001,0x24210001,0x24210001,0x08000000,0};
        uint32_t pc=0x80010000, other=pc+4;
        unsigned int cold=0,j;
        while(revival_jit_index(other)!=revival_jit_index(pc)) other+=4;
        for(j=0;j<15;j++) assert(!revival_jit_hot_probe(&c,pc,&cold));
        assert(revival_jit_hot_probe(&c,pc,&cold));
        s=revival_jit_lookup_hot(&c,pc,longblock,8,&fresh);
        assert(s && fresh && s->guest_count==6);
        s->verified=3;
        assert(revival_jit_hot_probe(&c,pc,&cold) && cold==16);
        assert(!revival_jit_lookup_hot(&c,other,live,8,&fresh));
        again=revival_jit_lookup_hot(&c,pc,longblock,8,&fresh);
        assert(again==s && !fresh && s->verified==3);
        longblock[2]=0x08000000;
        assert(!revival_jit_lookup_hot(&c,pc,longblock,8,&fresh));
        assert(!s->valid && !s->verified);
    }
    {
        uint32_t pc=0x80010000, other=pc+4;
        uint32_t longblock[6]={0x24210001,0x24210001,0x24210001,0x24210001,0x24210001,0x24210001};
        unsigned int cold=15,j;
        revival_jit_cache_reset(&c);
        while(revival_jit_index(other)!=revival_jit_index(pc)) other+=4;
        assert(revival_jit_reg_probe(&c,pc,&cold));
        assert(!revival_jit_lookup_regs(&c,pc,live,8,&fresh));
        for(j=0;j<8;j++) { cold=15;assert(!revival_jit_reg_probe(&c,pc,&cold)); }
        cold=15;assert(revival_jit_reg_probe(&c,pc,&cold));
        s=revival_jit_lookup_regs(&c,pc,longblock,6,&fresh);
        assert(s && fresh && s->code_count==10 && c.suppressed==8 && c.reg_guest==6 && c.reg_host==10);
        s->verified=3;
        assert(!revival_jit_lookup_regs(&c,other,live,8,&fresh));
        assert(revival_jit_reg_probe(&c,pc,&cold));
        assert(revival_jit_lookup_regs(&c,pc,longblock,6,&fresh)==s && !fresh && s->verified==3);
        longblock[0]=0x08000000;
        assert(!revival_jit_lookup_regs(&c,pc,longblock,6,&fresh) && !s->valid && !s->verified);
    }
    puts("JIT cache tests passed");return 0;
}
