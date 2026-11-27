/* SPDX-License-Identifier: GPL-2.0-or-later */
#include "pcm_queue.h"
#include <assert.h>
#include <stdio.h>
static struct revival_pcm_queue q;
static unsigned char in[REVIVAL_PCM_CAPACITY],out[REVIVAL_PCM_CAPACITY];
int main(void) {
    unsigned int i;
    for(i=0;i<sizeof(in);i++) in[i]=(unsigned char)(i*17u);
    assert(revival_pcm_read(&q,out,16)==0);
    for(i=0;i<16;i++) assert(out[i]==0);
    assert(revival_pcm_write(&q,in,sizeof(in))==sizeof(in));
    assert(revival_pcm_write(&q,in,4)==0);
    assert(revival_pcm_read(&q,out,100)==100 && !memcmp(in,out,100));
    assert(revival_pcm_write(&q,in,100)==100);
    assert(revival_pcm_read(&q,out,sizeof(out))==sizeof(out));
    assert(!memcmp(out,in+100,sizeof(in)-100) && !memcmp(out+sizeof(in)-100,in,100));
    assert(q.used==0);
    puts("PCM queue tests passed");return 0;
}
