/* SPDX-License-Identifier: GPL-2.0-or-later */
#ifndef REVIVAL_PSP_AUDIO_H
#define REVIVAL_PSP_AUDIO_H
#include <stdint.h>
typedef struct {
    uint32_t accepted,dropped,nonzero,submitted,errors,busy,invalid,tail;
    unsigned pending;
    int channel,release_rc;
} rv_psp_audio_metrics;
int rv_psp_audio_init(void);
void rv_psp_audio_feed(void *data,int bytes);
void rv_psp_audio_pump(void);
unsigned rv_psp_audio_room(void);
int rv_psp_audio_active(void);
void rv_psp_audio_finish(void);
rv_psp_audio_metrics rv_psp_audio_get_metrics(void);
struct out_driver;
void out_register_psp(struct out_driver *drv);
#endif
