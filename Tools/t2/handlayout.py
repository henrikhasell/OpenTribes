#!/usr/bin/env python3
"""Write ghost layouts by hand, compositionally, into ghost_fixups.json.

    python3 Tools/t2/handlayout.py

The emulated walk in ghostwalk.py recovers a layout from the bits a function
reads, which is all a layout needs to be -- except where the bits are not the
only input. ShapeBase reads one extra flag per mounted image slot when the
object is not yet in the scene, so the same class reads two lengths depending
on whether the update is its first, and no walk of the bits can see that.

The ShapeBase family is written here instead, from the retail binary's own
reading as decompiled by hand in two places: this port's own notes and
t2-demo-parser (MIT, Brian Beck, github.com/exogen/t2-demo-parser), which
reached the same functions independently. Every layout it emits is measured
against recorded sessions by Tools/t2/xcdiff.py, so what is written here is
checked rather than asserted.

Node indices are allocated by the builder, so a section can be described in
the order the binary reads it and moved about without renumbering anything.
"""
import json
import os

HERE = os.path.dirname(os.path.abspath(__file__))
OUT = os.path.join(HERE, "ghost_fixups.json")


class Graph:
    def __init__(self):
        self.nodes = []

    def add(self, node):
        self.nodes.append(node)
        return len(self.nodes) - 1


def emit(g, stmts, cont):
    """Wire `stmts` to run in order and then fall through to `cont`.

    Emitted back to front, so each statement knows where it goes next and no
    node ever needs its successor patched afterwards.
    """
    at = cont
    for s in reversed(stmts):
        at = s(g, at)
    return at


# --------------------------------------------------------------- the leaves
def bits(n, kind="readRaw"):
    """Take n bits. Which read it was only matters to a reader that keeps the
    value, and nothing here does -- the object is not built from these."""
    return lambda g, cont: g.add(["read", kind, n, cont])


def flag():
    """A bit whose value changes nothing that follows."""
    return lambda g, cont: g.add(["flag", cont, cont])


def string():
    return lambda g, cont: g.add(["read", "readString", 0, cont])


def datablock():
    return lambda g, cont: g.add(["read", "readDataBlockId", 11, cont])


def point():
    """A Torque compressed point: two bits of type, then three scaled deltas
    or three whole floats."""
    return lambda g, cont: g.add(["read", "readPoint", 0, cont])


def normal(n):
    """readNormalVector(n): a signed float of n+1 bits then one of n."""
    return bits(2 * n + 1)


# ------------------------------------------------------------ the branches
def if_flag(*body):
    def build(g, cont):
        inner = emit(g, list(body), cont)
        return g.add(["flag", cont, inner])
    return build


def if_not_flag(*body):
    """The set bit is the one that skips: Player's control-object shortcut
    returns early when its flag is set."""
    def build(g, cont):
        inner = emit(g, list(body), cont)
        return g.add(["flag", inner, cont])
    return build


def if_flag_else(yes, no):
    def build(g, cont):
        a = emit(g, list(yes), cont)
        b = emit(g, list(no), cont)
        return g.add(["flag", b, a])
    return build


def if_initial(*body):
    """Only when this is the object's first update. Not a bit off the wire."""
    def build(g, cont):
        inner = emit(g, list(body), cont)
        return g.add(["initial", cont, inner])
    return build


def repeat(n, *body):
    """A loop of a length fixed in the code, unrolled here as it is there."""
    def build(g, cont):
        at = cont
        for _ in range(n):
            at = emit(g, list(body), at)
        return at
    return build


def count_into(n, slot=0):
    """Take an n-bit count and keep it, without looping here.  Tribes 2 does
    not always read a length next to the thing it measures: Sky reads its fog
    volume count, then four more fields, and only then the volumes."""
    def build(g, cont):
        return g.add(["count", "readRaw", n, slot, cont])
    return build


def loop_over(slot, *body):
    """Run the body as many times as the count already in `slot` says."""
    def build(g, cont):
        eb = g.add(["endbody"])
        entry = emit(g, list(body), eb)
        return g.add(["loop", slot, entry, cont])
    return build


def counted(n, *body, slot=0):
    """A count of n bits off the wire, then that many passes over the body."""
    def build(g, cont):
        eb = g.add(["endbody"])
        entry = emit(g, list(body), eb)
        loop = g.add(["loop", slot, entry, cont])
        return g.add(["count", "readRaw", n, slot, loop])
    return build


def count_bits(n, slot=0):
    """Take an n-bit mask and keep how many bits stand in it, for a loop that
    runs once per set bit."""
    return lambda g, cont: g.add(["countbits", n, slot, cont])


def packet_data_of(n=10):
    """Take an n-bit ghost index, then read that object's packet data --
    whichever class the index turns out to name."""
    return lambda g, cont: g.add(["packetdataof", n, cont])


def layout(stmts, why, assumes=None):
    g = Graph()
    end = g.add(["end"])
    root = emit(g, stmts, end)
    out = {"why": why, "root": root, "nodes": g.nodes,
           "approx": 0, "explored": 0, "wide": False, "cyclic": False}
    if assumes:
        out["assumes"] = assumes
    return out


# ------------------------------------------------------- the shared layers
def game_base():
    """GameBase::unpackUpdate, 0x5e3360."""
    return [
        if_flag(datablock()),               # DataBlockMask
        if_flag(if_flag(bits(9))),          # TargetMask, then an explicit id
    ]


def shape_base():
    """ShapeBase::unpackUpdate, 0x5ef0e0, under one combined gate."""
    return game_base() + [if_flag(
        # DamageMask
        if_flag(bits(6), bits(2), flag(), normal(8)),
        # SoundMask: four slots
        if_flag(repeat(4, if_flag(if_flag(datablock())))),
        # ThreadMask: four animation slots
        if_flag(repeat(4, if_flag(bits(5), bits(2), flag(), flag()))),
        # ImageMask: eight mounted slots.  The last flag is read only when the
        # object is not yet in the scene, which is what makes this class need
        # to be told which kind of update it is reading.
        if_flag(repeat(8,
            if_flag(
                if_flag(datablock()),
                if_flag(if_flag_else([bits(10)], [string()])),
                flag(), flag(), flag(), flag(), flag(), bits(3),
                if_initial(flag()),
            ))),
        # Shield, cloak and invincible, which Tribes 2 gates together
        if_flag(
            if_flag(flag(), flag(),
                    if_flag_else([flag(), bits(32)], [flag()])),
            if_flag(if_flag_else([flag()], [normal(8), bits(5)])),
            if_flag(bits(32), bits(32)),
        ),
        # MountedMask
        if_flag(if_flag(bits(10), bits(5))),
    )]


def point3f():
    """Three whole floats."""
    return repeat(3, bits(32))


def affine():
    """readAffineTransform: a position, three quaternion terms and the sign of
    the fourth, which is recovered rather than sent."""
    return repeat(1, point3f(), point3f(), flag())


def move():
    """Move::unpack: three optional angles, three six-bit axes, then the keys."""
    return [
        if_flag(bits(16)), if_flag(bits(16)), if_flag(bits(16)),
        bits(6), bits(6), bits(6),
        flag(),
        repeat(6, flag()),
    ]


def vehicle():
    """Vehicle::unpackUpdate, 0x60d740's reader.  Everything past the control
    gate is skipped when this client is the one driving."""
    return shape_base() + [
        flag(),                                 # jetting
        if_not_flag(
            bits(9), bits(9),                   # steering yaw and pitch
            *move(),
            flag(),                             # frozen
            if_flag(point(),                    # PositionMask
                    repeat(4, bits(32)),        # angular position, a quaternion
                    point3f(),                  # linear momentum
                    point3f()),                 # angular momentum
            if_flag(bits(8)),                   # EnergyMask
        ),
    ]


def sky():
    """Sky::unpackUpdate, 0x5ab580.  Larger than any Torque-era Sky: the
    retail binary carries fog volumes, three cloud layers and a storm."""
    return [
        if_flag(                                # InitialUpdateMask
            string(),                           # the material list
            point3f(),                          # fog colour
            count_into(32),                     # how many fog volumes
            bits(8), bits(8),                   # sky textures, bottom texture
            point3f(),                          # the solid colour
            bits(8),                            # wind affects precipitation
            loop_over(0, repeat(6, bits(32))),  # each volume: distance, two
                                                # heights and a colour
            repeat(3, string(), bits(32), bits(32)),   # three cloud layers
            point3f(),                          # wind velocity
            bits(32),                           # the storm as it stands
            if_flag(repeat(5, bits(32))),       # and how it was started
        ),
        if_flag(bits(8)),                       # storm clouds shown
        if_flag(bits(8)),                       # storm fog shown
        if_flag(bits(32), bits(32)),            # visible and fog distance
        if_flag(bits(32), bits(32)),            # storm type and magnitude
        if_flag(repeat(3, bits(32))),           # the storm timeline
        if_flag(repeat(4, bits(32))),           # the storm cloud profile
        if_flag(point3f()),                     # a change of wind
    ]


# ------------------------------------------------------------- the classes
WHY_SHAPEBASE = (
    "ShapeBase reads a flag per mounted image slot only when the object is "
    "not yet in the scene, so one graph of the bits cannot describe both an "
    "initial body and an update -- the walk in ghostwalk.py read 126 to 166 "
    "bits where a session carries 276 to 351.  Written out from the retail "
    "binary's own reading (%s), with the field order cross-checked against "
    "t2-demo-parser (MIT, github.com/exogen/t2-demo-parser), and measured "
    "against recorded sessions by Tools/t2/xcdiff.py."
)

def static_shape():
    """StaticShape::unpackUpdate, 0x602da0."""
    return shape_base() + [
        if_flag(affine(), point3f()),           # PositionMask
        flag(),                                 # powered, always sent
    ]


WHY_VEHICLE = (
    "The walk explores past four and a half million states and comes back "
    "with a cycle in it, so there was no layout for this class at all.  "
    "Written out from the retail binary's own reading (%s), cross-checked "
    "against t2-demo-parser (MIT, github.com/exogen/t2-demo-parser).  Shares "
    "shape_base(), which carries the initial-only image flag."
)

LAYOUTS = {
    "Sky": layout(
        sky(),
        "The walk read Sky's three cloud layers as a counted loop and took "
        "the count off the wire, so the layout treated a thirty-two bit field "
        "as a repeat count and lost the rest of the packet -- which lost four "
        "guaranteed events with it and stalled the ordered queue.  The layers "
        "are a loop of exactly three; the wire count belongs to the fog "
        "volumes, several fields earlier.  Written out from 0x5ab580 and "
        "cross-checked against t2-demo-parser."),

    "Vehicle": layout(vehicle(), WHY_VEHICLE % "0x60d740"),

    "FlyingVehicle": layout(
        vehicle() + [
            # Its own control gate, separate from Vehicle's.
            if_not_flag(flag(), bits(3)),
        ],
        WHY_VEHICLE % "0x6116a0"),

    "HoverVehicle": layout(
        vehicle() + [bits(3)],              # thrust direction
        WHY_VEHICLE % "0x617340"),

    "WheeledVehicle": layout(
        vehicle() + [
            flag(),                         # braking
            if_flag(repeat(6, point3f())),  # six wheels, three floats each
        ],
        WHY_VEHICLE % "0x615a70"),

    "StaticShape": layout(static_shape(), WHY_SHAPEBASE % "0x602da0"),

    "Camera": layout(
        shape_base() + [
            # Set means this client drives it and nothing follows.
            if_not_flag(if_flag(repeat(5, bits(32)))),
        ],
        WHY_SHAPEBASE % "0x5cc8c0"),

    "Turret": layout(
        static_shape() + [
            if_flag(bits(8)),                   # capacitor energy
            if_not_flag(                        # set means this client drives it
                if_flag(bits(10), bits(10), bits(8))),   # barrel state
        ],
        WHY_SHAPEBASE % "0x655f60"),

    "Item": layout(
        shape_base() + [
            # InitialUpdateMask
            if_flag(flag(), flag(), flag(), if_flag(point3f())),
            if_flag(bits(10)),                  # ThrowSrcMask
            if_flag(flag(), bits(32)),          # RotationMask
            # PositionMask: a position, then a velocity unless it has settled
            if_flag(point3f(),
                    if_flag_else([], [point3f()]),
                    flag()),
        ],
        WHY_SHAPEBASE % "0x5f3e10"),

    "Player": layout(
        shape_base() + [
            if_flag(bits(3)),                       # ImpactMask
            # ActionMask.  The animation position is read behind a flag only
            # when the animation has not reached its end, and whether it has
            # is a bit already read -- so that bit has to branch rather than
            # be passed over.
            if_flag(bits(8), flag(),
                    if_flag_else([flag()],
                                 [flag(), if_flag(bits(6))])),
            if_flag(bits(8)),                       # and the arms
            if_not_flag(                            # set means we drive it
                if_flag(                            # MoveMask
                    bits(3),
                    if_flag(bits(7)),               # recovering
                    flag(), flag(),
                    point(),
                    if_flag(bits(13), normal(10)),  # velocity
                    bits(6), bits(6), bits(7),      # head and rotation
                    *move(),
                    flag(),                         # allowWarp
                ),
                bits(5),                            # energy
            ),
        ],
        WHY_SHAPEBASE % "0x5db2d0",
    ),
}

# --------------------------------------------- what the control object sends
# Tribes 2 sends the state of the object this client drives through
# readPacketData, vtable slot 66, rather than through unpackUpdate.  The
# shipped table carries only Camera, which is what a client gets while it is
# still watching; the moment it spawns, the control object is a Player.
def shape_base_packet():
    """ShapeBase::readPacketData: the energy level and the recharge rate."""
    return [bits(32), bits(32)]


PACKET_DATA = {
    "ShapeBase": layout(
        shape_base_packet(),
        "ShapeBase::readPacketData, the layer every control object opens with."),

    "Player": layout(
        shape_base_packet() + [
            bits(3),                        # action state
            if_flag(bits(7)),               # recovering, and for how long
            if_flag(bits(7)),               # a jump delay
            # Not mounted: the whole position and velocity.  This position is
            # the compression point the rest of the packet is relative to.
            if_flag(point3f(), point3f(), bits(4)),
            bits(32), bits(32), bits(32),   # head x, head z, rotation z
            # Piloting something: its packet data follows, and how wide that is
            # depends on which ghost the index names.
            if_flag(packet_data_of(10)),
            flag(), flag(),                 # move disabled, and pilot
        ],
        "Player::readPacketData, 0x5dab20 -- the control object a client gets "
        "the moment it spawns, which the shipped table had no layout for at "
        "all.  Cross-checked against t2-demo-parser (MIT, "
        "github.com/exogen/t2-demo-parser)."),

    "Turret": layout(
        shape_base_packet() + [
            bits(3),                        # turret state, a ranged u32 of 0..4
            bits(32), bits(32), bits(32),   # the rotation
        ],
        "Turret::readPacketData."),

}

PACKET_DATA["Vehicle"] = None    # filled in below, from vehicle_packet()


def vehicle_packet():
    """Vehicle::readPacketData, which the flying and hovering kinds share."""
    return shape_base_packet() + [
        bits(32), bits(32),                 # steering
        point3f(),                          # linear position, the new
                                            # compression point
        repeat(4, bits(32)),                # angular position, a quaternion
        point3f(), point3f(),               # linear and angular momentum
        flag(), flag(),                     # move disabled, frozen
    ]


# All three read exactly Vehicle's, so they get the same graph rather than a
# copy that could drift from it.
PACKET_DATA["Vehicle"] = layout(vehicle_packet(),
                                "Vehicle::readPacketData, 0x60d740.")

for _name in ("FlyingVehicle", "HoverVehicle"):
    PACKET_DATA[_name] = layout(
        vehicle_packet(),
        "%s::readPacketData, which is Vehicle's unchanged." % _name)

PACKET_DATA["WheeledVehicle"] = layout(
    vehicle_packet() + [
        flag(),                             # braking
        repeat(6, point3f()),               # six wheels, three floats each
    ],
    "WheeledVehicle::readPacketData, 0x615840 -- its parent's, then the brake "
    "and the wheels.")

# ------------------------------------------------------------- the events
# Most events came back from eventwalk.py agreeing with the shipped table, but
# agreeing on node count is not agreeing on bits.
EVENTS = {
    "SensorGroupColorEvent": layout(
        [
            bits(5),                            # which sensor group
            count_bits(32),                     # a mask of which colours follow
            loop_over(0, if_flag(repeat(4, bits(8)))),
        ],
        "The shipped table and eventwalk.py both give this fifty-eight nodes "
        "and both read 111 bits where a session carries 133.  Its colours are "
        "not a counted list: it sends a thirty-two bit mask and then an entry "
        "for each bit standing in it, each of those being a flag and, if set, "
        "four bytes of colour.  No op could say that until OpCountBits, which "
        "keeps a mask's population count.  Measured on a spawned session: the "
        "mask came back all ones with two colours carried, 5 + 32 + 32 + 2*32 "
        "= 133.  Cross-checked against t2-demo-parser (MIT, "
        "github.com/exogen/t2-demo-parser)."),
}

EV_OUT = os.path.join(HERE, "event_fixups.json")

PD_OUT = os.path.join(HERE, "packetdata_layouts.json")

if __name__ == "__main__":
    json.dump(EVENTS, open(EV_OUT, "w"), indent=1)
    for name, g in sorted(EVENTS.items()):
        print("event       %-16s %d nodes, root %d"
              % (name, len(g["nodes"]), g["root"]))
    json.dump(PACKET_DATA, open(PD_OUT, "w"), indent=1)
    for name, g in sorted(PACKET_DATA.items()):
        print("packet data %-16s %d nodes, root %d"
              % (name, len(g["nodes"]), g["root"]))
    existing = json.load(open(OUT)) if os.path.exists(OUT) else {}
    existing.update(LAYOUTS)
    json.dump(existing, open(OUT, "w"), indent=1)
    for name, g in sorted(LAYOUTS.items()):
        print("%-16s %d nodes, root %d" % (name, len(g["nodes"]), g["root"]))
