/* SPDX-License-Identifier: GPL-2.0-or-later */
#ifndef REVIVAL_AUDIO_QUEUE_H
#define REVIVAL_AUDIO_QUEUE_H
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#define RV_AUDIO_BLOCK 512u
#define RV_AUDIO_CAPACITY 4096u
typedef struct {
    int16_t pcm[RV_AUDIO_CAPACITY*2];
    unsigned read,count;
    uint32_t accepted,dropped,nonzero;
} rv_audio_queue;
static inline void rv_audio_reset(rv_audio_queue *q){memset(q,0,sizeof(*q));}
static inline unsigned rv_audio_push(rv_audio_queue *q,const void *data,unsigned frames){
    const unsigned char *bytes=(const unsigned char *)data;
    unsigned n=frames,room=RV_AUDIO_CAPACITY-q->count;
    if(n>room)n=room;
    for(unsigned i=0;i<n;i++){
        unsigned j=(q->read+q->count+i)%RV_AUDIO_CAPACITY;
        memcpy(&q->pcm[2*j],bytes+4*i,4);
        if(q->pcm[2*j]||q->pcm[2*j+1])q->nonzero++;
    }
    q->count+=n;q->accepted+=n;q->dropped+=frames-n;return n;
}
static inline int rv_audio_peek(const rv_audio_queue *q,int16_t out[RV_AUDIO_BLOCK*2]){
    if(q->count<RV_AUDIO_BLOCK)return 0;
    for(unsigned i=0;i<RV_AUDIO_BLOCK;i++){
        unsigned j=(q->read+i)%RV_AUDIO_CAPACITY;
        out[2*i]=q->pcm[2*j];out[2*i+1]=q->pcm[2*j+1];
    }
    return 1;
}
static inline void rv_audio_consume(rv_audio_queue *q){
    if(q->count>=RV_AUDIO_BLOCK){
        q->read=(q->read+RV_AUDIO_BLOCK)%RV_AUDIO_CAPACITY;q->count-=RV_AUDIO_BLOCK;
    }
}
#endif
