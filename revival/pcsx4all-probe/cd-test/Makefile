TARGET = pcsx4all_cd_probe
OBJS = cd_iso.o cd_ppf.o cd_main.o
CFLAGS = -O2 -G0
LIBS = -lstdc++ -lz
EXTRA_TARGETS = EBOOT.PBP
PSP_EBOOT_TITLE = Revival PCSX4ALL CD probe
PSPSDK = $(shell psp-config --pspsdk-path)
include $(PSPSDK)/lib/build.mak
