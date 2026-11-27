/* SPDX-License-Identifier: GPL-2.0-or-later */
#include <pspkernel.h>
#include <pspdebug.h>
#include "r3000a.h"
#include "psxevents.h"
#include <string.h>
PSP_MODULE_INFO("PCSX4ALL guest probe",0,1,0);
PSP_MAIN_THREAD_ATTR(PSP_THREAD_ATTR_USER);
extern "C" int revival_pcsx4all_probe_one(void);
extern void execI(void);
static const u32 start=0x80010000u, finish=0x80010020u;
/* Only original synthetic code, no BIOS or disc. */
static const u32 alu[]={
    0x24010007, /* addiu r1,zero,7 */
    0x2422fffd, /* addiu r2,r1,-3 */
    0x00221821, /* addu r3,r1,r2 */
    0x08004008, /* j 0x80010020 */
    0x24640001  /* delay: addiu r4,r3,1 */
};
static const u32 ram[]={
    0x3c018002, /* lui r1,0x8002 */
    0x2402002a, /* addiu r2,zero,42 */
    0xac220000, /* sw r2,0(r1) */
    0x8c230000, /* lw r3,0(r1) */
    0x00000000, /* nop: load delay */
    0x08004008, /* j finish */
    0x24640001  /* delay: r4=r3+1 */
};
struct result { u32 pc,cycle,gpr[32],ram_word; };
static void setup(R3000Acpu *cpu,const u32 *program,unsigned bytes)
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
    memcpy(psxM+0x10000,program,bytes);
}
static void snapshot(struct result *r)
{
    r->pc=psxRegs.pc;r->cycle=psxRegs.cycle;
    memcpy(r->gpr,psxRegs.GPR.r,sizeof(r->gpr));
    memcpy(&r->ram_word,psxM+0x20000,4);
}
static bool expected(const struct result *r,bool memory)
{
    if(memory) return r->pc==finish && r->gpr[0]==0 && r->gpr[1]==0x80020000u && r->gpr[2]==42 && r->gpr[3]==42 && r->gpr[4]==43 && r->ram_word==42;
    return r->pc==finish && r->gpr[0]==0 && r->gpr[1]==7 &&
        r->gpr[2]==4 && r->gpr[3]==11 && r->gpr[4]==12;
}
static void test(const char *name,const u32 *program,unsigned bytes,bool memory)
{
    struct result a={},b={};
    setup(&psxInt,program,bytes);
    for(unsigned i=0;i<16 && psxRegs.pc!=finish;i++) execI();
    snapshot(&a);
    setup(&psxRec,program,bytes);
    int dispatched=revival_pcsx4all_probe_one();
    snapshot(&b);
    pspDebugScreenPrintf("%s\nInterpreter: %s\nDynarec: %s\nGPR/PC: %s\nCycles: %lu / %lu\n",
        name,expected(&a,memory)?"PASS":"FAIL",dispatched&&expected(&b,memory)?"PASS":"FAIL",
        dispatched&&a.pc==b.pc&&a.ram_word==b.ram_word&&!memcmp(a.gpr,b.gpr,sizeof(a.gpr))?"PASS":"FAIL",
        (unsigned long)a.cycle,(unsigned long)b.cycle);
}
int main()
{
    pspDebugScreenInit();
    memset(&Config,0,sizeof(Config));Config.Cpu=CPU_DYNAREC;
    if(psxInit()<0) { pspDebugScreenPrintf("Init FAIL\n");sceKernelDelayThread(10000000);sceKernelExitGame();return 1; }
    test("ALU + branch",alu,sizeof(alu),false);
    test("RAM store/load",ram,sizeof(ram),true);
    sceKernelDelayThread(10000000);
    sceKernelExitGame();
    return 0;
}
