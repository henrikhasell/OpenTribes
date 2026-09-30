#!/usr/bin/env python3
"""Recover a Tribes 2 object's ghost update layout.

The shipped ghost tables came from a walk that could not step into a class's
parent unpackUpdate, so an object whose reading lives below that call came out
with a stub graph -- AudioEmitter takes 37 bits where its neighbours take 660.
This drives the same emulator with every function it might enter already in
its map.

    ghostwalk.py <Class> [more...]
    ghostwalk.py --all
"""
import csv, json, os, re, subprocess, sys, threading, time

sys.setrecursionlimit(2000000)
threading.stack_size(4096 * 1024 * 1024)

TOOLS = "/home/henrik/Documents/Repositories/BananaEngine/tools/t2"
META = "/home/henrik/Documents/Repositories/Tribes2Decompiled/metadata"
EXE = "/home/henrik/.wine/drive_c/Dynamix/Tribes2/GameData/Tribes2.exe"
OPENT = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
sys.path.insert(0, TOOLS)

import buildall                                              # noqa: E402
import pe                                                    # noqa: E402
import emu                                                   # noqa: E402
import flat                                                  # noqa: E402

P = pe.PE(EXE)

# ---------------------------------------------------------------- the map
# Every function the walk may enter, with the span to disassemble.  A call to
# anything outside this halts the walk, and a jump inside a function that was
# loaded with too small a span lands on an unmapped pc -- so the spans come
# from the distance to the next function, and the last gets a generous tail.
starts = sorted(set(int(r["va"], 16) for r in csv.DictReader(
    open(os.path.join(META, "function_sources.tsv")), delimiter="\t")))
SPANS = {}
for i, s in enumerate(starts):
    nxt = starts[i + 1] if i + 1 < len(starts) else s + 0x800
    SPANS[s] = min(max(nxt - s, 0x40), 0x4000)

CLASSES = {r["class"]: r for r in csv.DictReader(
    open(os.path.join(META, "classes_resolved.tsv")), delimiter="\t")}


# Helpers checked by hand to read nothing: hand-written vector maths, some of
# it in MMX, which the emulator has no model for.  Stepping over them is sound
# because they cannot move the stream's bit position.
for _pure in (0x425fb0, 0x425d30, 0x58c2f0, 0x439570, 0x437ca0, 0x442d60):
    emu.NOOP.add(_pure)


# ------------------------------------------------------- what never reads
# A function that cannot reach a stream read is one the walk should step over
# rather than emulate: its internals are hand-written vector maths, MMX and
# the like, and halting on an instruction in there loses the whole class.
_touch = {}


def touches_stream(va, depth=6):
    if va in _touch:
        return _touch[va]
    if va in emu.STREAM or va in flat.INLINE:
        return True
    _touch[va] = False                      # a call back on itself reads nothing new
    try:
        ins = emu.disasm(va, SPANS.get(va, 0x400))
    except Exception:
        _touch[va] = True                   # cannot tell, so do not skip it
        return True
    hit = False
    for a in sorted(ins):
        mn, ops, _ = ins[a]
        if mn == "call" and ops.startswith("0x"):
            t = int(ops.split()[0], 16)
            if t in emu.STREAM or t in flat.INLINE:
                hit = True
                break
            if depth > 0 and t != va and touches_stream(t, depth - 1):
                hit = True
                break
        elif mn == "call":
            hit = True                      # an indirect call could be a read
            break
        elif mn == "setne" and any(
                emu.disasm(va, SPANS.get(va, 0x400)).get(b, ("", "", 0))[0] == "incl"
                for b in (a + 3, a + 4, a + 5)):
            hit = True
            break
    if hit:
        _touch[va] = True
    return _touch[va]


def mark_pure(entry):
    """Add every callee below `entry` that never reads the stream to NOOP."""
    seen, stack, added = set(), [entry], 0
    while stack:
        va = stack.pop()
        if va in seen:
            continue
        seen.add(va)
        try:
            ins = emu.disasm(va, SPANS.get(va, 0x400))
        except Exception:
            continue
        for a in sorted(ins):
            mn, ops, _ = ins[a]
            if mn != "call" or not ops.startswith("0x"):
                continue
            t = int(ops.split()[0], 16)
            if t in emu.STREAM or t in flat.INLINE or t in emu.NOOP or t in emu.PURE:
                continue
            if not touches_stream(t):
                emu.NOOP.add(t)
                added += 1
            else:
                stack.append(t)
    return added


def vtable_of(name):
    """The table a class's create() seats at offset nought."""
    rep = CLASSES[name]["classrep_vtable"]
    if not rep:
        return None
    create = P.u32(int(rep, 16) + 4)
    if not create:
        return None

    def seats(fn, span=0x400):
        out = subprocess.run(["r2", "-q", "-e", "scr.color=0", "-c",
                              "pD %d @ 0x%x" % (span, fn), EXE],
                             capture_output=True, text=True).stdout
        seen, ctor = [], None
        for line in out.splitlines():
            m = re.search(r"0x([0-9a-f]+)\s+[0-9a-f.]+\s+(.*)", line)
            if not m:
                continue
            ins = m.group(2).split(";")[0].strip()
            if re.match(r"ret", ins):
                break
            mm = re.match(r"mov dword \[e[a-z]{2}(?: \+ 0)?\], (0x[0-9a-f]+)$", ins)
            if mm:
                v = int(mm.group(1), 16)
                if 0x700000 <= v < 0x7e0000 and v not in seen:
                    seen.append(v)
            mm = re.match(r"call 0x([0-9a-f]+)", ins)
            if mm and int(mm.group(1), 16) != 0x401fe0:
                ctor = int(mm.group(1), 16)
        return seen, ctor

    seen, ctor = seats(create)
    if not seen and ctor:
        seen, _ = seats(ctor)
    return seen


def unpack_update(name):
    """Slot 19 of the table whose slot 19 reads the stream."""
    for vt in vtable_of(name) or []:
        fn = P.u32(vt + 4 * 19)
        if fn and P.off(fn) is not None:
            return fn
    return None


# ------------------------------------------------------------------ the walk
def walk_one(entry, widefork, budget=600, cap=40000000):
    mark_pure(entry)
    flat.APPROX_INDIRECT = True
    flat.APPROXIMATED.clear()
    w = flat.Walk(SPANS)
    w.budget = budget
    if not widefork:
        w.no_widefork = True
    prog, root = w.build(entry, cap=cap, nargs=2)     # (connection, stream)
    return prog, root


def min_bits(nodes, root):
    """The fewest bits any path through the graph takes, as a sanity figure."""
    memo = {}

    def cost(i, depth=0):
        if i is None or depth > 400:
            return 0
        if i in memo:
            return memo[i]
        memo[i] = 0
        op = nodes[i][0]
        succ = nodes[i][1]
        if op[0] == "end":
            return 0
        bits = 0
        if op[0] == "read":
            bits = op[1] or 0
        nxt = [s for s in succ if s is not None]
        best = min((cost(s, depth + 1) for s in nxt), default=0)
        memo[i] = bits + best
        return memo[i]

    return cost(root)


def report(name):
    fn = unpack_update(name)
    if not fn:
        print("%-22s no unpackUpdate found" % name)
        return None
    result = {}
    for widefork in (True, False):
        try:
            t = time.time()
            prog, root = walk_one(fn, widefork)
            result = {"entry": fn, "nodes": len(prog.nodes), "root": root,
                      "widefork": widefork, "secs": round(time.time() - t, 1),
                      "prog": prog, "rootidx": root}
            break
        except Exception as e:
            result = {"entry": fn, "error": "%s" % e, "widefork": widefork}
    if "prog" in result:
        result["approx"] = len(flat.APPROXIMATED)
        print("%-22s 0x%06x  %6d nodes  widefork=%-5s %5.1fs  %s"
              % (name, fn, result["nodes"], result["widefork"], result["secs"],
                 "%d calls stepped over" % result["approx"]
                 if result["approx"] else "exact"))
    else:
        print("%-22s 0x%06x  %s" % (name, fn, result["error"]))
    return result


if __name__ == "__main__":
    args = [a for a in sys.argv[1:]]
    if args == ["--all"]:
        src = open(os.path.join(
            OPENT, "Engine/source/console/netClassTableT2.cpp"), encoding="latin1").read()
        m = re.search(r"const char \*const Objects\[\]\s*=\s*\{(.*?)\};", src, re.S)
        args = re.findall(r'"([A-Za-z0-9_]+)"', m.group(1))

    out = {}

    def go():
        ok = 0
        for n in args:
            if n not in CLASSES:
                print("%-22s not in the decompilation metadata" % n)
                continue
            r = report(n)
            if r and "prog" in r:
                g = buildall.compact(r["prog"].nodes, r["rootidx"])
                g["approx"] = r.get("approx", 0)
                g["explored"] = r["nodes"]
                out[n] = g
                ok += 1
        print()
        print("%d of %d classes walked; %d nodes after compacting"
              % (ok, len(args), sum(len(v["nodes"]) for v in out.values())))
        json.dump(out, open(os.path.join(
            os.path.dirname(os.path.abspath(__file__)), "ghost_graphs.json"), "w"))

    th = threading.Thread(target=go)
    th.start()
    th.join()
