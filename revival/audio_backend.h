/* SPDX-License-Identifier: GPL-2.0-or-later */
#ifndef REVIVAL_AUDIO_BACKEND_H
#define REVIVAL_AUDIO_BACKEND_H
#include <stdint.h>
#include <stdio.h>
int SetupSound(void);
void RemoveSound(void);
unsigned long SoundGetBytesBuffered(void);
void SoundFeedStreamData(unsigned char *p,long bytes);
unsigned long revival_audio_time_ms(void);
#define timeGetTime revival_audio_time_ms
void revival_audio_snapshot(FILE *f);
unsigned long long revival_audio_begin_mix(void);
void revival_audio_end_mix(unsigned long long started);
#endif
