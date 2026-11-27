/* SPDX-License-Identifier: GPL-2.0-or-later */
#include "jit_emit.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static uint32_t enc_i(unsigned op,unsigned rs,unsigned rt,unsigned imm) {return (op<<26)|(rs<<21)|(rt<<16)|(imm&65535u);}
static uint32_t enc_r(unsigned rs,unsigned rt,unsigned rd,unsigned sa,unsigned fn) {return (rs<<21)|(rt<<16)|(rd<<11)|(sa<<6)|fn;}
/* Independent minimal MIPS host-word simulator for this emitter's output.
 * All memory accesses must stay within the provided guest register array.
 * This is not a full PS1 CPU model. */
static void simulate(const uint32_t *code,size_t n,uint32_t *g) {
    uint32_t h[32]={0};
    size_t j;
    for(j=0;j<n;j++) {
        uint32_t w=code[j], a=h[(w>>21)&31], b=h[(w>>16)&31], z=0;
        unsigned op=w>>26,rs=(w>>21)&31,rt=(w>>16)&31,rd=(w>>11)&31,sa=(w>>6)&31,fn=w&63;
        unsigned imm=w&65535u; int32_t si=(int16_t)imm;
        if(op==0x23||op==0x2b) {
            assert(rs==4 && !(imm&3) && imm>0 && imm<128);
            if(op==0x23) { assert((rt>=8 && rt<=15)||rt==24||rt==25);h[rt]=g[imm/4]; } else g[imm/4]=h[rt];
        } else if(!op) {
            switch(fn) {
            case 0:z=b<<sa;break;case 2:z=b>>sa;break;case 3:z=(uint32_t)((int32_t)b>>sa);break;
            case 4:z=b<<(a&31);break;case 6:z=b>>(a&31);break;case 7:z=(uint32_t)((int32_t)b>>(a&31));break;
            case 0x21:z=a+b;break;case 0x23:z=a-b;break;case 0x24:z=a&b;break;
            case 0x25:z=a|b;break;case 0x26:z=a^b;break;case 0x27:z=~(a|b);break;
            case 0x2a:z=(int32_t)a<(int32_t)b;break;case 0x2b:z=a<b;break;
            case 8:assert(rs==31 && j+2==n && code[j+1]==0);return;
            default:assert(0);
            }
            if(rd) { assert((rd>=8 && rd<=15)||rd==24||rd==25);h[rd]=z; }
        } else {
            switch(op) {
            case 9:z=a+(uint32_t)si;break;
            case 10:z=(int32_t)a<si;break;
            case 11:z=a<(uint32_t)si;break;
            case 12:z=a&imm;break;case 13:z=a|imm;break;case 14:z=a^imm;break;
            case 15:z=imm<<16;break;default:assert(0);
            }
            if(rt) { assert((rt>=8 && rt<=15)||rt==24||rt==25);h[rt]=z; }
        }
    }
    assert(0);
}
int main(void) {
    uint32_t code[REVIVAL_JIT_MAX_WORDS], g[32]={0}, guest[32];
    size_t n,j;
    guest[0]=enc_i(9,0,1,0xffff); /* ADDIU r1,r0,-1 */
    guest[1]=enc_i(15,0,2,0x8000);
    guest[2]=enc_i(13,2,2,0x1234);
    guest[3]=enc_r(1,2,3,0,0x21);
    guest[4]=enc_r(2,1,4,0,0x23);
    guest[5]=enc_r(0,2,5,4,3);
    guest[6]=enc_r(1,2,6,0,6);
    guest[7]=enc_r(1,2,7,0,0x2a);
    guest[8]=enc_r(1,2,8,0,0x2b);
    guest[9]=enc_i(11,2,9,0xffff);
    guest[10]=enc_i(10,2,10,0);
    guest[11]=enc_i(12,2,11,0xffff);
    guest[12]=enc_i(14,2,12,0xffff);
    guest[13]=enc_r(1,2,13,0,0x24);
    guest[14]=enc_r(1,2,14,0,0x25);
    guest[15]=enc_r(1,2,15,0,0x26);
    guest[16]=enc_r(0,2,16,0,0x27);
    guest[17]=enc_r(0,1,17,31,0);
    guest[18]=enc_r(0,2,18,31,2);
    guest[19]=enc_r(1,2,19,0,4);
    guest[20]=enc_r(1,2,20,0,7);
    guest[21]=enc_i(9,1,0,1);
    g[0]=0xfeedface;g[31]=0x12345678;
    n=revival_jit_emit(guest,22,code,REVIVAL_JIT_MAX_WORDS);assert(n);
    simulate(code,n,g);
    assert(g[0]==0xfeedface && g[31]==0x12345678);
    assert(g[1]==0xffffffff && g[2]==0x80001234 && g[3]==0x80001233 && g[4]==0x80001235);
    assert(g[5]==0xf8000123 && g[6]==1 && g[7]==0 && g[8]==0 && g[9]==1 && g[10]==1);
    assert(g[11]==0x1234 && g[12]==0x8000edcb && g[13]==0x80001234 && g[14]==0xffffffff);
    assert(g[15]==0x7fffedcb && g[16]==0x7fffedcb && g[17]==0x80000000 && g[18]==1 && g[19]==0 && g[20]==0xffffffff);
    for(j=0;j<REVIVAL_JIT_MAX_GUEST;j++)guest[j]=enc_r(1,2,3,0,0x21);
    assert(revival_jit_emit(guest,REVIVAL_JIT_MAX_GUEST,code,REVIVAL_JIT_MAX_WORDS)==REVIVAL_JIT_MAX_WORDS);
    code[0]=0xdeadbeef;
    assert(!revival_jit_emit(guest,REVIVAL_JIT_MAX_GUEST,code,REVIVAL_JIT_MAX_WORDS-1) && code[0]==0xdeadbeef);
    guest[1]=0x08000000; /* J unsupported */
    assert(!revival_jit_emit(guest,2,code,REVIVAL_JIT_MAX_WORDS) && code[0]==0xdeadbeef);
    guest[0]=0x8c010000;assert(!revival_jit_emit(guest,1,code,REVIVAL_JIT_MAX_WORDS));
    guest[0]=0x20210001;assert(!revival_jit_emit(guest,1,code,REVIVAL_JIT_MAX_WORDS)); /* ADDI traps */
    assert(!revival_jit_emit(guest,0,code,REVIVAL_JIT_MAX_WORDS));
    assert(!revival_jit_emit(guest,33,code,REVIVAL_JIT_MAX_WORDS));

    /* Repeated guest writes stay in one host register. Zero is never read
     * from the guest array. 32 ADDIU instructions need one load, one final
     * store, 32 ALU instructions and the two return words. */
    for(j=0;j<32;j++) guest[j]=enc_i(9,1,1,1);
    n=revival_jit_emit_regs(guest,32,code,REVIVAL_JIT_MAX_WORDS);
    assert(n==36);
    g[1]=7;simulate(code,n,g);assert(g[1]==39);
    code[0]=0xdeadbeef;
    assert(!revival_jit_emit_regs(guest,32,code,35) && code[0]==0xdeadbeef);
    guest[31]=0x08000000;
    assert(!revival_jit_emit_regs(guest,32,code,REVIVAL_JIT_MAX_WORDS) && code[0]==0xdeadbeef);
    /* Seeded differential corpus: all supported opcode classes, aliasing,
     * zero, saturation of the eight cached registers, and boundary values. */
    {
        static const unsigned fn[]={0,2,3,4,6,7,0x21,0x23,0x24,0x25,0x26,0x27,0x2a,0x2b};
        uint32_t seed=0x65fe0021u, a[32],b[32],old[REVIVAL_JIT_MAX_WORDS];
        unsigned trial,t;
        for(trial=0;trial<4096;trial++) {
            size_t len=1+trial%32, oldn;
            for(t=0;t<32;t++) { seed=1664525u*seed+1013904223u;a[t]=seed;b[t]=seed; }
            for(t=0;t<len;t++) {
                unsigned rs,rt,rd,kind,shift;
                seed=1664525u*seed+1013904223u;rs=(seed>>16)&31u;rt=(seed>>21)&31u;rd=(seed>>26)&31u;
                seed=1664525u*seed+1013904223u;kind=(seed>>16)%21u;shift=(seed>>24)&31u;
                if(kind<14) guest[t]=enc_r(kind<3?0:rs,rt,rd,kind<3?shift:0,fn[kind]);
                else guest[t]=enc_i(kind+9-14,kind==20?0:rs,rt,seed);
            }
            oldn=revival_jit_emit(guest,len,old,REVIVAL_JIT_MAX_WORDS);
            n=revival_jit_emit_regs(guest,len,code,REVIVAL_JIT_MAX_WORDS);
            assert(oldn && n && n<=oldn);
            simulate(old,oldn,a);simulate(code,n,b);
            assert(!memcmp(a,b,sizeof(a)));
        }
    }
    puts("Allegrex emitter tests passed");return 0;
}
