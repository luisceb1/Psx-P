TARGET = pcsx4all_gpu_probe
OBJS = gpu_core.o gpu_unai.o gpu_main.o gpu_platform.o
CFLAGS = -O2 -G0
LIBS = -lstdc++
EXTRA_TARGETS = EBOOT.PBP
PSP_EBOOT_TITLE = Revival PCSX4ALL GPU probe
PSPSDK = $(shell psp-config --pspsdk-path)
include $(PSPSDK)/lib/build.mak
