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

TOOLS = os.environ.get("T2_TOOLS",
    os.path.expanduser("~/Documents/Repositories/BananaEngine/tools/t2"))
META = os.environ.get("T2_META",
    os.path.expanduser("~/Documents/Repositories/Tribes2Decompiled/metadata"))
EXE = os.environ.get("TRIBES2_EXE", os.path.expanduser(
    "~/.wine/drive_c/Dynamix/Tribes2/GameData/Tribes2.exe"))
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
    return (unpack_update_vt(name) or (None, 0))[0]


def unpack_update_vt(name):
    """Its entry and the vtable it came from, which the walk seats in the
    object so a virtual call to the parent's unpackUpdate resolves."""
    for vt in vtable_of(name) or []:
        fn = P.u32(vt + 4 * 19)
        if fn and P.off(fn) is not None:
            return fn, vt
    return None


def has_cycle(nodes, root):
    """A counted loop that did not unroll shows up as a back edge.

    Such a graph reads whichever way out of the cycle the bits happen to
    steer, which is how a body of six hundred bits comes out as thirty-six.
    """
    WHITE, GREY = 0, 1
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

    if not (0 <= root < len(nodes)):
        return False
    colour[root] = GREY
    stack = [(root, iter(succ(root)))]
    while stack:
        i, it = stack[-1]
        nxt = next(it, None)
        if nxt is None:
            colour[i] = 2
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


# ------------------------------------------------------------------ the walk
# The cap counts states explored, not nodes kept: ShapeBase explores 195,516 of
# them and compacts to 1,032.  So it has to be loose enough to let a real class
# through and still catch a walk that is going nowhere.
CAP = int(os.environ.get("T2_CAP", 8000000))


def walk_one(entry, widefork, budget=600, cap=None, wide_counters=False,
             vtable=0):
    cap = CAP if cap is None else cap
    mark_pure(entry)
    flat.APPROX_INDIRECT = True
    flat.APPROXIMATED.clear()
    w = flat.Walk(SPANS)
    w.budget = budget
    w.wide_counters = wide_counters
    if not widefork:
        w.no_widefork = True
    prog, root = w.build(entry, cap=cap, nargs=2, vtable=vtable)
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


def owner_of(pc):
    """The function start `pc` falls in, taken from the symbol spans."""
    best = None
    for va in SPANS:
        if va <= pc and (best is None or va > best):
            best = va
    return best


def walk_escalating(fn, widefork, wide, vt, tries=6):
    """Walk, and on a runaway step over whatever was spinning and try again.

    Two hundred thousand instructions between one stream read and the next is
    itself proof the function is not reading: Tribes 2's printf lands there
    formatting an assert message, and SeekerProjectile's steering loop runs a
    trajectory search.  Neither can move the bit position, so stepping over
    the one that spun is sound, and it is the only way past a helper whose
    indirect call stops `touches_stream` from ruling it out up front.
    """
    stepped = []
    for _ in range(tries):
        # The profiler that names the spinning pc costs a dict update on every
        # instruction, which is far too dear to leave on: AIObjective walks in
        # eleven seconds without it and overran a ten minute budget with it.
        # So the fast path runs blind, and only a walk that has already run
        # away is run again to find out where it did.
        flat.RUNAWAY_DIAG = False
        try:
            return walk_one(fn, widefork, wide_counters=wide, vtable=vt), stepped
        except RuntimeError as e:
            if not ("%s" % e).startswith("runaway"):
                raise
        flat.RUNAWAY_DIAG = True
        try:
            walk_one(fn, widefork, wide_counters=wide, vtable=vt)
            raise RuntimeError("runaway that did not repeat")
        except RuntimeError as e:
            m = re.match(r"runaway at 0x([0-9a-f]+) hot 0x([0-9a-f]+):", "%s" % e)
            if not m:
                raise
            owner = owner_of(int(m.group(2), 16))
            if owner is None or owner in emu.NOOP or owner == fn:
                raise
            emu.NOOP.add(owner)
            stepped.append(owner)
        finally:
            flat.RUNAWAY_DIAG = False
    raise RuntimeError("runaway after stepping over %s"
                       % " ".join("0x%x" % x for x in stepped))


def report(name):
    fnvt = unpack_update_vt(name)
    fn, vt = fnvt if fnvt else (None, 0)
    if not fn:
        print("%-22s no unpackUpdate found" % name)
        return None
    result = {}
    # A first pass with the narrow key, which merges well; then, only if its
    # graph came back with a cycle in it, a second with the object's own
    # counters in the key as well.
    for wide in (False, True):
        for widefork in (True, False):
            try:
                t = time.time()
                (prog, root), spun = walk_escalating(fn, widefork, wide, vt)
                result = {"entry": fn, "nodes": len(prog.nodes), "root": root,
                          "widefork": widefork, "secs": round(time.time() - t, 1),
                          "prog": prog, "rootidx": root, "wide": wide,
                          "spun": len(spun),
                          "approx": len(flat.APPROXIMATED)}
                break
            except Exception as e:
                result = {"entry": fn, "error": "%s" % e, "widefork": widefork}
        if "prog" not in result:
            continue
        g = buildall.compact(result["prog"].nodes, result["rootidx"])
        if not has_cycle(g["nodes"], g["root"]):
            result["graph"] = g
            break
        result["graph"] = g          # keep it, but try the wider key next
    if "prog" in result:
        result["approx"] = len(flat.APPROXIMATED)
        cyc = has_cycle(result["graph"]["nodes"], result["graph"]["root"]) \
            if result.get("graph") else True
        print("%-22s 0x%06x  %6d nodes %5.1fs  %-22s %s%s"
              % (name, fn, result["nodes"], result["secs"],
                 "%d calls stepped over" % result["approx"]
                 if result["approx"] else "exact",
                 "wide key, " if result.get("wide") else "",
                 "CYCLIC" if cyc else "acyclic"))
    else:
        print("%-22s 0x%06x  %s" % (name, fn, result["error"]))
    return result


def all_names():
    src = open(os.path.join(
        OPENT, "Engine/source/console/netClassTableT2.cpp"),
        encoding="latin1").read()
    m = re.search(r"const char \*const Objects\[\]\s*=\s*\{(.*?)\};", src, re.S)
    return re.findall(r'"([A-Za-z0-9_]+)"', m.group(1))


def one(name):
    """Walk a single class and return its compacted graph, or None."""
    r = report(name)
    if not r or not r.get("graph"):
        return None
    g = r["graph"]
    g["approx"] = r.get("approx", 0)
    g["explored"] = r["nodes"]
    g["wide"] = r.get("wide", False)
    g["cyclic"] = has_cycle(g["nodes"], g["root"])
    return g


def drive(names, seconds):
    """Walk each class in a child of its own, on a clock.

    The wide key merges so much less that a class which merely ran out of
    steps under the narrow one can run for hours under it.  A child that
    overruns is killed and said so, which is more use than a walk that never
    returns.
    """
    here = os.path.abspath(__file__)
    out = {}
    for n in names:
        try:
            r = subprocess.run([sys.executable, "-u", here, "--json", n],
                               capture_output=True, text=True, timeout=seconds)
        except subprocess.TimeoutExpired:
            print("%-22s gave up after %ds" % (n, seconds), flush=True)
            continue
        line = [l for l in r.stdout.splitlines() if l.startswith("{")]
        note = [l for l in r.stdout.splitlines() if not l.startswith("{")]
        for l in note:
            print(l, flush=True)
        if line:
            out[n] = json.loads(line[-1])
    return out


if __name__ == "__main__":
    args = [a for a in sys.argv[1:]]

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

    seconds = 150
    if args and args[0].startswith("--seconds="):
        seconds = int(args[0].split("=")[1])
        args = args[1:]
    if not args or args == ["--all"]:
        args = all_names()

    graphs = drive(args, seconds)
    print()
    print("%d of %d classes walked; %d nodes after compacting; %d cyclic"
          % (len(graphs), len(args),
             sum(len(v["nodes"]) for v in graphs.values()),
             sum(1 for v in graphs.values() if v.get("cyclic"))))
    json.dump(graphs, open(os.path.join(
        os.path.dirname(os.path.abspath(__file__)), "ghost_graphs.json"), "w"))
