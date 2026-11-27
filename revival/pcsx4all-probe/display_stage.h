/* SPDX-License-Identifier: GPL-2.0-or-later */
#ifndef REVIVAL_DISPLAY_STAGE_H
#define REVIVAL_DISPLAY_STAGE_H
#include <stdint.h>
#include <stddef.h>
/* Portable PS1 VRAM -> opaque PSP RGB5551 staging, no GPU/SDK dependency.
 * Coordinates are halfword X / scanline Y. RGB24 is packed R,G,B bytes.
 * Row wrapping and whole-VRAM interlace snapshot are provisional policies. */
typedef struct {
    unsigned x,y,w,h,rgb24,stage_w,stage_h,draw_w,draw_h,texture_h;
} revival_display_plan;
static inline int revival_display_make_plan(int x,int y,int w,int h,int rgb24,revival_display_plan *p){
    if(!p||x<0||x>=1024||y<0||y>=512||w<=0||w>640||h<=0||h>512)return 0;
    p->x=(unsigned)x;p->y=(unsigned)y;p->w=(unsigned)w;p->h=(unsigned)h;p->rgb24=!!rgb24;
    p->draw_w=480;p->draw_h=p->h*480/p->w;
    if(p->draw_h>272){p->draw_h=272;p->draw_w=p->w*272/p->h;}
    if(!p->draw_w)p->draw_w=1;
    if(!p->draw_h)p->draw_h=1;
    /* Preserve previous sampling in modest modes; stage wide/tall modes
       at the final display dimension, within PSP's 512x512 texture limit. */
    p->stage_w=p->w>512?p->draw_w:p->w;
    p->stage_h=p->h>256?p->draw_h:p->h;
    p->texture_h=p->stage_h>256?512:256;
    return 1;
}
static inline unsigned revival_vram_byte(const uint16_t *row,unsigned x){
    x&=2047u;
    return (row[x>>1]>>((x&1u)*8))&255u;
}
static inline uint16_t revival_display_color(const uint16_t *row,unsigned x,int rgb24){
    if(!rgb24)return (uint16_t)(row[x&1023u]|0x8000u);
    unsigned r=revival_vram_byte(row,x),g=revival_vram_byte(row,x+1),b=revival_vram_byte(row,x+2);
    return (uint16_t)(0x8000u|(r>>3)|((g>>3)<<5)|((b>>3)<<10));
}
static inline int revival_display_stage(const uint16_t *vram,size_t words,uint16_t *dst,size_t capacity,
    size_t stride,const revival_display_plan *p){
    if(!vram||!dst||!p||words<1024u*512u||p->x>=1024||p->y>=512||
        !p->w||p->w>640||!p->h||p->h>512||!p->stage_w||p->stage_w>512||
        !p->stage_h||p->stage_h>512||p->stage_w>stride||stride>capacity/p->stage_h)return 0;
    unsigned map[512];
    for(unsigned x=0;x<p->stage_w;x++){
        unsigned sx=x*p->w/p->stage_w;
        map[x]=p->rgb24?p->x*2+sx*3:p->x+sx;
    }
    for(unsigned y=0;y<p->stage_h;y++){
        unsigned sy=y*p->h/p->stage_h;
        const uint16_t *row=vram+((p->y+sy)&511u)*1024;
        uint16_t *out=dst+y*stride;
        if(!p->rgb24){
            for(unsigned x=0;x<p->stage_w;x++)out[x]=(uint16_t)(row[map[x]&1023u]|0x8000u);
        }else{
            for(unsigned x=0;x<p->stage_w;x++)out[x]=revival_display_color(row,map[x],1);
        }
    }
    return 1;
}
#endif
