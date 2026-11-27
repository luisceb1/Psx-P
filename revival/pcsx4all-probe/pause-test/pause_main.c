/* SPDX-License-Identifier: GPL-2.0-or-later */
/* PSP Go firmware pause/suspend baseline. No emulator dependencies or state files. */
#include <pspkernel.h>
#include <pspdebug.h>
#include <pspctrl.h>
#include <psppower.h>
#include <stdio.h>
#include <stdint.h>
PSP_MODULE_INFO("PSX-P Pause Probe",0,1,0);
PSP_MAIN_THREAD_ATTR(PSP_THREAD_ATTR_USER);
PSP_HEAP_SIZE_KB(1024);

static volatile int ready,exit_requested,exit_rc=-1,power_slot=-1;
static volatile unsigned power_events,power_flags,suspend_events,resume_events;
static uint32_t marker,guard[64],ticks;
static uint64_t session;

/* Callbacks only record events. No filesystem, GPU or plugin work here. */
static int on_exit(int a,int b,void *arg){
    (void)a;(void)b;(void)arg;
    exit_requested=1;
    return 0;
}
static int on_power(int a,int flags,void *arg){
    (void)a;(void)arg;
    power_flags=(unsigned)flags;
    power_events++;
    if(flags&PSP_POWER_CB_SUSPENDING)suspend_events++;
    if(flags&(PSP_POWER_CB_RESUMING|PSP_POWER_CB_RESUME_COMPLETE))resume_events++;
    return 0;
}
static int callback_thread(SceSize args,void *arg){
    (void)args;(void)arg;
    int cb=sceKernelCreateCallback("revival_exit",on_exit,NULL);
    if(cb>=0)exit_rc=sceKernelRegisterExitCallback(cb);
    else exit_rc=cb;
    cb=sceKernelCreateCallback("revival_power",on_power,NULL);
    if(cb>=0)power_slot=scePowerRegisterCallback(-1,cb);
    else power_slot=cb;
    ready=1;
    for(;;)sceKernelSleepThreadCB();
    return 0;
}
static uint32_t pattern(unsigned i){return marker^(0x57a15a7du+i*2654435761u);}
static int intact(void){
    if(!marker)return 0;
    for(unsigned i=0;i<64;i++)if(guard[i]!=pattern(i))return 0;
    return 1;
}
/* Open only for explicit observations, and close before the user pauses. */
static int record(const char *event){
    FILE *f=fopen("PSXPRV_PAUSE.TXT","a");
    if(!f)return 0;
    int ok=fprintf(f,"%s,session=%llu,ticks=%lu,marker=%08lx,intact=%d,exit_rc=%d,power_slot=%d,power_events=%u,last_flags=%08x,suspend=%u,resume=%u\n",
        event,(unsigned long long)session,(unsigned long)ticks,(unsigned long)marker,
        intact(),exit_rc,power_slot,power_events,power_flags,suspend_events,resume_events)>=0;
    if(fclose(f))ok=0;
    return ok;
}
int main(void){
    pspDebugScreenInit();
    session=sceKernelGetSystemTimeWide();
    int th=sceKernelCreateThread("revival_callbacks",callback_thread,0x11,0x1000,PSP_THREAD_ATTR_USER,NULL);
    if(th>=0)th=sceKernelStartThread(th,0,NULL);
    for(unsigned i=0;th>=0&&!ready&&i<100;i++)sceKernelDelayThread(10000);
    pspDebugScreenPrintf("PSX-P independent pause probe\n\nX: arm memory marker\nHOME: Pause Game, then Resume Game\nO: verify after resume\nSquare or HOME Exit: quit\n\nNo BIOS, game, GU, audio or SDL.\nNo valuable saves are used.\n\n");
    pspDebugScreenPrintf("Callback registration: exit=%d power=%d\n",exit_rc,power_slot);
    pspDebugScreenPrintf("Launch log: %s\n",record("launch")?"OK":"FAILED");
    unsigned last=0;
    for(ticks=0;ticks<4800&&!exit_requested;ticks++){
        SceCtrlData pad={0};
        unsigned now=sceCtrlPeekBufferPositive(&pad,1)>0?pad.Buttons:0;
        unsigned pressed=now&~last;
        last=now;
        if(pressed&PSP_CTRL_SQUARE){record("square_exit");break;}
        if(pressed&PSP_CTRL_CROSS){
            marker=0x50000000u|(ticks+1);
            for(unsigned i=0;i<64;i++)guard[i]=pattern(i);
            pspDebugScreenPrintf("Armed %08lx; log %s\n",(unsigned long)marker,record("armed")?"OK":"FAILED");
        }
        if(pressed&PSP_CTRL_CIRCLE){
            int ok=intact();
            pspDebugScreenPrintf("Marker %08lx: %s; notifications %u\n",(unsigned long)marker,ok?"PASS":"FAIL",power_events);
            pspDebugScreenPrintf("Resume log %s\n",record(ok?"resume_marker_PASS":"resume_marker_FAIL")?"OK":"FAILED");
        }
        sceKernelDelayThread(50000);
    }
    if(exit_requested)record("home_exit_callback");
    if(ticks>=4800)record("idle_limit");
    sceKernelExitGame();
    return 0;
}
