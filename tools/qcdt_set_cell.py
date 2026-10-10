#!/usr/bin/env python3
"""Rewrite one 32-bit cell of one property in every distinct DTB of a QCDT table.

A QCDT (dtbTool output, version 1) is a 12-byte header (magic, version, entry
count) followed by 20-byte entries (platform id, variant id, SoC revision, DTB
offset, DTB size). Entries of one board share a DTB blob, and each distinct blob
is rewritten once. The target is named by its node path (for example
/memory/adsp_region), property name and zero-based cell index; the cell must
hold the expected old value, so a table that already changed or a different
board revision fails instead of being rewritten. The rewrite replaces that
big-endian cell and nothing else, which keeps every offset, size and padding
byte: the output differs from the input in exactly the cells it names.

usage: qcdt_set_cell.py INPUT OUTPUT NODE_PATH PROPERTY CELL_INDEX OLD NEW
       qcdt_set_cell.py INPUT - NODE_PATH PROPERTY CELL_INDEX OLD NEW   (dry run)

OLD and NEW accept decimal or 0x-prefixed hexadecimal.
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


def _align4(value: int) -> int:
    return (value + 3) & ~3


def find_cells(dtb: bytes, node_path: str, prop: str, index: int) -> list[int]:
    """Return the byte offset of cell INDEX of every property PROP of node NODE_PATH."""
    magic, _total, off_struct, off_strings = struct.unpack_from(">IIII", dtb, 0)
    if magic != FDT_MAGIC:
        raise ValueError("not a flattened device tree")
    want_name = prop.encode("ascii")
    stack: list[str] = []
    hits: list[int] = []
    pos = off_struct
    while True:
        (token,) = struct.unpack_from(">I", dtb, pos)
        pos += 4
        if token == FDT_BEGIN_NODE:
            end = dtb.index(b"\0", pos)
            stack.append(dtb[pos:end].decode("ascii"))
            pos = _align4(end + 1)
        elif token == FDT_END_NODE:
            stack.pop()
        elif token == FDT_PROP:
            length, nameoff = struct.unpack_from(">II", dtb, pos)
            pos += 8
            start = off_strings + nameoff
            stop = dtb.index(b"\0", start)
            path = "/" + "/".join(part for part in stack if part)
            if path == node_path and dtb[start:stop] == want_name:
                if length % 4 or index < 0 or index * 4 + 4 > length:
                    raise ValueError(
                        "%s %s is %d bytes, cell %d is out of range"
                        % (node_path, prop, length, index)
                    )
                hits.append(pos + index * 4)
            pos = _align4(pos + length)
        elif token == FDT_NOP:
            continue
        elif token == FDT_END:
            return hits
        else:
            raise ValueError("bad token %#x at %#x" % (token, pos - 4))


def rewrite(table: bytes, node_path: str, prop: str, index: int, old: int, new: int) -> bytes:
    magic, version, count = QCDT_HEADER.unpack_from(table, 0)
    if magic != QCDT_MAGIC or version != 1:
        raise ValueError("not a version 1 QCDT table")
    out = bytearray(table)
    blobs = set()
    for entry in range(count):
        *_ids, offset, size = QCDT_ENTRY.unpack_from(
            table, QCDT_HEADER.size + entry * QCDT_ENTRY.size
        )
        blobs.add((offset, size))
    for offset, size in sorted(blobs):
        end = offset + size
        cells = find_cells(table[offset:end], node_path, prop, index)
        if len(cells) != 1:
            raise ValueError(
                "blob at %#x has %d matches for %s %s" % (offset, len(cells), node_path, prop)
            )
        cell = offset + cells[0]
        (value,) = struct.unpack_from(">I", table, cell)
        if value != old:
            raise ValueError("blob at %#x holds %#x, expected %#x" % (offset, value, old))
        struct.pack_into(">I", out, cell, new)
        print("blob %#x size %#x: cell at %#x %#x -> %#x" % (offset, size, cell, old, new))
    return bytes(out)


def main(argv: list[str]) -> int:
    if len(argv) != 8:
        print(__doc__, file=sys.stderr)
        return 2
    with open(argv[1], "rb") as handle:
        table = handle.read()
    result = rewrite(table, argv[3], argv[4], int(argv[5], 0), int(argv[6], 0), int(argv[7], 0))
    if argv[2] != "-":
        with open(argv[2], "xb") as handle:
            handle.write(result)
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
