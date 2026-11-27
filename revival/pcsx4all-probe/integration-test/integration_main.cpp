/* SPDX-License-Identifier: GPL-2.0-or-later */
#include <pspkernel.h>
#include <pspdebug.h>
#include <pspctrl.h>
#include <stdio.h>
#include <string.h>
#include "r3000a.h"
#include "psxevents.h"
#include "psxcounters.h"
#include "psxdma.h"
#ifdef REVIVAL_SPU_INTEGRATION
extern "C" {
#include "spu_config.h"
}
#include "plugin_lib.h"
static void CALLBACK probe_spu_irq(){psxHu32ref(0x1070)|=SWAPu32(0x200);ResetIoCycle();}
static void CALLBACK probe_spu_schedule(unsigned int cycles){
    if(Config.SpuUpdateFreq>0 && Config.SpuUpdateFreq<=SPU_UPDATE_FREQ_MAX)cycles>>=Config.SpuUpdateFreq;
    if(pl_frameskip_advice())cycles>>=1;
    psxEvqueueAdd(PSXINT_SPUIRQ,cycles);
}
#endif
#ifdef REVIVAL_CD_INTEGRATION
#include "cdrom.h"
#ifdef REVIVAL_EXE_INTEGRATION
#include "misc.h"
#endif
#endif
#include "plugins.h"
#include "gpu/gpulib/gpu.h"
#include "../disc_preflight.h"
#include "../disc_load.h"
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
#ifdef REVIVAL_EXE_INTEGRATION
static void exe_test(){
    static unsigned char header[2048],payload[2048];
    const char *path="REVIVAL_SYNTHETIC.EXE";
    EXE_HEADER h={};
    memcpy(h.id,"PS-X EXE",8);
    h.pc0=start;h.t_addr=start;h.t_size=sizeof(payload);
    h.gp0=0x80018000u;h.s_addr=0x801fff00u;
    memcpy(header,&h,sizeof(h));
    memcpy(payload,program,sizeof(program));
    FILE *file=fopen(path,"wb");
    bool made=false;
    if(file){
        made=fwrite(header,1,sizeof(header),file)==sizeof(header)
            && fwrite(payload,1,sizeof(payload),file)==sizeof(payload);
        if(fclose(file)!=0)made=false;
    }
    mark("PS-X EXE creation",made);
    if(!made)return;
    setup(&psxRec);
    memset(psxM+0x10000,0,sizeof(payload));
    bool loaded=Load(path)==0;
    bool correct=loaded&&psxRegs.pc==start&&psxRegs.GPR.n.gp==h.gp0
        &&psxRegs.GPR.n.sp==h.s_addr&&!memcmp(psxM+0x10000,payload,sizeof(payload));
    mark("PS-X EXE load",correct);
    bool ran=correct&&revival_pcsx4all_probe_one();
    GPU_updateLace();
    mark("PS-X EXE execution",ran&&psxRegs.pc==end_pc&&pixels());
}
#endif
#ifdef REVIVAL_HLE_INTEGRATION
static void hle_test(){
    Config.HLE=TRUE;
    Config.Cpu=CPU_DYNAREC;
    // Real reset path; no BIOS file, no disc bootstrap execution yet.
    psxCpu=&psxRec;
    psxReset();
    mark("HLE reset",psxRegs.pc==0xbfc00000u && psxRegs.CP0.r[12]==0x10900000u && psxRegs.writeok);
    mark("HLE vectors",psxRu32(0)==0xec000004u && psxMu32(0xa0)==0xec000001u && psxMu32(0xb0)==0xec000002u && psxMu32(0xc0)==0xec000003u);
    // A0 strlen through actual HLE opcode and interpreter, not a direct C call.
    memcpy(psxM+0x20000,"REVIVAL",8);
    psxRegs.pc=0x800000a0u;
    psxRegs.GPR.n.a0=0x80020000u;
    psxRegs.GPR.n.t1=0x1b;
    psxRegs.GPR.n.ra=0x80010030u;
    execI();
    mark("HLE A0 strlen",psxRegs.GPR.n.v0==7 && psxRegs.pc==0x80010030u);
    Config.HLE=FALSE;
}
#endif

#ifdef REVIVAL_SPU_INTEGRATION
#include "../disc_session.h"
static rv_disc_session disc_session={RV_DISC_IDLE};
static const rv_disc_ops disc_ops={CDR_init,CDR_open,CDR_shutdown};
static bool begin_fixture_disc(){
    if(disc_session.state!=RV_DISC_IDLE)return false;
    SetIsoFile("REVIVAL_BOOT.bin");
    return rv_disc_begin(&disc_session,&disc_ops)==0;
}
static bool end_fixture_disc(){
    if(rv_disc_end(&disc_session,&disc_ops)!=0||disc_session.state!=RV_DISC_IDLE)return false;
    unsigned char msf[]={0,2,0x16};
    // The backend must refuse further reads once the handle is closed.
    return CDR_readTrack(msf)!=0;
}
// Minimal original ISO9660 fixture, Mode 2 Form 1 raw sectors.
// No commercial content. All extents and addresses are fixed and bounded.
static unsigned char disc[32][2352];
static void both16(unsigned char *p,unsigned v){
    p[0]=v;p[1]=v>>8;p[2]=v>>8;p[3]=v;
}
static void both32(unsigned char *p,unsigned v){
    for(unsigned i=0;i<4;i++){p[i]=v>>(8*i);p[4+i]=v>>(8*(3-i));}
}
static unsigned char *data(unsigned n){return disc[n]+24;}
static unsigned dirent(unsigned char *p,unsigned lba,unsigned size,unsigned flags,const char *name,unsigned len){
    unsigned n=33+len+((len&1)?0:1);
    memset(p,0,n);p[0]=n;both32(p+2,lba);both32(p+10,size);
    p[18]=126;p[19]=11;p[20]=27;p[25]=flags;both16(p+28,1);
    p[32]=len;memcpy(p+33,name,len);return n;
}
static bool make_disc(){
    memset(disc,0,sizeof(disc));
    for(unsigned n=0;n<32;n++){
        memset(disc[n]+1,0xff,10);disc[n][13]=2;
        disc[n][14]=(n/10)*16+n%10;disc[n][15]=2;
        disc[n][18]=disc[n][22]=8;
    }
    unsigned char *p=data(16);
    p[0]=1;memcpy(p+1,"CD001",5);p[6]=1;
    memset(p+8,' ',64);memcpy(p+40,"REVIVAL_TEST",12);
    both32(p+80,32);both16(p+120,1);both16(p+124,1);both16(p+128,2048);
    both32(p+132,10);p[140]=18;p[151]=19;
    dirent(p+156,20,4096,2,"\0",1);
    p[881]=1;
    p=data(17);p[0]=255;memcpy(p+1,"CD001",5);p[6]=1;
    p=data(18);p[0]=1;p[2]=20;p[6]=1;
    p=data(19);p[0]=1;p[5]=20;p[7]=1;
    p=data(20);unsigned off=0;
    off+=dirent(p+off,20,4096,2,"\0",1);
    off+=dirent(p+off,20,4096,2,"\1",1);
    off+=dirent(p+off,22,32,0,"SYSTEM.CNF;1",12);
    dirent(p+off,23,4096,0,"PSX.EXE;1",9);
    const char cnf[]="BOOT = cdrom:\\PSX.EXE;1\r\n";
    memcpy(data(22),cnf,sizeof(cnf));
    EXE_HEADER h={};memcpy(h.id,"PS-X EXE",8);
    h.pc0=start;h.t_addr=start;h.t_size=2048;h.s_addr=0x801fff00u;
    memcpy(data(23),&h,sizeof(h));
    // 20,000 bounded iterations, then the existing red GPU fill program.
    const u32 loop[]={0x24014e20,0x24420001,0x2421ffff,0x1420fffd,0x24630001,
        0x08004010,0};
    memcpy(data(24),loop,sizeof(loop));memcpy(data(24)+64,program,sizeof(program));
    // Relocate the final jump to end at start+0x70.
    const u32 jump=0x0800401c;memcpy(data(24)+64+8*4,&jump,4);
    FILE *f=fopen("REVIVAL_BOOT.bin","wb");if(!f)return false;
    bool ok=fwrite(disc,1,sizeof(disc),f)==sizeof(disc);if(fclose(f))ok=false;
    f=fopen("REVIVAL_BOOT.cue","w");if(!f)return false;
    if(fprintf(f,"FILE \"REVIVAL_BOOT.bin\" BINARY\n  TRACK 01 MODE2/2352\n    INDEX 01 00:00:00\n")<0)ok=false;
    if(fclose(f))ok=false;return ok;
}
// Advance to an already scheduled event; never move guest time backwards.
static bool dispatch_event(psxEventNum event){
    if(!(psxRegs.interrupt&(1u<<event)))return false;
    u32 deadline=psxRegs.intCycle[event].sCycle+psxRegs.intCycle[event].cycle;
    if(psxRegs.cycle<deadline)psxRegs.cycle=deadline;
    psxBranchTest();
    return !(psxRegs.interrupt&(1u<<event));
}
static void dma_irq_setup(unsigned channel){
    psxMemWrite32(0x1f801074,0); // no guest exception during host diagnostic
    psxMemWrite32(0x1f801070,0);
    psxMemWrite32(0x1f8010f4,0x7f000000u|(1u<<23)|(1u<<(16+channel)));
    psxMemWrite32(0x1f8010f0,8u<<(4*channel));
}
static bool dma_irq_done(unsigned channel,psxEventNum event,u32 chcr){
    return dispatch_event(event) && !(psxMemRead32(chcr)&0x01000000u)
        && (psxHu32(0x1070)&8u) && (psxHu32(0x10f4)&(1u<<(24+channel)));
}
static void dma4_test(){
    const u32 src=0x30000,dst=0x30100;
    const u16 a[]={0x1234,0xabcd,0x55aa,0x8001,0x0000,0xffff,0x2468,0x1357};
    const u16 b[]={0xfedc,0x0123,0x4321,0x9876,0xaaaa,0x5555,0x0f0f,0xf0f0};
    // DMA write verified by independent SPU data-register readback.
    memcpy(psxM+src,a,sizeof(a));
    dma_irq_setup(4);
    psxMemWrite16(0x1f801da6,0x2000);
    psxMemWrite32(0x1f8010c0,src);
    psxMemWrite32(0x1f8010c4,0x00010004);
    psxMemWrite32(0x1f8010c8,0x01000201);
    bool queued=(psxRegs.interrupt&(1u<<PSXINT_SPUDMA))!=0;
    mark("DMA4 write queued",queued);
    mark("DMA4 write IRQ",dma_irq_done(4,PSXINT_SPUDMA,0x1f8010c8));
    bool write_ok=HW_DMA4_MADR==src+sizeof(a);
    psxMemWrite16(0x1f801da6,0x2000);
    for(unsigned i=0;i<8;i++){
        u16 value=psxMemRead16(0x1f801da8);
        if(value!=a[i])write_ok=false;
    }
    mark("DMA4 RAM to SPU",write_ok);
    // Independently seed SPU via MMIO; DMA read must not alter RAM guards.
    psxMemWrite16(0x1f801da6,0x2000);
    for(unsigned i=0;i<8;i++)psxMemWrite16(0x1f801da8,b[i]);
    psxMemWrite16(0x1f801da6,0x2000);
    memset(psxM+dst-4,0x5a,sizeof(b)+8);
    dma_irq_setup(4);
    psxMemWrite32(0x1f8010c0,dst);
    psxMemWrite32(0x1f8010c4,0x00010004);
    psxMemWrite32(0x1f8010c8,0x01000200);
    mark("DMA4 read queued",(psxRegs.interrupt&(1u<<PSXINT_SPUDMA))!=0);
    mark("DMA4 read IRQ",dma_irq_done(4,PSXINT_SPUDMA,0x1f8010c8));
    mark("DMA4 SPU to RAM",HW_DMA4_MADR==dst+sizeof(b)
        && !memcmp(psxM+dst,b,sizeof(b)) && psxMu32(dst-4)==0x5a5a5a5au
        && psxMu32(dst+sizeof(b))==0x5a5a5a5au);
}
// Bounded cooperative runner: host-defined, trusted fixture only.
// A return to the frontend is guaranteed between dispatches, not if a
// generated block or an upstream event handler itself fails to return.
extern "C" int revival_pcsx4all_probe_region_one(u32 begin,u32 end);
enum RunStop { RUN_TARGET, RUN_YIELD, RUN_REJECT, RUN_TIMEOUT, RUN_CANCEL, RUN_LIMIT };
static const char *run_name(RunStop s){
    switch(s){
        case RUN_TARGET:return "target";
        case RUN_YIELD:return "yield";
        case RUN_REJECT:return "rejected";
        case RUN_TIMEOUT:return "timeout";
        case RUN_CANCEL:return "cancel";
        case RUN_LIMIT:return "limit";
    }
    return "unknown";
}
static bool cancel_pressed(){
    SceCtrlData pad={};
    return sceCtrlPeekBufferPositive(&pad,1)>0 &&
        (pad.Buttons&(PSP_CTRL_START|PSP_CTRL_SELECT))==(PSP_CTRL_START|PSP_CTRL_SELECT);
}
struct RunResult { RunStop stop; bool yielded; u32 pc,cycle,gpr[32],frames,dispatches,slices; u64 us; };
static RunStop run_slice(bool native,u32 begin,u32 end,unsigned budget,u32 &count){
    for(unsigned i=0;i<budget;i++){
        if(psxRegs.pc==end)return RUN_TARGET;
        if(psxRegs.cycle>=psxRegs.io_cycle_counter)psxBranchTest();
        if(psxRegs.pc==end)return RUN_TARGET;
        if((psxRegs.pc&3u)||psxRegs.pc<begin||psxRegs.pc>=end)return RUN_REJECT;
        if(native){
            if(!revival_pcsx4all_probe_region_one(begin,end))return RUN_REJECT;
        }else execI();
        count++;
    }
    return psxRegs.pc==end?RUN_TARGET:RUN_YIELD;
}
static const u32 run_base=0x80024000u,run_end=0x80024020u;
static const unsigned run_iterations=500000;
static const u32 run_program[]={
    0x3c010007,0x3421a120, // r1 = 500000
    0x24420001,           // r2++
    0x2421ffff,           // r1--
    0x1420fffd,           // bne r1,zero,run_base+8
    0x24630001,           // r3++ in delay slot
    0x08009008,0          // jump to run_end
};
static void run_setup(bool native){
    Config.HLE=TRUE;Config.Cpu=native?CPU_DYNAREC:CPU_INTERPRETER;
    psxCpu=native?&psxRec:&psxInt;
    psxReset();
    // Allow event delivery but prevent the synthetic program from taking
    // guest hardware exceptions; guest code has no IRQ handler.
    psxMemWrite32(0x1f801074,0);
    psxMemWrite32(0x1f801070,0);
    // Known padding for upstream compiler lookahead.
    memset(psxM+0x24000,0,4096);
    memcpy(psxM+0x24000,run_program,sizeof(run_program));
    psxRegs.pc=run_base;
}
static RunResult run_fixture(bool native){
    run_setup(native);
    RunResult r={};
    const u32 initial_frames=frame_counter;
    const u64 initial=sceKernelGetSystemTimeWide();
    r.stop=RUN_LIMIT;
    const unsigned slice_budget=1024;
    const unsigned dispatch_limit=1600008;
    while(r.dispatches<dispatch_limit){
        if(cancel_pressed()){r.stop=RUN_CANCEL;break;}
        if(sceKernelGetSystemTimeWide()-initial>=15000000ULL){r.stop=RUN_TIMEOUT;break;}
        unsigned budget=dispatch_limit-r.dispatches;
        if(budget>slice_budget)budget=slice_budget;
        r.stop=run_slice(native,run_base,run_end,budget,r.dispatches);
        r.slices++;
        if(r.stop!=RUN_YIELD)break;
        r.yielded=true;
        // Yield the PSP thread between bounded slices, not guest timing.
        sceKernelDelayThread(1);
    }
    if(r.stop==RUN_YIELD)r.stop=RUN_LIMIT;
    r.us=sceKernelGetSystemTimeWide()-initial;
    r.pc=psxRegs.pc;r.cycle=psxRegs.cycle;
    r.frames=frame_counter-initial_frames;
    memcpy(r.gpr,psxRegs.GPR.r,sizeof(r.gpr));
    return r;
}
static bool run_expected(const RunResult &r){
    return r.stop==RUN_TARGET&&r.pc==run_end&&r.gpr[0]==0&&r.gpr[1]==0
        &&r.gpr[2]==run_iterations&&r.gpr[3]==run_iterations;
}
static void log_run(const char *engine,const RunResult &r){
    if(report){
        fprintf(report,"Runner %s,%s,pc=%08lx,cycles=%lu,frames=%lu,dispatches=%lu,slices=%lu,us=%llu\n",
            engine,run_name(r.stop),(unsigned long)r.pc,(unsigned long)r.cycle,
            (unsigned long)r.frames,(unsigned long)r.dispatches,(unsigned long)r.slices,
            (unsigned long long)r.us);
        fflush(report);
    }
}
static void runner_test(){
    // Verify rejection without executing or modifying the guest registers.
    run_setup(true);
    u32 old_pc=psxRegs.pc,old_cycle=psxRegs.cycle;
    bool range_ok=!revival_pcsx4all_probe_region_one(run_base+4,run_end)
        &&!revival_pcsx4all_probe_region_one(run_base+1,run_end)
        &&!revival_pcsx4all_probe_region_one(run_end,run_base)
        &&psxRegs.pc==old_pc&&psxRegs.cycle==old_cycle;
    mark("Runner range rejection",range_ok);
    RunResult a=run_fixture(false);log_run("interpreter",a);
    mark("Runner interpreter completion",run_expected(a));
    mark("Runner interpreter slice resume",a.yielded&&a.slices>1&&run_expected(a));
    RunResult b=run_fixture(true);log_run("dynarec",b);
    mark("Runner relocated dynarec completion",run_expected(b));
    mark("Runner dynarec slice resume",b.yielded&&b.slices>1&&run_expected(b));
    mark("Runner register cycle parity",run_expected(a)&&run_expected(b)&&a.cycle==b.cycle
        &&!memcmp(a.gpr,b.gpr,sizeof(a.gpr)));
    mark("Runner scheduled frames",run_expected(a)&&run_expected(b)&&a.frames>0&&a.frames==b.frames);
    Config.HLE=FALSE;
}
// Deterministic digital-pad checks through the actual SIO MMIO path.
extern unsigned short revival_pad_map(unsigned int);
extern void revival_pad_override(bool,unsigned int);
static bool pad_exchange(u16 ctrl,u16 buttons){
    const u8 command[]={1,0x42,0,0,0};
    const u8 expected[]={0xff,0x41,0x5a,(u8)buttons,(u8)(buttons>>8)};
    bool ok=true;
    psxMemWrite16(0x1f80104a,0);
    psxMemWrite16(0x1f80104a,ctrl);
    for(unsigned i=0;i<sizeof(command);i++){
        psxMemWrite8(0x1f801040,command[i]);
        u8 response=psxMemRead8(0x1f801040);
        if(report)fprintf(report,"PAD byte,%04x,%u,%02x,%02x\n",ctrl,i,response,expected[i]);
        if(response!=expected[i])ok=false;
    }
    if(report)fflush(report);
    return ok;
}
static void pad_test(){
    const unsigned int sample=PSP_CTRL_CROSS|PSP_CTRL_LEFT|PSP_CTRL_LTRIGGER;
    const unsigned int all=PSP_CTRL_SELECT|PSP_CTRL_START|PSP_CTRL_UP|PSP_CTRL_RIGHT
        |PSP_CTRL_DOWN|PSP_CTRL_LEFT|PSP_CTRL_LTRIGGER|PSP_CTRL_RTRIGGER
        |PSP_CTRL_TRIANGLE|PSP_CTRL_CIRCLE|PSP_CTRL_CROSS|PSP_CTRL_SQUARE;
    mark("PAD neutral mapping",revival_pad_map(0)==0xffff);
    mark("PAD combined mapping",revival_pad_map(sample)==0xbb7f);
    mark("PAD all digital mapping",revival_pad_map(all)==0x0306);
    revival_pad_override(true,sample);
    psxMemWrite32(0x1f801074,0);
    psxMemWrite32(0x1f801070,0);
    mark("PAD1 SIO response",pad_exchange(0x0002,0xbb7f));
    bool queued=(psxRegs.interrupt&(1u<<PSXINT_SIO))!=0;
    mark("PAD SIO IRQ",queued&&dispatch_event(PSXINT_SIO)&&(psxHu32(0x1070)&0x80u));
    mark("PAD2 neutral response",pad_exchange(0x2002,0xffff));
    psxMemWrite16(0x1f80104a,0);
    revival_pad_override(false,0);
}
extern unsigned probe_stage_w,probe_stage_h;
extern uint16_t probe_stage_pixel(unsigned,unsigned);
static void video_mode(unsigned mode){
    GPU_writeStatus(0x08000000u|mode);
    GPU_writeStatus(0x05000000);
    GPU_writeStatus(0x06c60260);
    GPU_writeStatus(0x07042018);
    GPU_writeStatus(0x03000000);
}
static bool video_submit(unsigned w,unsigned h,uint16_t top,uint16_t bottom){
    unsigned before=probe_presentations;
    GPU_requestScreenRedraw();GPU_updateLace();
    if(report){
        fprintf(report,"GU extended mode,%08lx,source=%dx%d,stage=%ux%u\n",
            (unsigned long)gpu.status.reg,(int)gpu.screen.w,(int)gpu.screen.h,
            probe_stage_w,probe_stage_h);fflush(report);
    }
    return probe_presentations>before&&gpu.screen.w==(int)w&&gpu.screen.h==(int)h
        &&probe_stage_w>0&&probe_stage_h>0
        &&probe_stage_pixel(0,0)==top
        &&probe_stage_pixel(probe_stage_w-1,probe_stage_h-1)==bottom;
}
static void video_test(){
    // Diagnostic VRAM fills only, after all earlier integration checks.
    // The extended presenter is not a field-timed renderer.
    for(unsigned y=0;y<480;y++)for(unsigned x=0;x<640;x++)
        gpu.vram[y*1024+x]=y<120?0x001f:0x7c00;
    video_mode(3);
    mark("GU 640 progressive",video_submit(640,240,0x801f,0xfc00));
    video_mode(0x27);
    mark("GU 640 interlace snapshot",video_submit(640,480,0x801f,0xfc00));
    // Packed two RGB888 pixels per three VRAM words. Red top, cyan bottom.
    for(unsigned y=0;y<240;y++){
        uint16_t *row=gpu.vram+y*1024;
        for(unsigned x=0;x<128;x++){
            row[3*x]=y<120?0x00ff:0xff00;
            row[3*x+1]=0;
            row[3*x+2]=y<120?0x0000:0xffff;
            // Two identical pixels; for cyan, bytes are 00 ff ff 00 ff ff.
            if(y>=120)row[3*x+1]=0x00ff;
            else row[3*x+1]=0xff00;
        }
    }
    video_mode(0x10);
    mark("GU RGB24 staged",video_submit(256,240,0x801f,0xffe0));
}
static bool native_disc_valid=false;
static u32 native_disc_pc=0,native_disc_regs[32],native_disc_cycles=0;
static bool fresh_disc=false;
static bool cd_ack(unsigned expected){
    psxMemWrite8(0x1f801800,1);
    unsigned cause=psxMemRead8(0x1f801803)&7;
    unsigned status=psxMemRead8(0x1f801801);
    psxMemWrite8(0x1f801803,0x1f);
    psxMemWrite8(0x1f801800,0);
    psxMemWrite32(0x1f801070,0);
    return cause==expected && !(status&1);
}
static void dma3_test(){
    // Reopen ONLY the freshly generated disc, after V2 display diagnostics.
    if(!fresh_disc){mark("DMA3 fixture reopen",false);return;}
    bool opened=begin_fixture_disc();
    mark("DMA3 fixture reopen",opened);
    if(!opened)return;
    cdrReset();
    psxMemWrite32(0x1f801074,0);
    psxMemWrite32(0x1f801070,0);
    // Setloc to PVD sector 00:02:16 in BCD, then ReadN via normal CD MMIO.
    psxMemWrite8(0x1f801800,0);
    psxMemWrite8(0x1f801802,0);
    psxMemWrite8(0x1f801802,2);
    psxMemWrite8(0x1f801802,0x16);
    psxMemWrite8(0x1f801801,2);
    bool loc=dispatch_event(PSXINT_CDR)&&cd_ack(3);
    mark("DMA3 Setloc ACK",loc);
    if(loc){
        psxMemWrite8(0x1f801801,6);
        bool read=dispatch_event(PSXINT_CDR)&&cd_ack(3);
        mark("DMA3 ReadN ACK",read);
        bool ready=read&&dispatch_event(PSXINT_CDREAD)&&cd_ack(1);
        mark("DMA3 sector ready",ready);
        if(ready){
            const u32 dst=0x31000;
            memset(psxM+dst-4,0x5a,2048+8);
            dma_irq_setup(3);
            psxMemWrite8(0x1f801803,0x80);
            psxMemWrite32(0x1f8010b0,dst);
            psxMemWrite32(0x1f8010b4,512);
            psxMemWrite32(0x1f8010b8,0x11000000);
            mark("DMA3 queued",(psxRegs.interrupt&(1u<<PSXINT_CDRDMA))!=0);
            mark("DMA3 CD to RAM",!memcmp(psxM+dst,data(16),2048)
                &&psxMu32(dst-4)==0x5a5a5a5au&&psxMu32(dst+2048)==0x5a5a5a5au);
            mark("DMA3 completion IRQ",dma_irq_done(3,PSXINT_CDRDMA,0x1f8010b8));
        }
    }
    if(psxRegs.interrupt&(1u<<PSXINT_CDREAD))psxEvqueueRemove(PSXINT_CDREAD);
    if(psxRegs.interrupt&(1u<<PSXINT_CDR))psxEvqueueRemove(PSXINT_CDR);
    mark("DMA3 disc session closed",end_fixture_disc());
}
extern unsigned probe_presentations;
static void disc_boot_test(){
    fresh_disc=make_disc();
    mark("Disc fixture creation",fresh_disc);
    if(!fresh_disc)return;
    // Validate the exact generated files from disk. External media still
    // cannot be selected or executed by this diagnostic.
    char name[128]={};
    FILE *cue=fopen("REVIVAL_BOOT.cue","rb");
    int cue_rc=cue?rv_pf_cue(cue,name):RV_PF_IO;
    if(cue&&fclose(cue))cue_rc=RV_PF_IO;
    bool cue_ok=cue_rc==RV_PF_OK&&!strcmp(name,"REVIVAL_BOOT.bin");
    mark("Disc preflight CUE",cue_ok);
    rv_pf_result meta={};
    FILE *bin=cue_ok?fopen("REVIVAL_BOOT.bin","rb"):NULL;
    int image_rc=bin?rv_pf_bin(bin,&meta):RV_PF_IO;
    if(bin&&fclose(bin))image_rc=RV_PF_IO;
    bool image_ok=image_rc==RV_PF_OK&&meta.sectors==32&&meta.exe_lba==23
        &&meta.pc==start&&meta.text_addr==start&&meta.text_bytes==2048
        &&meta.sp==0x801fff00u&&!strcmp(meta.boot_name,"PSX.EXE;1");
    mark("Disc preflight ISO and EXE",image_ok);
    if(report){
        fprintf(report,"Preflight status,%d,%d,sectors=%lu,boot=%s\n",cue_rc,image_rc,
            (unsigned long)meta.sectors,meta.boot_name);fflush(report);
    }
    fresh_disc=cue_ok&&image_ok;
    if(!fresh_disc)return;
    bool opened=begin_fixture_disc();
    mark("Disc BIN/CUE open",opened);if(!opened)return;
    unsigned char msf[]={0,2,0x16};
    bool sector=CDR_readTrack(msf)==0;
    unsigned char *b=sector?CDR_getBuffer():NULL;
    mark("Disc ISO9660 sector",b&&!memcmp(b+13,"CD001",5));
    Config.HLE=TRUE;Config.Cpu=CPU_DYNAREC;psxCpu=&psxRec;
    psxReset();
    mark("Disc identification",CheckCdrom()==0);
    // Execute actual HLE bootstrap opcode using the interpreter.
    execI();
    bool loaded=psxRegs.pc==start&&psxRegs.GPR.n.sp==0x801fff00u
        &&psxMu32(start)==0x24014e20u;
    mark("Disc HLE bootstrap",loaded);
    if(loaded){
        // Same private synthetic fixture, now through the new independent
        // reader. Compare before replacing the original HLE payload.
        rv_load_staged staged={};
        FILE *f=fopen("REVIVAL_BOOT.bin","rb");
        int rc=f?rv_load_stage(f,&staged):RV_PF_IO;
        if(f&&fclose(f))rc=RV_PF_IO;
        bool staged_ok=rc==RV_PF_OK&&staged.payload_bytes==2048
            &&staged.meta.text_addr==start&&staged.meta.pc==start
            &&!memcmp(staged.payload,data(24),2048);
        mark("Disc transactional stage",staged_ok);
        bool parity=staged_ok&&staged.meta.pc==psxRegs.pc
            &&staged.meta.gp==psxRegs.GPR.n.gp&&staged.meta.sp==psxRegs.GPR.n.sp
            &&!memcmp(staged.payload,psxM+0x10000,2048);
        mark("Disc HLE loader parity",parity);
        rv_pf_result committed={};
        bool committed_ok=false;
        if(parity){
            // Preserve adjacent RAM words; host tests independently verify
            // write into sentinel-filled RAM and failure without mutation.
            const u32 left=psxMu32(0xfffc),right=psxMu32(0x10800);
            rc=rv_load_commit(&staged,(unsigned char *)psxM,0x200000,&committed);
            if(rc==RV_PF_OK){
                psxCpu->Clear(committed.text_addr,committed.text_bytes/4);
                psxRegs.pc=committed.pc;
                psxRegs.GPR.n.gp=committed.gp;
                psxRegs.GPR.n.sp=committed.sp;
                committed_ok=!memcmp(psxM+0x10000,data(24),2048)
                    &&psxMu32(0xfffc)==left&&psxMu32(0x10800)==right;
            }
        }
        mark("Disc transactional RAM commit",committed_ok);
        rv_load_discard(&staged);
        // Keep baseline synthetic HLE execution available on admission
        // failure; no external guest code is ever selected.

        const u32 native_cycle_start=psxRegs.cycle;
        unsigned blocks=0;bool loop_ok=false;
        while(psxRegs.pc!=0x80010070u && blocks<20010){
            if(psxRegs.pc==0x80010040u)loop_ok=psxRegs.GPR.r[1]==0&&psxRegs.GPR.r[2]==20000&&psxRegs.GPR.r[3]==20000;
            psxBranchTest();
            if(!revival_pcsx4all_probe_one())break;
            blocks++;
        }
        mark("Disc loop registers",loop_ok);
        mark("Disc bounded native execution",loop_ok&&psxRegs.pc==0x80010070u&&psxRegs.GPR.r[0]==0);
        // Loop values are overwritten by the drawing routine; verify loop separately via dispatch count.
        if(report){fprintf(report,"Disc native blocks,%u\n",blocks);fflush(report);}
        GPU_updateLace();
        mark("Disc guest GPU output",gpu.vram[0]==0x001f);
        native_disc_valid=loop_ok&&psxRegs.pc==0x80010070u&&gpu.vram[0]==0x001f
            &&gpu.vram[239*1024+255]==0x001f;
        native_disc_pc=psxRegs.pc;
        memcpy(native_disc_regs,psxRegs.GPR.r,sizeof(native_disc_regs));
        native_disc_cycles=psxRegs.cycle-native_cycle_start;
    }
    mark("Disc session closed",end_fixture_disc());Config.HLE=FALSE;
}
// Direct synthetic boot, independent of the upstream HLE loader opcode.
// A successful result is not evidence that an arbitrary game can execute.
static RunStop run_disc_phase(u32 begin,u32 end,u32 limit,u32 &dispatches,u64 began){
    const unsigned slice_budget=256;
    dispatches=0;
    while(dispatches<limit){
        if(cancel_pressed())return RUN_CANCEL;
        if(sceKernelGetSystemTimeWide()-began>=8000000ULL)return RUN_TIMEOUT;
        unsigned budget=limit-dispatches;
        if(budget>slice_budget)budget=slice_budget;
        RunStop stop=run_slice(false,begin,end,budget,dispatches);
        if(stop!=RUN_YIELD)return stop;
        sceKernelDelayThread(1);
    }
    return RUN_LIMIT;
}
static void direct_disc_test(){
    if(!fresh_disc){mark("Direct disc stage",false);return;}
    rv_load_staged staged={};
    FILE *f=fopen("REVIVAL_BOOT.bin","rb");
    int rc=f?rv_load_stage(f,&staged):RV_PF_IO;
    if(f&&fclose(f))rc=RV_PF_IO;
    bool staged_ok=rc==RV_PF_OK&&staged.meta.sectors==32&&staged.meta.exe_lba==23
        &&staged.meta.pc==start&&staged.meta.text_addr==start
        &&staged.meta.sp==0x801fff00u&&staged.payload_bytes==2048
        &&!memcmp(staged.payload,data(24),2048);
    mark("Direct disc stage",staged_ok);
    if(!staged_ok){rv_load_discard(&staged);return;}
    // Real reset initializes HLE vectors but does not execute the bootstrap.
    // GPU display policy is set explicitly rather than via fake_bios_gpu_setup.
    Config.HLE=TRUE;Config.Cpu=CPU_INTERPRETER;psxCpu=&psxInt;
    psxReset();
    psxMemWrite32(0x1f801074,0);
    psxMemWrite32(0x1f801070,0);
    GPU_writeStatus(0x00000000);
    memset(gpu.vram,0,1024*512*2);
    video_mode(0);
    // A sentinel demonstrates that the direct reader, not the old HLE
    // loader, initializes text. Do not run if the new commit fails.
    memset(psxM+0x10000,0xa5,2048);
    const u32 left=psxMu32(0xfffc),right=psxMu32(0x10800);
    rv_pf_result committed={};
    rc=rv_load_commit(&staged,(unsigned char *)psxM,0x200000,&committed);
    bool copied=rc==RV_PF_OK&&!memcmp(psxM+0x10000,data(24),2048)
        &&psxMu32(0xfffc)==left&&psxMu32(0x10800)==right;
    mark("Direct disc RAM load",copied);
    rv_load_discard(&staged);
    if(!copied){Config.HLE=FALSE;return;}
    psxCpu->Clear(committed.text_addr,committed.text_bytes/4);
    psxRegs.pc=committed.pc;
    psxRegs.GPR.n.gp=committed.gp;
    psxRegs.GPR.n.sp=committed.sp;
    bool regs=psxRegs.pc==start&&psxRegs.GPR.n.gp==0&&psxRegs.GPR.n.sp==0x801fff00u;
    mark("Direct disc entry registers",regs);
    if(!regs){Config.HLE=FALSE;return;}
    const u64 began=sceKernelGetSystemTimeWide();
    const u32 cycle_start=psxRegs.cycle;
    u32 loop_dispatches=0,draw_dispatches=0;
    RunStop loop=run_disc_phase(start,0x80010040u,100000,loop_dispatches,began);
    bool loop_ok=loop==RUN_TARGET&&psxRegs.GPR.r[1]==0&&psxRegs.GPR.r[2]==20000
        &&psxRegs.GPR.r[3]==20000&&psxRegs.GPR.r[0]==0;
    mark("Direct disc interpreter loop",loop_ok);
    RunStop draw=loop_ok?run_disc_phase(0x80010040u,0x80010070u,64,draw_dispatches,began):RUN_REJECT;
    bool stopped=draw==RUN_TARGET&&psxRegs.pc==0x80010070u;
    mark("Direct disc interpreter completion",stopped);
    unsigned before=probe_presentations;
    GPU_requestScreenRedraw();GPU_updateLace();
    mark("Direct disc VRAM output",stopped&&pixels());
    mark("Direct disc GU presentation",stopped&&probe_presentations>before);
    bool parity=stopped&&native_disc_valid&&native_disc_pc==psxRegs.pc
        &&native_disc_regs[0]==psxRegs.GPR.r[0]
        &&native_disc_regs[1]==psxRegs.GPR.r[1]&&native_disc_regs[2]==psxRegs.GPR.r[2]
        &&native_disc_regs[3]==psxRegs.GPR.r[3]&&native_disc_regs[28]==psxRegs.GPR.n.gp
        &&native_disc_regs[29]==psxRegs.GPR.n.sp&&gpu.vram[0]==0x001f
        &&gpu.vram[239*1024+255]==0x001f;
    mark("Direct disc native result parity",parity);
    if(report){
        fprintf(report,"Direct disc runner,loop=%s,draw=%s,dispatches=%lu/%lu,cycles=%lu,native_cycles=%lu,us=%llu\n",
            run_name(loop),run_name(draw),(unsigned long)loop_dispatches,(unsigned long)draw_dispatches,
            (unsigned long)(psxRegs.cycle-cycle_start),(unsigned long)native_disc_cycles,
            (unsigned long long)(sceKernelGetSystemTimeWide()-began));fflush(report);
    }
    Config.HLE=FALSE;
}
#endif
#ifdef REVIVAL_SPU_INTEGRATION
#include "../core_session.h"
static rv_core_session core_session={0};
static R3000Acpu *core_cpu_owner=NULL;
static long core_start_cpu(){
    int rc=psxInit();
    mark("CPU init",rc==0);
    if(rc==0)core_cpu_owner=psxCpu;
    return rc;
}
static long core_stop_cpu(){
    if(!core_cpu_owner)return -1;
    // Diagnostics switch the active engine. Shutdown the one which allocated
    // the core tables and RAM, not the last engine selected for execution.
    psxCpu=core_cpu_owner;
    psxShutdown();
    core_cpu_owner=NULL;psxCpu=NULL;
    memset(&psxRegs,0,sizeof(psxRegs));
    return 0;
}
static long core_start_spu(){
    memset(&spu_config,0,sizeof(spu_config));
    spu_config.iHaveConfiguration=1;spu_config.iDisabled=1;
    spu_config.iVolume=768;spu_config.iUseInterpolation=2;
    long rc=SPUinit();
    if(rc==0){
        rc=SPUopen();
        if(rc!=0)SPUshutdown();
    }
    mark("SPU init null backend",rc==0);
    if(rc==0){SPUregisterCallback(probe_spu_irq);SPUregisterScheduleCb(probe_spu_schedule);}
    return rc;
}
static long core_stop_spu(){return SPUshutdown();}
static long core_start_gpu(){
    long rc=GPU_init();
    if(rc==0&&!gpu.vram)rc=-1;
    mark("GPU init",rc==0);
    if(rc!=0&&gpu.vram)GPU_shutdown();
    return rc;
}
static long core_stop_gpu(){
    long rc=GPU_shutdown();
    return rc==0&&gpu.vram? -1:rc;
}
static const rv_core_stage core_stages[RV_CORE_STAGES]={
    {core_start_cpu,core_stop_cpu},{core_start_spu,core_stop_spu},{core_start_gpu,core_stop_gpu}
};
#endif
int main(){
    pspDebugScreenInit();
    report=fopen("PCSX4ALL_INTEGRATION.TXT","w");
    memset(&Config,0,sizeof(Config));Config.Cpu=CPU_DYNAREC;
#ifdef REVIVAL_SPU_INTEGRATION
    long core_rc=rv_core_begin(&core_session,core_stages);
    if(core_rc!=0){
        if(report){fprintf(report,"Core init failure,%ld,remaining=%u\n",core_rc,core_session.active);fclose(report);}
        sceKernelExitGame();return 1;
    }
#else
    if(psxInit()<0){mark("CPU init",false);if(report)fclose(report);sceKernelExitGame();return 1;}
    mark("CPU init",true);
    if(GPU_init()!=0 || !gpu.vram){mark("GPU init",false);if(report)fclose(report);sceKernelExitGame();return 1;}
    mark("GPU init",true);
#endif
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
#ifdef REVIVAL_EXE_INTEGRATION
    exe_test();
#endif
#ifdef REVIVAL_HLE_INTEGRATION
    hle_test();
#endif
#ifdef REVIVAL_SPU_INTEGRATION
    psxMemWrite16(0x1f801daa,0x8000);
    mark("SPU MMIO control",(psxMemRead16(0x1f801daa)&0x8000)!=0);
    SPUasync(psxRegs.cycle,1);
    mark("SPU update returned",true);
    disc_boot_test();
#endif
#ifdef REVIVAL_SPU_INTEGRATION
    // Record native boot mode before changing it. The upstream HLE uses
    // GP1(08)=0x27 (640-wide/interlaced), rejected by pre-V6 PSP vout.
    extern unsigned probe_unsupported,probe_blanks;
    if(report){
        fprintf(report,"GU boot mode,%08lx,%d,%d,%d,%d,rgb24=%d,interlace=%d,blank=%d\n",
            (unsigned long)gpu.status.reg,(int)gpu.screen.x,(int)gpu.screen.y,
            (int)gpu.screen.w,(int)gpu.screen.h,(int)gpu.status.rgb24,
            (int)gpu.status.interlace,(int)gpu.status.blanking);
        fprintf(report,"GU boot counters,%u,%u,%u\n",probe_presentations,probe_unsupported,probe_blanks);
        fflush(report);
    }
    unsigned before=probe_presentations;
    // Explicit 256x240 NTSC progressive 15-bit window over guest red fill.
    GPU_writeStatus(0x08000000);
    GPU_writeStatus(0x05000000);
    GPU_writeStatus(0x06c60260);
    GPU_writeStatus(0x07042018);
    GPU_writeStatus(0x03000000);
    GPU_requestScreenRedraw();GPU_updateLace();
    if(report){
        fprintf(report,"GU test mode,%08lx,%d,%d,%d,%d\n",(unsigned long)gpu.status.reg,
            (int)gpu.screen.x,(int)gpu.screen.y,(int)gpu.screen.w,(int)gpu.screen.h);
        fprintf(report,"GU test counters,%u,%u,%u\n",probe_presentations,probe_unsupported,probe_blanks);
        fflush(report);
    }
    mark("GU submission",probe_presentations>before);
#else
    GPU_writeStatus(0x03000000);GPU_requestScreenRedraw();GPU_updateLace();
    mark("GU submission",probe_presentations>0);
#endif
    // Preserve the V2 visual observation window before additional tests.
    sceKernelDelayThread(10000000);
#ifdef REVIVAL_SPU_INTEGRATION
    if(report){fprintf(report,"DMA extension begin\n");fflush(report);}
    dma4_test();
    dma3_test();
    if(report){fprintf(report,"Runner extension begin\n");fflush(report);}
    runner_test();
    if(report){fprintf(report,"PAD extension begin\n");fflush(report);}
    pad_test();
    if(report){fprintf(report,"Video extension begin\n");fflush(report);}
    video_test();
    if(report){fprintf(report,"Direct disc extension begin\n");fflush(report);}
    direct_disc_test();
    // Retry a failed CD cleanup before leaving the diagnostic; no reopening.
    if(disc_session.state!=RV_DISC_IDLE){
        long rc=rv_disc_end(&disc_session,&disc_ops);
        if(report){fprintf(report,"Disc final shutdown,%ld\n",rc);fflush(report);}
    }
    // Never tear down lower-level core resources if the CD still owns them.
    long shutdown_rc=disc_session.state==RV_DISC_IDLE?rv_core_end(&core_session,core_stages):-1;
    mark("Core ordered shutdown",shutdown_rc==0&&core_session.active==0&&!gpu.vram
        &&!core_cpu_owner&&!psxCpu&&!psxM&&!psxH&&!psxP&&!psxR);
    if(report){fprintf(report,"Core shutdown,%ld,remaining=%u\n",shutdown_rc,core_session.active);fflush(report);}
#else
    GPU_shutdown();
#endif
    if(report)fclose(report);
    sceKernelExitGame();return 0;
}
