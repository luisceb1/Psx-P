/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Native PSP presentation of the software-rendered PS1 display window. */
#include <pspgu.h>
#include <pspdisplay.h>
#include <pspkernel.h>
#include <stdint.h>
#include <string.h>
#include "psxcommon.h"
#include "gpu/gpulib/gpu.h"
#include "plugin_lib.h"
#include "../display_stage.h"
#ifndef REVIVAL_GPU_INTEGRATION
PcsxConfig Config;
struct pl_data_t pl_data={};
uint32_t hSyncCount=0, frame_counter=0;
#endif
unsigned probe_presentations=0, probe_blanks=0, probe_unsupported=0;
uint64_t probe_present_us=0;
unsigned probe_stage_w=0,probe_stage_h=0;
static unsigned int list[4096] __attribute__((aligned(64)));
static uint16_t texture[512*512] __attribute__((aligned(64)));
static bool ready=false;
struct Vertex { uint16_t u,v; int16_t x,y,z; };
void pl_clear_borders() {}
int vout_init(void) {
    if(ready)return -1;
    probe_presentations=probe_blanks=probe_unsupported=0;
    probe_present_us=0;probe_stage_w=probe_stage_h=0;
    sceGuInit();
    sceGuStart(GU_DIRECT,list);
    sceGuDrawBuffer(GU_PSM_5551,(void*)0,512);
    sceGuDispBuffer(480,272,(void*)(512*272*2),512);
    sceGuOffset(2048-240,2048-136);
    sceGuViewport(2048,2048,480,272);
    sceGuScissor(0,0,480,272);
    sceGuEnable(GU_SCISSOR_TEST);
    sceGuDisable(GU_DEPTH_TEST);
    sceGuDisable(GU_BLEND);
    sceGuDisable(GU_ALPHA_TEST);
    sceGuClearColor(0xff000000);
    sceGuClear(GU_COLOR_BUFFER_BIT);
    sceGuFinish();sceGuSync(0,0);
    sceDisplayWaitVblankStart();
    sceGuDisplay(GU_TRUE);
    ready=true;
    return 0;
}
int vout_finish(void) {
    if(ready){sceGuSync(0,0);sceGuDisplay(GU_FALSE);sceGuTerm();ready=false;}
    probe_stage_w=probe_stage_h=0;
    return 0;
}
void vout_blank(void) {
    if(!ready)return;
    sceGuStart(GU_DIRECT,list);
    sceGuClearColor(0xff000000);sceGuClear(GU_COLOR_BUFFER_BIT);
    sceGuFinish();sceGuSync(0,0);sceDisplayWaitVblankStart();sceGuSwapBuffers();
    ++probe_blanks;
}
void vout_update(void) {
    if(!ready)return;
    revival_display_plan plan={};
    if(!revival_display_make_plan(gpu.screen.x,gpu.screen.y,gpu.screen.w,gpu.screen.h,gpu.status.rgb24,&plan)){
        ++probe_unsupported;return;
    }
    const uint64_t start=sceKernelGetSystemTimeWide();
    // For interlaced modes, present a snapshot of all currently stored rows.
    // This is not a field-timed deinterlacer.
    if(!revival_display_stage(gpu.vram,1024u*512u,texture,512u*512u,512,&plan)){
        ++probe_unsupported;return;
    }
    sceKernelDcacheWritebackRange(texture,512u*plan.stage_h*sizeof(uint16_t));
    int dw=plan.draw_w,dh=plan.draw_h;
    int w=plan.stage_w,h=plan.stage_h;
    sceGuStart(GU_DIRECT,list);
    sceGuClearColor(0xff000000);sceGuClear(GU_COLOR_BUFFER_BIT);
    sceGuEnable(GU_TEXTURE_2D);
    sceGuTexMode(GU_PSM_5551,0,0,GU_FALSE);
    sceGuTexImage(0,512,plan.texture_h,512,texture);
    sceGuTexFunc(GU_TFX_REPLACE,GU_TCC_RGB);
    sceGuTexFilter(GU_NEAREST,GU_NEAREST);
    sceGuTexWrap(GU_CLAMP,GU_CLAMP);
    Vertex *v=(Vertex*)sceGuGetMemory(2*sizeof(Vertex));
    v[0]={(uint16_t)0,(uint16_t)0,(int16_t)((480-dw)/2),(int16_t)((272-dh)/2),0};
    v[1]={(uint16_t)w,(uint16_t)h,(int16_t)((480-dw)/2+dw),(int16_t)((272-dh)/2+dh),0};
    sceGuDrawArray(GU_SPRITES,GU_TEXTURE_16BIT|GU_VERTEX_16BIT|GU_TRANSFORM_2D,2,0,v);
    sceGuDisable(GU_TEXTURE_2D);
    sceGuFinish();sceGuSync(0,0);
    probe_present_us+=sceKernelGetSystemTimeWide()-start;
    sceDisplayWaitVblankStart();sceGuSwapBuffers();
    probe_stage_w=plan.stage_w;probe_stage_h=plan.stage_h;
    ++probe_presentations;
}
uint16_t probe_stage_pixel(unsigned x,unsigned y){
    if(x>=probe_stage_w||y>=probe_stage_h)return 0;
    return texture[y*512+x];
}
void vout_set_config(const gpulib_config_t *) {}
