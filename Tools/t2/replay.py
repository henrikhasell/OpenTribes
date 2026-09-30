#!/usr/bin/env python3
"""Replay a captured Tribes 2 session through the layouts OpenTribes ships.

    replay.py [capture]

Reads the generated headers and the class table straight out of the tree, so
what is tested here is what the engine uses.  No server: the same bytes give
the same answer every run, which is what makes a change to the reader
judgeable.  A live server cycles missions and how far a decode gets varies
between boots.

Modelled on BananaEngine's tools/t2/replay.py, extended to walk the ghost
stream and the packet body as GameConnection::readPacket reads them.
"""
import os, re, struct, sys

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
GB   = os.path.join(ROOT, "Engine/source/T3D/gameBase")
EXE  = os.environ.get("TRIBES2_EXE", os.path.expanduser(
    "~/.wine/drive_c/Dynamix/Tribes2/GameData/Tribes2.exe"))

ObjectBits, DataBlockBits, EventBits = 7, 7, 6
GhostIdBitSize, GhostIndexBitSize = 10, 3
TargetWordBits = 4
DataBlockObjectIdBitSize = 11


# ------------------------------------------------------------------ the tables
def slurp(path):
    return open(path, encoding="latin1").read()


def parse_layouts(src, table):
    """name -> (nodes, select, root); a node is (op, bits, a, b)."""
    nodes = {}
    # The prefix says which table a node array belongs to, and there are now
    # several -- Ev, Gh, Pd, Rc, Er, Pr.  Matching any short one rather than a
    # list of them stops a new table being silently skipped: the recovered
    # event layouts were, and the correction in them never took effect.
    for m in re.finditer(r"sT2[A-Za-z]{0,3}Nodes_(\w+)\[\]\s*=\s*\{(.*?)\};", src, re.S):
        nodes[m.group(1)] = [
            (r[0], int(r[1]), int(r[2]), int(r[3]))
            for r in re.findall(
                r"\{\s*(\w+),\s*(-?\d+),\s*(-?\d+),\s*(-?\d+)\s*\}", m.group(2))]
    sel = {}
    for m in re.finditer(r"sT2[A-Za-z]{0,3}Sel_(\w+)\[\]\s*=\s*\{([^}]*)\};", src):
        sel[m.group(1)] = [int(x) for x in m.group(2).split(",") if x.strip()]
    out = {}
    m = re.search(r"%s\[\]\s*=\s*\{(.*?)\};" % table, src, re.S)
    for name, arr, cnt, selname, root in re.findall(
            r'\{\s*"([^"]+)",\s*(\w+),\s*(\d+),\s*(\w+),\s*(-?\d+)\s*\}', m.group(1)):
        key = arr.split("Nodes_")[-1]
        out[name] = (nodes[key], sel.get(key, []), int(root))
    return out


def parse_class_table(name):
    src = slurp(os.path.join(ROOT, "Engine/source/console/netClassTableT2.cpp"))
    m = re.search(r"const char \*const %s\[\]\s*=\s*\{(.*?)\};" % name, src, re.S)
    return re.findall(r'"([A-Za-z0-9_]+)"', m.group(1))


DBL = parse_layouts(slurp(os.path.join(GB, "t2DataBlockTables.h")), "sT2DataBlocks")
GHL = parse_layouts(slurp(os.path.join(GB, "t2GhostTables.h")), "sT2Ghosts")
def overlay(into, path, table):
    """Merge a generated table over another, loudly.  Swallowing the failure
    here hid a whole table once: the recovered events parsed to nothing
    because their node arrays carry a prefix the pattern did not know, and the
    correction in them silently never applied."""
    p = os.path.join(GB, path)
    if not os.path.exists(p):
        return
    got = parse_layouts(slurp(p), table)
    if not got:
        raise RuntimeError("%s parsed to no layouts at all" % path)
    into.update(got)


# the recovered layouts take precedence, as they do in the engine
overlay(GHL, "t2GhostTablesRecovered.h", "sT2GhostsRecovered")
EVL = parse_layouts(slurp(os.path.join(GB, "t2EventTables.h")), "sT2Events")
# the written-out event layouts take precedence, as they do in the engine
overlay(EVL, "t2EventTablesRecovered.h", "sT2EventsRecovered")
PDL = parse_layouts(slurp(os.path.join(GB, "t2PacketDataTables.h")), "sT2PacketData")
# the written-out control-object layouts take precedence, as they do in the engine
overlay(PDL, "t2PacketDataTablesRecovered.h", "sT2PacketDataRecovered")

OBJECTS   = parse_class_table("Objects")
DATABLOCKS = parse_class_table("DataBlocks")
EVENTS    = parse_class_table("Events")


# ---------------------------------------------------------------- the huffman
def huff_decode_table():
    """Tribes 2's tree: the engine's frequencies, a letter or digit weighted one
    above the table, which reshapes it."""
    b = open(EXE, "rb").read()
    freqs = list(struct.unpack("<256I", b[0x354bdc:0x354bdc + 1024]))
    pops, wraps = [], []
    for i in range(256):
        alnum = (48 <= i <= 57) or (65 <= i <= 90) or (97 <= i <= 122)
        pops.append(freqs[i] + (1 if alnum else 0) + 1)
        wraps.append(("leaf", i))
    cur = 256
    while cur != 1:
        min1, min2, i1, i2 = 0xfffffffe, 0xffffffff, -1, -1
        for i in range(cur):
            if pops[i] < min1:
                min2, i2 = min1, i1
                min1, i1 = pops[i], i
            elif pops[i] < min2:
                min2, i2 = pops[i], i
        node, pop = ("node", wraps[i1], wraps[i2]), pops[i1] + pops[i2]
        merge, nuke = (i2, i1) if i1 > i2 else (i1, i2)
        wraps[merge], pops[merge] = node, pop
        if i2 != cur - 1:
            wraps[nuke], pops[nuke] = wraps[cur - 1], pops[cur - 1]
        cur -= 1
    out, stack = {}, [(wraps[0], "")]
    while stack:
        n, path = stack.pop()
        if n[0] == "leaf":
            out[path] = n[1]
        else:
            stack.append((n[1], path + "0"))
            stack.append((n[2], path + "1"))
    return out


DEC = huff_decode_table()


# ------------------------------------------------------------- the bit reader
class Stream:
    def __init__(self, data):
        self.d = data
        self.p = 0
        self.strbuf = b""          # the prefix buffer readPacket sets per packet

    def flag(self):
        if self.p >= len(self.d) * 8:
            raise EOFError("past the end of the packet")
        v = (self.d[self.p >> 3] >> (self.p & 7)) & 1
        self.p += 1
        return v

    def rint(self, n):
        v = 0
        for i in range(n):
            v |= self.flag() << i
        return v

    def signed(self, n):
        return self.rint(n) - (1 << (n - 1))

    def huff(self):
        compressed = self.flag()
        n = self.rint(8)
        if not compressed:
            return bytes(self.rint(8) for _ in range(n))
        out, cur = [], ""
        while len(out) < n:
            cur += str(self.flag())
            if cur in DEC:
                out.append(DEC[cur])
                cur = ""
            elif len(cur) > 24:
                raise RuntimeError("huffman code ran away")
        return bytes(out)

    def rs(self):
        """BitStream::readString, which shares a prefix with the last string."""
        if self.flag():
            off = self.rint(8)
            tail = self.huff()
            self.strbuf = self.strbuf[:off] + tail
        else:
            self.strbuf = self.huff()
        return self.strbuf.decode("latin1")

    def point(self, scale=0.01):
        t = self.rint(2)
        if t == 3:
            return (self.rint(32), self.rint(32), self.rint(32))
        bits = (16, 18, 20, 32)[t]
        return (self.signed(bits), self.signed(bits), self.signed(bits))

    def unpackString(self):
        """NetConnection::unpackString, for a remote command's arguments."""
        code = self.rint(2)
        if code == 0:
            return ""
        if code == 1:
            return self.rs()
        if code == 2:
            return self.rint(DataBlockObjectIdBitSize - 1)
        self.flag()
        if self.flag():
            return self.rint(7)
        if self.flag():
            return self.rint(15)
        return self.rint(31)


# -------------------------------------------------------------- the layouts
TRACE = os.environ.get("T2_TRACE")
KEEP_GOING = bool(os.environ.get("T2_KEEP"))
VERBOSE = bool(os.environ.get("T2_VERBOSE"))

# Tracing the whole session buries the one packet that matters, so a single
# packet number can be asked for by itself.
ONLY = int(os.environ.get("T2_ONLY", 0))
TAILS = []
SESSION = None   # set per packet, for OpPacketDataOf


def walk(st, layout, name, notes=None, initial=False):
    nodes, sel, root = layout
    slots = [0] * 8
    tr = TRACE in (name, "*")

    def run(at, depth):
        guard = 0
        while 0 <= at < len(nodes):
            guard += 1
            if guard > 65536:
                raise RuntimeError("graph loop in %s" % name)
            op, bits, a, b = nodes[at]
            if tr:
                print("    @%-5d %-3d %-14s bits=%-3d a=%-4d b=%-4d"
                      % (st.p, at, op, bits, a, b))
            if op in ("OpEnd", "OpEndBody"):
                return
            if op == "OpFlag":
                at = b if st.flag() else a
                continue
            if op == "OpInitial":
                # Not a bit off the wire: which of the two this update is.
                at = b if initial else a
                continue
            if op == "OpPacketDataOf":
                # A ghost index, then that object's packet data -- whose width
                # is the piloted vehicle's, not this class's.
                idx = st.rint(bits)
                cls = SESSION.ghosts.get(idx) if SESSION else None
                if cls is None:
                    raise RuntimeError(
                        "%s reads the packet data of ghost %d, which we never "
                        "made" % (name, idx))
                if cls not in PDL:
                    raise RuntimeError("no packet data layout for %s" % cls)
                walk(st, PDL[cls], cls, initial=initial)
                at = a
                continue
            if op == "OpSelect":
                at = sel[a + st.rint(bits)]
                continue
            if op == "OpReadBr":
                v = st.rint(bits) if bits else 0
                if b >= 0:
                    slots[b & 7] = v
                at = sel[a + (1 if v else 0)]
                continue
            if op == "OpLoop":
                c = slots[bits & 7]
                if c > 4096:
                    raise RuntimeError("loop count %d in %s" % (c, name))
                for _ in range(c):
                    run(b, depth + 1)
                at = a
                continue
            if op == "OpCount":
                slots[b & 7] = st.rint(bits) if bits else 0
                at = a
                continue
            if op == "OpCountBits":
                # A mask, kept as how many bits stand in it.
                v = st.rint(bits) if bits else 0
                slots[b & 7] = bin(v).count("1")
                at = a
                continue
            if op == "OpString":
                s = st.rs()
                if notes is not None:
                    notes.append(s)
                if tr:
                    print("      -> %r" % s[:48])
            elif op == "OpPoint":
                st.point()
            elif bits:
                st.rint(bits)
            at = a
        raise RuntimeError("node %d out of range in %s" % (at, name))

    run(root, 0)


# --------------------------------------------------------------- the session
class Session:
    """What the client would hold between packets."""

    def __init__(self):
        self.ghosts = {}        # index -> Tribes 2 class name
        self.datablocks = 0
        self.last_db = None
        self.events = {}
        self.ghost_updates = 0
        self.ghost_new = 0
        self.ghost_always = 0
        self.conn_messages = []
        self.commands = []
        self.shapes = []
        self.control = None
        self.seq = -1
        self.seqs = []
        self.gapat = []
        self.new_here = False
        self.pkt = 0


def read_body(st, S):
    """GameConnection::readPacket's client branch."""
    st.rint(32)                                   # move ack

    if st.flag():                                 # damage flash / white out
        if st.flag():
            st.rint(7)
        if st.flag():
            st.rint(7)

    if st.flag():                                 # a pair of indicators
        st.flag()
        st.flag()

    # A three-way block whose meaning is unsettled: read here as the observer
    # camera, and by t2-demo-parser as a seeker's lock.  The widths agree.
    if st.flag():
        if st.flag():
            st.rint(32), st.rint(32), st.rint(32)
        mode = st.rint(2)
        if mode == 1:
            if st.flag():
                st.rint(GhostIdBitSize)
        elif mode == 2:
            st.rint(32), st.rint(32), st.rint(32)

    st.flag()                                     # sensor ping
    st.flag()                                     # sensor jammed

    if st.flag():                                 # control object
        if st.flag():
            gi = st.rint(GhostIdBitSize)
            name = S.ghosts.get(gi)
            S.control = name
            if VERBOSE:
                print("  control object ghost %d = %s" % (gi, name))
            if name is None:
                raise RuntimeError("control object is ghost %d, which we never made" % gi)
            if name not in PDL:
                raise RuntimeError("no packet data layout for %s" % name)
            was = st.p
            walk(st, PDL[name], name)
            if VERBOSE:
                print("  control %s %d bits" % (name, st.p - was))
        else:
            # the compression point, three whole floats
            st.rint(32), st.rint(32), st.rint(32)

    while st.flag():                              # client target words
        st.rint(TargetWordBits)
        st.rint(32)

    if st.flag():                                 # a forced field of view
        st.rint(8)


def read_events(st, S):
    unordered = True
    while True:
        at = st.p
        bit = st.flag()
        if unordered and not bit:
            unordered = False
            bit = st.flag()
        if VERBOSE:
            print("  ev top @%d %s bit=%d"
                  % (at, "unordered" if unordered else "ordered", bit))
        if not unordered and not bit:
            break
        if not unordered:
            # The sequence is either one past the last or spelt out in seven
            # bits.  Following it is what tells a lost event from a packet that
            # merely ends with bits nobody wrote on purpose.
            if st.flag():
                S.seq = (S.seq + 1) & 0x7f
            else:
                S.seq = st.rint(7)
            S.seqs.append(S.seq)
            if S.seqs[-2:-1] and S.seq != ((S.seqs[-2] + 1) & 0x7f) \
                    and S.seq != S.seqs[-2]:
                S.gapat.append((S.pkt, S.seqs[-2], S.seq))

        cid = st.rint(EventBits)
        if cid >= len(EVENTS):
            raise RuntimeError("event class id %d out of range" % cid)
        name = EVENTS[cid]
        S.events[name] = S.events.get(name, 0) + 1
        if VERBOSE:
            print("  event %s @%d" % (name, st.p))

        if name == "SimDataBlockEvent":
            if st.flag():
                st.rint(DataBlockObjectIdBitSize)
                c = st.rint(DataBlockBits)
                idx = st.rint(DataBlockObjectIdBitSize)
                st.rint(DataBlockObjectIdBitSize + 1)
                cn = DATABLOCKS[c] if c < len(DATABLOCKS) else "?%d" % c
                if cn not in DBL:
                    raise RuntimeError("no layout for datablock %s" % cn)
                notes = []
                was = st.p
                walk(st, DBL[cn], cn, notes)
                if VERBOSE:
                    print("  datablock %s %d bits" % (cn, st.p - was))
                S.datablocks += 1
                S.last_db = (idx, cn)
                for s in notes:
                    if s.lower().endswith(".dts"):
                        S.shapes.append(s)
                        break
        elif name == "NetStringEvent":
            st.rint(GhostIdBitSize)
            if st.flag():
                st.rs()
        elif name == "RemoteCommandEvent":
            argc = st.rint(5)
            args = [st.unpackString() for _ in range(argc)]
            S.commands.append(args)
        elif name == "GhostingMessageEvent":
            seq = st.rint(32)
            msg = st.rint(3)
            gc = st.rint(GhostIdBitSize + 1)
            S.conn_messages.append((msg, seq, gc))
        elif name == "CRCChallengeEvent":
            st.rint(32), st.rint(32), st.rint(32)
            st.flag()
        elif name == "SetMissionCRCEvent":
            st.rint(32)
        elif name == "LightningStrikeEvent":
            # A flag, then a ghost id, two positions and an optional target.
            # A clear flag ends it, which is what the port's own reader does.
            if st.flag():
                st.rint(11)
                st.rint(10), st.rint(10)
                if st.flag():
                    st.rint(11)
        elif name == "Sim2DAudioEvent":
            st.rint(DataBlockObjectIdBitSize)
        elif name == "Sim3DAudioEvent":
            st.rint(DataBlockObjectIdBitSize)
            if st.flag():
                st.rint(8), st.rint(8), st.rint(8)
                st.flag()
            st.point(0.5)
        elif name == "GhostAlwaysObjectEvent":
            index = st.rint(GhostIdBitSize)
            if st.flag():
                cid = st.rint(ObjectBits)
                if cid >= len(OBJECTS):
                    raise RuntimeError("ghost always class id %d out of range" % cid)
                cn = OBJECTS[cid]
                if cn not in GHL:
                    raise RuntimeError("no layout for ghost always %s" % cn)
                was = st.p
                walk(st, GHL[cn], cn, initial=True)
                if VERBOSE:
                    print("   ga %d %s %d bits" % (index, cn, st.p - was))
                S.ghosts[index] = cn
                S.ghost_always += 1
        elif name in EVL:
            walk(st, EVL[name], name)
        else:
            raise RuntimeError("no layout for event %s" % name)


def read_ghosts(st, S):
    if VERBOSE:
        print("  ghosts start @%d of %d" % (st.p, len(st.d) * 8))
    if not st.flag():
        return
    idsize = st.rint(GhostIndexBitSize) + 3
    if VERBOSE:
        print("  idsize %d @%d" % (idsize, st.p))
    while st.flag():
        index = st.rint(idsize)
        if VERBOSE:
            print("   ghost index %d @%d" % (index, st.p))
        if st.flag():                             # retired
            S.ghosts.pop(index, None)
            continue
        if index not in S.ghosts:
            cid = st.rint(ObjectBits)
            if cid >= len(OBJECTS):
                raise RuntimeError("new ghost class id %d out of range" % cid)
            S.ghosts[index] = OBJECTS[cid]
            S.new_here = True
            S.ghost_new += 1
            if VERBOSE:
                print("  ghost %d is new, a %s" % (index, OBJECTS[cid]))
        name = S.ghosts[index]
        if name not in GHL:
            raise RuntimeError("no layout for ghost %s" % name)
        # An update's length is the only thing comparable with another reader's,
        # so it is printed the same way a ghost-always body's is.
        was = st.p
        walk(st, GHL[name], name, initial=S.new_here)
        if VERBOSE:
            print("   ghost %d %s %s %d bits"
                  % (index, "new" if S.new_here else "upd", name, st.p - was))
        S.new_here = False
        S.ghost_updates += 1


def main(path):
    data = open(path, "rb").read()
    S = Session()
    off = pkt = data_pkt = 0
    stopped = where = None
    fails = []

    while off + 4 <= len(data):
        n, = struct.unpack_from("<I", data, off)
        off += 4
        if n == 0 or off + n > len(data):
            break
        p = data[off:off + n]
        off += n
        pkt += 1
        if ONLY:
            globals()["VERBOSE"] = pkt == ONLY
        if not (p[0] & 1):
            continue                              # out of band
        if VERBOSE:
            print("packet %d (%d bytes)" % (pkt, n))

        S.pkt = pkt
        globals()["SESSION"] = S
        st = Stream(p)
        try:
            st.flag()                             # game packet
            st.rint(1)                            # connect sequence
            st.rint(9)                            # send sequence
            st.rint(9)                            # ack start
            ptype = st.rint(2)
            abc = st.rint(3)
            st.rint(8 * abc)                      # ack mask
            if ptype != 0:
                continue
            data_pkt += 1
            if st.flag():
                st.rint(10), st.rint(10)          # current rate
            if st.flag():
                st.rint(10), st.rint(10)          # max rate
            where = "body"
            read_body(st, S)
            where = "events"
            read_events(st, S)
            where = "ghosts"
            read_ghosts(st, S)

            # A body read short is only caught by whatever fails after it, and
            # the last thing in a packet has nothing after it -- the leftover
            # bits are the only sign.  Counting them is what turns "the packet
            # ended cleanly" into "the packet was understood".
            where = "tail"
            left = len(p) * 8 - st.p
            TAILS.append((pkt, len(p), left))
            if left > 7:
                raise RuntimeError("%d bits left unread" % left)
        except Exception as e:
            first = ("packet %d (%d of them data), in the %s, after %d "
                     "datablocks (last %s): %s"
                     % (pkt, data_pkt, where, S.datablocks, S.last_db, e))
            if stopped is None:
                stopped = first
            fails.append(first)
            if not KEEP_GOING:
                break

    print("packets        : %d, of which %d data" % (pkt, data_pkt))
    print("datablocks     : %d   last %s" % (S.datablocks, S.last_db))
    print("ghost always   : %d" % S.ghost_always)
    print("ghosts created : %d, updates %d, live %d"
          % (S.ghost_new, S.ghost_updates, len(S.ghosts)))
    print("control object : %s" % S.control)
    print("shape files    : %d" % len(S.shapes))
    print("connection msgs: %s" % S.conn_messages[:12])
    print("commands       : %d, first %s"
          % (len(S.commands), [a[0] for a in S.commands[:6]]))
    print("events         : %s"
          % dict(sorted(S.events.items(), key=lambda kv: -kv[1])))
    if os.environ.get("T2_TAILS"):
        h = {}
        for _, _, l in TAILS: h[l] = h.get(l, 0) + 1
        print("tail bits      : %s" % dict(sorted(h.items())))
    gaps = []
    for a, b in zip(S.seqs, S.seqs[1:]):
        if b != ((a + 1) & 0x7f):
            gaps.append((a, b))
    print("event seqs     : %d, gaps %d %s"
          % (len(S.seqs), len(gaps), gaps[:6]))
    print("seq gaps at    : %s" % S.gapat[:8])
    print("stopped        : %s" % stopped)
    if KEEP_GOING:
        print("packets failed : %d of %d" % (len(fails), data_pkt))
        for f in fails[:8]:
            print("   %s" % f)
    if S.ghosts:
        kinds = {}
        for v in S.ghosts.values():
            kinds[v] = kinds.get(v, 0) + 1
        print("live ghosts    : %s" % dict(sorted(kinds.items(), key=lambda kv: -kv[1])))


if __name__ == "__main__":
    here = os.path.dirname(os.path.abspath(__file__))
    main(sys.argv[1] if len(sys.argv) > 1 else os.path.join(here, "stock.capture"))
