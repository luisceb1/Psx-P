/* SPDX-License-Identifier: GPL-2.0-or-later */
#ifndef REVIVAL_PRESENT_H
#define REVIVAL_PRESENT_H
#include <stdint.h>
/* vram: 1024x512 words. dst: 480x272, pitch in bytes. Return 1 on success. */
int revival_present(const uint16_t *vram, void *dst, int pitch,
    int x, int y, int width, int height, int left, int top,
    int rgb24, int red_high);
#endif
