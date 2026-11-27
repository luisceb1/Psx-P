/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Fail closed: these dependencies are NOT implemented by the guest probe. */
#include <pspkernel.h>
#include <pspdebug.h>
#include "r3000a.h"
#include "psxhw.h"
#include "psxdma.h"
#include "psxhle.h"
#include "cdrom.h"
#include "sio.h"
#include "mdec.h"
#include "misc.h"
#include "plugins.h"
#include "plugin_lib.h"
static void __attribute__((noreturn)) unexpected(const char *name)
{
    pspDebugScreenPrintf("Unexpected peripheral: %s\n",name);
    sceKernelDelayThread(10000000);
    sceKernelExitGame();
    for(;;) {}
}
#define STOP unexpected(__func__)
#define V0(name) void name(void) { STOP; }
#define V1(name,T) void name(T) { STOP; }
#define R0(T,name) T name(void) { STOP; }
#ifndef REVIVAL_EXE_INTEGRATION
char CdromId[10]={0};
#endif
struct pl_data_t pl_data={};
static void trap(void) { STOP; }
#ifndef REVIVAL_HLE_INTEGRATION
void (*biosA0[256])(void)={trap};
void (*biosB0[256])(void)={trap};
void (*biosC0[256])(void)={trap};
void (*psxHLEt[256])(void)={trap};
V0(psxBiosInit) V0(psxBiosShutdown) V0(psxBiosException)
#endif
#ifndef REVIVAL_GPU_INTEGRATION
V0(psxHwReset)
u8 psxHwRead8(u32) { STOP; }
u16 psxHwRead16(u32) { STOP; }
u32 psxHwRead32(u32) { STOP; }
void psxHwWrite8(u32,u8) { STOP; }
void psxHwWrite16(u32,u16) { STOP; }
void psxHwWrite32(u32,u32) { STOP; }
#else
#ifndef REVIVAL_HLE_INTEGRATION
V0(mdecInit) V0(sioInit)
#endif
#ifndef REVIVAL_CD_INTEGRATION
V0(cdrReset)
#endif
#ifndef REVIVAL_HLE_INTEGRATION
void psxDma0(u32,u32,u32){STOP;}
void psxDma1(u32,u32,u32){STOP;}
#endif
#ifndef REVIVAL_CD_INTEGRATION
void psxDma3(u32,u32,u32){STOP;}
#endif
#endif
#ifndef REVIVAL_CD_INTEGRATION
V0(cdrInterrupt) V0(cdrReadInterrupt) V0(cdrLidSeekInterrupt)
V0(cdrPlayInterrupt) V0(cdrDmaInterrupt)
R0(unsigned char,cdrRead0) R0(unsigned char,cdrRead1)
R0(unsigned char,cdrRead2) R0(unsigned char,cdrRead3)
V1(cdrWrite0,unsigned char) V1(cdrWrite1,unsigned char)
V1(cdrWrite2,unsigned char) V1(cdrWrite3,unsigned char)
#else
#ifndef REVIVAL_EXE_INTEGRATION
int CheckCdrom(void){STOP;}
#endif
void CALLBACK SPUplayADPCMchannel(xa_decode_t*){STOP;}
unsigned int CALLBACK SPUgetADPCMBufferRoom(void){STOP;}
#endif
#ifndef REVIVAL_GPU_INTEGRATION
V0(gpuInterrupt) V0(spuInterrupt) V0(gpuotcInterrupt)
#else
void CALLBACK SPUwriteDMAMem(unsigned short*,int,unsigned int){STOP;}
void CALLBACK SPUreadDMAMem(unsigned short*,int,unsigned int){STOP;}
#endif
#ifndef REVIVAL_HLE_INTEGRATION
V0(mdec0Interrupt) V0(mdec1Interrupt)
V1(mdecWrite0,u32) V1(mdecWrite1,u32)
R0(u32,mdecRead0) R0(u32,mdecRead1)
V0(sioInterrupt) V0(sioSyncMcds)
R0(unsigned char,sioRead8) R0(u16,sioRead16) R0(u32,sioRead32)
V1(sioWrite8,unsigned char) V1(sioWrite16,u16) V1(sioWrite32,u32)
R0(u16,sioReadBaud16) R0(u16,sioReadCtrl16) R0(u16,sioReadMode16) R0(u16,sioReadStat16)
V1(sioWriteBaud16,u16) V1(sioWriteCtrl16,u16) V1(sioWriteMode16,u16)
#ifndef REVIVAL_GPU_INTEGRATION
V0(GPU_updateLace) R0(uint32_t,GPU_readData) R0(uint32_t,GPU_readStatus)
V1(GPU_writeData,uint32_t)
void GPU_vBlank(int,int) { STOP; }
#endif
void CALLBACK SPUasync(uint32_t,uint32_t) { STOP; }
unsigned short CALLBACK SPUreadRegister(unsigned long) { STOP; }
void CALLBACK SPUwriteRegister(unsigned long,unsigned short,unsigned int) { STOP; }
V0(EmuUpdate)
int freeze_rw(void*,enum FreezeMode,void*,unsigned) { STOP; }
