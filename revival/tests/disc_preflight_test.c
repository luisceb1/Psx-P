/* SPDX-License-Identifier: GPL-2.0-or-later */
#include "disc_inspect.h"
#include "disc_load.h"
#include <assert.h>
static unsigned char disc[32][2352];
static unsigned char *payload(unsigned n){return disc[n]+24;}
static void le32(unsigned char *p,uint32_t v){for(unsigned i=0;i<4;i++)p[i]=(unsigned char)(v>>(8*i));}
static void both32(unsigned char *p,uint32_t v){le32(p,v);for(unsigned i=0;i<4;i++)p[4+i]=(unsigned char)(v>>(8*(3-i)));}
static unsigned entry(unsigned char *p,unsigned lba,unsigned bytes,unsigned flags,const char *name,unsigned n){
    unsigned len=33+n+!(n&1);
    memset(p,0,len);p[0]=(unsigned char)len;both32(p+2,lba);both32(p+10,bytes);
    p[25]=(unsigned char)flags;p[28]=1;p[31]=1;p[32]=(unsigned char)n;memcpy(p+33,name,n);
    return len;
}
static void fixture(void){
    memset(disc,0,sizeof(disc));
    for(unsigned i=0;i<32;i++){
        memset(disc[i]+1,255,10);disc[i][15]=2;disc[i][18]=disc[i][22]=8;
    }
    unsigned char *p=payload(16);p[0]=1;memcpy(p+1,"CD001",5);p[6]=1;
    both32(p+80,32);p[129]=p[130]=8;entry(p+156,20,4096,2,"\0",1);
    p=payload(17);p[0]=255;memcpy(p+1,"CD001",5);p[6]=1;
    p=payload(20);unsigned off=0;off+=entry(p+off,20,4096,2,"\0",1);
    off+=entry(p+off,20,4096,2,"\1",1);
    off+=entry(p+off,22,32,0,"SYSTEM.CNF;1",12);
    entry(p+off,23,4096,0,"PSX.EXE;1",9);
    const char cnf[]="BOOT = cdrom:\\PSX.EXE;1\r\n";
    memcpy(payload(22),cnf,sizeof(cnf));
    p=payload(23);memcpy(p,"PS-X EXE",8);le32(p+0x10,0x80010000);
    le32(p+0x18,0x80010000);le32(p+0x1c,2048);le32(p+0x30,0x801fff00);
}
static int cue(const char *s,char name[128]){
    FILE *f=tmpfile();assert(f);assert(fwrite(s,1,strlen(s),f)==strlen(s));rewind(f);
    int rc=rv_pf_cue(f,name);fclose(f);return rc;
}
static int image(size_t bytes,rv_pf_result *r){
    FILE *f=tmpfile();assert(f);assert(fwrite(disc,1,bytes,f)==bytes);rewind(f);
    int rc=rv_pf_bin(f,r);fclose(f);return rc;
}
static unsigned char guest_ram[0x200000];
static void loader_test(void){
    fixture();
    /* Two payload sectors with distinct original synthetic data. */
    unsigned char *p=payload(20);
    unsigned off=0;off+=entry(p+off,20,4096,2,"\0",1);
    off+=entry(p+off,20,4096,2,"\1",1);
    off+=entry(p+off,22,32,0,"SYSTEM.CNF;1",12);
    entry(p+off,23,6144,0,"PSX.EXE;1",9);
    le32(payload(23)+0x1c,4096);
    memset(payload(24),0x36,2048);memset(payload(25),0x79,2048);
    FILE *f=tmpfile();assert(f);assert(fwrite(disc,1,sizeof(disc),f)==sizeof(disc));rewind(f);
    rv_load_staged s;memset(&s,0,sizeof(s));
    memset(guest_ram,0xa5,sizeof(guest_ram));
    assert(rv_load_stage(f,&s)==RV_PF_OK);
    assert(!fclose(f));
    assert(s.payload_bytes==4096&&s.meta.pc==0x80010000);
    rv_pf_result m;memset(&m,0,sizeof(m));rv_pf_result before=m;
    assert(rv_load_commit(&s,guest_ram,0x10000,&m)==RV_LOAD_DESTINATION);
    assert(!memcmp(&m,&before,sizeof(m)));
    for(size_t i=0;i<sizeof(guest_ram);i++)assert(guest_ram[i]==0xa5);
    assert(rv_load_commit(&s,guest_ram,sizeof(guest_ram),&m)==RV_PF_OK);
    assert(m.text_bytes==4096&&m.sp==0x801fff00);
    assert(!memcmp(guest_ram+0x10000,payload(24),2048));
    assert(!memcmp(guest_ram+0x10800,payload(25),2048));
    for(size_t i=0;i<sizeof(guest_ram);i++)
        if(i<0x10000||i>=0x11000)assert(guest_ram[i]==0xa5);
    rv_load_discard(&s);assert(!s.payload&&!s.payload_bytes);
    f=tmpfile();assert(f);assert(fwrite(disc,1,sizeof(disc)-1,f)==sizeof(disc)-1);rewind(f);
    assert(rv_load_stage(f,&s)==RV_PF_PROFILE&&!s.payload&&!s.payload_bytes);
    fclose(f);
    assert(!memcmp(guest_ram+0x10000,payload(24),2048)&&!memcmp(guest_ram+0x10800,payload(25),2048));
    puts("transactional loader: PASS (two sectors, destination rejection, exact payload, guards, truncated read)");
}
int main(int argc,char **argv){
    char name[128]="unchanged";
    assert(cue("FILE \"Fixture.bin\" BINARY\r\nTRACK 01 MODE2/2352\r\nINDEX 01 00:00:00\r\n",name)==RV_PF_OK);
    assert(!strcmp(name,"Fixture.bin"));
    assert(cue("FILE \"Fixture.bin\" BINARY\nTRACK 01 AUDIO\nINDEX 01 00:00:00\n",name)==RV_PF_PROFILE);
    assert(!strcmp(name,"Fixture.bin"));
    fixture();
    rv_pf_result r;memset(&r,0,sizeof(r));
    assert(image(sizeof(disc),&r)==RV_PF_OK);
    assert(r.sectors==32&&r.exe_lba==23&&r.exe_bytes==4096&&r.pc==0x80010000&&r.text_bytes==2048
        &&r.text_addr==0x80010000&&r.sp==0x801fff00&&!strcmp(r.boot_name,"PSX.EXE;1"));
    rv_pf_result saved=r;
    assert(image(sizeof(disc)-1,&r)==RV_PF_PROFILE&&!memcmp(&r,&saved,sizeof(r)));
    fixture();payload(16)[0]=0;
    assert(image(sizeof(disc),&r)==RV_PF_VOLUME&&!memcmp(&r,&saved,sizeof(r)));
    fixture();memset(payload(23),0,2048);
    assert(image(sizeof(disc),&r)==RV_PF_EXE&&!memcmp(&r,&saved,sizeof(r)));
    loader_test();
    if(argc==2){
        char path[512];
        rv_inspect_result result=rv_inspect_dir(argv[1]);
        assert(result.phase==RV_INSPECT_CUE_OPEN&&!rv_inspect_ok(&result));
        assert(rv_inspect_path(path,sizeof(path),argv[1],"GAME.CUE"));
        FILE *f=fopen(path,"wb");assert(f);
        const char text[]="FILE \"Fixture.bin\" BINARY\nTRACK 01 MODE2/2352\nINDEX 01 00:00:00\n";
        assert(fwrite(text,1,sizeof(text)-1,f)==sizeof(text)-1);assert(!fclose(f));
        result=rv_inspect_dir(argv[1]);
        assert(result.phase==RV_INSPECT_BIN_OPEN&&!rv_inspect_ok(&result));
        fixture();
        assert(rv_inspect_path(path,sizeof(path),argv[1],"Fixture.bin"));
        f=fopen(path,"wb");assert(f);
        assert(fwrite(disc,1,sizeof(disc),f)==sizeof(disc));assert(!fclose(f));
        result=rv_inspect_dir(argv[1]);
        assert(rv_inspect_ok(&result)&&result.meta.pc==0x80010000&&!strcmp(result.meta.boot_name,"PSX.EXE;1"));
        puts("disc inspector frontend: PASS (missing CUE, missing BIN, external fixture)");
    }
    puts("disc preflight: PASS (CUE, ISO9660, boot, EXE, read-only failure)");
    return 0;
}
