/* SPDX-License-Identifier: GPL-2.0-or-later */
#include <pspkernel.h>
#include <pspdebug.h>
#include "r3000a.h"
#include "psxevents.h"
#include <string.h>
#include <stdio.h>
#include <psppower.h>
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
static const unsigned loop_count=20000;
static const u32 loop[]={
    0x24014e20, /* r1=20000 */
    0x24420001, /* r2++ */
    0x2421ffff, /* r1-- */
    0x1420fffd, /* bne r1,zero,start+4 */
    0x24630001, /* delay: r3++ */
    0x08004008, /* j finish */
    0
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
static FILE *report;
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
    if(report){fprintf(report,"%s,%s,%s,%s,%lu,%lu\n",name,expected(&a,memory)?"PASS":"FAIL",dispatched&&expected(&b,memory)?"PASS":"FAIL",dispatched&&a.pc==b.pc&&a.ram_word==b.ram_word&&!memcmp(a.gpr,b.gpr,sizeof(a.gpr))?"PASS":"FAIL",(unsigned long)a.cycle,(unsigned long)b.cycle);fflush(report);}
}
static bool loop_ok(const result &r)
{
    return r.pc==finish && r.gpr[0]==0 && r.gpr[1]==0 &&
        r.gpr[2]==loop_count && r.gpr[3]==loop_count;
}
static void benchmark()
{
    result a={},b={};
    setup(&psxInt,loop,sizeof(loop));
    u64 t=sceKernelGetSystemTimeWide();
    for(unsigned i=0;i<100000 && psxRegs.pc!=finish;i++) execI();
    u64 ia=sceKernelGetSystemTimeWide()-t;
    snapshot(&a);
    setup(&psxRec,loop,sizeof(loop));
    t=sceKernelGetSystemTimeWide();
    unsigned blocks=0;
    for(;blocks<loop_count+4 && psxRegs.pc!=finish;blocks++)
        if(!revival_pcsx4all_probe_one()) break;
    u64 da=sceKernelGetSystemTimeWide()-t;
    snapshot(&b);
    bool pass=loop_ok(a)&&loop_ok(b)&&!memcmp(a.gpr,b.gpr,sizeof(a.gpr));
    pspDebugScreenPrintf("Loop: %s (%u blocks)\nInt/JIT us: %llu/%llu\n",pass?"PASS":"FAIL",blocks,(unsigned long long)ia,(unsigned long long)da);
    if(report) {fprintf(report,"loop,%s,%llu,%llu,%lu,%lu,%u\n",pass?"PASS":"FAIL",(unsigned long long)ia,(unsigned long long)da,(unsigned long)a.cycle,(unsigned long)b.cycle,blocks);fflush(report);}
}
static void invalidate()
{
    setup(&psxRec,alu,sizeof(alu));
    bool first=revival_pcsx4all_probe_one()&&psxRegs.pc==finish&&psxRegs.GPR.r[1]==7;
    // Explicit invalidation of the block entry through the real core API.
    // Not a test of arbitrary guest self-modifying writes.
    psxMemWrite32(start,0x24010009u);
    psxRegs.pc=start;
    memset(psxRegs.GPR.r,0,32*sizeof(u32));
    bool second=revival_pcsx4all_probe_one()&&psxRegs.pc==finish&&psxRegs.GPR.r[1]==9&&psxRegs.GPR.r[2]==6&&psxRegs.GPR.r[3]==15&&psxRegs.GPR.r[4]==16;
    pspDebugScreenPrintf("Code rewrite: %s\n",first&&second?"PASS":"FAIL");
    if(report){fprintf(report,"rewrite,%s\n",first&&second?"PASS":"FAIL");fflush(report);}
}
int main()
{
    pspDebugScreenInit();
    memset(&Config,0,sizeof(Config));Config.Cpu=CPU_DYNAREC;
    if(psxInit()<0) { pspDebugScreenPrintf("Init FAIL\n");sceKernelDelayThread(10000000);sceKernelExitGame();return 1; }
    report=fopen("PCSX4ALL_PROBE.TXT","w");
    scePowerSetClockFrequency(333,333,166);
    if(report){fprintf(report,"PCSX4ALL guest probe v2 cpu=%d bus=%d\n",scePowerGetCpuClockFrequencyInt(),scePowerGetBusClockFrequencyInt());fflush(report);}
    test("ALU + branch",alu,sizeof(alu),false);
    test("RAM store/load",ram,sizeof(ram),true);
    benchmark();
    invalidate();
    if(report) fclose(report);
    sceKernelDelayThread(10000000);
    sceKernelExitGame();
    return 0;
}
