/* SPDX-License-Identifier: GPL-2.0-or-later */
#ifndef REVIVAL_TIMER_POLICY_H
#define REVIVAL_TIMER_POLICY_H
#include <stdint.h>
/* Coarse legacy scheduler: target-reset or overflow, not full PS1 timers. */
static inline uint32_t revival_timer_delay(uint32_t count, uint32_t target, uint32_t rate, uint32_t bias, uint32_t mode) {
    uint32_t limit = (mode & 8u) ? (target & 65535u) : 65536u;
    uint32_t current = count & 65535u;
    uint32_t ticks = limit > current ? limit-current : 1u;
    uint64_t cycles = ((uint64_t)ticks * (rate ? rate : 1u) + (bias ? bias : 1u)-1u)/(bias ? bias : 1u);
    return cycles > 0x7fffffffu ? 0x7fffffffu : cycles ? (uint32_t)cycles : 1u;
}
static inline int revival_timer_irq(uint32_t mode, int fired) {
    uint32_t enable = (mode & 8u) ? 0x10u : 0x20u;
    return (mode & enable) && ((mode & 0x40u) || !fired);
}
#endif
