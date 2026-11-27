TARGET = revival_disc_inspector
OBJS = inspect_main.o storage.o
CFLAGS = -O2 -G0
CXXFLAGS = $(CFLAGS)
ASFLAGS = $(CFLAGS)
LIBS = -lpspdebug -lpspctrl -lpspdisplay -lpspge
EXTRA_TARGETS = EBOOT.PBP
PSP_EBOOT_TITLE = PSX-P Disc Inspector
PSPSDK = $(shell psp-config --pspsdk-path)
include $(PSPSDK)/lib/build.mak
