/* SPDX-License-Identifier: GPL-2.0-or-later */
#include <pspkernel.h>
#include <pspdebug.h>
#include <stdint.h>
#include <stdio.h>
#include "plugins.h"
#include "gpu/gpulib/gpu.h"
PSP_MODULE_INFO("PCSX4ALL GPU probe",0,1,0);
PSP_MAIN_THREAD_ATTR(PSP_THREAD_ATTR_USER);
extern unsigned probe_presentations, probe_blanks;
static FILE *report;
static void check(const char *name,bool ok)
{
    pspDebugScreenPrintf("%s: %s\n",name,ok?"PASS":"FAIL");
    if(report){fprintf(report,"%s,%s\n",name,ok?"PASS":"FAIL");fflush(report);}
}
static void gp0(uint32_t w){GPU_writeData(w);}
static uint32_t xy(unsigned x,unsigned y){return x|(y<<16);}
static uint16_t pixel(unsigned x,unsigned y){return gpu.vram[y*1024+x];}
int main()
{
    pspDebugScreenInit();
    report=fopen("PCSX4ALL_GPU.TXT","w");
    if(report){fprintf(report,"PCSX4ALL GPU probe v1\n");fflush(report);}
    gpu_unai_config_ext.lighting=1;
    gpu_unai_config_ext.blending=1;
    gpu_unai_config_ext.dithering=1;
    if(GPU_init()!=0 || !gpu.vram){check("GPU init",false);sceKernelDelayThread(10000000);sceKernelExitGame();return 1;}
    check("GPU init",true);
    // GP0(02h): fill red, aligned 16x2 rectangle.
    gp0(0x020000ff);gp0(xy(16,16));gp0(xy(16,2));
    GPU_updateLace();
    check("VRAM fill",pixel(16,16)==0x001f && pixel(31,17)==0x001f && pixel(32,16)==0);
    // GP0(A0h): CPU -> VRAM, two 16-bit pixels.
    gp0(0xa0000000);gp0(xy(64,32));gp0(xy(2,1));gp0(0x7c0003e0);
    GPU_updateLace();
    check("VRAM upload",pixel(64,32)==0x03e0 && pixel(65,32)==0x7c00);
    // GP0(C0h): VRAM -> CPU, same packed pixel pair.
    gp0(0xc0000000);gp0(xy(64,32));gp0(xy(2,1));
    check("VRAM readback",GPU_readData()==0x7c0003e0);
    // GP0(80h): VRAM -> VRAM copy.
    gp0(0x80000000);gp0(xy(64,32));gp0(xy(80,40));gp0(xy(2,1));
    GPU_updateLace();
    check("VRAM copy",pixel(80,40)==0x03e0 && pixel(81,40)==0x7c00);
    // Enable drawing to display and full drawing area; opaque green rectangle.
    gp0(0xe1000400);gp0(0xe3000000);gp0(0xe407ffff);gp0(0xe5000000);
    gp0(0x6000ff00);gp0(xy(128,64));gp0(xy(8,4));
    GPU_updateLace();
    check("Rectangle raster",pixel(128,64)==0x03e0 && pixel(135,67)==0x03e0 && pixel(136,64)==0);
    GPU_writeStatus(0x03000000);GPU_requestScreenRedraw();GPU_updateLace();
    check("Presentation callback",probe_presentations>0);
    GPU_shutdown();
    if(report)fclose(report);
    sceKernelDelayThread(10000000);sceKernelExitGame();return 0;
}
