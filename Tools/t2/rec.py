"""Read a Tribes 2 recording offline, through the layouts the engine ships.

No build, no window, no server: the same file gives the same answer every run,
which is what makes a change to the start-block reconstruction judgeable.

    python3 Tools/t2/rec.py "$TRIBES2_DIR/Classic/recordings/demo004.rec"

The container is retail's:

    [u8 17]"Tribes2 Recording"  u32 version  u32 lengthMs  u32 startSize
    startSize bytes of start block, then raw deflate holding the block log,
    each record [u16 type:4|size:12][size bytes].

The start block's field order is retail's `GameConnection::readDemoStartBlock`
(0x005fb5c0) and the chain under it; see `recnotes.md` for where each step
came from.  Everything inside a datablock, ghost, event or packet-data body
comes from the generated tables the engine compiles, read straight out of the
headers, so what is tested here is what the engine does.
"""
import os, re, struct, sys, zlib

HERE = os.path.dirname(os.path.abspath(__file__))
SRC  = os.path.join(HERE, '..', '..', 'Engine', 'source')

TRACE = os.environ.get('T2_TRACE')


def slurp(*parts):
    return open(os.path.join(SRC, *parts), encoding='latin1').read()


# ------------------------------------------------------------------ the tables
def parse_class_names(src, which):
    m = re.search(r'#define NET_CLASS_TABLE_TRIBES2_%s\( X \)(.*?)\n\n' % which,
                  src, re.S)
    return re.findall(r'X\(\s*(\w+)\s*\)', m.group(1))


def parse_layouts(src, table):
    """name -> (nodes, select, root); a node is (op, bits, a, b)."""
    nodes = {}
    for m in re.finditer(r'sT2\w*Nodes_(\w+)\[\]\s*=\s*\{(.*?)\};', src, re.S):
        nodes[m.group(1)] = [(op, int(bits), int(a), int(b)) for op, bits, a, b
                             in re.findall(r'\{\s*(\w+),\s*(-?\d+),\s*(-?\d+),'
                                           r'\s*(-?\d+)\s*\}', m.group(2))]
    sel = {}
    for m in re.finditer(r'sT2\w*Sel_(\w+)\[\]\s*=\s*\{([^}]*)\};', src):
        sel[m.group(1)] = [int(x) for x in m.group(2).split(',') if x.strip()]
    out = []
    m = re.search(r'%s\[\]\s*=\s*\{(.*?)\n   \};' % table, src, re.S)
    for name, arr, cnt, selname, root in re.findall(
            r'\{\s*"([^"]+)",\s*(\w+),\s*(\d+),\s*(\w+),\s*(-?\d+)\s*\}',
            m.group(1)):
        key = arr.split('Nodes_')[-1]
        assert len(nodes[key]) == int(cnt), '%s: %d nodes, table says %s' % (
            name, len(nodes[key]), cnt)
        out.append((name, (nodes[key], sel.get(key, []), int(root))))
    return out


CLS        = slurp('console', 'netClassTableTribes2.h')
OBJECTS    = parse_class_names(CLS, 'OBJECTS')
DATABLOCKS = parse_class_names(CLS, 'DATABLOCKS')
EVENTS     = parse_class_names(CLS, 'EVENTS')

GB = ('T3D', 'gameBase')
DB_LAYOUTS = parse_layouts(slurp(*GB, 't2DataBlockTables.h'), 'sT2DataBlocks')
GH_LAYOUTS = dict(parse_layouts(slurp(*GB, 't2GhostTablesRecovered.h'),
                                'sT2GhostsRecovered'))
EV_LAYOUTS = dict(parse_layouts(slurp(*GB, 't2EventTables.h'), 'sT2Events'))
EV_LAYOUTS.update(parse_layouts(slurp(*GB, 't2EventTablesRecovered.h'),
                                'sT2EventsRecovered'))
PD_LAYOUTS = dict(parse_layouts(slurp(*GB, 't2PacketDataTables.h'),
                                'sT2PacketData'))
PD_LAYOUTS.update(parse_layouts(slurp(*GB, 't2PacketDataTablesRecovered.h'),
                                'sT2PacketDataRecovered'))

assert len(DB_LAYOUTS) == len(DATABLOCKS), 'datablock layouts are not one per id'
# Indexed by class id, not keyed by name: the engine's class table and the
# layout table agree on order but not always on spelling.
DB_NAMES   = [name for name, _ in DB_LAYOUTS]
DB_LAYOUTS = [layout for _, layout in DB_LAYOUTS]


# --------------------------------------------------------------- the huffman
def parse_char_freqs():
    src = slurp('core', 'stream', 'bitStream.cpp')
    m = re.search(r'csm_charFreqs\[256\]\s*=\s*\{(.*?)\};', src, re.S)
    freqs = [int(x) for x in re.findall(r'(\d+)', m.group(1))]
    assert len(freqs) == 256
    return freqs


def build_huffman():
    """HuffmanProcessor::buildTables, literally.

    Replicated rather than tabulated because the tie-breaking in the merge
    loop is what decides the codes: two symbols of equal weight give different
    trees depending on which the scan happens to reach first, so a tree built
    by any other method is a different tree that still looks plausible.
    """
    freqs = parse_char_freqs()
    leaves = []
    for i in range(256):
        alnum = (0x30 <= i <= 0x39) or (0x41 <= i <= 0x5a) or (0x61 <= i <= 0x7a)
        leaves.append({'pop': freqs[i] + (1 if alnum else 0) + 1, 'symbol': i})
    nodes = [None]                              # slot 0 is reserved for the root
    wrap = [('leaf', i) for i in range(256)]

    def pop(w):
        return leaves[w[1]]['pop'] if w[0] == 'leaf' else nodes[w[1]]['pop']

    def index(w):
        return -(w[1] + 1) if w[0] == 'leaf' else w[1]

    n = 256
    while n != 1:
        min1, min2, i1, i2 = 0xfffffffe, 0xffffffff, -1, -1
        for i in range(n):
            p = pop(wrap[i])
            if p < min1:
                min1, i2, min2, i1 = p, i1, min1, i
            elif p < min2:
                min2, i2 = p, i
        nodes.append({'pop': pop(wrap[i1]) + pop(wrap[i2]),
                      'index0': index(wrap[i1]), 'index1': index(wrap[i2])})
        merge, nuke = (i2, i1) if i1 > i2 else (i1, i2)
        wrap[merge] = ('node', len(nodes) - 1)
        if i2 != n - 1:
            wrap[nuke] = wrap[n - 1]
        n -= 1
    nodes[0] = nodes[wrap[0][1]]
    return nodes


HUFF_NODES = build_huffman()


# ------------------------------------------------------------- the bit reader
class Bits:
    """BitStream, reading.  `read(n)` is readBits(n*8): no alignment anywhere."""

    def __init__(self, data, name='?'):
        self.d = data
        self.p = 0
        self.name = name

    def flag(self):
        if self.p >= len(self.d) * 8:
            raise EOFError('%s: past end at bit %d' % (self.name, self.p))
        v = (self.d[self.p >> 3] >> (self.p & 7)) & 1
        self.p += 1
        return v

    def rint(self, n):
        v = 0
        for i in range(n):
            v |= self.flag() << i
        return v

    def sint(self, n):
        return -self.rint(n - 1) if self.flag() else self.rint(n - 1)

    def r8(self):
        return self.rint(8)

    def r32(self):
        return self.rint(32)

    def f32(self):
        return struct.unpack('<f', struct.pack('<I', self.rint(32)))[0]

    def point(self):
        """readCompressedPoint: two bits of type, then three signed ints."""
        t = self.rint(2)
        if t == 3:
            return (self.f32(), self.f32(), self.f32())
        bits = (16, 18, 20, 32)[t]
        return (self.sint(bits), self.sint(bits), self.sint(bits))

    def string(self):
        """readString with no prior-string state -- the start block has none.

        Retail clears the stream's string buffer right after seating the start
        block in it, so no string here carries the shared prefix a packet's
        would, and reading one as if it did eats a bit that is not there.
        """
        if self.flag():                         # huffman coded
            out = []
            for _ in range(self.rint(8)):
                at = 0
                while at >= 0:
                    at = (HUFF_NODES[at]['index1'] if self.flag()
                          else HUFF_NODES[at]['index0'])
                out.append(-(at + 1))
            return bytes(out).decode('latin1')
        n = self.rint(8)
        return bytes(self.r8() for _ in range(n)).decode('latin1')


# ----------------------------------------------------------------- body walks
class Walk:
    """T2Layout::runGraph, reading."""

    def __init__(self, st, resolve=None, values=None):
        self.st = st
        self.resolve = resolve                  # ghost index -> class name
        self.strings = []
        # When a caller wants the values and not just the cursor: node index
        # -> what that node read.  A layout that names its nodes can then be
        # compared field by field against another reader, which is the only
        # check that says anything about what a body *means*.
        self.values = values

    def run(self, name, layout, initial=False):
        nodes, sel, root = layout
        slots = [0] * 8
        st = self.st

        def go(at, depth):
            if depth > 8:
                raise RuntimeError('%s nested deeper than 8' % name)
            guard = 0
            while 0 <= at < len(nodes):
                guard += 1
                if guard > 65536:
                    raise RuntimeError('%s spinning at node %d' % (name, at))
                op, bits, a, b = nodes[at]
                if op in ('OpEnd', 'OpEndBody'):
                    return
                if op == 'OpFlag':
                    f = st.flag()
                    self.keep(at, bool(f))
                    at = b if f else a
                    continue
                if op == 'OpInitial':
                    at = b if initial else a
                    continue
                if op == 'OpCountBits':
                    slots[b & 7] = bin(st.rint(bits) if bits else 0).count('1')
                    at = a
                    continue
                if op == 'OpSelect':
                    at = sel[a + (st.rint(bits) if bits else 0)]
                    continue
                if op == 'OpReadBr':
                    v = st.rint(bits) if bits else 0
                    if b >= 0:
                        slots[b & 7] = v
                    at = sel[a + (1 if v else 0)]
                    continue
                if op == 'OpLoop':
                    c = slots[bits & 7]
                    if c > 4096:
                        raise RuntimeError('%s loop count %d' % (name, c))
                    for _ in range(c):
                        go(b, depth + 1)
                    at = a
                    continue
                if op == 'OpCount':
                    slots[b & 7] = st.rint(bits) if bits else 0
                    self.keep(at, slots[b & 7])
                elif op == 'OpPacketDataOf':
                    idx = st.rint(bits) if bits else 0
                    cls = self.resolve(idx) if self.resolve else None
                    if not cls:
                        raise RuntimeError('%s reads packet data of ghost %d, '
                                           'which we cannot name' % (name, idx))
                    self.packet_data(cls)
                elif op == 'OpPoint':
                    self.keep(at, st.point())
                elif op == 'OpString':
                    self.strings.append(st.string())
                    self.keep(at, self.strings[-1])
                elif bits:
                    self.keep(at, st.rint(bits))
                at = a
            if at >= 0:
                raise RuntimeError('%s node %d past end' % (name, at))

        go(root, 0)

    def keep(self, at, value):
        # A node inside a loop is read once per iteration, so the second value
        # must not replace the first: a repeated node accumulates a list, which
        # is what the run list of a terrain body actually is.
        if self.values is None:
            return
        if at in self.values:
            if isinstance(self.values[at], list):
                self.values[at].append(value)
            else:
                self.values[at] = [self.values[at], value]
        else:
            self.values[at] = value

    def datablock(self, class_id):
        name = DB_NAMES[class_id]
        self.run(name, DB_LAYOUTS[class_id])
        return name

    def ghost(self, name, initial):
        if name not in GH_LAYOUTS:
            raise RuntimeError('no ghost layout for %s' % name)
        self.run(name, GH_LAYOUTS[name], initial)

    def event(self, name):
        if name == 'SimDataBlockEvent':
            raise RuntimeError('SimDataBlockEvent is read by its own code')
        if name == 'NetStringEvent':
            self.st.rint(10)
            if self.st.flag():
                self.strings.append(self.st.string())
            return
        if name == 'RemoteCommandEvent':
            for _ in range(self.st.rint(5)):
                self.tagged_string()
            return
        if name not in EV_LAYOUTS:
            raise RuntimeError('no event layout for %s' % name)
        self.run(name, EV_LAYOUTS[name])

    def tagged_string(self):
        code = self.st.rint(2)
        if code == 0:
            return ''
        if code == 1:
            return self.string_shared()
        if code == 2:
            return self.st.rint(10)
        self.st.flag()
        if self.st.flag():
            return self.st.rint(7)
        if self.st.flag():
            return self.st.rint(15)
        return self.st.rint(31)

    def string_shared(self):
        return self.st.string()

    def packet_data(self, name):
        if name not in PD_LAYOUTS:
            raise RuntimeError('no packet data layout for %s' % name)
        self.run(name, PD_LAYOUTS[name])


# ------------------------------------------------------------- the start block
def trace(step, st):
    if TRACE:
        print('  %-34s @ bit %d (byte %.2f)' % (step, st.p, st.p / 8.0))


def read_datablock_event(st, walk):
    """SimDataBlockEvent::unpack (0x005ffc90).

    The widths are not literals in the binary: it asks for log2(nextPow2(n))
    of the net class count, which is 11 for the datablock id space and 7 for
    the class, and biases the class by 0x80 -- retail's flat table starts the
    datablock ids there.  On the wire the value is the table index.
    """
    if not st.flag():
        return None                             # the event's own leading flag
    dbid = st.rint(11)
    cls = st.rint(7)
    index = st.rint(11)
    total = st.rint(12)
    if cls >= len(DATABLOCKS):
        raise RuntimeError('datablock class %d out of range' % cls)
    name = walk.datablock(cls)
    return (dbid, name, index, total)


def read_move(st):
    """GameConnection's move read (0x00601800)."""
    for _ in range(3):
        if st.flag():
            st.rint(16)
    for _ in range(3):
        st.rint(6)
    st.flag()
    for _ in range(6):
        st.flag()


def read_path_manager(st):
    """PathManager::readState (0x00591ce0): a path's positions and times are
    read one pair at a time, not as two runs."""
    paths = st.r32()
    if paths > 4096:
        raise RuntimeError('path count %d' % paths)
    for _ in range(paths):
        st.r32()                                # the path's own word
        nodes = st.r32()
        if nodes > 65536:
            raise RuntimeError('path node count %d' % nodes)
        for _ in range(nodes):
            st.r32(); st.r32(); st.r32()        # position
            st.r32()                            # time
    return paths


def read_connection_protocol(st):
    """ConnectionProtocol::readDemoStartBlock (0x0043d820) -- the one step in
    the chain that is still Torque's, word for word."""
    seqs = [st.r32() for _ in range(32)]
    last, acked, send, mask, connectSeq, ackAck = [st.r32() for _ in range(6)]
    return {'seqsAtSend': seqs, 'lastSeqRecvd': last, 'highestAcked': acked,
            'lastSendSeq': send, 'ackMask': mask, 'connectSequence': connectSeq,
            'lastRecvAckAck': ackAck, 'established': st.r8()}


def read_event_start_block(st, walk, out):
    """NetConnection::eventReadStartBlock (0x00583ac0)."""
    out['eventSeq'] = st.r32()
    while st.flag():
        cid = st.rint(6)
        if cid >= len(EVENTS):
            raise RuntimeError('event class %d out of range' % cid)
        out['events'].append(EVENTS[cid])
        walk.event(EVENTS[cid])


def read_ghost_start_block(st, walk, out):
    """NetConnection::ghostReadStartBlock (0x00585220).

    Retail creates, unpacks and registers one ghost at a time; Torque creates
    every ghost first and only then unpacks any of them.  A body here can
    therefore name a ghost earlier in the block and nothing later.
    """
    out['ghostSeq'] = st.r32()
    out['ghostRunStart'] = st.p        # where a crafted run would be spliced in
    while True:
        header = st.p
        if not st.flag():
            break
        index = st.rint(10)
        tag = st.rint(7)
        if tag >= len(OBJECTS):
            raise RuntimeError('ghost class %d out of range' % tag)
        name = OBJECTS[tag]
        out['ghosts'][index] = name
        body = st.p
        ordinal = len(out['ghostRecords'])
        forced = out['widths'].get(ordinal)
        if forced is not None:
            st.p = body + forced             # a width retail confirmed
            out['ghostRecords'].append((header, body, forced, index, name))
            out['ghosts'][index] = name
            continue
        try:
            walk.ghost(name, True)
        except Exception as e:
            raise GhostFault(index, name, st.p - body,
                             measure_ghost(st, body, index + 1), str(e),
                             ordinal)
        out['ghostRecords'].append((header, body, st.p - body, index, name))
        # A body cannot be checked where it stands.  The next index is usually
        # index + 1, but a sender walking its ghost array hands out 1023, 1022,
        # ... as readily, so treating that as a fault reported two bodies both
        # readers agree on as broken.  It is noted and the walk carries on:
        # the verdict is whether the block consumes exactly, and a fault is
        # then localised by diffing against the second reader, not guessed at
        # from what a header looks like.
        if not header_at(st, st.p, index + 1) and not tail_follows(st, walk, out):
            out['unverified'].append((ordinal, index, name, st.p - body))


class GhostFault(Exception):
    """A ghost body our layout reads to the wrong width.

    The start block is not where this gets fixed: it is the class's own
    recovered layout, and the fix belongs in the recovery tooling.  What the
    reader can do is say which class and by how much.
    """

    def __init__(self, index, name, ours, retail, why, ordinal=None):
        self.index, self.name, self.ours, self.retail, self.why = (
            index, name, ours, retail, why)
        self.ordinal = ordinal          # which ghost in the run, not its slot
        Exception.__init__(self, '%s (ghost %d): our layout reads %d bits, '
                                 'retail wrote %s%s'
                           % (name, index, ours,
                              '%d' % retail if retail else 'more than we can '
                              'pin down',
                              '' if why is None else ' (%s)' % why))


def header_at(st, bit, index):
    """Whether a ghost header for `index` starts here.

    The indices in a start block have run 0, 1, 2, ... with no gap in every
    file measured, which is what makes this usable as an oracle -- but it is
    an observation about the sender's ghost array, not something the format
    guarantees, so a miss is reported and never worked around silently.
    """
    here = st.p
    try:
        st.p = bit
        if bit >= len(st.d) * 8 - 18 or not st.flag():
            return False
        return st.rint(10) == index and st.rint(7) < len(OBJECTS)
    finally:
        st.p = here


def measure_ghost(st, body, index, window=8192):
    """How wide retail's body was: the one length that lands on the next
    header.  None if that is not unique, which means it cannot be claimed."""
    hits = [L for L in range(window) if header_at(st, body + L, index)]
    return hits[0] if len(hits) == 1 else None


def plausible_header(st, bit, used):
    """A header that could be real: set flag, an unused index, a known class.

    Weak on its own -- a thousand offsets pass it -- which is why it is only
    ever used with the lookahead below.
    """
    here = st.p
    try:
        if bit >= len(st.d) * 8 - 18:
            return None
        st.p = bit
        if not st.flag():
            return None
        index = st.rint(10)
        tag = st.rint(7)
        if tag >= len(OBJECTS) or index in used:
            return None
        name = OBJECTS[tag]
        return (index, name, st.p) if name in GH_LAYOUTS else None
    finally:
        st.p = here


def propose_widths(st, body, used, window=8192, depth=2,
                   next_index=None):
    """Where retail's body could have ended, by what follows it.

    One body's width is not observable on its own.  What is observable is that
    the bits after it have to be another header, and that header's body has to
    land somewhere another header could be -- so a candidate is judged by
    whether the run can carry on from it.  Two levels of that is enough to cut
    a thousand offsets to a handful; the rest is retail's to arbitrate.
    """
    def ok(bit, left):
        hdr = plausible_header(st, bit, used)
        if not hdr:
            return False
        if left <= 0:
            return True
        index, name, after = hdr
        here = st.p
        try:
            st.p = after
            Walk(st).run(name, GH_LAYOUTS[name], True)
            return ok(st.p, left - 1) or run_ends(st, st.p)
        except Exception:
            return False
        finally:
            st.p = here

    # The sequential test first, because it is far the strongest: the sender
    # walks its ghost array in order often enough that the next index is
    # usually index + 1, and asking for that one index rules out the thousand
    # offsets the lookahead alone accepts.  Then lookahead, then the weak
    # form -- and whatever comes back is a proposal, not an answer.
    seq = ([L for L in range(window) if header_at(st, body + L, next_index)]
           if next_index is not None else [])
    if seq:
        return seq, 'index %d follows' % next_index
    for left in (depth, depth - 1, 0):
        hits = [L for L in range(window) if ok(body + L, left)]
        if hits:
            return hits, 'lookahead %d' % left
    return [], 'nothing plausible'


def run_ends(st, bit):
    """Whether a clear flag stands here, which is how a ghost run finishes."""
    return bit < len(st.d) * 8 and not (st.d[bit >> 3] >> (bit & 7)) & 1


def read_net_start_block(st, walk, out):
    """NetConnection::readDemoStartBlock (0x00588260)."""
    out['protocol'] = read_connection_protocol(st)
    trace('connection protocol', st)
    out['roundTrip'] = st.r32()
    out['packetLoss'] = st.r32()
    out['paths'] = read_path_manager(st)
    trace('path manager', st)
    out['notifies'] = st.r32()
    read_event_start_block(st, walk, out)
    trace('event start block', st)
    read_ghost_start_block(st, walk, out)
    trace('ghost start block', st)


def read_process_list(st):
    """The read at 0x006021b0, run over two globals: a bool and four words."""
    st.rint(8)
    return [st.r32() for _ in range(4)]


def read_string_table(st):
    """ConnectionStringTable::readDemoStartBlock (0x005898b0).

    Retail reads this from `replayDemoRecord` itself, before the connection's
    own start block -- not from inside `NetConnection::readDemoStartBlock`
    where Torque puts it.  1024 slots, each a flag and, if set, a string.
    """
    out = {}
    for i in range(1024):
        if st.flag():
            out[i] = st.string()
    return out


def read_start_block(block, stop_before=None, widths=None):
    """GameConnection::readDemoStartBlock (0x005fb5c0), under the string table
    read that `replayDemoRecord` does first."""
    st = Bits(block, 'start block')
    out = {'datablocks': [], 'events': [], 'ghosts': {}, 'demoValues': [],
           'ghostFaults': [], 'ghostRecords': [], 'unverified': [],
           'strings': None,
           'widths': dict(widths or {})}
    walk = Walk(st, resolve=lambda i: out['ghosts'].get(i))

    out['stringTable'] = read_string_table(st)
    trace('connection string table', st)

    out['datablockTotal'] = st.r32()
    while st.flag():                            # the loop's own flag, then the
        db = read_datablock_event(st, walk)     # event's, which gates its body
        if db is not None:
            out['datablocks'].append(db)
    trace('datablocks', st)

    out['firstPerson'] = st.rint(8)
    out['conn'] = [st.r32() for _ in range(6)]      # 0x8384..0x8434
    out['moveFields'] = [st.r32() for _ in range(16)]   # 0x83b8[16]
    trace('connection words', st)

    moves = st.r32()
    if moves > 4096:
        raise RuntimeError('move count %d' % moves)
    for _ in range(moves):
        read_move(st)
    out['moves'] = moves
    trace('moves', st)

    while st.flag():
        out['demoValues'].append(st.string())
    trace('$DemoValue', st)

    if stop_before == 'targets':
        out['stoppedAt'] = st.p
        out['strings'] = walk.strings
        return out, st

    read_target_block(st, out)
    trace('targets', st)

    try:
        read_net_start_block(st, walk, out)
    except GhostFault as e:
        e.out, e.st = out, st                   # so the caller can still report
        raise                                   # everything read up to here

    read_tail(st, walk, out)

    out['strings'] = walk.strings
    out['end'] = st.p
    return out, st


def read_tail(st, walk, out):
    """Steps 10 to 13: the control object, the mission name, the two lists."""
    out['controlGhost'] = st.r32()
    if out['controlGhost'] != 0xffffffff:
        cls = out['ghosts'].get(out['controlGhost'])
        if not cls:
            raise RuntimeError('control object is ghost %d, which the block '
                               'never created' % out['controlGhost'])
        walk.packet_data(cls)
    trace('control object', st)

    out['missionName'] = st.string()
    out['missionWord'] = st.r32()
    out['processLists'] = [read_process_list(st), read_process_list(st)]
    out['end'] = st.p
    trace('tail', st)


def tail_follows(st, walk, out):
    """Whether the run has legitimately ended here.

    A ghost run ends with a clear flag, and so does a body our layout read to
    the wrong width if the bit it stops on happens to be clear.  What tells
    them apart is the rest of the block: the real end is followed by a tail
    that parses and lands on the last byte.  Asking that, rather than trusting
    the one bit, is what keeps a wrong width from reading as a short run.
    """
    here = st.p
    try:
        if st.flag():                   # the run's own closing flag comes first
            return False
        probe = dict(out)
        probe['strings'] = None
        read_tail(st, walk, probe)
        return len(st.d) * 8 - st.p < 8
    except Exception:
        return False
    finally:
        st.p = here


def read_target_block(st, out):
    """The target manager's state (0x00670660), on the global at 0x009e8dbc.

    Tribes 2's targeting has no Torque counterpart, so nothing about this step
    could be carried over: a word of its own, a 32-by-32 grid of flag-gated
    words -- sensor group against target type -- and then 512 target slots,
    each a flag and, if set, two optional words, five optional names, a
    five-bit and a nine-bit field, a datablock id for slots from 32 up, and
    seven bits of scale.

    The slot count and the grid bounds are the loop bounds in the binary, and
    the whole shape is checked by emulating the retail function over the same
    file: it asks for the same 11,573 bits in the same order.
    """
    out['targetWord'] = st.r32()
    grid = 0
    for _ in range(32):
        for _ in range(32):
            if st.flag():
                st.r32()
                grid += 1
    targets = []
    for i in range(512):
        if not st.flag():
            continue
        t = {'slot': i, 'names': []}
        t['a'] = st.r32() if st.flag() else None
        t['b'] = st.r32() if st.flag() else None
        for _ in range(5):
            t['names'].append(st.string() if st.flag() else None)
        t['type'] = st.rint(5)
        t['owner'] = st.rint(9)
        # Only a slot from 32 up carries a datablock: the first 32 are the
        # sensor groups' own, and retail skips the read for them outright.
        t['datablock'] = (st.rint(11) if st.flag() else None) if i >= 32 else None
        t['scale'] = st.rint(7)
        targets.append(t)
    out['targetGrid'] = grid
    out['targets'] = targets


# ---------------------------------------------------------------- the container
HEADER = b'Tribes2 Recording'
VERSION = 0x00330004


def read_container(path):
    d = open(path, 'rb').read()
    n = d[0]
    name = d[1:1 + n]
    if name != HEADER:
        raise RuntimeError('not a recording: %r' % name[:32])
    o = 1 + n
    version, length, size = struct.unpack_from('<III', d, o)
    o += 12
    block = d[o:o + size]
    if len(block) != size:
        raise RuntimeError('start block is short: %d of %d' % (len(block), size))
    log = zlib.decompressobj(-15).decompress(d[o + size:])
    return {'version': version, 'protocol': version >> 16, 'length': length,
            'startSize': size, 'block': block, 'log': log, 'total': len(d)}


def read_log(log):
    """The block records tile the inflated log exactly or the framing is wrong;
    there is no slack to absorb an off-by-one."""
    o = 0
    out = []
    while o + 2 <= len(log):
        ts, = struct.unpack_from('<H', log, o)
        o += 2
        t, size = ts >> 12, ts & 0xfff
        if o + size > len(log):
            raise RuntimeError('record %d of type %d runs past the log end'
                               % (len(out), t))
        out.append((t, log[o:o + size]))
        o += size
    if o != len(log):
        raise RuntimeError('log has %d bytes over' % (len(log) - o))
    return out


# -------------------------------------------------------------------- the main
def main(argv):
    path = argv[1] if len(argv) > 1 else os.path.join(
        os.environ.get('TRIBES2_DIR', ''), 'Classic', 'recordings', 'demo004.rec')
    c = read_container(path)
    print('%s' % path)
    print('version     : 0x%08x  (protocol %d, format %d)%s'
          % (c['version'], c['protocol'], c['version'] & 0xffff,
             '' if c['version'] == VERSION else '  -- NOT retail\'s'))
    print('length      : %d ms (%d:%02d)'
          % (c['length'], c['length'] // 60000, c['length'] // 1000 % 60))
    print('start block : %d bytes' % c['startSize'])

    records = read_log(c['log'])
    kinds = {}
    for t, body in records:
        k = kinds.setdefault(t, [0, 0])
        k[0] += 1
        k[1] += len(body)
    print('block log   : %d bytes, %d records, tiled exactly'
          % (len(c['log']), len(records)))
    for t in sorted(kinds):
        print('  type %-2d   : %6d records, %8d bytes' % (t, *kinds[t]))

    mode = argv[2] if len(argv) > 2 and not argv[2].startswith('-') else None
    pinned = {}
    if '--widths' in argv:
        import json
        pinned = {int(k): v for k, v in
                  json.load(open(argv[argv.index('--widths') + 1])).items()}
    fault = None
    try:
        out, st = read_start_block(c['block'], stop_before=mode,
                                   widths=pinned)
    except GhostFault as e:
        fault, out, st = e, e.out, e.st
    print('datablocks  : %d read, header says %d'
          % (len(out['datablocks']), out['datablockTotal']))
    if out['datablocks']:
        print('  first     : %s' % (out['datablocks'][0],))
        print('  last      : %s' % (out['datablocks'][-1],))
    print('moves       : %d' % out['moves'])
    print('$DemoValue  : %d' % len(out['demoValues']))
    for i, v in enumerate(out['demoValues'][:3]):
        print('  [%d]       : %r' % (i, v[:120]))
    if mode == 'targets':
        print('position    : bit %d of %d (%.1f%% of the block)'
              % (out['stoppedAt'], len(c['block']) * 8,
                 100.0 * out['stoppedAt'] / (len(c['block']) * 8)))
        return 0
    print('targets     : %d of 512 slots, %d grid entries'
          % (len(out['targets']), out['targetGrid']))
    print('connection  : established %s, seq %d acked %d, round trip %.1f ms, '
          '%d paths' % (bool(out['protocol']['established']),
                        out['protocol']['lastSeqRecvd'],
                        out['protocol']['highestAcked'],
                        struct.unpack('<f', struct.pack('<I',
                                                        out['roundTrip']))[0],
                        out['paths']))
    counts = {}
    for name in out['ghosts'].values():
        counts[name] = counts.get(name, 0) + 1
    print('ghosts      : %d -- %s' % (len(out['ghosts']), counts))
    print('events      : %s' % (out['events'] or 'none'))
    if fault is not None:
        print('STOPPED     : %s' % fault)
        print('              at bit %d of %d; %d bits of the block unread'
              % (st.p, len(c['block']) * 8, len(c['block']) * 8 - st.p))
        initial = sum(1 for _, (nodes, _, _) in GH_LAYOUTS.items()
                      if any(n[0] == 'OpInitial' for n in nodes))
        print('              a ghost layout, not the start block.  %d of %d '
              'ghost graphs model an initial' % (initial, len(GH_LAYOUTS)))
        print('              body at all, and every body here is one.')
        return 1
    print('mission     : %r' % out['missionName'])
    # `startDemoRecord` declares `getPosition() + 1` bytes, so the block is
    # padded to a byte and then given one spare.  Exact consumption is that
    # arithmetic coming out, not a loose tolerance -- which is the difference
    # between "it read to the end" and "it read to somewhere near the end".
    exact = (out['end'] + 7) // 8 + 1 == c['startSize']
    print('consumed    : %d bits, %d bytes declared -- %s'
          % (out['end'], c['startSize'],
             'EXACT (ceil/8 + 1 is what retail writes)' if exact
             else 'NOT exact: ceil/8 + 1 = %d' % ((out['end'] + 7) // 8 + 1)))
    return 0 if exact else 1


if __name__ == '__main__':
    sys.exit(main(sys.argv))
