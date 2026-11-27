/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Single emulation-thread PCM queue; no timing adjustment or blocking output. */
#include <pspaudio.h>
#include <pspkernel.h>
#include "../audio_queue.h"
#include "audio_psp.h"
#include "out.h"
static rv_audio_queue q;
static int16_t playback[RV_AUDIO_BLOCK*2] __attribute__((aligned(64)));
static int channel=-1;
static uint32_t submitted,errors,busy,invalid,tail;
static int release_rc;
int rv_psp_audio_init(void){
    if(channel>=0)return -1;
    rv_audio_reset(&q);submitted=errors=busy=invalid=tail=0;release_rc=0;
    channel=sceAudioChReserve(PSP_AUDIO_NEXT_CHANNEL,RV_AUDIO_BLOCK,PSP_AUDIO_FORMAT_STEREO);
    return channel<0?channel:0;
}
void rv_psp_audio_pump(void){
    if(channel<0||q.count<RV_AUDIO_BLOCK)return;
    int rest=sceAudioGetChannelRestLen(channel);
    if(rest<0){errors++;return;}
    if(rest>0)return; // Never overwrite the still-owned hardware buffer.
    if(!rv_audio_peek(&q,playback))return;
    sceKernelDcacheWritebackRange(playback,sizeof(playback));
    int rc=sceAudioOutputPanned(channel,0x1800,0x1800,playback);
    if(rc<0){
        if((unsigned)rc==SCE_AUDIO_ERROR_OUTPUT_BUSY)busy++;
        else errors++;
        return;
    }
    rv_audio_consume(&q);submitted++;
}
void rv_psp_audio_feed(void *data,int bytes){
    if(channel<0)return;
    if(bytes==0){rv_psp_audio_pump();return;}
    if(!data||bytes<0||bytes>32768||(bytes&3)){invalid++;return;}
    rv_psp_audio_pump();
    rv_audio_push(&q,data,(unsigned)bytes/4u);
    rv_psp_audio_pump();
}
unsigned rv_psp_audio_room(void){return RV_AUDIO_CAPACITY-q.count;}
int rv_psp_audio_active(void){
    if(channel<0)return 0;
    if(q.count>=RV_AUDIO_BLOCK)return 1;
    int rest=sceAudioGetChannelRestLen(channel);
    if(rest<0){errors++;return 0;}
    return rest>0;
}
static int psp_busy(void){
    rv_psp_audio_pump();
    return channel<0||q.count>=2*RV_AUDIO_BLOCK;
}
void rv_psp_audio_finish(void){
    if(channel<0)return;
    // Explicitly discard incomplete/pending frames, never block guest timing.
    tail+=q.count;q.count=0;
    for(unsigned i=0;i<100;i++){
        int rest=sceAudioGetChannelRestLen(channel);
        if(rest<=0){if(rest<0)errors++;break;}
        sceKernelDelayThread(1000);
    }
    release_rc=sceAudioChRelease(channel);
    if(release_rc>=0)channel=-1;
    else errors++;
}
rv_psp_audio_metrics rv_psp_audio_get_metrics(void){
    rv_psp_audio_metrics m={q.accepted,q.dropped,q.nonzero,submitted,errors,busy,invalid,tail,
        q.count,channel,release_rc};
    return m;
}
void out_register_psp(struct out_driver *drv){
    drv->name="psp-pcm";drv->init=rv_psp_audio_init;drv->finish=rv_psp_audio_finish;
    drv->busy=psp_busy;drv->feed=rv_psp_audio_feed;
}
