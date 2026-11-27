/* SPDX-License-Identifier: GPL-2.0-or-later */
#include "disc_inspect.h"
int main(int argc,char **argv){
    if(argc!=2){fprintf(stderr,"Usage: disc_inspect DIRECTORY_CONTAINING_GAME.CUE\n");return 2;}
    rv_inspect_result r=rv_inspect_dir(argv[1]);
    rv_inspect_print(stdout,&r);
    return rv_inspect_ok(&r)?0:1;
}
