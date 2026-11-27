/***************************************************************************
                            psp.c  -  description
                             -------------------
    begin                : Sat Mar 01 2003
    copyright            : (C) 2002 by Pete Bernert
    email                : BlackDove@addcom.de
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version. See also the license.txt file for *
 *   additional informations.                                              *
 *                                                                         *
 ***************************************************************************/

//*************************************************************************//
// History of changes:
//
// 2006/10/20 - Create by Yoshihiro
//
//*************************************************************************//

/* PSP Revival: replace incomplete historical SDL output with bounded PCM queue. */
#include <SDL.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include "audio_backend.h"
#include "pcm_queue.h"
#include "log.h"
#include <pspkernel.h>

static struct revival_pcm_queue queue;
static int opened;
static unsigned int produced, consumed, silent, dropped, nonzero;
static uint64_t mix_us;
static unsigned int mix_calls;
static void audio_callback(void *userdata, Uint8 *stream, int len) {
    unsigned int got;
    (void)userdata;
    if(len<=0) return;
    got=revival_pcm_read(&queue,stream,(unsigned int)len);
    consumed+=got/4;
    silent+=((unsigned int)len-got)/4;
}
int SetupSound(void) {
    SDL_AudioSpec want, got;
    if(opened) return 1;
    memset(&queue,0,sizeof(queue)); memset(&want,0,sizeof(want));
    produced=consumed=silent=dropped=nonzero=mix_calls=0; mix_us=0;
    if(SDL_InitSubSystem(SDL_INIT_AUDIO)<0) { DLog("SPU SDL init: %s", SDL_GetError()); return 0; }
    want.freq=44100; want.format=AUDIO_S16LSB; want.channels=2; want.samples=512;
    want.callback=audio_callback;
    if(SDL_OpenAudio(&want,&got)<0) { DLog("SPU SDL open: %s",SDL_GetError()); SDL_QuitSubSystem(SDL_INIT_AUDIO); return 0; }
    if(got.freq!=44100 || got.format!=AUDIO_S16LSB || got.channels!=2) {
        DLog("SPU unsupported audio format"); SDL_CloseAudio(); SDL_QuitSubSystem(SDL_INIT_AUDIO); return 0;
    }
    opened=1;
    DLog("SPU P.E.Op.S. audio open: %d Hz, %d channels, %d samples",got.freq,got.channels,got.samples);
    SDL_PauseAudio(0);
    return 1;
}
void RemoveSound(void) {
    if(!opened) return;
    SDL_CloseAudio(); opened=0;
    SDL_QuitSubSystem(SDL_INIT_AUDIO);
}
void SoundFeedStreamData(unsigned char *p,long bytes) {
    unsigned int n,i,wrote;
    if(!opened || bytes<=0 || bytes>32768) return;
    n=(unsigned int)bytes & ~3u;
    SDL_LockAudio();
    wrote=revival_pcm_write(&queue,p,n);
    produced+=n/4; dropped+=(n-wrote)/4;
    for(i=0;i<n;i+=4) if(p[i]||p[i+1]||p[i+2]||p[i+3]) ++nonzero;
    SDL_UnlockAudio();
}
unsigned long SoundGetBytesBuffered(void) {
    unsigned int n=0;
    if(opened) { SDL_LockAudio(); n=queue.used; SDL_UnlockAudio(); }
    return n;
}
unsigned long revival_audio_time_ms(void) { return SDL_GetTicks(); }
void revival_audio_snapshot(FILE *f) {
    unsigned int a,b,c,d,e;
    if(opened) SDL_LockAudio();
    a=produced;b=consumed;c=silent;d=dropped;e=nonzero;
    if(opened) SDL_UnlockAudio();
    fprintf(f,",%u,%u,%u,%u,%u,%llu,%u",a,b,c,d,e,(unsigned long long)mix_us,mix_calls);
}

unsigned long long revival_audio_begin_mix(void) { return sceKernelGetSystemTimeWide(); }
void revival_audio_end_mix(unsigned long long started) { mix_us += sceKernelGetSystemTimeWide()-started; ++mix_calls; }
