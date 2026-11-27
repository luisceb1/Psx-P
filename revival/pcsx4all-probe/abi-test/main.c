/* SPDX-License-Identifier: GPL-2.0-or-later */
#include <pspkernel.h>
#include <pspdebug.h>
#include <stdint.h>
#include <string.h>
PSP_MODULE_INFO("PCSX4ALL ABI probe",0,1,0);
PSP_MAIN_THREAD_ATTR(PSP_THREAD_ATTR_USER);
extern void revival_pcsx4all_call(void *fn, void *regs, uint32_t pc, uint32_t result[2]);
static uint32_t code[16] __attribute__((aligned(64)));
static uint32_t regs[32] __attribute__((aligned(64)));
static int run(unsigned delta,int stacked)
{
    uint32_t result[2]={0,0};
    unsigned n=0;
    /* Synthetic host block, not PS1 guest code. */
    code[n++]=0x24420010; /* addiu v0,v0,16 */
    code[n++]=0x24030000|delta; /* addiu v1,zero,delta */
    code[n++]=0x24101234; /* addiu s0,zero,0x1234 */
    code[n++]=0xafd00004; /* sw s0,4(fp) */
    if(stacked) code[n++]=0x8fbf0010; /* lw ra,16(sp) */
    code[n++]=0x03e00008; /* jr ra */
    code[n++]=0x00000000;
    sceKernelDcacheWritebackInvalidateRange(code,n*sizeof(code[0]));
    sceKernelIcacheInvalidateRange(code,n*sizeof(code[0]));
    revival_pcsx4all_call(code,regs,0x80010000u,result);
    return result[0]==0x80010010u && result[1]==delta &&
        regs[1]==0x1234u && regs[0]==0xfeedfaceu && regs[2]==0xcafebabeu;
}
int main(void)
{
    int direct,stacked,rewrite;
    pspDebugScreenInit();
    memset(regs,0,sizeof(regs));
    regs[0]=0xfeedfaceu;regs[2]=0xcafebabeu;
    direct=run(7,0);
    stacked=run(9,1);
    rewrite=run(13,1);
    pspDebugScreenPrintf("PCSX4ALL bridge (synthetic only)\nRA return: %s\nStack return: %s\nCode rewrite: %s\n",
        direct?"PASS":"FAIL",stacked?"PASS":"FAIL",rewrite?"PASS":"FAIL");
    sceKernelDelayThread(10000000);
    sceKernelExitGame();
    return 0;
}
