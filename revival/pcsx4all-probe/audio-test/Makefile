TARGET = revival_audio_probe
OBJS = audio_main.o audio_psp.o spu-relocatable.o
CFLAGS = -O2 -G0
LIBS = -lpspaudio -lpspdebug -lpspctrl -lpspdisplay -lpspge
EXTRA_TARGETS = EBOOT.PBP
PSP_EBOOT_TITLE = PSX-P Audio Probe
PSPSDK = $(shell psp-config --pspsdk-path)
include $(PSPSDK)/lib/build.mak
