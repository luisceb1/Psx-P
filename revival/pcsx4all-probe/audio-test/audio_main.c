/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Separate native PCM and real SPU mixing diagnostic, without guest CPU. */
#include <pspkernel.h>
#include <pspdebug.h>
#include <pspctrl.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "../audio_queue.h"
#include "audio_psp.h"
#include "out.h"
#include "spu_config.h"
PSP_MODULE_INFO("PSX-P Audio Probe",0,1,0);
PSP_MAIN_THREAD_ATTR(PSP_THREAD_ATTR_USER);
long SPUinit(void);long SPUopen(void);long SPUclose(void);long SPUshutdown(void);
void SPUwriteRegister(unsigned long,unsigned short,unsigned int);
void SPUasync(unsigned int,unsigned int);
static FILE *report;
static void mark(const char *s,int pass){
    pspDebugScreenPrintf("%s: %s\n",s,pass?"PASS":"FAIL");
    if(report){fprintf(report,"%s,%s\n",s,pass?"PASS":"FAIL");fflush(report);}
}
static void stats(const char *tag){
    rv_psp_audio_metrics m=rv_psp_audio_get_metrics();
    if(report){fprintf(report,"%s,accepted=%lu,nonzero=%lu,dropped=%lu,blocks=%lu,errors=%lu,busy=%lu,invalid=%lu,tail=%lu,pending=%u,channel=%d,release=%d\n",
        tag,(unsigned long)m.accepted,(unsigned long)m.nonzero,(unsigned long)m.dropped,
        (unsigned long)m.submitted,(unsigned long)m.errors,(unsigned long)m.busy,
        (unsigned long)m.invalid,(unsigned long)m.tail,m.pending,m.channel,m.release_rc);fflush(report);}
}
static int room(unsigned frames,uint64_t began){
    while(rv_psp_audio_room()<frames){
        rv_psp_audio_pump();
        if(sceKernelGetSystemTimeWide()-began>3000000ULL)return 0;
        sceKernelDelayThread(1000);
    }
    return 1;
}
static int drain(uint64_t began){
    while(rv_psp_audio_active()){
        rv_psp_audio_pump();
        if(sceKernelGetSystemTimeWide()-began>3000000ULL)return 0;
        sceKernelDelayThread(1000);
    }
    return 1;
}
static void pcm_test(void){
    int rc=rv_psp_audio_init();mark("PCM channel reserve",rc==0);
    if(rc)return;
    int16_t pcm[RV_AUDIO_BLOCK*2];
    unsigned phase=0;
    uint64_t began=sceKernelGetSystemTimeWide();
    for(unsigned block=0;block<64;block++){
        if(!room(RV_AUDIO_BLOCK,began))break;
        for(unsigned i=0;i<RV_AUDIO_BLOCK;i++){
            // Quiet triangle tone ~441 Hz at 44.1kHz; left then right.
            int v=phase<50?(int)phase*96-2400:7200-(int)phase*96;
            phase=(phase+1)%100;
            pcm[2*i]=block<32?(int16_t)v:0;
            pcm[2*i+1]=block>=32?(int16_t)v:0;
        }
        rv_psp_audio_feed(pcm,sizeof(pcm));
    }
    int drained=drain(began);
    rv_psp_audio_metrics m=rv_psp_audio_get_metrics();
    mark("PCM complete stereo blocks",drained&&m.submitted==64&&m.nonzero>0&&!m.dropped&&!m.errors&&!m.invalid);
    rv_psp_audio_finish();stats("PCM metrics");m=rv_psp_audio_get_metrics();
    mark("PCM release",m.channel<0&&!m.release_rc);
}
static void spu_test(void){
    memset(&spu_config,0,sizeof(spu_config));
    spu_config.iHaveConfiguration=1;spu_config.iDisabled=1;
    spu_config.iVolume=768;spu_config.iUseInterpolation=2;
    int init=SPUinit()==0,opened=init&&SPUopen()==0;
    mark("SPU null baseline open",opened);
    if(!opened){if(init)SPUshutdown();return;}
    static struct out_driver native;
    out_register_psp(&native);
    int selected=native.init()==0;
    mark("SPU native backend open",selected);
    if(!selected){SPUclose();SPUshutdown();return;}
    // Existing null driver has no resources; replace only in this probe.
    out_current->finish();out_current=&native;
    // Original short looped ADPCM waveform in SPU RAM. No game audio asset.
    SPUwriteRegister(0x1f801da6,0x2000,0);
    SPUwriteRegister(0x1f801da8,0x0704,0); // filter 0, shift 4, loop start/end/repeat
    for(unsigned i=0;i<7;i++)SPUwriteRegister(0x1f801da8,i<3?0x3333:0xdddd,0);
    SPUwriteRegister(0x1f801c00,0x2000,0);
    SPUwriteRegister(0x1f801c02,0x2000,0);
    SPUwriteRegister(0x1f801c04,0x0400,0);
    SPUwriteRegister(0x1f801c06,0x2000,0);
    SPUwriteRegister(0x1f801c08,0x000f,0);
    SPUwriteRegister(0x1f801c0a,0x0000,0);
    SPUwriteRegister(0x1f801c0e,0x2000,0);
    SPUwriteRegister(0x1f801daa,0xc000,0);
    SPUwriteRegister(0x1f801d88,1,0);
    uint64_t began=sceKernelGetSystemTimeWide();
    unsigned i;
    for(i=1;i<=128;i++){
        if(!room(256,began))break;
        // Isolated synthetic mixer clock; no guest CPU timing is changed.
        SPUasync(i*256u*768u,1);
        if(sceKernelGetSystemTimeWide()-began>3000000ULL)break;
    }
    int drained=drain(began);
    rv_psp_audio_metrics m=rv_psp_audio_get_metrics();
    mark("SPU generated nonzero PCM",m.nonzero>0);
    mark("SPU PCM submitted",i==129&&drained&&m.submitted>0&&!m.dropped&&!m.errors&&!m.invalid);
    SPUclose();SPUshutdown();stats("SPU metrics");m=rv_psp_audio_get_metrics();
    mark("SPU channel release",m.channel<0&&!m.release_rc);
}
int main(void){
    pspDebugScreenInit();
    report=fopen("PSXPRV_AUDIO.TXT","w");
    pspDebugScreenPrintf("Quiet PCM left/right, then SPU tone.\n");
    pcm_test();
    // Separate both audible phases.
    sceKernelDelayThread(500000);
    spu_test();
    if(report)fclose(report);
    pspDebugScreenPrintf("Square to exit; hardware listening required.\n");
    for(unsigned i=0;i<400;i++){
        SceCtrlData pad={0};if(sceCtrlPeekBufferPositive(&pad,1)>0&&(pad.Buttons&PSP_CTRL_SQUARE))break;
        sceKernelDelayThread(50000);
    }
    sceKernelExitGame();return 0;
}
