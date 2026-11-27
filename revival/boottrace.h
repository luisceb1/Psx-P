/* SPDX-License-Identifier: GPL-2.0-or-later */
#ifndef REVIVAL_BOOTTRACE_H
#define REVIVAL_BOOTTRACE_H
#include <stdio.h>
struct revival_cd_trace {
    unsigned int commands, queued, delivered, data_ready;
    unsigned int ack_writes, ack_accepted;
    unsigned int last_cmd, last_irq, last_ack;
};
extern struct revival_cd_trace revival_cd_trace;
extern int revival_cd_ack_fix;
extern int revival_timer_fix;
extern int revival_fastcore;
extern unsigned int revival_timer_events[3], revival_timer_irqs[3], revival_hw_exceptions;
void revival_boot_header(FILE *f);
void revival_boot_snapshot(FILE *f);
#endif
