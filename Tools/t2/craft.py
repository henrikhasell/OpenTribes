"""Build a recording whose ghost run stops after one body, then a marker.

Phase 1 of `GHOST_DERIVATION_PLAN.md`.  Everything up to and including the
ghost under test is kept from a real recording -- the string table, all 1,508
datablocks, the connection words, the moves, the `$DemoValue` run, the target
manager, the connection protocol block, the path manager, and every ghost in
front of this one.  Then the run is closed and a tail is written: no control
object, a string that lands in `$MissionName`, the trailing word and the two
process lists.

That string is the measurement.  It is read through the Huffman reader, so it
only comes back intact if the body was consumed in exactly the bits allotted
-- a one-bit oracle on one ghost body, with everything in front of it being
retail's own bytes that retail reads correctly by construction.

    python3 Tools/t2/craft.py <demo.rec> Player 343 out.rec
    python3 Tools/t2/craft.py <demo.rec> Player 342 short.rec   # must fail

Measured against retail: at 343 bits `playDemo` returns 1 and `$MissionName`
reads back the marker; at 342 and at 344 retail dies with an access violation
in `resolveGhost` (0x00584f8a), because a cursor one bit out reads the closing
flag as a set one and then a nonsense ghost header.  A wrong width is a dead
client, which is a blunt signal but an unambiguous one -- and it means a sweep
has to relaunch per failure.

The deflated block log is copied byte for byte and never has to agree with
anything: the driver holds `$timeScale` at zero, so no tick runs and no packet
is ever pumped.
"""
import os, struct, sys, zlib

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import rec

MARKER = 'PROBE'


class Out:
    """A bit writer that matches `rec.Bits`: LSB first within each byte."""

    def __init__(self):
        self.bits = []

    def bit(self, v):
        self.bits.append(v & 1)

    def int(self, v, n):
        for i in range(n):
            self.bit((v >> i) & 1)

    def u32(self, v):
        self.int(v, 32)

    def copy(self, src, at, n):
        """`n` bits of `src` from bit `at`, which is how a real body is lifted."""
        for i in range(at, at + n):
            self.bit(src[i >> 3] >> (i & 7))

    def string(self, s):
        """readString's uncompressed form: a clear flag, a length, the bytes.

        No Huffman encoder needed to be read by one -- retail's reader takes
        either form, and this is the one that can be written by hand.
        """
        self.bit(0)
        self.int(len(s), 8)
        for ch in s.encode('latin-1'):
            self.int(ch, 8)

    def bytes(self):
        out = bytearray((len(self.bits) + 7) // 8)
        for i, b in enumerate(self.bits):
            if b:
                out[i >> 3] |= 1 << (i & 7)
        return bytes(out)


def find_ghost(out, cls, nth=0):
    seen = 0
    for header, body, width, index, name in out['ghostRecords']:
        if name == cls:
            if seen == nth:
                return header, body, width, index
            seen += 1
    raise SystemExit('no %s ghost in that recording -- it carries %s'
                     % (cls, sorted(set(n for *_, n in out['ghostRecords']))))


def craft_ordinal(path, ordinal, width, widths=None, marker=MARKER):
    """The same, located by position in the run rather than by class.

    Past the first body our layout cannot read, a position is only reachable
    with the widths already measured pinned -- which is why this takes them.
    """
    c = rec.read_container(path)
    try:
        out, st = rec.read_start_block(c['block'], widths=widths or {})
    except rec.GhostFault as e:
        out, st = e.out, e.st
    if ordinal >= len(out['ghostRecords']):
        raise SystemExit('the run only reaches ghost %d with those widths'
                         % (len(out['ghostRecords']) - 1))
    _, body, measured, index, name = out['ghostRecords'][ordinal]
    return assemble(c, body, width, marker), measured, name, index


def assemble(c, body, width, marker):
    src = c['block']
    w = Out()
    w.copy(src, 0, body)                    # the block, and the run, up to it
    w.copy(src, body, width)                # the body, allotted `width` bits
    w.bit(0)                                # and the run ends here
    w.u32(0xffffffff)                       # no control object, no packet data
    w.string(marker)                        # -> $MissionName
    w.u32(0)                                # the trailing word
    for _ in range(2):                      # the two process lists
        w.int(0, 8)
        for _ in range(4):
            w.u32(0)
    block = w.bytes()
    head = bytearray()
    head.append(len(rec.HEADER))
    head += rec.HEADER
    head += struct.pack('<III', c['version'], c['length'], len(block))
    z = zlib.compressobj(6, zlib.DEFLATED, -15)
    return bytes(head) + block + z.compress(c['log']) + z.flush()


def craft(path, cls, width, marker=MARKER, nth=0):
    """The run truncated after one ghost, that ghost allotted `width` bits.

    Keeping every ghost in front of the one under test is not caution, it is
    required: a body can name an earlier ghost -- retail creates, unpacks and
    registers them one at a time -- and a run with those ghosts missing
    crashes retail inside `resolveGhost` (0x00584f8a) rather than answering.
    Everything in front is retail's own bytes, which retail reads correctly by
    construction, so a marker that dies can only be about the bits we changed.
    """
    c = rec.read_container(path)
    try:
        out, st = rec.read_start_block(c['block'])
    except rec.GhostFault as e:
        out, st = e.out, e.st               # the records before the fault stand

    header, body, measured, index = find_ghost(out, cls, nth)
    return assemble(c, body, width, marker), measured


def main(argv):
    if len(argv) < 5:
        raise SystemExit(__doc__)
    path, cls, width, dest = argv[1], argv[2], int(argv[3]), argv[4]
    if cls.isdigit():                       # by ordinal, with widths as JSON
        import json
        widths = {int(k): v for k, v in
                  json.loads(argv[5] if len(argv) > 5 else '{}').items()}
        data, measured, cls, index = craft_ordinal(path, int(cls), width,
                                                   widths)
        open(dest, 'wb').write(data)
        print('%s: run truncated after ghost %s (index %d, %s), allotted %d '
              'bits (our layout reads %d), marker %r'
              % (dest, argv[2], index, cls, width, measured, MARKER))
        return
    data, measured = craft(path, cls, width)
    open(dest, 'wb').write(data)
    print('%s: run truncated after the %s ghost, allotted %d bits '
          '(our layout reads %d), marker %r'
          % (dest, cls, width, measured, MARKER))
    print('%d bytes' % len(data))


if __name__ == '__main__':
    main(sys.argv)
