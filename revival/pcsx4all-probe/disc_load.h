/* SPDX-License-Identifier: GPL-2.0-or-later */
#ifndef REVIVAL_DISC_LOAD_H
#define REVIVAL_DISC_LOAD_H
#include <stdlib.h>
#include "disc_preflight.h"
/* Transactional read/stage/commit for the narrow synthetic-media profile.
 * Staging never touches guest RAM; caller closes read handle before commit.
 * This is not a general boot API and does not execute guest code. */
enum { RV_LOAD_NO_MEMORY=7, RV_LOAD_DESTINATION=8, RV_LOAD_STATE=9 };
typedef struct {
    rv_pf_result meta;
    unsigned char *payload;
    size_t payload_bytes;
} rv_load_staged;
static inline void rv_load_discard(rv_load_staged *s){
    if(s){free(s->payload);memset(s,0,sizeof(*s));}
}
static inline int rv_load_stage(FILE *f,rv_load_staged *out){
    rv_load_staged s;
    int rc;
    if(!f||!out||out->payload)return RV_LOAD_STATE;
    memset(&s,0,sizeof(s));
    /* Read payload sectors exactly once into the private staging buffer.
       Metadata alone is never published as a successful load. */
    rc=rv_pf_bin_metadata(f,&s.meta);if(rc)return rc;
    s.payload_bytes=s.meta.text_bytes;
    s.payload=(unsigned char *)malloc(s.payload_bytes);
    if(!s.payload)return RV_LOAD_NO_MEMORY;
    for(uint32_t i=0;i<s.meta.text_bytes/2048u;i++){
        rc=rv_pf_sector(f,s.meta.exe_lba+1+i,s.meta.sectors,s.payload+i*2048u);
        if(rc){rv_load_discard(&s);return rc;}
    }
    *out=s;return RV_PF_OK;
}
static inline int rv_load_commit(const rv_load_staged *s,unsigned char *ram,size_t capacity,
    rv_pf_result *committed){
    uint32_t offset;
    if(!s||!s->payload||!committed||!ram||!s->payload_bytes||
        s->payload_bytes!=s->meta.text_bytes)return RV_LOAD_STATE;
    if(s->meta.text_addr<0x80000000u||s->meta.text_addr>=0x80200000u||
        (s->meta.text_addr&3u)||s->meta.text_bytes%2048u||
        s->meta.text_bytes>0x80200000u-s->meta.text_addr||
        s->meta.pc<s->meta.text_addr||s->meta.pc-s->meta.text_addr>=s->meta.text_bytes||
        (s->meta.pc&3u))return RV_LOAD_DESTINATION;
    offset=s->meta.text_addr-0x80000000u;
    if(offset>capacity||s->payload_bytes>capacity-offset)return RV_LOAD_DESTINATION;
    /* All potentially failing work has completed. Only the payload range
       changes, then publish metadata. CPU/cache setup is a separate step. */
    memcpy(ram+offset,s->payload,s->payload_bytes);
    *committed=s->meta;return RV_PF_OK;
}
#endif
