/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Headless presentation adapter, not a graphics renderer. */
#include "psxcommon.h"
#include "gpu/gpulib/gpu.h"
#include "plugin_lib.h"
PcsxConfig Config;
struct pl_data_t pl_data={};
uint32_t hSyncCount=0, frame_counter=0;
unsigned probe_presentations=0, probe_blanks=0;
void pl_clear_borders() {}
int vout_init(void) { return 0; }
int vout_finish(void) { return 0; }
void vout_update(void) { ++probe_presentations; }
void vout_blank(void) { ++probe_blanks; }
void vout_set_config(const gpulib_config_t *) {}
