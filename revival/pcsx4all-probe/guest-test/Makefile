TARGET = pcsx4all_guest_probe
OBJS = core-relocatable.o guest_main.o platform_stubs.o
CFLAGS = -O2 -G0
LIBS = -lstdc++
EXTRA_TARGETS = EBOOT.PBP
PSP_EBOOT_TITLE = Revival PCSX4ALL guest probe
PSPSDK = $(shell psp-config --pspsdk-path)
include $(PSPSDK)/lib/build.mak
