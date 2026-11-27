/* SPDX-License-Identifier: GPL-2.0-or-later */
#include "storage.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
int main(void) {
    char out[128];
    assert(!revival_app_dir("ef0:/PSP/GAME/PSXPRV/EBOOT.PBP",out,sizeof out));
    assert(!strcmp(out,"ef0:/PSP/GAME/PSXPRV/"));
    assert(!revival_app_dir("ms0:/PSP/GAME/My Emulator/EBOOT.PBP",out,sizeof out));
    assert(!strcmp(out,"ms0:/PSP/GAME/My Emulator/"));
    assert(!revival_app_dir("ms0:/EBOOT.PBP",out,sizeof out));
    assert(!strcmp(out,"ms0:/"));
    assert(revival_app_dir("EBOOT.PBP",out,sizeof out)==-1 && !out[0]);
    assert(revival_app_dir("host0:/EBOOT.PBP",out,sizeof out)==-1);
    assert(revival_app_dir("ms0:/PSP/../EBOOT.PBP",out,sizeof out)==-1);
    assert(revival_app_dir("ms0:/PSP/./EBOOT.PBP",out,sizeof out)==-1);
    assert(revival_app_dir("ms0:/PSP//EBOOT.PBP",out,sizeof out)==-1);
    assert(revival_app_dir("ms0:/PSP/",out,sizeof out)==-1);
    assert(revival_app_dir("ms0:/PSP/EBOOT.PBP",out,5)==-1 && !out[0]);
    assert(revival_app_dir(NULL,out,sizeof out)==-1);
    assert(revival_app_dir("ms0:/EBOOT.PBP",NULL,0)==-1);
    puts("storage tests passed (12 scenarios)");
    return 0;
}
