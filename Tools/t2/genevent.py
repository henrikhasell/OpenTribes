#!/usr/bin/env python3
"""Write the event layouts recovered by eventwalk.py that nothing else reads.

The sixteen entries already in t2EventTables.h and the eleven readers in
eventsT2.cpp stay where they are: the walk agrees with every one of them on
node count, so replacing them would gain nothing and risks losing the places
where a hand-written reader is the better answer.  RemoteCommandEvent is the
example -- its argument loop does not unroll, so its graph reads one argument
where the reader reads all of them.

What is emitted here is only what had no reader at all.
"""
import json, os, re

HERE = os.path.dirname(os.path.abspath(__file__))
GB = os.path.join(HERE, "..", "..", "Engine/source/T3D/gameBase")
OUT = os.path.join(GB, "t2EventTablesRecovered.h")

OPS = {"readFlag": "OpFlag", "readInt": "OpInt", "readFloatN": "OpFloat",
       "readSignedFloat": "OpSignedFloat", "readRangedF32": "OpRangedF32",
       "readRangedU32": "OpRangedU32", "readColor": "OpColor",
       "readRaw": "OpRaw", "readDataBlockId": "OpDataBlockId",
       "readSTString": "OpString", "readString": "OpString"}


def slurp(p):
    return open(p, encoding="latin1").read()


E = json.load(open(os.path.join(HERE, "event_graphs.json")))
have = set(re.findall(r'\{ "(\w+)", sT2EvNodes_', slurp(os.path.join(
    GB, "t2EventTables.h"))))
ev = slurp(os.path.join(GB, "eventsT2.cpp"))
byhand = set(re.findall(r'"(\w+)"', re.search(
    r"sByHand\[\]\s*=\s*\{(.*?)\};", ev, re.S).group(1)))

# What had no reader at all, plus anything written out by hand -- a layout
# the shipped table gets wrong is corrected here rather than edited in place.
FIX = os.path.join(HERE, "event_fixups.json")
fixups = json.load(open(FIX)) if os.path.exists(FIX) else {}
E.update(fixups)
names = sorted(set(n for n in E if n not in have and n not in byhand
                   and not E[n].get("cyclic")) | set(fixups))

L = []
A = L.append
A("/// @file")
A("/// Event layouts recovered by walking unpack, for the events that had no")
A("/// reader at all.")
A("///")
A("/// An event's unpack is not at a fixed vtable slot the way unpackUpdate")
A("/// is: it sits four words past the .data word holding the class's")
A("/// getClassRep.  Finding it that way brought back all twenty-six, and")
A("/// every one that t2EventTables.h already carried came back with the same")
A("/// node count -- an independent check on a table recovered another way.")
A("/// Only the ones nothing could read are written here; the rest keep")
A("/// whatever already reads them.")
A("///")
A("/// Recovered by Tools/t2/eventwalk.py; see Tools/t2/README.md.")
A("")
A("// HH: Added for Tribes 2 compatibility.")
A("")
A("namespace T2Protocol")
A("{")

for cls in names:
    e = E[cls]
    body, sel = [], []
    for n in e["nodes"]:
        if n[0] == "end":
            body.append("      { OpEnd, 0, -1, -1 },")
        elif n[0] == "endbody":
            body.append("      { OpEndBody, 0, -1, -1 },")
        elif n[0] == "loop":
            body.append("      { OpLoop, %d, %d, %d },"
                        % (n[1], n[3] if n[3] is not None else -1,
                           n[2] if n[2] is not None else -1))
        elif n[0] == "br":
            kind, bits, slot, z, nz = n[1], n[2], n[3], n[4], n[5]
            b = 0 if kind in ("readSTString", "readString") else (bits or 0)
            base = len(sel)
            sel.append(z if z is not None else -1)
            sel.append(nz if nz is not None else -1)
            body.append("      { OpReadBr, %d, %d, %d }," % (min(b, 32), base, slot))
        elif n[0] == "count":
            kind, bits, slot, nxt = n[1], n[2], n[3], n[4]
            b = 0 if kind in ("readSTString", "readString") else (bits or 0)
            body.append("      { OpCount, %d, %d, %d },"
                        % (min(b, 32), nxt if nxt is not None else -1, slot))
        elif n[0] == "countbits":
            body.append("      { OpCountBits, %d, %d, %d },"
                        % (n[1], n[3] if n[3] is not None else -1, n[2]))
        elif n[0] == "flag":
            body.append("      { OpFlag, 1, %d, %d },"
                        % (n[1] if n[1] is not None else -1,
                           n[2] if n[2] is not None else -1))
        elif n[0] == "sel":
            base = len(sel)
            for c in n[3]:
                sel.append(c if c is not None else -1)
            body.append("      { OpSelect, %d, %d, -1 }," % (n[2], base))
        elif n[0] == "nop":
            body.append("      { OpEnd, 0, -1, -1 },")
        else:
            kind, bits, nxt = n[1], n[2], n[3]
            b = 0 if kind in ("readSTString", "readString") else (bits or 0)
            body.append("      { %s, %d, %d, -1 },"
                        % (OPS.get(kind, "OpRaw"), min(b, 32),
                           nxt if nxt is not None else -1))
    if sel:
        A("   static const S16 sT2ErSel_%s[] = { %s };"
          % (cls, ", ".join(str(x) for x in sel)))
    if e.get("why"):
        A("   /// %s: %s" % (cls, e["why"]))
    else:
        A("   /// %s: %d nodes from %d explored, %s"
          % (cls, len(e["nodes"]), e.get("explored", 0),
             "exact" if not e.get("approx")
             else "%d call(s) stepped over" % e["approx"]))
    A("   static const T2DataNode sT2ErNodes_%s[] =" % cls)
    A("   {")
    L.extend(body)
    A("   };")
    A("")

A("   static const T2DataBlockLayout sT2EventsRecovered[] =")
A("   {")
for cls in names:
    e = E[cls]
    has = any(n[0] in ("sel", "br") for n in e["nodes"])
    A('      { "%s", sT2ErNodes_%s, %d, %s, %d },'
      % (cls, cls, len(e["nodes"]),
         ("sT2ErSel_%s" % cls) if has else "NULL", e["root"]))
A("   };")
A("}")
A("")

open(OUT, "w").write("\n".join(L))
print("wrote %s: %d events, %d nodes"
      % (os.path.normpath(OUT), len(names),
         sum(len(E[c]["nodes"]) for c in names)))
print("emitted: %s" % " ".join(names))
