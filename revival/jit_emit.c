/* SPDX-License-Identifier: GPL-2.0-or-later */
#include "jit_emit.h"
#include <string.h>
enum { A0=4, T0=8, T1=9, RA=31 };
static uint32_t r(unsigned rs,unsigned rt,unsigned rd,unsigned sa,unsigned fn) {
    return (rs<<21)|(rt<<16)|(rd<<11)|(sa<<6)|fn;
}
static uint32_t i(unsigned op,unsigned rs,unsigned rt,unsigned imm) {
    return (op<<26)|(rs<<21)|(rt<<16)|(imm&65535u);
}
static uint32_t load(unsigned host,unsigned guest) {
    return guest ? i(0x23,A0,host,guest*4u) : r(0,0,host,0,0x21);
}
size_t revival_jit_emit(const uint32_t *guest,size_t n,uint32_t *out,size_t cap) {
    uint32_t tmp[REVIVAL_JIT_MAX_WORDS];
    size_t j,k=0;
    if(!guest||!out||!n||n>REVIVAL_JIT_MAX_GUEST) return 0;
    for(j=0;j<n;j++) {
        uint32_t w=guest[j];
        unsigned op=w>>26, rs=(w>>21)&31u, rt=(w>>16)&31u,
            rd=(w>>11)&31u, sa=(w>>6)&31u, fn=w&63u;
        if(!op) {
            switch(fn) {
            case 0: case 2: case 3: /* SLL, SRL, SRA */
                if(rs) return 0;
                if(!rd) break;
                tmp[k++]=load(T0,rt);
                tmp[k++]=r(0,T0,T0,sa,fn);
                tmp[k++]=i(0x2b,A0,T0,rd*4u);
                break;
            case 4: case 6: case 7: /* variable shifts */
            case 0x21: case 0x23: /* ADDU/SUBU */
            case 0x24: case 0x25: case 0x26: case 0x27:
            case 0x2a: case 0x2b:
                if(sa) return 0;
                if(!rd) break;
                tmp[k++]=load(T0,rs);tmp[k++]=load(T1,rt);
                tmp[k++]=r(T0,T1,T0,0,fn);
                tmp[k++]=i(0x2b,A0,T0,rd*4u);
                break;
            default: return 0; /* traps, branches, HI/LO, etc. */
            }
        } else {
            switch(op) {
            case 9: case 10: case 11: case 12: case 13: case 14:
                if(!rt) break;
                tmp[k++]=load(T0,rs);
                tmp[k++]=i(op,T0,T0,w);
                tmp[k++]=i(0x2b,A0,T0,rt*4u);
                break;
            case 15:
                if(rs) return 0;
                if(!rt) break;
                tmp[k++]=i(15,0,T0,w);
                tmp[k++]=i(0x2b,A0,T0,rt*4u);
                break;
            default: return 0;
            }
        }
    }
    tmp[k++]=r(RA,0,0,0,8); /* JR ra */
    tmp[k++]=0; /* mandatory branch delay slot */
    if(k>cap) return 0;
    memcpy(out,tmp,k*sizeof(*out));
    return k;
}
