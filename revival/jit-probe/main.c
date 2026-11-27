/* SPDX-License-Identifier: GPL-2.0-or-later */
#include <pspkernel.h>
#include <pspdebug.h>
#include <stdint.h>
#include "jit_emit.h"
PSP_MODULE_INFO("Revival JIT probe",0,1,0);
PSP_MAIN_THREAD_ATTR(PSP_THREAD_ATTR_USER);
static uint32_t code[REVIVAL_JIT_MAX_WORDS] __attribute__((aligned(64)));
static uint32_t gpr[32];
int main(void) {
    const uint32_t guest[]={0x24010007,0x2422fffd,0x00221821,0x00032080,0x34850001};
    size_t n;
    int ok=0;
    pspDebugScreenInit();
    n=revival_jit_emit(guest,sizeof(guest)/sizeof(guest[0]),code,REVIVAL_JIT_MAX_WORDS);
    if(n) {
        void (*run)(uint32_t *)=(void (*)(uint32_t *))(uintptr_t)code;
        sceKernelDcacheWritebackInvalidateRange(code,n*sizeof(code[0]));
        sceKernelIcacheInvalidateRange(code,n*sizeof(code[0]));
        run(gpr);
        ok=(gpr[0]==0 && gpr[1]==7 && gpr[2]==4 && gpr[3]==11 && gpr[4]==44 && gpr[5]==45);
    }
    pspDebugScreenPrintf("Allegrex generated code: %s (%u words)\n",ok?"PASS":"FAIL",(unsigned)n);
    sceKernelDelayThread(5000000);
    sceKernelExitGame();
    return 0;
}
