TARGET = pcsx4all_integration_probe
OBJS = core-relocatable.o gpu_core.o gpu_unai.o integration_main.o integration_stubs.o integration_vout.o dma_core.o hw_core.o
CFLAGS = -O2 -G0
LIBS = -lstdc++ -lpspgu -lpspge -lpspdisplay
EXTRA_TARGETS = EBOOT.PBP
PSP_EBOOT_TITLE = Revival PCSX4ALL integration probe
PSPSDK = $(shell psp-config --pspsdk-path)
include $(PSPSDK)/lib/build.mak
