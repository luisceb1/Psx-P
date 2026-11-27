/* SPDX-License-Identifier: GPL-2.0-or-later */
#ifndef REVIVAL_PIXEL_H
#define REVIVAL_PIXEL_H
#include <stdint.h>
static inline uint32_t revival_swap_rb(uint32_t p) {
    return (p & UINT32_C(0xff00ff00)) | ((p & UINT32_C(0x000000ff)) << 16) | ((p & UINT32_C(0x00ff0000)) >> 16);
}
#endif
