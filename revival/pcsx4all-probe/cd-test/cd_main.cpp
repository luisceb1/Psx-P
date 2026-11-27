/* SPDX-License-Identifier: GPL-2.0-or-later */
#include <pspkernel.h>
#include <pspdebug.h>
#include <stdio.h>
#include <string.h>
#include "psxcommon.h"
#include "plugins.h"
#include "cdrom.h"
PSP_MODULE_INFO("PCSX4ALL CD probe",0,1,0);
PSP_MAIN_THREAD_ATTR(PSP_THREAD_ATTR_USER);
PcsxConfig Config;
cdrStruct cdr;
char CdromId[10]={0};
char CdromLabel[33]={0};
static unsigned char sector[2352];
static FILE *report;
static void check(const char *name,bool ok){
    pspDebugScreenPrintf("%s: %s\n",name,ok?"PASS":"FAIL");
    if(report){fprintf(report,"%s,%s\n",name,ok?"PASS":"FAIL");fflush(report);}
}
int main(){
    pspDebugScreenInit();
    report=fopen("PCSX4ALL_CD.TXT","w");
    // Original synthetic raw sectors, no copyrighted game or BIOS.
    const char *bin="REVIVAL_SYNTHETIC.bin",*cue="REVIVAL_SYNTHETIC.cue";
    FILE *f=fopen(bin,"wb");
    bool made=f!=NULL;
    if(f){
        for(unsigned n=0;n<32;n++){
            memset(sector,0,sizeof(sector));
            memset(sector+1,0xff,10);sector[13]=2;sector[14]=(n/10)*16+n%10;sector[15]=2;
            sector[24]=n;sector[25]=n^0xa5;
            if(fwrite(sector,1,sizeof(sector),f)!=sizeof(sector)){made=false;break;}
        }
        fclose(f);
    }
    f=fopen(cue,"w");
    if(f){fprintf(f,"FILE \"REVIVAL_SYNTHETIC.bin\" BINARY\n  TRACK 01 MODE2/2352\n    INDEX 01 00:00:00\n");fclose(f);}else made=false;
    check("Synthetic image creation",made);
    if(made){
        SetIsoFile(bin);
        bool opened=CDR_init()==0 && CDR_open()==0;
        check("BIN/CUE open",opened);
        if(opened){
            unsigned char tn[2]={},td[3]={};
            check("Track count",CDR_getTN(tn)==0&&tn[0]==1&&tn[1]==1);
            check("Track start",CDR_getTD(1,td)==0&&td[0]==0&&td[1]==2&&td[2]==0);
            unsigned char msf[3]={0,2,0};
            bool ok=CDR_readTrack(msf)==0;
            unsigned char *p=ok?CDR_getBuffer():NULL;
            check("Sector zero",p&&p[12]==0&&p[13]==0xa5);
            msf[2]=0x17;ok=CDR_readTrack(msf)==0;p=ok?CDR_getBuffer():NULL;
            check("Sector seventeen",p&&p[12]==17&&p[13]==(17^0xa5));
            CDR_close();
        }
        CDR_shutdown();
    }
    if(report)fclose(report);
    sceKernelDelayThread(10000000);sceKernelExitGame();return 0;
}
