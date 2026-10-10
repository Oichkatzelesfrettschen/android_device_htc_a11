#!/usr/bin/env python3
"""Rewrite qcom,idle-timeout in every distinct DTB of a QCDT table, in place of bytes.

A QCDT (dtbTool output, version 1) is a 12-byte header (magic, version, entry
count) followed by 20-byte entries (platform id, variant id, SoC revision, DTB
offset, DTB size). Entries of one board share a DTB blob. The kgsl-3d0 node
carries qcom,idle-timeout as one big-endian 32-bit cell; the rewrite replaces
that cell and nothing else, so the table keeps every offset, size and padding
byte, and the output differs from the input in exactly the cells it names.

usage: qcdt_set_idle_timeout.py INPUT OUTPUT OLD_MS NEW_MS
"""

from __future__ import annotations

import struct
import sys

QCDT_MAGIC = b"QCDT"
QCDT_HEADER = struct.Struct("<4sII")
QCDT_ENTRY = struct.Struct("<5I")
FDT_MAGIC = 0xD00DFEED
FDT_BEGIN_NODE = 1
FDT_END_NODE = 2
FDT_PROP = 3
FDT_NOP = 4
FDT_END = 9
PROP_NAME = b"qcom,idle-timeout"
PROP_LABEL = PROP_NAME.decode("ascii")


def _align4(value: int) -> int:
    return (value + 3) & ~3


def find_cells(dtb: bytes, name: bytes) -> list[int]:
    """Return the byte offsets of the 4-byte payload of every property called name."""
    magic, _total, off_struct, off_strings = struct.unpack_from(">IIII", dtb, 0)
    if magic != FDT_MAGIC:
        raise ValueError("not a flattened device tree")
    hits: list[int] = []
    pos = off_struct
    while True:
        (token,) = struct.unpack_from(">I", dtb, pos)
        pos += 4
        if token == FDT_BEGIN_NODE:
            end = dtb.index(b"\0", pos)
            pos = _align4(end + 1)
        elif token == FDT_PROP:
            length, nameoff = struct.unpack_from(">II", dtb, pos)
            pos += 8
            start = off_strings + nameoff
            stop = dtb.index(b"\0", start)
            if dtb[start:stop] == name:
                if length != 4:
                    raise ValueError("%s is %d bytes, expected one cell" % (PROP_LABEL, length))
                hits.append(pos)
            pos = _align4(pos + length)
        elif token in (FDT_END_NODE, FDT_NOP):
            continue
        elif token == FDT_END:
            return hits
        else:
            raise ValueError("bad token %#x at %#x" % (token, pos - 4))


def rewrite(table: bytes, old: int, new: int) -> bytes:
    magic, version, count = QCDT_HEADER.unpack_from(table, 0)
    if magic != QCDT_MAGIC or version != 1:
        raise ValueError("not a version 1 QCDT table")
    out = bytearray(table)
    blobs = set()
    for index in range(count):
        *_ids, offset, size = QCDT_ENTRY.unpack_from(
            table, QCDT_HEADER.size + index * QCDT_ENTRY.size
        )
        blobs.add((offset, size))
    for offset, size in sorted(blobs):
        end = offset + size
        cells = find_cells(table[offset:end], PROP_NAME)
        if len(cells) != 1:
            raise ValueError("blob at %#x has %d %s properties" % (offset, len(cells), PROP_LABEL))
        cell = offset + cells[0]
        (value,) = struct.unpack_from(">I", table, cell)
        if value != old:
            raise ValueError("blob at %#x holds %d, expected %d" % (offset, value, old))
        struct.pack_into(">I", out, cell, new)
        print("blob %#x size %#x: cell at %#x %d -> %d" % (offset, size, cell, old, new))
    return bytes(out)


def main(argv: list[str]) -> int:
    if len(argv) != 5:
        print(__doc__, file=sys.stderr)
        return 2
    with open(argv[1], "rb") as handle:
        table = handle.read()
    result = rewrite(table, int(argv[3]), int(argv[4]))
    with open(argv[2], "xb") as handle:
        handle.write(result)
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
