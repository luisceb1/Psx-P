/* SPDX-License-Identifier: GPL-2.0-or-later */
#include "timer_policy.h"
#include <assert.h>
#include <stdio.h>
int main(void) {
    assert(revival_timer_delay(0,0,1,60,8)==1);
    assert(revival_timer_delay(0,60,1,60,8)==1);
    assert(revival_timer_delay(0,61,1,60,8)==2);
    assert(revival_timer_delay(0,0,1,60,0)==1093);
    assert(revival_timer_delay(100,50,0,60,8)==1);
    assert(!revival_timer_irq(0x48,0));
    assert(revival_timer_irq(0x58,1));
    assert(revival_timer_irq(0x18,0));
    assert(!revival_timer_irq(0x18,1));
    assert(revival_timer_irq(0x60,1));
    assert(!revival_timer_irq(0x40,0));
    puts("timer policy tests passed");
    return 0;
}
