/* SPDX-License-Identifier: GPL-2.0-or-later */
#ifndef REVIVAL_JIT_POLICY_H
#define REVIVAL_JIT_POLICY_H
#define REVIVAL_JIT_WARMUP 3u
static inline unsigned int revival_jit_verified_next(unsigned int v) {
    return v<REVIVAL_JIT_WARMUP?v+1u:REVIVAL_JIT_WARMUP;
}
static inline int revival_jit_can_run(unsigned int verified,unsigned int native_blocks) {
    return verified>=REVIVAL_JIT_WARMUP && (native_blocks&255u)!=255u;
}
#endif
