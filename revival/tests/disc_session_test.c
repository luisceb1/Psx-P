/* SPDX-License-Identifier: GPL-2.0-or-later */
#include "disc_session.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static char trace[64];
static unsigned used;
static long init_rc,open_rc,shutdown_rc;
static int allocated,opened;
static void record(char c){assert(used+1<sizeof(trace));trace[used++]=c;trace[used]=0;}
static long mock_init(void){
    record('I');assert(!allocated&&!opened);
    if(!init_rc)allocated=1;
    return init_rc;
}
static long mock_open(void){
    record('O');assert(allocated&&!opened);
    /* Model a partially acquired handle even on failed open. */
    opened=1;return open_rc;
}
static long mock_shutdown(void){
    record('S');assert(allocated);
    if(!shutdown_rc){opened=0;allocated=0;}
    return shutdown_rc;
}
static void reset_trace(void){used=0;trace[0]=0;}
int main(void){
    rv_disc_session s={RV_DISC_IDLE};
    const rv_disc_ops ops={mock_init,mock_open,mock_shutdown};
    assert(rv_disc_end(&s,&ops)==0&&!used);
    assert(rv_disc_begin(&s,&ops)==0&&s.state==RV_DISC_OPEN);
    assert(rv_disc_begin(&s,&ops)==RV_DISC_BAD_STATE&&!strcmp(trace,"IO"));
    assert(rv_disc_end(&s,&ops)==0&&s.state==RV_DISC_IDLE);
    assert(rv_disc_end(&s,&ops)==0&&!strcmp(trace,"IOS")&&!allocated&&!opened);
    reset_trace();
    for(unsigned i=0;i<3;i++){
        assert(rv_disc_begin(&s,&ops)==0&&rv_disc_end(&s,&ops)==0);
    }
    assert(!strcmp(trace,"IOSIOSIOS"));
    reset_trace();init_rc=-1;
    assert(rv_disc_begin(&s,&ops)==-1&&s.state==RV_DISC_IDLE&&!strcmp(trace,"I"));
    init_rc=0;open_rc=-3;reset_trace();
    assert(rv_disc_begin(&s,&ops)==-3&&s.state==RV_DISC_IDLE&&!strcmp(trace,"IOS"));
    assert(!allocated&&!opened);
    reset_trace();shutdown_rc=-4;
    assert(rv_disc_begin(&s,&ops)==-3&&s.state==RV_DISC_INITIALIZED&&!strcmp(trace,"IOS"));
    assert(rv_disc_begin(&s,&ops)==RV_DISC_BAD_STATE&&!strcmp(trace,"IOS"));
    shutdown_rc=0;open_rc=0;
    assert(rv_disc_end(&s,&ops)==0&&s.state==RV_DISC_IDLE&&!allocated&&!opened);
    assert(rv_disc_begin(&s,&ops)==0&&rv_disc_end(&s,&ops)==0);
    assert(!strcmp(trace,"IOSSIOS"));
    puts("disc session: PASS (ordered ownership, repeat, init/open failure, shutdown retry)");
    return 0;
}
