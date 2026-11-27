/* SPDX-License-Identifier: GPL-2.0-or-later */
#ifndef REVIVAL_DISC_PREFLIGHT_H
#define REVIVAL_DISC_PREFLIGHT_H
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stddef.h>
/* Read-only, deliberately narrow admission profile. Not a guest loader.
 * Callers own FILE handles. Never selects paths or mutates emulator state. */
enum rv_pf_status { RV_PF_OK, RV_PF_IO, RV_PF_PROFILE, RV_PF_VOLUME, RV_PF_DIRECTORY, RV_PF_BOOT, RV_PF_EXE };
typedef struct {
    uint32_t sectors,exe_lba,exe_bytes,pc,text_addr,text_bytes,gp,sp;
    char boot_name[32];
    uint32_t volume_sectors; /* logical ISO9660 extent, not raw BIN length */
} rv_pf_result;
typedef struct {uint32_t lba,bytes;} rv_pf_extent;
static inline unsigned rv_pf_upper(unsigned c){return c>='a'&&c<='z'?c-32:c;}
static inline int rv_pf_eq(const char *a,size_t n,const char *b){
    size_t i=0;
    for(;i<n&&b[i];i++)if(rv_pf_upper((unsigned char)a[i])!=rv_pf_upper((unsigned char)b[i]))return 0;
    return i==n&&b[i]==0;
}
static inline int rv_pf_space(unsigned c){return c==' '||c=='\t'||c=='\r';}
/* Match exactly three whitespace-delimited CUE tokens. */
static inline int rv_pf_cue_words(const char *buf,size_t a,size_t b,
    const char *first,const char *second,const char *third){
    const char *expected[3]={first,second,third};
    for(unsigned i=0;i<3;i++){
        size_t end=a;
        while(end<b&&!rv_pf_space((unsigned char)buf[end]))end++;
        if(!rv_pf_eq(buf+a,end-a,expected[i]))return 0;
        a=end;while(a<b&&rv_pf_space((unsigned char)buf[a]))a++;
    }
    return a==b;
}
static inline int rv_pf_cue(FILE *f,char name[128]){
    char buf[1025],temp[128];size_t n,pos=0;unsigned stage=0;
    if(!f||!name)return RV_PF_IO;
    n=fread(buf,1,sizeof(buf),f);
    if(ferror(f))return RV_PF_IO;
    if(n==sizeof(buf)||memchr(buf,0,n))return RV_PF_PROFILE;
    if(n>=3&&(unsigned char)buf[0]==0xef&&(unsigned char)buf[1]==0xbb&&
        (unsigned char)buf[2]==0xbf)pos=3;
    while(pos<n){
        size_t a=pos,b,q;
        while(pos<n&&buf[pos]!='\n')pos++;
        b=pos;if(pos<n)pos++;
        while(a<b&&rv_pf_space((unsigned char)buf[a]))a++;
        while(b>a&&rv_pf_space((unsigned char)buf[b-1]))b--;
        if(a==b)continue;
        /* REM is non-semantic metadata; all track/index lines stay strict. */
        if(b-a>=3&&rv_pf_eq(buf+a,3,"REM")&&
            (b-a==3||rv_pf_space((unsigned char)buf[a+3])))continue;
        if(stage==0){
            if(b-a<13||!rv_pf_eq(buf+a,4,"FILE")||!rv_pf_space((unsigned char)buf[a+4]))return RV_PF_PROFILE;
            a+=5;while(a<b&&rv_pf_space((unsigned char)buf[a]))a++;
            if(a==b||buf[a++]!='"')return RV_PF_PROFILE;
            q=a;while(q<b&&buf[q]!='"')q++;
            if(q==b||q-a<5||q-a>=sizeof(temp)||buf[a]=='.')return RV_PF_PROFILE;
            for(size_t i=a;i<q;i++){
                unsigned c=(unsigned char)buf[i];
                if(!((c>='A'&&c<='Z')||(c>='a'&&c<='z')||(c>='0'&&c<='9')||
                    c==' '||c=='_'||c=='-'||c=='.'||c=='('||c==')'||
                    c=='['||c==']'||c==','||c=='\''||c=='!'||c=='&'||c=='+'))return RV_PF_PROFILE;
            }
            if(!rv_pf_eq(buf+q-4,4,".BIN"))return RV_PF_PROFILE;
            memcpy(temp,buf+a,q-a);temp[q-a]=0;
            a=q+1;if(a==b||!rv_pf_space((unsigned char)buf[a]))return RV_PF_PROFILE;
            while(a<b&&rv_pf_space((unsigned char)buf[a]))a++;
            if(!rv_pf_eq(buf+a,b-a,"BINARY"))return RV_PF_PROFILE;
        }else if(stage==1){
            if(!rv_pf_cue_words(buf,a,b,"TRACK","01","MODE2/2352"))return RV_PF_PROFILE;
        }else if(stage==2){
            if(!rv_pf_cue_words(buf,a,b,"INDEX","01","00:00:00"))return RV_PF_PROFILE;
        }else return RV_PF_PROFILE;
        stage++;
    }
    if(stage!=3)return RV_PF_PROFILE;
    strcpy(name,temp);return RV_PF_OK;
}
static inline uint32_t rv_pf_le32(const unsigned char *p){
    return (uint32_t)p[0]|((uint32_t)p[1]<<8)|((uint32_t)p[2]<<16)|((uint32_t)p[3]<<24);
}
static inline uint32_t rv_pf_be32(const unsigned char *p){
    return ((uint32_t)p[0]<<24)|((uint32_t)p[1]<<16)|((uint32_t)p[2]<<8)|p[3];
}
static inline int rv_pf_both32(const unsigned char *p,uint32_t *v){
    uint32_t a=rv_pf_le32(p);if(a!=rv_pf_be32(p+4))return 0;*v=a;return 1;
}
static inline int rv_pf_extent_ok(rv_pf_extent e,uint32_t volume){
    uint32_t n=e.bytes/2048u+(e.bytes%2048u!=0);
    return e.bytes&&e.lba<volume&&n<=volume-e.lba;
}
static inline int rv_pf_sector(FILE *f,uint32_t lba,uint32_t sectors,unsigned char out[2048]){
    unsigned char raw[2352];
    static const unsigned char sync[12]={0,255,255,255,255,255,255,255,255,255,255,0};
    if(lba>=sectors)return RV_PF_VOLUME;
    if(fseek(f,(long)lba*2352L,SEEK_SET)||fread(raw,1,sizeof(raw),f)!=sizeof(raw))return RV_PF_IO;
    if(memcmp(raw,sync,sizeof(sync))||raw[15]!=2||memcmp(raw+16,raw+20,4)||(raw[18]&0x20))
        return RV_PF_PROFILE;
    memcpy(out,raw+24,2048);return RV_PF_OK;
}
/* Found: 1, absent: 0, unsupported/malformed: -1. Root-only profile. */
static inline int rv_pf_find(FILE *f,uint32_t sectors,rv_pf_extent root,const char *name,
    rv_pf_extent *out){
    unsigned char sec[2048];unsigned found=0;
    for(uint32_t off=0;off<root.bytes;off+=2048){
        if(rv_pf_sector(f,root.lba+off/2048,sectors,sec)!=RV_PF_OK)return -1;
        for(unsigned i=0;i<2048;){
            unsigned len=sec[i];if(!len)break;
            if(len<34||len>2048-i)return -1;
            const unsigned char *p=sec+i;unsigned names=p[32];
            if(!names||33u+names>len||p[1]!=0)return -1;
            if(rv_pf_eq((const char *)p+33,names,name)){
                rv_pf_extent e;
                if(found++||p[25]!=0||p[26]!=0||p[27]!=0||
                    !rv_pf_both32(p+2,&e.lba)||!rv_pf_both32(p+10,&e.bytes)||!rv_pf_extent_ok(e,sectors))
                    return -1;
                *out=e;
            }
            i+=len;
        }
    }
    return found?1:0;
}
static inline int rv_pf_boot_name(const unsigned char *buf,size_t n,char name[32]){
    size_t pos=0;unsigned found=0;char temp[32];
    while(pos<n&&buf[pos]){
        size_t a=pos,b,k;
        while(pos<n&&buf[pos]&&buf[pos]!='\n')pos++;
        b=pos;if(pos<n&&buf[pos]=='\n')pos++;
        while(a<b&&rv_pf_space(buf[a]))a++;
        while(b>a&&rv_pf_space(buf[b-1]))b--;
        if(a==b)continue;
        k=a;while(k<b&&buf[k]!='='&&!rv_pf_space(buf[k]))k++;
        if(!rv_pf_eq((const char *)buf+a,k-a,"BOOT"))continue;
        if(found++)return 0;
        a=k;while(a<b&&rv_pf_space(buf[a]))a++;
        if(a==b||buf[a++]!='=')return 0;
        while(a<b&&rv_pf_space(buf[a]))a++;
        if(b-a<7||!rv_pf_eq((const char *)buf+a,6,"CDROM:"))return 0;
        a+=6;if(a<b&&(buf[a]=='\\'||buf[a]=='/'))a++;
        /* A root filename without an explicit version resolves only to ;1.
           Other explicit versions and directory components stay unsupported. */
        size_t end=b;
        if(b-a>=2&&buf[b-2]==';'&&buf[b-1]=='1')end=b-2;
        if(end==a||end-a+2>=sizeof(temp))return 0;
        for(size_t i=a;i<end;i++){
            unsigned c=rv_pf_upper(buf[i]);
            if(!((c>='A'&&c<='Z')||(c>='0'&&c<='9')||c=='_'||c=='.'||c=='-'))return 0;
        }
        for(size_t i=0;i<end-a;i++)temp[i]=(char)rv_pf_upper(buf[a+i]);
        temp[end-a]=';';temp[end-a+1]='1';temp[end-a+2]=0;
    }
    if(found!=1)return 0;
    while(pos<n)if(buf[pos++])return 0; /* only NUL padding after a NUL */
    strcpy(name,temp);return 1;
}
static inline int rv_pf_exe(const unsigned char h[2048],uint32_t bytes,rv_pf_result *r){
    uint32_t pc=rv_pf_le32(h+0x10),gp=rv_pf_le32(h+0x14),
        addr=rv_pf_le32(h+0x18),size=rv_pf_le32(h+0x1c),sp=rv_pf_le32(h+0x30);
    if(memcmp(h,"PS-X EXE",8)||bytes<2048||!size||size%2048||
        size>bytes-2048||addr<0x80000000u||addr>=0x80200000u||(addr&3u)||
        size>0x80200000u-addr||pc<addr||pc-addr>=size||(pc&3u)||
        (sp&&(sp<0x80000000u||sp>=0x80200000u||(sp&3u))))return 0;
    r->pc=pc;r->gp=gp;r->text_addr=addr;r->text_bytes=size;r->sp=sp?sp:0x801fff00u;
    return 1;
}
/* Internal metadata pass. It does not establish payload readability.
 * The public inspector and transactional loader must read every payload sector. */
static inline int rv_pf_bin_metadata(FILE *f,rv_pf_result *out){
    unsigned char sec[2048];rv_pf_result r;rv_pf_extent root,cnf,exe;
    long bytes;uint32_t volume;int rc,found;
    if(!f||!out)return RV_PF_IO;
    memset(&r,0,sizeof(r));
    if(fseek(f,0,SEEK_END)||(bytes=ftell(f))<0)return RV_PF_IO;
    if(bytes<18L*2352L||bytes>900L*1024L*1024L||bytes%2352L)return RV_PF_PROFILE;
    r.sectors=(uint32_t)(bytes/2352L);
    rc=rv_pf_sector(f,16,r.sectors,sec);if(rc)return rc;
    if(sec[0]!=1||memcmp(sec+1,"CD001",5)||sec[6]!=1||
        !rv_pf_both32(sec+80,&volume)||volume<18u||volume>r.sectors||
        sec[128]!=0||sec[129]!=8||sec[130]!=8||sec[131]!=0)return RV_PF_VOLUME;
    if(sec[156]<34||sec[157]!=0||!(sec[181]&2)||!rv_pf_both32(sec+158,&root.lba)||
        !rv_pf_both32(sec+166,&root.bytes)||!rv_pf_extent_ok(root,volume)||
        root.bytes%2048||root.bytes>65536)return RV_PF_DIRECTORY;
    r.volume_sectors=volume;
    rc=rv_pf_sector(f,17,volume,sec);if(rc)return rc;
    if(sec[0]!=255||memcmp(sec+1,"CD001",5)||sec[6]!=1)return RV_PF_PROFILE;
    found=rv_pf_find(f,volume,root,"SYSTEM.CNF;1",&cnf);
    if(found<0)return RV_PF_DIRECTORY;
    if(found){
        if(cnf.bytes>2048)return RV_PF_BOOT;
        rc=rv_pf_sector(f,cnf.lba,volume,sec);if(rc)return rc;
        if(!rv_pf_boot_name(sec,cnf.bytes,r.boot_name))return RV_PF_BOOT;
    }else strcpy(r.boot_name,"PSX.EXE;1");
    found=rv_pf_find(f,volume,root,r.boot_name,&exe);
    if(found!=1)return RV_PF_BOOT;
    rc=rv_pf_sector(f,exe.lba,volume,sec);if(rc)return rc;
    if(!rv_pf_exe(sec,exe.bytes,&r))return RV_PF_EXE;
    r.exe_lba=exe.lba;r.exe_bytes=exe.bytes;
    *out=r;return RV_PF_OK;
}
static inline int rv_pf_bin(FILE *f,rv_pf_result *out){
    rv_pf_result r;
    unsigned char sec[2048];
    int rc;
    if(!out)return RV_PF_IO;
    rc=rv_pf_bin_metadata(f,&r);if(rc)return rc;
    /* Inspection continues to require every declared payload sector. */
    for(uint32_t i=0;i<r.text_bytes/2048u;i++){
        rc=rv_pf_sector(f,r.exe_lba+1+i,r.volume_sectors,sec);if(rc)return rc;
    }
    *out=r;return RV_PF_OK;
}
#endif
