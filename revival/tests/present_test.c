/* SPDX-License-Identifier: GPL-2.0-or-later */
#include "present.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static uint16_t vram[1024*512];
static uint32_t dst[272][512];
int main(void) {
    unsigned char *rgb = (unsigned char *)vram;
    memset(dst,0xa5,sizeof dst);
    vram[0]=0x001f; vram[1]=0x03e0; vram[2]=0x7c00;
    assert(revival_present(vram,dst,2048,0,0,3,1,0,0,0,0));
    assert(dst[0][0]==0xf8 && dst[0][200]==0xf800 && dst[0][400]==0xf80000);
    assert(dst[271][0]==0xf8 && dst[0][480]==0xa5a5a5a5);
    assert(revival_present(vram,dst,2048,0,0,3,1,0,0,0,1));
    assert(dst[0][0]==0xf80000 && dst[0][400]==0xf8);
    rgb[0]=0x12;rgb[1]=0x34;rgb[2]=0x56;
    assert(revival_present(vram,dst,2048,0,0,1,2,1,1,1,0));
    assert(dst[0][479]==0 && dst[271][0]==0 && dst[271][479]==0x563412);
    assert(revival_present(vram,dst,2048,0,0,1,1,0,0,1,1));
    assert(dst[271][479]==0x123456);
    assert(!revival_present(vram,dst,2048,1023,0,1,1,0,0,1,0));
    assert(dst[271][479]==0x123456);
    assert(!revival_present(vram,dst,1919,0,0,1,1,0,0,0,0));
    assert(!revival_present(vram,dst,2048,0,511,1,2,0,0,0,0));
    puts("presentation tests passed");
    return 0;
}
