/* SPDX-License-Identifier: GPL-2.0-or-later */
#include <pspkernel.h>
#include <pspdebug.h>
#include <stdint.h>
#include "jit_cache.h"
#include <string.h>
PSP_MODULE_INFO("Revival JIT probe",0,1,0);
PSP_MAIN_THREAD_ATTR(PSP_THREAD_ATTR_USER);
static struct revival_jit_cache cache;
static uint32_t gpr[32];
static int run_block(const struct revival_jit_slot *slot,int fresh,uint32_t expected) {
    void (*run)(uint32_t *);
    if(!slot)return 0;
    memset(gpr,0,sizeof(gpr));gpr[31]=0x12345678;
    if(fresh) {
        sceKernelDcacheWritebackInvalidateRange((void *)slot->code,slot->code_count*4u);
        sceKernelIcacheInvalidateRange((void *)slot->code,slot->code_count*4u);
    }
    run=(void (*)(uint32_t *))(uintptr_t)slot->code;
    run(gpr);
    return gpr[0]==0 && gpr[1]==expected && gpr[2]==expected-3 &&
           gpr[3]==expected*2-3 && gpr[4]==(expected*2-3)*4 &&
           gpr[5]==(((expected*2-3)*4)|1u) && gpr[31]==0x12345678;
}
int main(void) {
    uint32_t guest[]={0x24010007,0x2422fffd,0x00221821,0x00032080,0x34850001};
    const struct revival_jit_slot *slot;
    int fresh, first, hit, rewritten, rejected;
    pspDebugScreenInit();
    revival_jit_cache_reset(&cache);
    slot=revival_jit_lookup(&cache,0x80010000,guest,5,&fresh);
    first=fresh && run_block(slot,fresh,7);
    slot=revival_jit_lookup(&cache,0x80010000,guest,5,&fresh);
    hit=!fresh && run_block(slot,fresh,7);
    guest[0]=0x24010009;
    slot=revival_jit_lookup(&cache,0x80010000,guest,5,&fresh);
    rewritten=fresh && run_block(slot,fresh,9);
    guest[0]=0x08000000;
    rejected=!revival_jit_lookup(&cache,0x80010000,guest,5,&fresh);
    pspDebugScreenPrintf("Allegrex synthetic probe\nFirst execution: %s\nCache hit: %s\nRewritten code: %s\nUnsupported opcode: %s\n",
       first?"PASS":"FAIL",hit?"PASS":"FAIL",rewritten?"PASS":"FAIL",rejected?"PASS":"FAIL");
    sceKernelDelayThread(10000000);
    sceKernelExitGame();
    return 0;
}
