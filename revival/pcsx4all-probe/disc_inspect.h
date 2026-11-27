/* SPDX-License-Identifier: GPL-2.0-or-later */
#ifndef REVIVAL_DISC_INSPECT_H
#define REVIVAL_DISC_INSPECT_H
#include "disc_preflight.h"
/* Frontend only: read GAME.CUE and its validated local BIN basename.
 * No directory creation, media writes, plugin initialization or execution. */
enum rv_inspect_phase {RV_INSPECT_PATH,RV_INSPECT_CUE_OPEN,RV_INSPECT_CUE_PARSE,
    RV_INSPECT_BIN_OPEN,RV_INSPECT_BIN_PARSE,RV_INSPECT_COMPLETE};
typedef struct {
    enum rv_inspect_phase phase;
    int status;
    char bin_name[128];
    rv_pf_result meta;
} rv_inspect_result;
static inline int rv_inspect_path(char *out,size_t capacity,const char *dir,const char *name){
    int n;
    if(!out||!capacity||!dir||!*dir||!name||!*name)return 0;
    n=snprintf(out,capacity,"%s%s%s",dir,dir[strlen(dir)-1]=='/'?"":"/",name);
    return n>=0&&(size_t)n<capacity;
}
static inline rv_inspect_result rv_inspect_dir(const char *dir){
    rv_inspect_result r;
    char path[512];
    FILE *f;
    memset(&r,0,sizeof(r));r.status=RV_PF_IO;r.phase=RV_INSPECT_PATH;
    if(!rv_inspect_path(path,sizeof(path),dir,"GAME.CUE"))return r;
    r.phase=RV_INSPECT_CUE_OPEN;
    f=fopen(path,"rb");if(!f)return r;
    r.phase=RV_INSPECT_CUE_PARSE;
    r.status=rv_pf_cue(f,r.bin_name);
    if(fclose(f))r.status=RV_PF_IO;
    if(r.status!=RV_PF_OK)return r;
    r.status=RV_PF_IO;r.phase=RV_INSPECT_PATH;
    if(!rv_inspect_path(path,sizeof(path),dir,r.bin_name))return r;
    r.phase=RV_INSPECT_BIN_OPEN;
    f=fopen(path,"rb");if(!f)return r;
    r.phase=RV_INSPECT_BIN_PARSE;
    r.status=rv_pf_bin(f,&r.meta);
    if(fclose(f))r.status=RV_PF_IO;
    if(r.status!=RV_PF_OK){memset(&r.meta,0,sizeof(r.meta));return r;}
    r.phase=RV_INSPECT_COMPLETE;return r;
}
static inline const char *rv_inspect_phase_name(enum rv_inspect_phase p){
    switch(p){
        case RV_INSPECT_PATH:return "path";
        case RV_INSPECT_CUE_OPEN:return "cue_open";
        case RV_INSPECT_CUE_PARSE:return "cue_parse";
        case RV_INSPECT_BIN_OPEN:return "bin_open";
        case RV_INSPECT_BIN_PARSE:return "bin_parse";
        case RV_INSPECT_COMPLETE:return "complete";
    }
    return "unknown";
}
static inline const char *rv_inspect_status_name(int s){
    switch(s){
        case RV_PF_OK:return "accepted_profile";
        case RV_PF_IO:return "io_error";
        case RV_PF_PROFILE:return "unsupported_profile";
        case RV_PF_VOLUME:return "volume_check";
        case RV_PF_DIRECTORY:return "directory_check";
        case RV_PF_BOOT:return "boot_check";
        case RV_PF_EXE:return "exe_check";
    }
    return "unknown";
}
static inline int rv_inspect_ok(const rv_inspect_result *r){
    return r&&r->phase==RV_INSPECT_COMPLETE&&r->status==RV_PF_OK;
}
static inline void rv_inspect_print(FILE *f,const rv_inspect_result *r){
    fprintf(f,"PSX-P Revival disc inspection V16\nread_only=1\nguest_execution=0\n");
    fprintf(f,"phase=%s\nstatus=%s\ncode=%d\n",
        rv_inspect_phase_name(r->phase),rv_inspect_status_name(r->status),r->status);
    if(r->bin_name[0])fprintf(f,"bin=%s\n",r->bin_name);
    if(rv_inspect_ok(r)){
        fprintf(f,"boot=%s\nsectors=%lu\nvolume_sectors=%lu\nexe_lba=%lu\nexe_bytes=%lu\ntext_addr=%08lx\ntext_bytes=%lu\npc=%08lx\ngp=%08lx\nsp=%08lx\n",
            r->meta.boot_name,(unsigned long)r->meta.sectors,(unsigned long)r->meta.volume_sectors,
            (unsigned long)r->meta.exe_lba,
            (unsigned long)r->meta.exe_bytes,(unsigned long)r->meta.text_addr,
            (unsigned long)r->meta.text_bytes,(unsigned long)r->meta.pc,
            (unsigned long)r->meta.gp,(unsigned long)r->meta.sp);
    }
    fprintf(f,"result=%s\n",rv_inspect_ok(r)?"PROFILE_MATCH_NOT_EXECUTION":"NOT_ADMITTED_BY_CURRENT_PROFILE");
}
#endif
