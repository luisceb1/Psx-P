/* SPDX-License-Identifier: GPL-2.0-or-later */
#include "audio_clock.h"
#include <assert.h>
#include <stdio.h>
int main(void) {
    struct revival_audio_clock c={0};
    unsigned int i,n=0;
    assert(revival_audio_clock_add(&c,0)==0);
    assert(revival_audio_clock_add(&c,REVIVAL_AUDIO_BLOCK_CYCLES-1)==0);
    assert(revival_audio_clock_add(&c,1)==1 && c.remainder==0);
    for(i=0;i<1000;i++) n+=revival_audio_clock_add(&c,49152);
    assert(n==49152000u/REVIVAL_AUDIO_BLOCK_CYCLES);
    assert(c.remainder==49152000u%REVIVAL_AUDIO_BLOCK_CYCLES);
    assert(revival_audio_clock_add(&c,UINT32_MAX)==REVIVAL_AUDIO_MAX_BLOCKS);
    assert(c.remainder<REVIVAL_AUDIO_BLOCK_CYCLES);
    puts("audio clock tests passed");return 0;
}
