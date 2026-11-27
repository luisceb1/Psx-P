#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
"""Compare emitted MIPS32 words with the actual PSPSDK assembly encodings."""
import struct
import sys
from pathlib import Path
data = Path(sys.argv[1]).read_bytes()
expected = {
    "CLZ": 0x70000020 | (9 << 21) | (8 << 16) | (8 << 11),
    "MOVN": 0x0000000b | (9 << 21) | (10 << 16) | (8 << 11),
    "MOVZ": 0x0000000a | (9 << 21) | (10 << 16) | (8 << 11),
    "MULT": 0x00000018 | (9 << 21) | (10 << 16),
    "MULTU": 0x00000019 | (9 << 21) | (10 << 16),
    "DIV": 0x0000001a | (9 << 21) | (10 << 16),
    "DIVU": 0x0000001b | (9 << 21) | (10 << 16),
    "MFLO": 0x00000012 | (8 << 11),
    "MFHI": 0x00000010 | (8 << 11),
    "JR": 0x03e00008,
    "NOP": 0,
}
if len(data) < 4 * len(expected):
    raise SystemExit("Unexpected short text section")
for i, (name, upstream) in enumerate(expected.items()):
    actual, = struct.unpack_from("<I", data, 4*i)
    print(f"{name}: upstream={upstream:08x} allegrex={actual:08x} {'MATCH' if actual == upstream else 'DIFFERENT'}")
# Diagnostic only: a mismatch is an explicit porting task, not a build failure.
