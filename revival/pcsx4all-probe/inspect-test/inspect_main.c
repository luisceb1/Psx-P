/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Independent PSP media inspector. No emulator dependencies. */
#include <pspkernel.h>
#include <pspdebug.h>
#include <pspctrl.h>
#include <stdio.h>
#include "../../storage.h"
#include "../disc_inspect.h"
PSP_MODULE_INFO("PSX-P Disc Inspector",0,1,0);
PSP_MAIN_THREAD_ATTR(PSP_THREAD_ATTR_USER);
PSP_HEAP_SIZE_KB(1024);
static int exit_cb(int a,int b,void *p){
    (void)a;(void)b;(void)p;sceKernelExitGame();return 0;
}
static int callback_thread(SceSize size,void *arg){
    (void)size;(void)arg;
    int cb=sceKernelCreateCallback("inspector_exit",exit_cb,NULL);
    if(cb>=0&&sceKernelRegisterExitCallback(cb)>=0)
        for(;;)sceKernelSleepThreadCB();
    return 0;
}
int main(int argc,char **argv){
    char app[384],dir[512],report[512];
    pspDebugScreenInit();
    int th=sceKernelCreateThread("inspector_callbacks",callback_thread,0x11,0x1000,PSP_THREAD_ATTR_USER,NULL);
    if(th>=0)sceKernelStartThread(th,0,NULL);
    pspDebugScreenPrintf("PSX-P Revival Disc Inspector\nRead-only; no guest execution.\n\n");
    if(argc<1||revival_app_dir(argv[0],app,sizeof(app))||
        !rv_inspect_path(dir,sizeof(dir),app,"DISC")||
        !rv_inspect_path(report,sizeof(report),app,"PSXPRV_DISC_INSPECT.TXT")){
        pspDebugScreenPrintf("App path unavailable; not opening any file.\n");
    }else{
        pspDebugScreenPrintf("Inspecting DISC/GAME.CUE...\n");
        rv_inspect_result r=rv_inspect_dir(dir);
        pspDebugScreenPrintf("Phase: %s\nStatus: %s\n",rv_inspect_phase_name(r.phase),rv_inspect_status_name(r.status));
        if(rv_inspect_ok(&r)){
            pspDebugScreenPrintf("Boot: %s\nSectors: %lu\nText bytes: %lu\nPC: %08lx\n",
                r.meta.boot_name,(unsigned long)r.meta.sectors,
                (unsigned long)r.meta.text_bytes,(unsigned long)r.meta.pc);
            pspDebugScreenPrintf("Profile match only; cannot run yet.\n");
        }else pspDebugScreenPrintf("Not admitted by current narrow profile.\n");
        FILE *f=fopen(report,"w");
        if(f){rv_inspect_print(f,&r);int failed=ferror(f);if(fclose(f))failed=1;
            pspDebugScreenPrintf("Report: %s\n",failed?"FAILED":"PSXPRV_DISC_INSPECT.TXT");
        }else pspDebugScreenPrintf("Report write unavailable.\n");
    }
    pspDebugScreenPrintf("\nSquare or HOME Exit to quit.\n");
    for(unsigned i=0;i<1200;i++){
        SceCtrlData pad={0};
        if(sceCtrlPeekBufferPositive(&pad,1)>0&&(pad.Buttons&PSP_CTRL_SQUARE))break;
        sceKernelDelayThread(50000);
    }
    sceKernelExitGame();return 0;
}
