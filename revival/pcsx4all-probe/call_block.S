/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Original PSP bridge for PCSX4ALL's documented indirect block ABI.
 * void revival_pcsx4all_call(void *fn, void *regs, u32 pc, u32 result[2])
 * Generated block receives fp=regs, v0=pc, ra and 16(sp)=return address.
 * Returns v0=next PC, v1=cycle delta. Never called by this compile probe.
 */
    .text
    .set noreorder
    .set nomacro
    .globl revival_pcsx4all_call
    .ent revival_pcsx4all_call
revival_pcsx4all_call:
    addiu $sp,$sp,-72
    sw $a3,20($sp)
    sw $gp,24($sp)
    sw $s0,28($sp)
    sw $s1,32($sp)
    sw $s2,36($sp)
    sw $s3,40($sp)
    sw $s4,44($sp)
    sw $s5,48($sp)
    sw $s6,52($sp)
    sw $s7,56($sp)
    sw $fp,60($sp)
    sw $ra,64($sp)
    addu $t9,$a0,$zero
    addu $fp,$a1,$zero
    addu $v0,$a2,$zero
    addu $v1,$zero,$zero
    lui $ra,%hi(.Lreturned)
    addiu $ra,$ra,%lo(.Lreturned)
    sw $ra,16($sp)
    jr $t9
    nop
.Lreturned:
    lw $t0,20($sp)
    sw $v0,0($t0)
    sw $v1,4($t0)
    lw $gp,24($sp)
    lw $s0,28($sp)
    lw $s1,32($sp)
    lw $s2,36($sp)
    lw $s3,40($sp)
    lw $s4,44($sp)
    lw $s5,48($sp)
    lw $s6,52($sp)
    lw $s7,56($sp)
    lw $fp,60($sp)
    lw $ra,64($sp)
    jr $ra
    addiu $sp,$sp,72
    .end revival_pcsx4all_call
