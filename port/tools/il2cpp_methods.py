#!/usr/bin/env python3
"""List IL2CPP methods with their code addresses, from the game's own files.

usage: il2cpp_methods.py <global-metadata.dat> <libil2cpp.so> <regex>

Metadata version 24 (Unity 2017.4), 32-bit ARM. Prints "rva Type::Method" for
every method whose "Type::Method" matches the regex. The table of method
pointers is found as the word pair (count, pointer) that the code registration
starts with.
"""
import re
import struct
import sys

m = open(sys.argv[1], "rb").read()
so = open(sys.argv[2], "rb").read()
pat = re.compile(sys.argv[3])


def hdr(i):
    return struct.unpack_from("<I", m, i)[0]


assert hdr(0) == 0xFAB11BAF and hdr(4) == 24
str_off = hdr(24)
meth_off, meth_size = hdr(48), hdr(52)
type_off, type_size = hdr(160), hdr(164)
TYPE_SIZE = 0x68 if type_size % 0x68 == 0 else 0x64
assert type_size % TYPE_SIZE == 0
METH_SIZE = 56
assert meth_size % METH_SIZE == 0


def name(i):
    e = m.index(b"\0", str_off + i)
    return m[str_off + i : e].decode("utf8", "replace")


methods = []
for k in range(meth_size // METH_SIZE):
    nm, decl, ret, pstart, attr, gen, midx = struct.unpack_from("<7i", m, meth_off + k * METH_SIZE)
    methods.append((nm, decl, midx))

types = []
for k in range(type_size // TYPE_SIZE):
    nm, ns = struct.unpack_from("<2i", m, type_off + k * TYPE_SIZE)
    types.append((nm, ns))

count = max(x[2] for x in methods) + 1

# PT_LOAD: vaddr -> file offset
phoff = struct.unpack_from("<I", so, 28)[0]
phnum = struct.unpack_from("<H", so, 44)[0]
segs = []
for i in range(phnum):
    t, off, va, pa, fs, ms = struct.unpack_from("<6I", so, phoff + i * 32)
    if t == 1:
        segs.append((va, off, fs))


def fo(v):
    for va, off, fs in segs:
        if va <= v < va + fs:
            return v - va + off
    return None


table = None
data_va, data_off, data_fs = segs[-1]
for i in range(data_off, data_off + data_fs - 8, 4):
    c, p = struct.unpack_from("<2I", so, i)
    if c == count and fo(p) is not None:
        first = struct.unpack_from("<4I", so, fo(p))
        if all(x == 0 or fo(x) is not None for x in first):
            table = fo(p)
            break
if table is None:
    sys.exit("no method pointer table with %d entries" % count)
print("# %d methods, table at file offset 0x%x" % (count, table))

for nm, decl, midx in methods:
    tn = name(types[decl][0])
    full = "%s::%s" % (tn, name(nm))
    if pat.search(full):
        a = struct.unpack_from("<I", so, table + midx * 4)[0] if midx >= 0 else 0
        print("0x%08x %s" % (a, full))
