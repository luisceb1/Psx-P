/* SPDX-License-Identifier: GPL-2.0-or-later */
#include "display_stage.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static uint16_t vram[1024*512],dst[512*512];
static void putbyte(unsigned y,unsigned x,unsigned value){
    uint16_t *p=&vram[y*1024+((x&2047u)>>1)];
    unsigned shift=(x&1u)*8;
    *p=(uint16_t)((*p&~(255u<<shift))|(value<<shift));
}
int main(void){
    revival_display_plan p;
    assert(revival_display_make_plan(1023,511,2,2,0,&p));
    vram[511*1024+1023]=0x001f;vram[511*1024]=0x03e0;
    vram[1023]=0x7c00;vram[0]=0x1234;
    memset(dst,0x5a,sizeof(dst));
    assert(revival_display_stage(vram,1024*512,dst,512*512,512,&p));
    assert(dst[0]==0x801f&&dst[1]==0x83e0&&dst[512]==0xfc00&&dst[513]==0x9234);
    assert(dst[2]==0x5a5a&&dst[514]==0x5a5a);
    assert(revival_display_make_plan(1023,0,2,1,1,&p));
    putbyte(0,2046,255);putbyte(0,2047,0);putbyte(0,0,0);
    putbyte(0,1,0);putbyte(0,2,255);putbyte(0,3,255);
    assert(revival_display_stage(vram,1024*512,dst,512*512,512,&p));
    assert(dst[0]==0x801f&&dst[1]==0xffe0);
    assert(revival_display_make_plan(0,0,640,480,0,&p));
    assert(p.draw_w==362&&p.draw_h==272&&p.stage_w==362&&p.stage_h==272&&p.texture_h==512);
    memset(vram,0,sizeof(vram));
    vram[478*1024+638]=0x7c00;
    assert(revival_display_stage(vram,1024*512,dst,512*512,512,&p));
    assert(dst[271*512+361]==0xfc00);
    assert(revival_display_make_plan(0,0,256,240,0,&p)&&p.stage_w==256&&p.stage_h==240&&p.texture_h==256);
    dst[0]=0x5a5a;
    assert(!revival_display_stage(vram,1024*512,dst,1,512,&p)&&dst[0]==0x5a5a);
    assert(!revival_display_make_plan(0,0,641,480,0,&p));
    assert(!revival_display_make_plan(0,0,640,513,0,&p));
    assert(!revival_display_make_plan(-1,0,256,240,0,&p));
    puts("display staging: PASS (RGB5551, RGB24, wrap, scaling, capacity)");
    return 0;
}
