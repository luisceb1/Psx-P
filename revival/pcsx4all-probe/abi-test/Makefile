TARGET = pcsx4all_abi_probe
OBJS = main.o call_block.o
VPATH = ..
CFLAGS = -O2 -G0 -Wall -Wextra -Werror -std=gnu99
EXTRA_TARGETS = EBOOT.PBP
PSP_EBOOT_TITLE = Revival PCSX4ALL ABI probe
PSPSDK = $(shell psp-config --pspsdk-path)
include $(PSPSDK)/lib/build.mak
