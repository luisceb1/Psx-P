TARGET = pcsx4all_integration_probe
OBJS = core-relocatable.o gpu_core.o gpu_unai.o integration_main.o integration_stubs.o integration_vout.o dma_core.o hw_core.o
ifeq ($(CD_INTEGRATION),1)
OBJS = core-relocatable.o gpu_core.o gpu_unai.o integration_cd_main.o integration_cd_stubs.o integration_vout.o dma_core.o hw_core.o cd_iso.o cd_ppf.o cd_controller.o cd_xa.o
TARGET = pcsx4all_cd_integration_probe
endif
ifeq ($(EXE_INTEGRATION),1)
OBJS += exe_loader.o
TARGET = pcsx4all_exe_probe
endif
ifeq ($(HLE_INTEGRATION),1)
OBJS += bios_core.o hle_core.o mdec_core.o sio_core.o pad_core.o
TARGET = pcsx4all_hle_probe
endif
ifeq ($(SPU_INTEGRATION),1)
OBJS += spu-relocatable.o
TARGET = pcsx4all_full_probe
endif
CFLAGS = -O2 -G0
LIBS = -lstdc++ -lpspgu -lpspge -lpspdisplay
ifeq ($(CD_INTEGRATION),1)
LIBS += -lz
endif
EXTRA_TARGETS = EBOOT.PBP
PSP_EBOOT_TITLE = Revival PCSX4ALL integration probe
PSPSDK = $(shell psp-config --pspsdk-path)
include $(PSPSDK)/lib/build.mak
