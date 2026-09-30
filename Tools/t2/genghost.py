#!/usr/bin/env python3
"""Write the ghost layouts recovered by ghostwalk.py as a C header.

The shipped t2GhostTables.h stays where it is: these are consulted first and
it answers for the classes the walk still cannot reach, so nothing regresses.
"""
import json, os

HERE = os.path.dirname(os.path.abspath(__file__))
OUT = os.path.join(os.path.dirname(os.path.abspath(__file__)),
                   "..", "..", "Engine/source/T3D/gameBase",
                   "t2GhostTablesRecovered.h")

G = json.load(open(os.path.join(HERE, "ghost_graphs.json")))

OPS = {"readFlag": "OpFlag", "readInt": "OpInt", "readFloatN": "OpFloat",
       "readSignedFloat": "OpSignedFloat", "readRangedF32": "OpRangedF32",
       "readRangedU32": "OpRangedU32", "readColor": "OpColor",
       "readRaw": "OpRaw", "readDataBlockId": "OpDataBlockId",
       "readSTString": "OpString", "readString": "OpString",
       "readPoint": "OpPoint"}

L = []
A = L.append
A("/// @file")
A("/// Ghost update layouts recovered a second time, with the walk able to")
A("/// follow a class into its parent.")
A("///")
A("/// The tables in t2GhostTables.h came from a walk that halted on a call it")
A("/// could not enter, so a class whose reading sits below its parent's")
A("/// unpackUpdate came out a stub: AudioEmitter's graph had six nodes and")
A("/// took 37 bits where its neighbours take six hundred, which lost the rest")
A("/// of any packet it shared.  These were walked with every function the")
A("/// emulator might enter already in its map, and with calls it still cannot")
A("/// resolve stepped over rather than halted on.")
A("///")
A("/// A class here whose walk rested on stepping over a call carries the count")
A("/// in its comment: those layouts are the ones to doubt first when a packet")
A("/// will not read.  The rest are exact.")
A("///")
A("/// Recovered by Tools/t2/ghostwalk.py; see Tools/t2/README.md.")
A("")
A("#ifndef _T2GHOSTTABLESRECOVERED_H_")
A("#define _T2GHOSTTABLESRECOVERED_H_")
A("")
A("namespace T2Protocol")
A("{")

def has_cycle(nodes, root):
    """A counted loop that did not unroll shows up as a back edge.

    Such a graph reads whichever way out of the cycle the bits happen to
    steer, which is how a body of six hundred bits comes out as thirty-six.
    A layout like that is worse than none, because it looks like an answer.
    """
    WHITE, GREY, BLACK = 0, 1, 2
    colour = [WHITE] * len(nodes)

    def succ(i):
        n = nodes[i]
        if n[0] == "loop":
            return [x for x in (n[3], n[2]) if x is not None]
        if n[0] == "flag":
            return [x for x in (n[1], n[2]) if x is not None]
        if n[0] == "sel":
            return [x for x in n[3] if x is not None]
        if n[0] == "br":
            return [x for x in (n[4], n[5]) if x is not None]
        if n[0] == "count":
            return [x for x in (n[4],) if x is not None]
        if n[0] in ("end", "endbody"):
            return []
        return [x for x in (n[3],) if x is not None]

    stack = [(root, iter(succ(root)) if 0 <= root < len(nodes) else iter(()))]
    if 0 <= root < len(nodes):
        colour[root] = GREY
    while stack:
        i, it = stack[-1]
        nxt = next(it, None)
        if nxt is None:
            colour[i] = BLACK
            stack.pop()
            continue
        if not (0 <= nxt < len(nodes)):
            continue
        if colour[nxt] == GREY:
            return True
        if colour[nxt] == WHITE:
            colour[nxt] = GREY
            stack.append((nxt, iter(succ(nxt))))
    return False


cyclic = [c for c in sorted(G) if G[c].get("cyclic")
           or has_cycle(G[c]["nodes"], G[c]["root"])]
for c in cyclic:
    del G[c]

# A graph can be acyclic and still wrong, and the only judge of that is a
# recorded session: Tools/t2/vetghost.py replays the captures and writes the
# classes whose layout breaks a packet here, with the reason.  The shipped
# t2GhostTables.h answers for them instead, which is what the walk's own
# failures already fall back to.
VETO = os.path.join(HERE, "ghost_veto.json")
vetoed = json.load(open(VETO)) if os.path.exists(VETO) else {}
for c in sorted(vetoed):
    G.pop(c, None)

# And a layout the walk got wrong in a way a session could name exactly is
# written out by hand rather than left to it.  Each fixup carries why, so the
# next walk can be checked against the reason rather than the result.
FIX = os.path.join(HERE, "ghost_fixups.json")
fixups = json.load(open(FIX)) if os.path.exists(FIX) else {}
for c, g in fixups.items():
    G[c] = g

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
        elif n[0] == "br":
            kind, bits, slot, z, nz = n[1], n[2], n[3], n[4], n[5]
            b = 0 if kind in ("readSTString", "readString") else (bits or 0)
            b = min(b, 32)
            base = len(sel)
            sel.append(z if z is not None else -1)
            sel.append(nz if nz is not None else -1)
            body.append("      { OpReadBr, %d, %d, %d }," % (b, base, slot))
        elif n[0] == "count":
            kind, bits, slot, nxt = n[1], n[2], n[3], n[4]
            b = 0 if kind in ("readSTString", "readString") else (bits or 0)
            b = min(b, 32)
            body.append("      { OpCount, %d, %d, %d },"
                        % (b, nxt if nxt is not None else -1, slot))
        elif n[0] == "flag":
            body.append("      { OpFlag, 1, %d, %d },"
                        % (n[1] if n[1] is not None else -1,
                           n[2] if n[2] is not None else -1))
        elif n[0] == "initial":
            # Not a bit off the wire: which kind of update this is.
            body.append("      { OpInitial, 0, %d, %d },"
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
            b = min(b, 32)
            body.append("      { %s, %d, %d, -1 },"
                        % (OPS.get(kind, "OpRaw"), b,
                           nxt if nxt is not None else -1))
    if sel:
        A("   static const S16 sT2RcSel_%s[] = { %s };"
          % (cls, ", ".join(str(x) for x in sel)))
    note = ("exact" if not e.get("approx")
            else "%d call%s stepped over" % (e["approx"],
                                             "" if e["approx"] == 1 else "s"))
    if e.get("wide"):
        note += ", walked with the object's own counters in the key"
    A("   /// %s: %d nodes from %d explored, %s"
      % (cls, len(e["nodes"]), e.get("explored", 0), note))
    A("   static const T2DataNode sT2RcNodes_%s[] =" % cls)
    A("   {")
    L.extend(body)
    A("   };")
    A("")

A("   static const T2DataBlockLayout sT2GhostsRecovered[] =")
A("   {")
for cls in names:
    e = G[cls]
    has = any(n[0] in ("sel", "br") for n in e["nodes"])
    A('      { "%s", sT2RcNodes_%s, %d, %s, %d },'
      % (cls, cls, len(e["nodes"]),
         ("sT2RcSel_%s" % cls) if has else "NULL", e["root"]))
A("   };")
A("}")
A("")
A("#endif")

open(OUT, "w").write("\n".join(L) + "\n")
print("wrote %s: %d classes, %d nodes"
      % (OUT, len(names), sum(len(G[c]["nodes"]) for c in names)))
print("exact: %s" % " ".join(c for c in names if not G[c].get("approx")))
print("dropped for having a cycle, so a counted loop that did not unroll: %s"
      % " ".join(cyclic))
