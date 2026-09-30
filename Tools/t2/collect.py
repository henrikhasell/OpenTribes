#!/usr/bin/env python3
"""Gather what the tree and a recorded session say about each net class.

Writes coverage.json for the spreadsheet builder.  Everything here is read
out of the repository or measured off the capture; nothing is asserted from
memory.
"""
import json, os, re, struct, subprocess, sys

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
GB = os.path.join(ROOT, "Engine/source/T3D/gameBase")
SRC = os.path.join(ROOT, "Engine/source")
HERE = os.path.dirname(os.path.abspath(__file__))
CAP = os.environ.get("T2_CAPTURE",
                     os.path.join(HERE, "stock.capture"))


def slurp(p):
    return open(p, encoding="latin1").read()


# ------------------------------------------------------------- the class table
tab = slurp(os.path.join(SRC, "console/netClassTableT2.cpp"))


def names(which):
    m = re.search(r"const char \*const %s\[\]\s*=\s*\{(.*?)\};" % which, tab, re.S)
    return re.findall(r'"([A-Za-z0-9_]+)"', m.group(1))


OBJECTS, DATABLOCKS, EVENTS = names("Objects"), names("DataBlocks"), names("Events")

aliases = dict(re.findall(r'\{ "(\w+)", "(\w+)" \}', tab))          # port -> t2
rev_alias = {v: k for k, v in aliases.items()}


# ----------------------------------------------------------------- the layouts
def layout_counts(header, table):
    src = slurp(os.path.join(GB, header))
    out = {}
    for name, arr, cnt, sel, root in re.findall(
            r'\{\s*"([^"]+)",\s*(\w+),\s*(\d+),\s*(\w+),\s*(-?\d+)\s*\}',
            re.search(r"%s\[\]\s*=\s*\{(.*?)\};" % table, src, re.S).group(1)):
        out[name] = int(cnt)
    return out


DBL = layout_counts("t2DataBlockTables.h", "sT2DataBlocks")
GHL = layout_counts("t2GhostTables.h", "sT2Ghosts")
EVL = layout_counts("t2EventTables.h", "sT2Events")
PDL = layout_counts("t2PacketDataTables.h", "sT2PacketData")


# ------------------------------------------------------- what the port declares
def declared(pattern):
    hit = subprocess.run(["grep", "-rhoE", pattern, SRC, "--include=*.cpp"],
                         capture_output=True, text=True).stdout
    return set(re.findall(r"\(\s*(\w+)", hit))


port_db = declared(r"IMPLEMENT_CO_DATABLOCK_V1\(\s*\w+")
port_obj = declared(r"IMPLEMENT_CO_NETOBJECT_V1\(\s*\w+")
# which way an event travels, from the macro the port registers it with.
# grep -E has no non-capturing group, so each macro is asked for separately.
direction = {}
for macro, way in (("CLIENTEVENT", "server to client"),
                   ("SERVEREVENT", "client to server"),
                   ("NETEVENT", "either way")):
    for n in declared(r"IMPLEMENT_CO_%s_V1\(\s*\w+" % macro):
        direction[n] = way

# the port registers a class with one of those three macros, so whatever the
# direction pass found is also the set of classes it has
port_ev = set(direction)
stub_db = declared(r"IMPLEMENT_DATABLOCK_T2\(\s*\w+")
stub_obj = declared(r"IMPLEMENT_GHOST_T2\(\s*\w+")
stub_ev = declared(r"IMPLEMENT_EVENT_T2\(\s*\w+")

# the events the port unpacks itself
ev_src = slurp(os.path.join(GB, "eventsT2.cpp"))
byhand = set(re.findall(r'"(\w+)"',
                        re.search(r"sByHand\[\]\s*=\s*\{(.*?)\};", ev_src, re.S).group(1)))

# the datablocks whose own reader still runs (a T2 layout is preferred when one
# exists, so this is only the fallback path)
shapebase_native = "ShapeBaseData" in port_db


# --------------------------------------------------------- measure the capture
def measure():
    src = slurp(os.path.join(HERE, "replay.py")).split("if __name__ ==")[0]
    g = {"__name__": "collector", "__file__": os.path.join(HERE, "replay.py")}
    exec(compile(src, "replay.py", "exec"), g)
    Stream, Session = g["Stream"], g["Session"]
    walk, read_body, read_events = g["walk"], g["read_body"], g["read_events"]
    GHLL, OBJ = g["GHL"], g["OBJECTS"]
    GhostIdBitSize = g["GhostIdBitSize"]
    GhostIndexBitSize = g["GhostIndexBitSize"]
    ObjectBits = g["ObjectBits"]

    data = open(CAP, "rb").read()
    S = Session()
    db_seen, ev_seen, gh_seen, ga_seen = {}, {}, {}, {}
    gh_bits = {}
    off = pkt = dpkt = 0
    fails = []

    # the datablock and event tallies come out of read_events, so hook the
    # session's counters by re-reading its own dictionaries afterwards
    while off + 4 <= len(data):
        n, = struct.unpack_from("<I", data, off)
        off += 4
        if n == 0 or off + n > len(data):
            break
        p = data[off:off + n]
        off += n
        pkt += 1
        if not (p[0] & 1):
            continue
        st = Stream(p)
        try:
            st.flag(); st.rint(1); st.rint(9); st.rint(9)
            t = st.rint(2); abc = st.rint(3); st.rint(8 * abc)
            if t != 0:
                continue
            dpkt += 1
            if st.flag():
                st.rint(10); st.rint(10)
            if st.flag():
                st.rint(10); st.rint(10)
            read_body(st, S)
            before_db = dict(S.events)
            read_events(st, S)
            if not st.flag():
                continue
            idsize = st.rint(GhostIndexBitSize) + 3
            while st.flag():
                index = st.rint(idsize)
                if st.flag():
                    S.ghosts.pop(index, None)
                    continue
                if index not in S.ghosts:
                    cid = st.rint(ObjectBits)
                    if cid >= len(OBJ):
                        raise RuntimeError("ghost class id %d out of range" % cid)
                    S.ghosts[index] = OBJ[cid]
                name = S.ghosts[index]
                a = st.p
                walk(st, GHLL[name], name)
                gh_seen[name] = gh_seen.get(name, 0) + 1
                gh_bits.setdefault(name, []).append(st.p - a)
        except Exception as e:
            fails.append("packet %d: %s" % (pkt, e))
    return {
        "packets": pkt, "data_packets": dpkt,
        "datablocks": S.datablocks,
        "events": S.events,
        "ghost_always": S.ghost_always,
        "ghost_stream": gh_seen,
        "ghost_bits": {k: [min(v), max(v)] for k, v in gh_bits.items()},
        "live": {k: sum(1 for x in S.ghosts.values() if x == k)
                 for k in set(S.ghosts.values())},
        "shapes": len(S.shapes),
        "control": S.control,
        "fails": fails,
    }


cap = measure() if os.path.exists(CAP) else None

json.dump({
    "objects": OBJECTS, "datablocks": DATABLOCKS, "events": EVENTS,
    "aliases": aliases,
    "layout_db": DBL, "layout_gh": GHL, "layout_ev": EVL, "layout_pd": PDL,
    "port_db": sorted(port_db), "port_obj": sorted(port_obj),
    "port_ev": sorted(port_ev),
    "stub_db": sorted(stub_db), "stub_obj": sorted(stub_obj),
    "stub_ev": sorted(stub_ev),
    "byhand": sorted(byhand),
    "direction": direction,
    "capture": cap,
}, open(os.environ.get("T2_COVERAGE",
                        os.path.join(HERE, "coverage.json")), "w"), indent=1)

print("objects %d datablocks %d events %d" % (len(OBJECTS), len(DATABLOCKS), len(EVENTS)))
print("layouts: db %d gh %d ev %d pd %d" % (len(DBL), len(GHL), len(EVL), len(PDL)))
if cap:
    print("capture: %d packets, %d datablocks, %d ghost always, %d ghost updates, %d failures"
          % (cap["packets"], cap["datablocks"], cap["ghost_always"],
             sum(cap["ghost_stream"].values()), len(cap["fails"])))
