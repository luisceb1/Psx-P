TARGET = revival_pause_probe
OBJS = pause_main.o
CFLAGS = -O2 -G0
CXXFLAGS = $(CFLAGS)
ASFLAGS = $(CFLAGS)
LIBS = -lpsppower -lpspdebug -lpspctrl -lpspdisplay -lpspge
EXTRA_TARGETS = EBOOT.PBP
PSP_EBOOT_TITLE = PSX-P Pause Probe
PSPSDK = $(shell psp-config --pspsdk-path)
include $(PSPSDK)/lib/build.mak
