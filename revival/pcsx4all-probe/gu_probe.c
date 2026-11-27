/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Compile-only native API experiment: textured sprite presentation.
 * Caller would own GU init, valid buffers and synchronized texture memory.
 * No integration with either emulator or claims of PS1 raster accuracy. */
#include <pspgu.h>
#include <stdint.h>
struct vertex { uint16_t u,v; int16_t x,y,z; };
void revival_gu_sprite_probe(const void *texture, struct vertex *verts)
{
    sceGuEnable(GU_TEXTURE_2D);
    sceGuTexMode(GU_PSM_5650,0,0,GU_FALSE);
    sceGuTexImage(0,512,256,512,texture);
    sceGuTexFunc(GU_TFX_REPLACE,GU_TCC_RGB);
    sceGuTexFilter(GU_NEAREST,GU_NEAREST);
    sceGuDrawArray(GU_SPRITES,GU_TEXTURE_16BIT|GU_VERTEX_16BIT|GU_TRANSFORM_2D,
                   2,0,verts);
    sceGuDisable(GU_TEXTURE_2D);
}
