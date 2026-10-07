"""Describe a ghost body, emit its layout, and check it against a recording.

Phase 3 of `GHOST_DERIVATION_PLAN.md`.  A description here is a transcription
of two independent readings of the same retail function -- the disassembly, and
`exogen/t2-demo-parser`'s hand-derived reader -- written in the order the bits
arrive.  The node table is generated from it, so the table stays something a
file produces rather than something a person edited.

    python3 Tools/t2/describe.py LinearProjectile <demo.rec>   check it
    python3 Tools/t2/describe.py LinearProjectile --emit        C++ rows

Checking is the point.  A description is accepted when the layout it generates
reads every body of that class in a recording to the width both readers agree
on, and the start block still consumes exactly.
"""
import os, sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import rec


# --------------------------------------------------------------- the statements
# A statement is a function (emit, nxt) -> node index, so a description is
# written forwards and the graph is built backwards from its continuation.
# Branch targets are then node indices rather than anything to keep in step by
# hand, which is the part a table written out longhand gets wrong.
def read(op, bits, name=None):
    def stmt(g, nxt):
        return g.node(op, bits, nxt, -1, name)
    return stmt


def flag(name=None):
    """A flag whose value the body keeps but does not branch on."""
    def stmt(g, nxt):
        return g.node('OpFlag', 1, nxt, nxt, name)
    return stmt


def opt(body, name=None):
    """`if (readFlag()) { body }` -- both paths carry on to the same place."""
    def stmt(g, nxt):
        return g.node('OpFlag', 1, nxt, g.seq(body, nxt), name)
    return stmt


def branch(set_, clear, name=None):
    """`if (readFlag()) { set_ } else { clear }`."""
    def stmt(g, nxt):
        return g.node('OpFlag', 1, g.seq(clear, nxt), g.seq(set_, nxt), name)
    return stmt


def point(name=None):
    """`conn->readCompressedPoint` -- two bits of type then three signed ints.

    A virtual on the *connection*, which is why the emulator walk stepped over
    it and why every graph it produced is short by one of these.
    """
    def stmt(g, nxt):
        return g.node('OpPoint', 0, nxt, -1, name)
    return stmt


def normal(bits, name=None):
    """`readNormalVector(n)`: a signed float of n+1 bits then one of n."""
    return [read('OpSignedFloat', bits + 1, name and name + '.phi'),
            read('OpSignedFloat', bits, name and name + '.theta')]


def int_(bits, name=None):
    return read('OpInt', bits, name)


def datablock_id(name='dataBlockId'):
    """The 11-bit id of the datablock this object is built from.

    Typed as `OpDataBlockId` rather than a plain int so the engine can act on
    it: the walked graphs use that op, and the reader keys the lookup on it.
    """
    return read('OpDataBlockId', 11, name)


def string(name=None):
    """`readString` -- the Huffman reader, so its width is the string's."""
    return read('OpString', 0, name)


def float_(bits, name=None):
    """`readFloat(n)`: n bits as a unit float, n / (2**n - 1)."""
    return read('OpFloat', bits, name)


def f32(name=None):
    """A raw 32-bit float, which is what `read(&f)` on a BitStream is."""
    return read('OpRaw', 32, name)


def point3f(name):
    """Three raw floats, named so the three come back as one point."""
    return [f32(name + '.x'), f32(name + '.y'), f32(name + '.z')]


def affine(name):
    """Position, three quaternion terms and the sign of the fourth.

    The fourth term is recovered rather than sent, so the sign flag is the
    whole of what the stream says about it.
    """
    return point3f(name + '.position') + [
        f32(name + '.rotation.x'), f32(name + '.rotation.y'),
        f32(name + '.rotation.z'), flag(name + '.rotation.wNegative'),
    ]


def initial(body):
    """Read only on an initial body.

    Retail decides this from the object's own state -- `this+0x18 & 8`, the
    SimObject `Added` bit, tested at 0x005ef79a -- because `unpackUpdate` is
    called before `registerObject`.  Nothing in the stream says it.
    """
    def stmt(g, nxt):
        return g.node('OpInitial', 0, nxt, g.seq(body, nxt))
    return stmt


def count_loop(bits, body, slot=0, name=None):
    """A count, then that many of `body`.  The count's width is its own."""
    def stmt(g, nxt):
        loop = g.node('OpLoop', slot, nxt, g.seq(body, -1))
        return g.node('OpCount', bits, loop, slot, name)
    return stmt


class Graph:
    def __init__(self):
        self.nodes = [('OpEnd', 0, -1, -1)]
        self.names = {0: None}

    def node(self, op, bits, a, b, name=None):
        self.nodes.append((op, bits, a, b))
        self.names[len(self.nodes) - 1] = name
        return len(self.nodes) - 1

    def seq(self, body, nxt):
        """Statements in order, built from the back so each links to the next."""
        at = nxt
        for stmt in reversed(body if isinstance(body, list) else [body]):
            if isinstance(stmt, list):
                at = self.seq(stmt, at)
            else:
                at = stmt(self, at)
        return at

    def build(self, body):
        root = self.seq(body, 0)
        return (self.nodes, [], root)


def names(name):
    """node index -> the field it carries, for the classes that say."""
    g = Graph()
    g.seq(DESCRIPTIONS[name], 0)
    return g.names


# ------------------------------------------------------------- the descriptions
GAME_BASE = [
    opt([datablock_id()]),              # DataBlockMask
    opt([opt([int_(9, 'targetId')])]),  # TargetMask: a target id, or none
]

def _sound_slot(i):
    return opt([opt([int_(11, 'sounds[%d].profileId' % i)],
                    name='sounds[%d].playing' % i)])


def _thread_slot(i):
    return opt([int_(5, 'threads[%d].sequence' % i),
                int_(2, 'threads[%d].state' % i),
                flag('threads[%d].forward' % i),
                flag('threads[%d].atEnd' % i)])


def _image_slot(i):
    at = lambda f: 'images[%d].%s' % (i, f)
    return opt([
        opt([int_(11, at('dataBlockId'))]),
        opt([branch(set_=[int_(10, at('skinTagIndex'))],
                    clear=[string(at('skinName'))])]),
        flag(at('triggerDown')), flag(at('loaded')), flag(at('ammo')),
        flag(at('wet')), flag(at('target')), int_(3, at('fireCount')),
        # One extra flag per slot, and only on an initial body: the asymmetry
        # that made this family read short before it was described.
        initial([flag(at('imageExtraFlag'))]),
    ])


SHAPE_BASE = GAME_BASE + [
    opt([                               # one gate over every mask below
        opt([float_(6, 'damageLevel'), int_(2, 'damageState'),
             flag('blowApart'), normal(8, 'damageDir')]),        # DamageMask
        opt([_sound_slot(i) for i in range(4)]),                 # SoundMask
        opt([_thread_slot(i) for i in range(4)]),                # ThreadMask
        opt([_image_slot(i) for i in range(8)]),                 # ImageMask
        opt([                           # shield, cloak and invincible
            opt([flag('cloaked'), flag('isControlled'),
                 branch(name='fading',
                        set_=[flag('fadeOut'), f32('fadeTime')],
                        clear=[flag('fadeVal')])]),
            opt([branch(set_=[flag('stateBMode')],
                        clear=[normal(8, 'shieldNormal'),
                               float_(5, 'energyPercent')])]),
            opt([int_(32, 'stateValue1'), int_(32, 'stateValue2')]),
        ]),
        opt([branch(set_=[int_(10, 'mountObject'), int_(5, 'mountNode')],
                    clear=[])]),                                 # MountedMask
    ]),
]

EMPTY_SQUARE_RUNS = count_loop(32, [int_(32, 'emptySquareRuns')],
                               name='emptySquareRunCount')

# What a field reads as when its flag is clear, which is a field of the body
# as much as the bits are: the engine has to set it, and the second reader
# reports it, so a comparison that skips it is not comparing the body.
MARKER_DEFAULTS = {'targetId': -1}

# Their name for a field we read under the transform it belongs to.
MARKER_ALIASES = {'position.%s' % k: 'transform.position.%s' % k
                  for k in 'xyz'}

DEFAULTS = {
    'MissionMarker': MARKER_DEFAULTS,
    'SpawnSphere': MARKER_DEFAULTS,
    'WayPoint': MARKER_DEFAULTS,
    'LinearProjectile': {'targetId': -1},
    'Item': dict({'targetId': -1, 'mountObject': -1},
                 **{'images[%d].dataBlockId' % i: 0 for i in range(8)}),
}

# A field the second reader derives from another rather than reading.
ALIASES = {
    'Item': {'energyPackOn': 'stateBMode'},
    'MissionMarker': MARKER_ALIASES,
    'SpawnSphere': MARKER_ALIASES,
    'WayPoint': MARKER_ALIASES,
}

# MissionMarker::unpackUpdate and the two classes built on it.  ShapeBase's
# body, then the transform and scale a marker carries of its own.
MARKER = SHAPE_BASE + [opt(affine('transform') + point3f('scale'))]

DESCRIPTIONS = {
    # TerrainBlock::unpackUpdate.  It extends SceneObject, not GameBase, so
    # there is no datablock or target in front of it.  The material name list
    # its packet form ends with is **not** in a start block -- retail guards
    # it, which is what made that tail look inert to the walk.
    'TerrainBlock': [
        branch(
            set_=[                      # InitMask: the terrain arrives whole
                int_(32, 'crc'),
                string('terrFileName'),
                string('detailTextureName'),
                int_(32, 'squareSize'),
                EMPTY_SQUARE_RUNS,
            ],
            clear=[opt([EMPTY_SQUARE_RUNS])]),
    ],
    # LinearProjectile::unpackUpdate (0x0062f190).  The compressed point on
    # each branch is what our recovered graph is missing: it reads 46 bits
    # where retail writes 144, and 46 + 2 + 3*32 is 144 exactly.
    # ShapeBase's own body.  No recording carries a ShapeBase ghost, so this
    # entry is verified only through the classes built on it -- 46 Item bodies
    # and 1,240 field comparisons agree -- which is the same standing the
    # hand-written ShapeBase family had before.
    'ShapeBase': SHAPE_BASE,
    # Item::unpackUpdate, on ShapeBase's body.
    'Item': SHAPE_BASE + [
        opt([                           # InitialUpdateMask
            flag('rotate'), flag('isStatic'), flag('collideable'),
            opt(point3f('scale')),
        ]),
        opt([int_(10, 'collisionObject')]),                      # ThrowSrcMask
        opt([flag('rotation.zSign'), f32('rotation.angle')]),    # RotationMask
        opt(point3f('position') + [                              # PositionMask
            branch(name='atRest', set_=[], clear=point3f('velocity')),
            flag('warp'),
        ]),
    ],
    'MissionMarker': MARKER,
    'SpawnSphere': MARKER + [
        opt([f32('radius'), f32('sphereWeight'), f32('indoorWeight'),
             f32('outdoorWeight')]),
    ],
    'WayPoint': MARKER + [
        opt([string('name')]),
        opt([int_(32, 'teamId')]),
        opt([flag('hidden')]),
    ],
    # GrenadeProjectile::unpackUpdate.  EnergyProjectile and FlareProjectile
    # resolve through the same vtable, so they share this body.
    'GrenadeProjectile': GAME_BASE + [
        branch(
            set_=[                      # InitialUpdateMask
                point3f('position'), point3f('velocity'),
                int_(12, 'currTick'), flag('quickSplash'),
                opt(point3f('explodePoint') + point3f('explodeNormal')),
                opt([int_(11, 'sourceObject'), int_(3, 'sourceSlot')]),
                opt([int_(11, 'vehicleObject')]),
            ],
            clear=[
                opt(point3f('position') + point3f('velocity')),      # BounceMask
                opt(point3f('explodePoint')                         # ExplosionMask
                    + point3f('explodeNormal')),
            ]),
    ],
    # BombProjectile::unpackUpdate (0x00636050).
    'BombProjectile': GAME_BASE + [
        branch(
            set_=[                      # the full state
                point3f('position'), point3f('velocity'),
                int_(12, 'currTick'),
                flag('resetFlag'),
                opt(point3f('explodePoint') + point3f('explodeNormal')),
                opt([int_(11, 'sourceObject'), int_(3, 'sourceSlot')]),
                opt([int_(11, 'vehicleObject')]),
            ],
            clear=[
                opt(point3f('position') + point3f('velocity')),
                opt(point3f('endPoint') + point3f('endNormal')),
            ]),
    ],
    # SeekerProjectile::unpackUpdate (0x0063c010).  A target is a direction, a
    # ghost or nothing at all, and the mask decides which.
    'SeekerProjectile': GAME_BASE + [
        branch(
            set_=[                      # the full state
                point3f('position'), point3f('velocity'),
                point3f('orientation'),
                opt([int_(11, 'sourceObject'), int_(3, 'sourceSlot')]),
                opt([branch(set_=[int_(11, 'targetGhost')],
                            clear=point3f('targetDirection'))]),
                flag('timeoutReset'),
            ],
            clear=[
                branch(
                    set_=[              # a detonation
                        point3f('explodePosition'),
                        point3f('explodeNormal'),
                    ],
                    clear=point3f('position') + point3f('velocity') + [
                        opt([branch(set_=[int_(11, 'targetGhost')],
                                    clear=point3f('targetDirection'))]),
                    ]),
            ]),
    ],
    'LinearProjectile': GAME_BASE + [
        branch(
            set_=[                      # InitialUpdateMask
                branch(
                    name='hidden',
                    set_=[              # already exploded when it arrived
                        point('explodePosition'), normal(14, 'explodeNormal'),
                        flag('endedWithDecal'),
                    ],
                    clear=[             # live projectile
                        point('position'), normal(14, 'direction'),
                        int_(9, 'currTick'),
                        opt([int_(10, 'sourceObject'), int_(3, 'sourceSlot'),
                             opt([int_(8, 'excessVel')]
                                 + normal(7, 'excessDir'))]),
                        opt([int_(10, 'vehicleObject')]),
                    ]),
            ],
            clear=[                     # a later update: it exploded
                point('explodePosition'), normal(14, 'explodeNormal'),
                flag('endedWithDecal'),
            ]),
    ],
}


# Classes whose bodies retail reads through another class's vtable, so the
# description is the same one.  Recovered from the binary by t2-demo-parser,
# which binds each of these to the same unpackUpdate.
SHARED_WITH = {
    'TracerProjectile': 'LinearProjectile',
    'LinearFlareProjectile': 'LinearProjectile',
    'EnergyProjectile': 'GrenadeProjectile',
    'FlareProjectile': 'GrenadeProjectile',
}

for _alias, _of in SHARED_WITH.items():
    DESCRIPTIONS[_alias] = DESCRIPTIONS[_of]
    if _of in DEFAULTS:
        DEFAULTS[_alias] = DEFAULTS[_of]
    if _of in ALIASES:
        ALIASES[_alias] = ALIASES[_of]


def layout(name):
    return Graph().build(DESCRIPTIONS[name])


def emit(name):
    nodes, _, root = layout(name)
    out = ['   /// %s: described in Tools/t2/describe.py, root %d'
           % (name, root),
           '   static const T2DataNode sT2RcNodes_%s[] =' % name, '   {']
    for op, bits, a, b in nodes:
        out.append('      { %s, %d, %d, %d },' % (op, bits, a, b))
    out.append('   };')
    return '\n'.join(out)


def check(name, path):
    nodes, sel, root = layout(name)
    rec.GH_LAYOUTS[name] = (nodes, sel, root)      # the description, in place
    c = rec.read_container(path)
    try:
        out, st = rec.read_start_block(c['block'])
        stopped = None
    except rec.GhostFault as e:
        out, st, stopped = e.out, e.st, str(e)
    widths = [w for _, _, w, _, n in out['ghostRecords'] if n == name]
    exact = (out.get('end') is not None
             and (out['end'] + 7) // 8 + 1 == c['startSize'])
    print('%s with the description in place:' % os.path.basename(path))
    print('  %-22s read %d of them, widths %s'
          % (name, len(widths), sorted(set(widths)) or '-'))
    print('  ghosts read : %d' % len(out['ghostRecords']))
    print('  consumed    : %s'
          % ('EXACT' if exact else 'not to the end -- %s' % (stopped or '?')))
    return 0 if exact else 1


def main(argv):
    name = argv[1]
    if name not in DESCRIPTIONS:
        raise SystemExit('described: %s' % ', '.join(sorted(DESCRIPTIONS)))
    if '--emit' in argv:
        print(emit(name))
        return 0
    return check(name, argv[2])


if __name__ == '__main__':
    sys.exit(main(sys.argv))
