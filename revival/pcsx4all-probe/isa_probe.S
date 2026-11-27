/* SPDX-License-Identifier: GPL-2.0-or-later */
/* PSPSDK assembler is the reference for actual host instruction encodings.
 * No generated guest code is executed here. */
    .text
    .set noreorder
    .set nomacro
    .globl revival_isa_probe
revival_isa_probe:
    clz $8,$9
    movn $8,$9,$10
    movz $8,$9,$10
    mult $9,$10
    multu $9,$10
    div $0,$9,$10
    divu $0,$9,$10
    mflo $8
    mfhi $8
    jr $31
    nop
