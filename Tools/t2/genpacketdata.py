#!/usr/bin/env python3
"""Write the control object's packet-data layouts as a C header.

    python3 Tools/t2/genpacketdata.py

Tribes 2 sends the state of the object a client drives through readPacketData
-- vtable slot 66 -- rather than through unpackUpdate, so it needs a table of
its own. The shipped t2PacketDataTables.h carries only Camera, which is the
control object a client gets while it is still watching; the moment it spawns
the control object is a Player, and every other one a server hands out is
written out here from Tools/t2/handlayout.py.
"""
import json, os

HERE = os.path.dirname(os.path.abspath(__file__))
OUT = os.path.join(HERE, "..", "..", "Engine/source/T3D/gameBase",
                   "t2PacketDataTablesRecovered.h")

OPS = {"readFlag": "OpFlag", "readInt": "OpInt", "readFloatN": "OpFloat",
       "readSignedFloat": "OpSignedFloat", "readRangedF32": "OpRangedF32",
       "readRangedU32": "OpRangedU32", "readColor": "OpColor",
       "readRaw": "OpRaw", "readDataBlockId": "OpDataBlockId",
       "readSTString": "OpString", "readString": "OpString",
       "readPoint": "OpPoint"}

G = json.load(open(os.path.join(HERE, "packetdata_layouts.json")))

L = []
A = L.append
A("/// @file")
A("/// What the object a client drives sends each tick, per class.")
A("///")
A("/// Tribes 2 sends it through readPacketData -- vtable slot 66 -- rather")
A("/// than through unpackUpdate, so it is a table of its own.  The shipped")
A("/// t2PacketDataTables.h has only Camera, which is what a client watching")
A("/// a mission is given; the moment it spawns the control object is a")
A("/// Player, and without a layout for that the packet could not be read at")
A("/// all.")
A("///")
A("/// Player's is the one with a shape no walk could recover: when the player")
A("/// is piloting, what follows is the vehicle's own packet data, and how")
A("/// wide that is depends on which ghost the index names rather than on any")
A("/// bit in the stream.  OpPacketDataOf asks the connection.")
A("///")
A("/// Written by Tools/t2/handlayout.py; see Tools/t2/README.md.")
A("")
A("#ifndef _T2PACKETDATATABLESRECOVERED_H_")
A("#define _T2PACKETDATATABLESRECOVERED_H_")
A("")
A("// HH: Added for Tribes 2 compatibility.")
A("")
A("namespace T2Protocol")
A("{")

names = sorted(G)
for cls in names:
    e = G[cls]
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
        elif n[0] == "count":
            body.append("      { OpCount, %d, %d, %d },"
                        % (min(n[2] or 0, 32), n[4] if n[4] is not None else -1,
                           n[3]))
        elif n[0] == "flag":
            body.append("      { OpFlag, 1, %d, %d },"
                        % (n[1] if n[1] is not None else -1,
                           n[2] if n[2] is not None else -1))
        elif n[0] == "initial":
            body.append("      { OpInitial, 0, %d, %d },"
                        % (n[1] if n[1] is not None else -1,
                           n[2] if n[2] is not None else -1))
        elif n[0] == "packetdataof":
            body.append("      { OpPacketDataOf, %d, %d, -1 },"
                        % (n[1], n[2] if n[2] is not None else -1))
        elif n[0] == "sel":
            base = len(sel)
            for c in n[3]:
                sel.append(c if c is not None else -1)
            body.append("      { OpSelect, %d, %d, -1 }," % (n[2], base))
        else:
            kind, bits, nxt = n[1], n[2], n[3]
            b = 0 if kind in ("readSTString", "readString") else (bits or 0)
            body.append("      { %s, %d, %d, -1 },"
                        % (OPS.get(kind, "OpRaw"), min(b, 32),
                           nxt if nxt is not None else -1))
    if sel:
        A("   static const S16 sT2PrSel_%s[] = { %s };"
          % (cls, ", ".join(str(x) for x in sel)))
    A("   /// %s: %s" % (cls, e.get("why", "")))
    A("   static const T2DataNode sT2PrNodes_%s[] =" % cls)
    A("   {")
    L.extend(body)
    A("   };")
    A("")

A("   static const T2DataBlockLayout sT2PacketDataRecovered[] =")
A("   {")
for cls in names:
    e = G[cls]
    has = any(n[0] in ("sel",) for n in e["nodes"])
    A('      { "%s", sT2PrNodes_%s, %d, %s, %d },'
      % (cls, cls, len(e["nodes"]),
         ("sT2PrSel_%s" % cls) if has else "NULL", e["root"]))
A("   };")
A("}")
A("")
A("#endif")
A("")

open(OUT, "w").write("\n".join(L))
print("wrote %s: %d classes, %d nodes"
      % (os.path.normpath(OUT), len(names),
         sum(len(G[c]["nodes"]) for c in names)))
print("classes: %s" % " ".join(names))
