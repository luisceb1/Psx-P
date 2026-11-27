/* SPDX-License-Identifier: GPL-2.0-or-later */
#include "storage.h"
#include <string.h>
int revival_app_dir(const char *eboot, char *out, size_t capacity) {
    const char *last, *p, *end;
    size_t n;
    if (!out || !capacity) return -1;
    out[0] = '\0';
    if (!eboot || (strncmp(eboot,"ef0:/",5) && strncmp(eboot,"ms0:/",5))) return -1;
    last = strrchr(eboot,'/');
    if (!last || !last[1] || strchr(eboot,'\\') || strchr(eboot+4,':')) return -1;
    /* Reject empty and dot segments to avoid ambiguous paths. */
    for (p=eboot+5; *p; p=end+1) {
        end=strchr(p,'/');
        n=end ? (size_t)(end-p) : strlen(p);
        if (!n || (n==1 && p[0]=='.') || (n==2 && p[0]=='.' && p[1]=='.')) return -1;
        if (!end) break;
    }
    n=(size_t)(last-eboot)+1;
    if (n>=capacity) return -1;
    memcpy(out,eboot,n);
    out[n]='\0';
    return 0;
}
