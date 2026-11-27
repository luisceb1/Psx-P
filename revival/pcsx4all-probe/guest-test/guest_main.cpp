/* SPDX-License-Identifier: GPL-2.0-or-later */
#include <pspkernel.h>
#include <pspdebug.h>
#include "r3000a.h"
#include <string.h>
PSP_MODULE_INFO("PCSX4ALL guest probe",0,1,0);
PSP_MAIN_THREAD_ATTR(PSP_THREAD_ATTR_USER);
extern "C" int revival_pcsx4all_probe_one(void);
extern void execI(void);
static const u32 start=0x80010000u, finish=0x80010020u;
/* Only original synthetic code, no BIOS or disc. */
static const u32 program[]={
    0x24010007, /* addiu r1,zero,7 */
    0x2422fffd, /* addiu r2,r1,-3 */
    0x00221821, /* addu r3,r1,r2 */
    0x08004008, /* j 0x80010020 */
    0x24640001  /* delay: addiu r4,r3,1 */
};
struct result { u32 pc,cycle,gpr[32]; };
static void setup(R3000Acpu *cpu)
{
    psxCpu=cpu;
    cpu->Reset();
    memset(&psxRegs,0,sizeof(psxRegs));
    memset(psxM,0,0x200000);
    memset(psxH,0,0x10000);
    memset(psxR,0,0x80000);
    memset(psxP,0,0x10000);
    psxRegs.psxM=psxM;psxRegs.psxH=psxH;
    psxRegs.psxP=psxP;psxRegs.psxR=psxR;
    psxRegs.writeok=1;psxRegs.pc=start;
    /* No peripherals/events during this short, trusted program.
     * This does not test the actual event queue. */
    psxRegs.intCycle[PSXINT_NEXT_EVENT].cycle=0x100000;
    psxRegs.io_cycle_counter=0x100000;
    memcpy(psxM+0x10000,program,sizeof(program));
}
static void snapshot(struct result *r)
{
    r->pc=psxRegs.pc;r->cycle=psxRegs.cycle;
    memcpy(r->gpr,psxRegs.GPR.r,sizeof(r->gpr));
}
static bool expected(const struct result *r)
{
    return r->pc==finish && r->gpr[0]==0 && r->gpr[1]==7 &&
        r->gpr[2]==4 && r->gpr[3]==11 && r->gpr[4]==12;
}
int main()
{
    struct result a={},b={};
    pspDebugScreenInit();
    memset(&Config,0,sizeof(Config));Config.Cpu=CPU_DYNAREC;
    if(psxInit()<0) { pspDebugScreenPrintf("Init FAIL\n");sceKernelDelayThread(10000000);sceKernelExitGame();return 1; }
    setup(&psxInt);
    for(unsigned i=0;i<16 && psxRegs.pc!=finish;i++) execI();
    snapshot(&a);
    setup(&psxRec);
    int dispatched=revival_pcsx4all_probe_one();
    snapshot(&b);
    pspDebugScreenPrintf("PCSX4ALL synthetic guest\nInterpreter: %s\nDynarec: %s\nGPR/PC: %s\nCycles: %lu / %lu\n",
        expected(&a)?"PASS":"FAIL",dispatched&&expected(&b)?"PASS":"FAIL",
        dispatched&&a.pc==b.pc&&!memcmp(a.gpr,b.gpr,sizeof(a.gpr))?"PASS":"FAIL",
        (unsigned long)a.cycle,(unsigned long)b.cycle);
    sceKernelDelayThread(10000000);
    sceKernelExitGame();
    return 0;
}
