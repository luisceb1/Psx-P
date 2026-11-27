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
int revival_jit_supported(uint32_t w) {
    unsigned op=w>>26, rs=(w>>21)&31u, sa=(w>>6)&31u, fn=w&63u;
    if(!op) {
        switch(fn) {
        case 0:case 2:case 3:return !rs;
        case 4:case 6:case 7:case 0x21:case 0x23:case 0x24:
        case 0x25:case 0x26:case 0x27:case 0x2a:case 0x2b:return !sa;
        default:return 0;
        }
    }
    return (op>=9 && op<=14) || (op==15 && !rs);
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

/* Block-local lazy register cache. Only caller-saved temporaries are used;
 * A0 (GPR pointer), RA, stack, GP and all callee-saved registers are intact.
 * No external calls or guest memory access. Unmapped operands use T0/T1.
 * Allocate on first touch; when the eight slots fill, use the old spill path.
 * This intentionally avoids eviction complexity and keeps the old word bound. */
struct reg_emit {
    uint32_t words[REVIVAL_JIT_MAX_WORDS];
    unsigned map[32], used, loaded, dirty;
    size_t count;
};
static unsigned reg_host(struct reg_emit *e,unsigned guest) {
    static const unsigned host[]={10,11,12,13,14,15,24,25};
    if(!guest) return 0;
    if(!e->map[guest] && e->used<sizeof(host)/sizeof(host[0]))
        e->map[guest]=host[e->used++];
    return e->map[guest];
}
static unsigned reg_read(struct reg_emit *e,unsigned guest,unsigned scratch) {
    unsigned h=reg_host(e,guest), bit=1u<<guest;
    if(!guest) return 0;
    if(!h) { e->words[e->count++]=load(scratch,guest);return scratch; }
    if(!(e->loaded&bit)) { e->words[e->count++]=load(h,guest);e->loaded|=bit; }
    return h;
}
static unsigned reg_write(struct reg_emit *e,unsigned guest) {
    unsigned h=reg_host(e,guest);
    if(!h) return T0;
    e->loaded|=1u<<guest;
    e->dirty|=1u<<guest;
    return h;
}
size_t revival_jit_emit_regs(const uint32_t *guest,size_t n,uint32_t *out,size_t cap) {
    struct reg_emit e;
    size_t j;
    unsigned g;
    if(!guest||!out||!n||n>REVIVAL_JIT_MAX_GUEST) return 0;
    memset(&e,0,sizeof(e));
    for(j=0;j<n;j++) {
        uint32_t w=guest[j];
        unsigned op=w>>26,rs=(w>>21)&31u,rt=(w>>16)&31u,
            rd=(w>>11)&31u,sa=(w>>6)&31u,fn=w&63u;
        unsigned dst=op?rt:rd,a=0,b=0,d;
        if(!revival_jit_supported(w)) return 0;
        if(!dst) continue;
        /* Read both sources before marking a destination written, including
         * when guest destination aliases either source. */
        if(!op) {
            if(fn!=0 && fn!=2 && fn!=3) a=reg_read(&e,rs,T0);
            b=reg_read(&e,rt,T1);
            d=reg_write(&e,dst);
            e.words[e.count++]=r(a,b,d,sa,fn);
        } else {
            if(op!=15) a=reg_read(&e,rs,T0);
            d=reg_write(&e,dst);
            e.words[e.count++]=i(op,a,d,w);
        }
        if(!e.map[dst]) e.words[e.count++]=i(0x2b,A0,d,dst*4u);
    }
    for(g=1;g<32;g++)
        if(e.dirty&(1u<<g)) e.words[e.count++]=i(0x2b,A0,e.map[g],g*4u);
    e.words[e.count++]=r(RA,0,0,0,8);
    e.words[e.count++]=0;
    /* At most two loads, one ALU and one eventual store per guest op.
     * Each final writeback replaces an earlier omitted store. */
    if(e.count>cap) return 0;
    memcpy(out,e.words,e.count*sizeof(*out));
    return e.count;
}
