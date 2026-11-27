/* SPDX-License-Identifier: GPL-2.0-or-later */
#include "audio_queue.h"
#include <assert.h>
#include <stdio.h>
static rv_audio_queue q;
static int16_t in[RV_AUDIO_CAPACITY*2],out[RV_AUDIO_BLOCK*2];

/* Independent linear FIFO model; deterministic mixed chunk sizes and silence. */
static int16_t model[RV_AUDIO_CAPACITY*2];
static unsigned model_count;
static uint32_t accepted,dropped,nonzero,rng=0x13579bdfu;
static unsigned next_sample(void){
    rng=rng*1664525u+1013904223u;
    return rng>>16;
}
static void compare_state(void){
    assert(q.count==model_count&&q.read<RV_AUDIO_CAPACITY);
    assert(q.accepted==accepted&&q.dropped==dropped&&q.nonzero==nonzero);
    for(unsigned i=0;i<model_count;i++){
        unsigned j=(q.read+i)%RV_AUDIO_CAPACITY;
        assert(q.pcm[2*j]==model[2*i]&&q.pcm[2*j+1]==model[2*i+1]);
    }
    if(model_count>=RV_AUDIO_BLOCK){
        assert(rv_audio_peek(&q,out)&&!memcmp(out,model,sizeof(out)));
        /* Repeated peeks cannot consume or reorder frames. */
        assert(rv_audio_peek(&q,out)&&!memcmp(out,model,sizeof(out)));
        assert(q.count==model_count);
    }else{
        memset(out,0x5a,sizeof(out));
        assert(!rv_audio_peek(&q,out));
        const unsigned char *p=(const unsigned char *)out;
        for(size_t i=0;i<sizeof(out);i++)assert(p[i]==0x5a);
    }
}
static void model_push(unsigned frames){
    /* Offset by one byte to verify the public void* input is not required
       to be sample-aligned. */
    static unsigned char unaligned[sizeof(in)+1];
    for(unsigned i=0;i<frames;i++){
        in[2*i]=(int16_t)((int)(next_sample()%60001u)-30000);
        in[2*i+1]=(int16_t)((int)(next_sample()%60001u)-30000);
        if(i%7u==0)in[2*i]=in[2*i+1]=0;
    }
    memcpy(unaligned+1,in,frames*4u);
    unsigned n=RV_AUDIO_CAPACITY-model_count;
    if(n>frames)n=frames;
    assert(rv_audio_push(&q,unaligned+1,frames)==n);
    memcpy(model+2*model_count,in,n*4u);
    for(unsigned i=0;i<n;i++)
        if(in[2*i]||in[2*i+1])nonzero++;
    model_count+=n;accepted+=n;dropped+=frames-n;
    compare_state();
}
static void model_consume(void){
    rv_audio_consume(&q);
    if(model_count>=RV_AUDIO_BLOCK){
        model_count-=RV_AUDIO_BLOCK;
        memmove(model,model+2*RV_AUDIO_BLOCK,model_count*4u);
    }
    compare_state();
}
static void test_model(void){
    rv_audio_reset(&q);model_count=accepted=dropped=nonzero=0;
    compare_state();
    assert(rv_audio_push(&q,NULL,0)==0);
    model_consume();
    model_push(RV_AUDIO_BLOCK-1);
    model_consume(); /* incomplete block remains queued */
    model_push(1);
    model_consume();
    model_push(RV_AUDIO_CAPACITY-3);
    model_push(9); /* partial acceptance at the ring boundary */
    for(unsigned step=0;step<4096;step++){
        unsigned action=next_sample()%5u;
        if(action<2u)model_push(next_sample()%1025u);
        else model_consume();
    }
    while(model_count>=RV_AUDIO_BLOCK)model_consume();
    rv_audio_reset(&q);model_count=accepted=dropped=nonzero=0;
    compare_state();
}

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
    test_model();
    puts("audio queue: PASS (partial blocks, stereo order, wrap, overflow, no consume on peek, 4096 model operations)");
    return 0;
}
