/* SPDX-License-Identifier: GPL-2.0-or-later */
#include "core_session.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static unsigned owned,used;
static int fail_start=-1,fail_stop=-1;
static char trace[64];
static void record(char c){assert(used+1<sizeof(trace));trace[used++]=c;trace[used]=0;}
static long start(unsigned i){
    record((char)('A'+i));assert(owned==((1u<<i)-1));
    if(fail_start==(int)i)return -10-(long)i;
    owned|=1u<<i;return 0;
}
static long stop(unsigned i){
    record((char)('a'+i));assert(owned==((1u<<(i+1))-1));
    if(fail_stop==(int)i)return -20-(long)i;
    owned&=~(1u<<i);return 0;
}
static long a(void){return start(0);}static long b(void){return start(1);}static long c(void){return start(2);}
static long x(void){return stop(0);}static long y(void){return stop(1);}static long z(void){return stop(2);}
static const rv_core_stage ops[RV_CORE_STAGES]={{a,x},{b,y},{c,z}};
static void clear(void){used=0;trace[0]=0;}
int main(void){
    rv_core_session s={0};
    assert(!rv_core_end(&s,ops)&&!used);
    for(unsigned i=0;i<3;i++){
        assert(!rv_core_begin(&s,ops)&&s.active==7&&owned==7);
        assert(rv_core_begin(&s,ops)==RV_CORE_BAD_STATE);
        assert(!rv_core_end(&s,ops)&&!s.active&&!owned);
        assert(!rv_core_end(&s,ops));
    }
    assert(!strcmp(trace,"ABCcbaABCcbaABCcba"));
    const char *failed[]={"A","ABa","ABCba"};
    for(int i=0;i<3;i++){
        clear();fail_start=i;
        assert(rv_core_begin(&s,ops)==-10-i&&!owned&&!s.active);
        assert(!strcmp(trace,failed[i]));
    }
    fail_start=-1;clear();
    assert(!rv_core_begin(&s,ops));fail_stop=1;
    assert(rv_core_end(&s,ops)==-21&&s.active==3&&owned==3);
    assert(rv_core_begin(&s,ops)==RV_CORE_BAD_STATE&&!strcmp(trace,"ABCcb"));
    fail_stop=-1;
    assert(!rv_core_end(&s,ops)&&!s.active&&!owned&&!strcmp(trace,"ABCcbba"));
    clear();fail_start=2;fail_stop=1;
    assert(rv_core_begin(&s,ops)==-12&&s.active==3&&owned==3&&!strcmp(trace,"ABCb"));
    fail_stop=-1;
    assert(!rv_core_end(&s,ops)&&!s.active&&!owned&&!strcmp(trace,"ABCbba"));
    puts("core session: PASS (ordered lifecycle, repeated cycles, rollback and teardown retry)");
    return 0;
}
