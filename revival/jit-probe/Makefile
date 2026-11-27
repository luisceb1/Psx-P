TARGET = revival_jit_probe
OBJS = main.o jit_emit.o
VPATH = ..
INCDIR = ..
CFLAGS = -O2 -Wall -Wextra -std=gnu99
EXTRA_TARGETS = EBOOT.PBP
PSP_EBOOT_TITLE = Revival Allegrex JIT probe
PSPSDK = $(shell psp-config --pspsdk-path)
include $(PSPSDK)/lib/build.mak
