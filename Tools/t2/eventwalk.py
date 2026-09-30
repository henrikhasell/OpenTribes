#!/usr/bin/env python3
"""Recover the bit layout of every Tribes 2 net event by walking its unpack.

    eventwalk.py [--json NAME] [--seconds=N] [class ...]

The object walk in ghostwalk.py finds unpackUpdate through the class's vtable;
an event's unpack sits four slots past its class-rep pointer instead, and that
pointer is what the .data vtable is found by.  Everything after that -- the
flat x86 walk, the memo key, the acyclicity gate -- is shared with the object
walk, so a layout recovered here is judged the same way.
"""
import json, os, re, subprocess, sys, threading, time

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.environ.get(
    "T2_TOOLS", os.path.expanduser(
        "~/Documents/Repositories/BananaEngine/tools/t2")))
sys.path.insert(0, HERE)

import ghostwalk as G
import buildall
import emu
import flat
import struct
import csv

META = os.environ.get("T2_META", os.path.expanduser(
    "~/Documents/Repositories/Tribes2Decompiled/metadata"))

P = emu.P
_rows = {r["class"]: r for r in csv.DictReader(
    open(os.path.join(META, "classes_resolved.tsv")), delimiter="\t")}


def vtable_of_event(name):
    """The .data word holding the class's getClassRep, which is its vtable."""
    gcr = _rows.get(name, {}).get("get_class_rep_va")
    if not gcr:
        return None
    needle = struct.pack("<I", int(gcr, 16))
    i = 0
    while True:
        i = P.d.find(needle, i)
        if i < 0:
            return None
        for nm, vaddr, vsize, rawptr, rawsize in P.secs:
            if nm == ".data" and rawptr <= i < rawptr + rawsize:
                return P.image_base + vaddr + (i - rawptr)
        i += 1


def unpack_of(name):
    vt = vtable_of_event(name)
    if vt is None:
        return None, 0
    fn = P.u32(vt + 16)
    return (fn, vt) if fn and P.off(fn) is not None else (None, 0)


def one(name):
    fn, vt = unpack_of(name)
    if not fn:
        print("%-30s no unpack found" % name)
        return None
    result = {}
    for wide in (False, True):
        for widefork in (True, False):
            try:
                t = time.time()
                G.mark_pure(fn)
                (prog, root), _ = G.walk_escalating(fn, widefork, wide, vt)
                result = {"entry": fn, "nodes": len(prog.nodes),
                          "prog": prog, "rootidx": root, "wide": wide,
                          "secs": round(time.time() - t, 1),
                          "approx": len(flat.APPROXIMATED)}
                break
            except Exception as e:
                result = {"entry": fn, "error": "%s" % e}
        if "prog" not in result:
            continue
        g = buildall.compact(result["prog"].nodes, result["rootidx"])
        result["graph"] = g
        if not G.has_cycle(g["nodes"], g["root"]):
            break
    if "graph" not in result:
        print("%-30s 0x%06x  %s" % (name, fn, result.get("error")))
        return None
    g = result["graph"]
    g["approx"] = result.get("approx", 0)
    g["explored"] = result["nodes"]
    g["wide"] = result.get("wide", False)
    g["cyclic"] = G.has_cycle(g["nodes"], g["root"])
    print("%-30s 0x%06x  %5d nodes %5.1fs  %-20s %s"
          % (name, fn, len(g["nodes"]), result["secs"],
             "%d calls stepped over" % g["approx"] if g["approx"] else "exact",
             "CYCLIC" if g["cyclic"] else "acyclic"))
    return g


def all_names():
    src = open(os.path.join(
        G.OPENT, "Engine/source/console/netClassTableT2.cpp"),
        encoding="latin1").read()
    m = re.search(r"const char \*const Events\[\]\s*=\s*\{(.*?)\};", src, re.S)
    return re.findall(r'"([A-Za-z0-9_]+)"', m.group(1))


def drive(names, seconds):
    out = {}
    for n in names:
        try:
            r = subprocess.run([sys.executable, "-u", os.path.abspath(__file__),
                                "--json", n],
                               capture_output=True, text=True, timeout=seconds)
        except subprocess.TimeoutExpired:
            print("%-30s gave up after %ds" % (n, seconds), flush=True)
            continue
        for l in r.stdout.splitlines():
            if l.startswith("{"):
                out[n] = json.loads(l)
            else:
                print(l, flush=True)
    return out


if __name__ == "__main__":
    args = sys.argv[1:]
    if args and args[0] == "--json":
        name = args[1]

        def emit():
            g = one(name)
            if g is not None:
                print(json.dumps(g))

        th = threading.Thread(target=emit)
        th.start()
        th.join()
        raise SystemExit

    seconds = 300
    if args and args[0].startswith("--seconds="):
        seconds = int(args[0].split("=")[1])
        args = args[1:]
    if not args:
        args = all_names()
    graphs = drive(args, seconds)
    print()
    print("%d of %d events walked; %d nodes after compacting; %d cyclic"
          % (len(graphs), len(args),
             sum(len(v["nodes"]) for v in graphs.values()),
             sum(1 for v in graphs.values() if v.get("cyclic"))))
    json.dump(graphs, open(os.path.join(HERE, "event_graphs.json"), "w"))
