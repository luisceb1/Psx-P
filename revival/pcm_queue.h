/* SPDX-License-Identifier: GPL-2.0-or-later */
#ifndef REVIVAL_PCM_QUEUE_H
#define REVIVAL_PCM_QUEUE_H
#include <string.h>
#define REVIVAL_PCM_CAPACITY 16384u
struct revival_pcm_queue { unsigned int head,tail,used; unsigned char data[REVIVAL_PCM_CAPACITY]; };
static inline unsigned int revival_pcm_write(struct revival_pcm_queue *q,const unsigned char *p,unsigned int n) {
    unsigned int a;
    if(n>REVIVAL_PCM_CAPACITY-q->used) n=REVIVAL_PCM_CAPACITY-q->used;
    a=REVIVAL_PCM_CAPACITY-q->tail; if(a>n) a=n;
    memcpy(q->data+q->tail,p,a); memcpy(q->data,p+a,n-a);
    q->tail=(q->tail+n)%REVIVAL_PCM_CAPACITY; q->used+=n; return n;
}
static inline unsigned int revival_pcm_read(struct revival_pcm_queue *q,unsigned char *p,unsigned int n) {
    unsigned int k=n<q->used?n:q->used, a=REVIVAL_PCM_CAPACITY-q->head;
    if(a>k) a=k;
    memcpy(p,q->data+q->head,a); memcpy(p+a,q->data,k-a); memset(p+k,0,n-k);
    q->head=(q->head+k)%REVIVAL_PCM_CAPACITY;q->used-=k;return k;
}
#endif
