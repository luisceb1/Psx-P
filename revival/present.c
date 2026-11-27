/* SPDX-License-Identifier: GPL-2.0-or-later */
#include "present.h"
#include <stddef.h>
#include <string.h>
int revival_present(const uint16_t *vram, void *dst, int pitch,
    int x, int y, int width, int height, int left, int top,
    int rgb24, int red_high)
{
    int ix[480], sx, sy, row, col, full, last = -1;
    uint32_t step_x, step_y;
    unsigned char *out = dst;
    if (!vram || !dst || ((uintptr_t)dst & 3u) || pitch < 1920 ||
        pitch > 8192 || (pitch & 3) || x < 0 || x >= 1024 ||
        y < 0 || y >= 512 || width <= 0 || width > 640 ||
        height <= 0 || height > 512 || left < 0 || left > 640-width ||
        top < 0 || top >= height || y > 512-(height-top) ||
        (rgb24 ? width*3 > (1024-x)*2 : width > 1024-x)) return 0;
    full = width+left;
    step_x = ((uint32_t)full << 16)/480u;
    step_y = ((uint32_t)height << 16)/272u;
    for (col=0; col<480; ++col) ix[col] = (int)((col*step_x)>>16)-left;
    for (row=0; row<272; ++row) {
        uint32_t *d = (uint32_t *)(out+row*pitch);
        sy = (int)((row*step_y)>>16)-top;
        if (row && sy == last) { memcpy(d,out+(row-1)*pitch,1920); continue; }
        last = sy;
        if (sy < 0) { memset(d,0,1920); continue; }
        if (rgb24) {
            const unsigned char *s = (const unsigned char *)(vram+(y+sy)*1024+x);
            for (col=0; col<480; ++col) {
                sx = ix[col];
                if (sx < 0) d[col]=0;
                else {
                    const unsigned char *p=s+sx*3;
                    d[col]=red_high ? ((uint32_t)p[0]<<16)|((uint32_t)p[1]<<8)|p[2] :
                                      ((uint32_t)p[2]<<16)|((uint32_t)p[1]<<8)|p[0];
                }
            }
        } else {
            const uint16_t *s = vram+(y+sy)*1024+x;
            for (col=0; col<480; ++col) {
                uint32_t p;
                sx=ix[col];
                if (sx < 0) { d[col]=0; continue; }
                p=s[sx];
                d[col]=red_high ? ((p<<19)&0xf80000)|((p<<6)&0xf800)|((p>>7)&0xf8) :
                                  ((p<<3)&0xf8)|((p<<6)&0xf800)|((p<<9)&0xf80000);
            }
        }
    }
    return 1;
}
