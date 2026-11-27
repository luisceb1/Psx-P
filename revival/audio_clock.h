/* SPDX-License-Identifier: GPL-2.0-or-later */
#ifndef REVIVAL_AUDIO_CLOCK_H
#define REVIVAL_AUDIO_CLOCK_H
#include <stdint.h>
#define REVIVAL_AUDIO_BLOCK_CYCLES (768u * 45u)
#define REVIVAL_AUDIO_MAX_BLOCKS 8u
struct revival_audio_clock { uint32_t remainder; };
static inline unsigned int revival_audio_clock_add(struct revival_audio_clock *c,uint32_t elapsed) {
    uint32_t limit=REVIVAL_AUDIO_BLOCK_CYCLES*REVIVAL_AUDIO_MAX_BLOCKS;
    uint32_t total, blocks;
    /* Bound work even after a long scheduling gap, without overflow. */
    if(elapsed>limit) elapsed=limit;
    total=c->remainder+elapsed;
    blocks=total/REVIVAL_AUDIO_BLOCK_CYCLES;
    c->remainder=total%REVIVAL_AUDIO_BLOCK_CYCLES;
    return blocks;
}
#endif
