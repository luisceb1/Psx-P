/* SPDX-License-Identifier: GPL-2.0-or-later */
#ifndef REVIVAL_JIT_EMIT_H
#define REVIVAL_JIT_EMIT_H
#include <stddef.h>
#include <stdint.h>
#define REVIVAL_JIT_MAX_GUEST 32u
#define REVIVAL_JIT_MAX_WORDS (4u*REVIVAL_JIT_MAX_GUEST+2u)
/* Emit a leaf Allegrex function: void block(uint32_t gpr[32]).
 * Straight-line, non-trapping ALU only. Zero means reject the entire block.
 * No external calls, guest memory accesses, cycle or interrupt handling.
 * The caller is responsible for executable memory and D/I cache coherency.
 */
int revival_jit_supported(uint32_t word);
size_t revival_jit_emit(const uint32_t *guest,size_t n,uint32_t *out,size_t cap);
#endif
