/* SPDX-License-Identifier: GPL-2.0-or-later */
#ifndef REVIVAL_CORE_SESSION_H
#define REVIVAL_CORE_SESSION_H
/* Single-threaded lifecycle for CPU, SPU, GPU, in that order.
 * Starts own their partial-failure cleanup. A failed stop retains ownership
 * and prevents lower-level teardown until a successful retry. */
typedef struct { unsigned active; } rv_core_session;
typedef struct { long (*start)(void); long (*stop)(void); } rv_core_stage;
enum { RV_CORE_STAGES=3, RV_CORE_BAD_STATE=-2 };
static inline long rv_core_end(rv_core_session *s,const rv_core_stage stages[RV_CORE_STAGES]){
    if(!s||!stages)return RV_CORE_BAD_STATE;
    for(unsigned i=RV_CORE_STAGES;i>0;i--){
        unsigned bit=1u<<(i-1);
        if(!(s->active&bit))continue;
        if(!stages[i-1].stop)return RV_CORE_BAD_STATE;
        long rc=stages[i-1].stop();
        if(rc)return rc;
        s->active&=~bit;
    }
    return 0;
}
static inline long rv_core_begin(rv_core_session *s,const rv_core_stage stages[RV_CORE_STAGES]){
    if(!s||!stages||s->active)return RV_CORE_BAD_STATE;
    for(unsigned i=0;i<RV_CORE_STAGES;i++)
        if(!stages[i].start||!stages[i].stop)return RV_CORE_BAD_STATE;
    for(unsigned i=0;i<RV_CORE_STAGES;i++){
        long rc=stages[i].start();
        if(rc){
            rv_core_end(s,stages);
            return rc;
        }
        s->active|=1u<<i;
    }
    return 0;
}
#endif
