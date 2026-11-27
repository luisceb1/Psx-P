/* SPDX-License-Identifier: GPL-2.0-or-later */
#ifndef REVIVAL_STORAGE_H
#define REVIVAL_STORAGE_H
#include <stddef.h>
/* Extract the app directory from an absolute ef0:/ or ms0:/ EBOOT path.
 * Returns 0 on success; -1 for invalid input or insufficient output space.
 * On failure, a nonempty output buffer is cleared. No implicit root fallback.
 * Input and output buffers must not overlap. */
int revival_app_dir(const char *eboot, char *out, size_t capacity);
#endif
