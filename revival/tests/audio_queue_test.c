/* SPDX-License-Identifier: GPL-2.0-or-later */
#include "audio_queue.h"
#include <assert.h>
#include <stdio.h>
static rv_audio_queue q;
static int16_t in[RV_AUDIO_CAPACITY*2],out[RV_AUDIO_BLOCK*2];
int main(void){
    rv_audio_reset(&q);
    for(unsigned i=0;i<RV_AUDIO_CAPACITY;i++){in[2*i]=(int16_t)i;in[2*i+1]=-(int16_t)i;}
    assert(rv_audio_push(&q,in,256)==256&&!rv_audio_peek(&q,out));
    assert(rv_audio_push(&q,in+512,256)==256&&rv_audio_peek(&q,out));
    assert(!memcmp(out,in,sizeof(out))&&q.count==512);
    rv_audio_consume(&q);assert(q.count==0);
    assert(rv_audio_push(&q,in,RV_AUDIO_CAPACITY)==RV_AUDIO_CAPACITY);
    assert(rv_audio_push(&q,in,4)==0&&q.dropped==4);
    for(unsigned i=0;i<RV_AUDIO_CAPACITY/RV_AUDIO_BLOCK;i++){
        assert(rv_audio_peek(&q,out)&&!memcmp(out,in+2*i*RV_AUDIO_BLOCK,sizeof(out)));
        rv_audio_consume(&q);
    }
    assert(q.count==0&&!rv_audio_peek(&q,out)&&q.accepted==RV_AUDIO_CAPACITY+512);
    puts("audio queue: PASS (partial blocks, stereo order, wrap, overflow, no consume on peek)");
    return 0;
}
