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
/* Compare every staged pixel against a separate scalar reference, and
   verify every destination word outside the active image stays untouched. */
static uint16_t reference_pixel(const revival_display_plan *p,unsigned x,unsigned y){
    unsigned sy=(p->y+y*p->h/p->stage_h)%512u;
    unsigned sx=x*p->w/p->stage_w;
    if(!p->rgb24)return (uint16_t)(vram[sy*1024+(p->x+sx)%1024u]|0x8000u);
    unsigned channels[3];
    for(unsigned k=0;k<3;k++){
        unsigned pos=(p->x*2u+sx*3u+k)%2048u;
        uint16_t word=vram[sy*1024+pos/2u];
        channels[k]=(pos%2u?word/256u:word%256u)/8u;
    }
    return (uint16_t)(32768u+channels[0]+32u*channels[1]+1024u*channels[2]);
}
static void test_matrix(void){
    static const unsigned sizes[][2]={
        {1,1},{256,240},{320,240},{368,240},{512,256},
        {640,240},{640,480},{256,512},{640,512},{1,512},{640,1}
    };
    static const unsigned origins[][2]={{0,0},{17,29},{1023,511},{768,300}};
    for(unsigned i=0;i<1024u*512u;i++)
        vram[i]=(uint16_t)((i*4051u)^(i/37u)^(i>>8));
    unsigned cases=0;
    for(size_t s=0;s<sizeof(sizes)/sizeof(sizes[0]);s++)
        for(size_t o=0;o<sizeof(origins)/sizeof(origins[0]);o++)
            for(int rgb=0;rgb<=1;rgb++){
                revival_display_plan plan;
                assert(revival_display_make_plan((int)origins[o][0],(int)origins[o][1],
                    (int)sizes[s][0],(int)sizes[s][1],rgb,&plan));
                assert(plan.draw_w>0&&plan.draw_w<=480&&plan.draw_h>0&&plan.draw_h<=272);
                assert(plan.stage_w>0&&plan.stage_w<=512&&plan.stage_h>0&&plan.stage_h<=plan.texture_h);
                size_t stride=plan.stage_w<512?plan.stage_w+1u:512u;
                memset(dst,0x5a,sizeof(dst));
                assert(revival_display_stage(vram,1024u*512u,dst,512u*512u,stride,&plan));
                for(unsigned y=0;y<plan.stage_h;y++){
                    for(unsigned x=0;x<plan.stage_w;x++)
                        assert(dst[y*stride+x]==reference_pixel(&plan,x,y));
                    for(size_t x=plan.stage_w;x<stride;x++)assert(dst[y*stride+x]==0x5a5a);
                }
                for(size_t i=plan.stage_h*stride;i<512u*512u;i++)assert(dst[i]==0x5a5a);
                cases++;
            }
    assert(cases==88);
    puts("display staging reference: PASS (88 mode/origin/format cases, all pixels and padding)");
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
    test_matrix();
    puts("display staging: PASS (RGB5551, RGB24, wrap, scaling, capacity)");
    return 0;
}
