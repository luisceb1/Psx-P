/* SPDX-License-Identifier: GPL-2.0-or-later */
#ifndef REVIVAL_DISC_SESSION_H
#define REVIVAL_DISC_SESSION_H
/* Single-threaded ownership for a CD plugin whose shutdown closes media
 * and releases plugin-owned caches. No guest execution or file selection. */
typedef enum {
    RV_DISC_IDLE=0, RV_DISC_INITIALIZED, RV_DISC_OPEN
} rv_disc_state;
typedef struct {
    rv_disc_state state;
} rv_disc_session;
typedef struct {
    long (*init)(void);
    long (*open)(void);
    long (*shutdown)(void);
} rv_disc_ops;
enum { RV_DISC_BAD_STATE=-2 };
static inline long rv_disc_end(rv_disc_session *s,const rv_disc_ops *ops){
    if(!s||!ops||!ops->shutdown)return RV_DISC_BAD_STATE;
    if(s->state==RV_DISC_IDLE)return 0;
    /* A failed shutdown prevents reopening until cleanup succeeds. */
    s->state=RV_DISC_INITIALIZED;
    long rc=ops->shutdown();
    if(rc==0)s->state=RV_DISC_IDLE;
    return rc;
}
static inline long rv_disc_begin(rv_disc_session *s,const rv_disc_ops *ops){
    if(!s||!ops||!ops->init||!ops->open||!ops->shutdown||s->state!=RV_DISC_IDLE)
        return RV_DISC_BAD_STATE;
    long rc=ops->init();
    if(rc!=0)return rc;
    s->state=RV_DISC_INITIALIZED;
    rc=ops->open();
    if(rc!=0){
        /* Shutdown owns cleanup, including a partially opened disc. */
        rv_disc_end(s,ops);
        return rc;
    }
    s->state=RV_DISC_OPEN;
    return 0;
}
#endif
