/* SPDX-License-Identifier: GPL-2.0-or-later */
#include <pspkernel.h>
#include <pspdebug.h>
#include <stdio.h>
#include <string.h>
#include "r3000a.h"
#include "psxevents.h"
#include "psxdma.h"
#ifdef REVIVAL_CD_INTEGRATION
#include "cdrom.h"
#endif
#include "plugins.h"
#include "gpu/gpulib/gpu.h"
PSP_MODULE_INFO("PCSX4ALL integrated",0,1,0);
PSP_MAIN_THREAD_ATTR(PSP_THREAD_ATTR_USER);
extern "C" int revival_pcsx4all_probe_one(void);
extern void execI(void);
extern unsigned probe_presentations;
static const u32 start=0x80010000u;
/* Guest instructions fill entire 256x240 red.
 * All GP0 writes originate from guest SW, not host calls. */
static const u32 program[]={
    0x3c011f80, /* lui r1,1f80 */
    0x3c020200, /* lui r2,0200 */
    0x344200ff, /* ori r2,00ff */
    0xac221810, /* sw r2,1810(r1) */
    0xac201810, /* sw zero,1810(r1): x=y=0 */
    0x3c0300f0, /* lui r3,00f0 */
    0x34630100, /* ori r3,0100 */
    0xac231810, /* sw r3,1810(r1): size */
    0x0800400c, /* jump to end */
    0x00000000
};
/* End outside code, start+0x30. */
static const u32 end_pc=0x80010030u;
static FILE *report;
static void mark(const char *name,bool ok){
    if(report){fprintf(report,"%s,%s\n",name,ok?"PASS":"FAIL");fflush(report);}
}
static void setup(R3000Acpu *cpu){
    psxCpu=cpu;cpu->Reset();
    memset(&psxRegs,0,sizeof(psxRegs));
    memset(psxM,0,0x200000);memset(psxH,0,0x10000);
    memset(psxR,0,0x80000);memset(psxP,0,0x10000);
    psxRegs.psxM=psxM;psxRegs.psxH=psxH;psxRegs.psxP=psxP;psxRegs.psxR=psxR;
    psxRegs.writeok=1;psxRegs.pc=start;
    psxRegs.intCycle[PSXINT_NEXT_EVENT].cycle=0x100000;
    psxRegs.io_cycle_counter=0x100000;
    memcpy(psxM+0x10000,program,sizeof(program));
    GPU_writeStatus(0x00000000);
    memset(gpu.vram,0,1024*512*2);
}
static bool pixels(){
    return gpu.vram[0]==0x001f && gpu.vram[239*1024+255]==0x001f &&
        gpu.vram[256]==0 && gpu.vram[240*1024]==0;
}
static void dma_test(){
    // Known, bounded RAM packet through original MMIO register decoder.
    setup(&psxRec);
    psxEvqueueInit();
    const u32 addr=0x20000;
    const u32 packet[]={0x0200ff00,0x00000000,0x00020100,0x00000000};
    memcpy(psxM+addr,packet,sizeof(packet));
    psxMemWrite32(0x1f8010f0,1u<<11); // DPCR enable DMA2
    psxMemWrite32(0x1f8010f4,(1u<<18)|(1u<<23));
    psxMemWrite32(0x1f8010a0,addr);
    psxMemWrite32(0x1f8010a4,0x00010004);
    psxMemWrite32(0x1f8010a8,0x01000201);
    GPU_updateLace();
    mark("DMA2 RAM to VRAM",gpu.vram[0]==0x03e0 && gpu.vram[1024+255]==0x03e0 && gpu.vram[2*1024]==0 && HW_DMA2_MADR==addr+16);
    bool queued=(psxRegs.interrupt & (1u<<PSXINT_GPUDMA))!=0;
    mark("DMA2 event queued",queued);
    if(queued){
        psxRegs.cycle=psxRegs.intCycle[PSXINT_GPUDMA].sCycle+psxRegs.intCycle[PSXINT_GPUDMA].cycle;
        psxBranchTest();
        mark("DMA2 completion IRQ",!(HW_DMA2_CHCR&0x01000000u) && !(psxRegs.interrupt&(1u<<PSXINT_GPUDMA)) && (psxHu32(0x1070)&8u));
    }
}
#ifdef REVIVAL_CD_INTEGRATION
static void cd_test(){
    setup(&psxRec);
    psxEvqueueInit();
    // Reset controller without opening a disc; issue Getstat through real MMIO.
    cdrReset();
    psxMemWrite8(0x1f801800,0);
    psxMemWrite8(0x1f801801,1);
    bool queued=(psxRegs.interrupt&(1u<<PSXINT_CDR))!=0;
    mark("CD Getstat event queued",queued);
    if(queued){
        psxRegs.cycle=psxRegs.intCycle[PSXINT_CDR].sCycle+psxRegs.intCycle[PSXINT_CDR].cycle;
        psxBranchTest();
        bool ready=(psxMemRead8(0x1f801800)&0x20)!=0;
        unsigned char response=psxMemRead8(0x1f801801);
        mark("CD Getstat response",ready && !(psxRegs.interrupt&(1u<<PSXINT_CDR)));
        if(report){fprintf(report,"CD status,%02x\n",response);fflush(report);}
    }
}
#endif
int main(){
    pspDebugScreenInit();
    report=fopen("PCSX4ALL_INTEGRATION.TXT","w");
    memset(&Config,0,sizeof(Config));Config.Cpu=CPU_DYNAREC;
    if(psxInit()<0){mark("CPU init",false);sceKernelExitGame();return 1;}
    mark("CPU init",true);
    if(GPU_init()!=0 || !gpu.vram){mark("GPU init",false);sceKernelExitGame();return 1;}
    mark("GPU init",true);
    setup(&psxInt);
    for(unsigned i=0;i<32 && psxRegs.pc!=end_pc;i++)execI();
    GPU_updateLace();
    bool a=psxRegs.pc==end_pc&&pixels();
    u32 cycles=psxRegs.cycle, regs[32];memcpy(regs,psxRegs.GPR.r,sizeof(regs));
    mark("Interpreter GP0",a);
    setup(&psxRec);
    bool dispatched=revival_pcsx4all_probe_one();
    GPU_updateLace();
    bool b=dispatched&&psxRegs.pc==end_pc&&pixels();
    mark("Dynarec GP0",b);
    mark("GPR and cycles",a&&b&&cycles==psxRegs.cycle&&!memcmp(regs,psxRegs.GPR.r,sizeof(regs)));
    dma_test();
#ifdef REVIVAL_CD_INTEGRATION
    cd_test();
#endif
    GPU_writeStatus(0x03000000);GPU_requestScreenRedraw();GPU_updateLace();
    mark("GU submission",probe_presentations>0);
    if(report)fclose(report);
    sceKernelDelayThread(10000000);GPU_shutdown();sceKernelExitGame();return 0;
}
