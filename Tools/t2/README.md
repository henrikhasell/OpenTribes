# Replaying a Tribes 2 session

A live Tribes 2 server is not a test fixture. It cycles missions, its
scripts run on a clock, and how far a decode gets varies between boots. So a
change to the reader is judged against a recorded session instead: the same
bytes give the same answer every run.

    python3 Tools/t2/capture.py 28014 127.0.0.1 28000 stock.capture

sits between a client and a server and writes every datagram the server sends
as a 32-bit length and then its bytes. Point a client at the proxy's port
rather than the server's and play through whatever is to be decoded. A stock
`Tribes2.exe` client is the useful one to record, because it completes the
mission handshake the port cannot yet finish, so the capture contains the
traffic that comes after.

    python3 Tools/t2/replay.py stock.capture

walks that session through the layouts in `Engine/source/T3D/gameBase/t2*.h`
and the class table in `Engine/source/console/netClassTableT2.cpp`, reading
both straight out of the tree so what is tested is what the engine uses. It
reports how far it got and what stopped it:

    packets        : 293, of which 274 data
    datablocks     : 1508   last (1507, 'AudioProfile')
    ghost always   : 123
    ghosts created : 21, updates 22, live 144
    control object : Camera

A packet that reads out with bits left over is reported as a failure, not
passed over: a body read short is otherwise only caught by whatever fails
after it, and the last thing in a packet has nothing after it. Without that
check a session looks understood when it is not.

`T2_KEEP=1` carries on past a packet that will not read and lists every
failure rather than the first. `T2_TRACE=<class>` prints every node of that
class's layout as it is walked, with the bit each one took, which is how a
reader that disagrees with its layout is tracked down. `T2_VERBOSE=1` names
each packet, event and ghost as it goes.

`TRIBES2_EXE` points at the retail binary, which is read for the Huffman
frequency table; the default is the usual Wine install.

Nothing from the game is kept here -- neither the capture nor the binary --
only the code that reads them.

## The coverage workbook

    T2_CAPTURE=stock.capture python3 Tools/t2/collect.py
    python3 Tools/t2/mkbook.py

writes `tribes2-wire-protocol.xlsx` in the root of the tree: the wire layer by
layer with every field's width, the constants beside stock Torque's, and a row
per net class saying whether its layout is known, what reads it, and whether a
recorded session ever exercised it.

`collect.py` reads the class table and the layout headers out of the tree and
measures the rest off the capture, so the workbook says what the code says
rather than what anyone remembers. It leaves `coverage.json` behind as an
intermediate. The workbook itself is not tracked -- it is regenerated, not
edited.

## Joining with the test client

`connect()` takes the five arguments `JoinGame()` sends: name, race and
gender, skin, voice, voice pitch. The race and gender has to be one of
`"Human Male"`, `"Human Female"` or `"Bioderm"` -- anything else and the
server logs "Invalid race/gender combo passed" and falls back, which is worth
not triggering while chasing a protocol fault.

## Recovering a ghost layout

`t2GhostTables.h` came from a walk that halted on any call it could not step
into, so a class whose reading sits below its parent's `unpackUpdate` came out
a stub -- `AudioEmitter`'s graph had six nodes and took 37 bits where its
neighbours take six hundred, which loses the rest of any packet it shares.

    python3 Tools/t2/ghostwalk.py --all      # writes ghost_graphs.json
    python3 Tools/t2/genghost.py             # writes t2GhostTablesRecovered.h

walks each object's `unpackUpdate` with every function the emulator might
enter already in its map, steps over the pure-maths helpers rather than
halting on their MMX, and steps over calls it still cannot resolve -- counting
those, so a layout that rests on the assumption says so in its comment.

A recovered graph is only written out if it is **acyclic**. A counted loop that
failed to unroll shows up as a back edge, and such a graph reads whichever way
out of the cycle the bits happen to steer: that is how a six hundred bit body
comes out as thirty-six. A layout like that is worse than none, because it
looks like an answer. `t2GhostTables.h` answers for whatever is dropped, and
`T2Layout::readGhost` consults the recovered table first, falling back only
when the name is absent -- never after a walk has already moved the stream.

The walk reaches a class's parent only because the class's own vtable is seated
in the emulated object. Without it the virtual call a class makes to its
parent's `unpackUpdate` dispatches through a null pointer, and `Precipitation`
came out with that call merely stepped over. Memory the walk never wrote reads
what the loaded image holds, too, which is what makes a jump table resolve:
`jmp *table(,%eax,4)` is how Tribes 2 compiles a switch, and reading zero there
lost `ShapeBase` and everything under it.

## Judging a recovered layout against a session

Acyclic is not the same as right.

    python3 Tools/t2/vetghost.py live.capture dm.capture stock.capture

replays the captures, blames the classes that appear only in packets that
failed, and then -- because sharing a packet with the guilty class looks
exactly the same -- vetoes each suspect on its own and keeps it out only if the
sessions measurably read better without it. What it decides goes in
`ghost_veto.json`, which `genghost.py` leaves out.

`Sky` is the case the tool was written for. Its cloud layers are read by a loop
of exactly three; the walk took the bound off the wire instead, so the layout
read a thirty-two bit field as a repeat count and lost the rest of the packet.
Nothing in the graph's shape says which reading is right -- `TerrainBlock`
really does read its material count as a thirty-two bit word -- so only a
session can tell them apart. Of seven classes the co-occurrence heuristic
suspected, the leave-one-out measurement cleared six.

Losing those bits lost four guaranteed events with them, and losing a
guaranteed event stalls the whole ordered queue: the client had read all fifty
ghost-always objects and sat in phase two holding a `MissionStartPhase3` it
would never act on.

## Checking against a second reader

[t2-demo-parser](https://github.com/exogen/t2-demo-parser) (MIT, Brian Beck)
decodes the same three tables out of the same retail binary, by reading the
disassembly rather than by emulating it. Two independent readings of one
recorded session settle far more than one can:

    git clone https://github.com/exogen/t2-demo-parser
    T2_DEMO_PARSER=../t2-demo-parser \
      python3 Tools/t2/xcdiff.py live.capture play.capture

replays each capture through both and prints, per class, the body lengths each
side saw. Where they agree the layout is settled; where they differ exactly one
is wrong about a field, and the capture says which.

That is how two layouts here were fixed. `TerrainBlock` read 1219 bits where a
session carries 1735: the walk judged the guarded call ending its unpackUpdate
to read nothing, and it reads the material-name list. `Precipitation` read 243
bits on one session and 339 on another where both carry 379. Both are written
out in `ghost_fixups.json`, each with why.

Twenty-two classes agree exactly, over four sessions and some fourteen hundred
objects, and nothing the two both read disagrees: AIObjective, AudioEmitter,
Camera, ForceFieldBare, InteriorInstance, Item, Lightning, MissionArea,
MissionMarker, PhysicalZone, Player, Precipitation, Sky, SpawnSphere,
StaticShape, Sun, TSStatic, TerrainBlock, Turret, VehicleBlocker, WaterBlock,
WayPoint.

`crosscheck.mjs` is the harness that drives it, and `tsresolve.mjs` lets Node
read that checkout without a build step. A proxy capture holds only the
server's side, so the harness seeds the parser with
`passiveObserverProtocolState`; without it the ack window rejects every packet
and the run comes back clean having read almost nothing.

### Writing a layout by hand

Some layouts cannot come from a walk of the bits at all, because the bits are
not the only input. `ShapeBase::unpackUpdate` reads one flag per mounted image
slot only when the object is not yet in the scene -- so a ghost-always body and
a newly scoped ghost read one length and an update to a ghost already made
reads another, and no amount of looking at the stream can tell which rule
applies.

`T2DataOp` gained `OpInitial` for it: a node that branches on which kind of
update this is rather than on a bit, with `T2Layout::readGhost` taking that
from its caller. `Tools/t2/handlayout.py` then writes the ShapeBase family out
compositionally --

    python3 Tools/t2/handlayout.py     # -> ghost_fixups.json
    python3 Tools/t2/genghost.py       # -> t2GhostTablesRecovered.h

-- describing each section in the order the binary reads it and letting the
builder allocate node indices. `shape_base()` is shared by Player, Camera,
Item, StaticShape and Turret, so the layer is written once and measured five
times over.

Every one of them agrees with t2-demo-parser to the bit. Player's nine bodies
in one session came out 276, 295 four times, 329 three times and 351, against
the 126 to 166 bits the walk had been reading.

### What the control object sends

A client is given an object to drive, and its state comes each tick through
`readPacketData` -- vtable slot 66 -- not through `unpackUpdate`. That is a
table of its own, and the shipped `t2PacketDataTables.h` had one entry:
`Camera`, which is what a client watching a mission is given. The moment it
spawns the control object is a `Player`, and there was no layout for that at
all.

    python3 Tools/t2/genpacketdata.py    # -> t2PacketDataTablesRecovered.h

writes the seven that were missing, from `handlayout.py`. Getting a session to
exercise them takes a spawn, which the mod does on request:

    commandToServer('ClientJoinGame');

Player's is the one no walk could have produced. When the player is piloting,
what follows is the vehicle's own packet data, and how wide that is depends on
which ghost the index names -- so `OpPacketDataOf` asks the connection, through
a resolver `T2Layout::readPacketData` takes from its caller.

Camera's shipped layout was also wrong, and in a way only a session showed: it
began at the position, missing the two floats `ShapeBase::readPacketData` sends
first. Sixty-four bits at the head of every packet carrying control data, which
threw out the event section after it.

### Where the coverage stands

All fifty-three ghost classes, all twenty-six events, all fifty-four
datablocks and eight control objects have a layout. Eight recorded sessions --
4,050 packets, including one where the client spawns and plays -- read end to
end with nothing left over and no gap in the guaranteed-event sequence.

Seventy-four layouts agree with the second reader exactly and none disagree:
twenty-three ghost classes, forty-nine datablock classes, and both control
objects. The three class-id tables match its own to the entry, 53 / 54 / 26.

Live, the client completes all four mission phases and reads ghost bodies
without a warning. What stops it now is the renderer: Tribes 2's scripts know
nothing of Torque 3D's render pass manager, so `SceneManager` asserts as soon
as it tries to draw.

### Three things the bits cannot say

The layouts are a description of a bit stream, and three times that turned out
not to be enough. Each needed an op, and each op is a place where the caller
knows something the stream does not:

- `OpInitial` -- ShapeBase reads a flag per mounted image slot only when the
  object is not yet in the scene.
- `OpPacketDataOf` -- Player reads the packet data of whatever it is piloting.
- `OpCountBits` -- `SensorGroupColorEvent` sends a thirty-two bit mask and then
  an entry per bit standing in it, not a counted list. The shipped table and
  `eventwalk.py` both gave it fifty-eight nodes, agreeing with each other and
  both reading 111 bits where a session carries 133. Matching node counts is
  not matching bits.

## Recovering an event layout## Recovering an event layout## Recovering an event layout

    python3 Tools/t2/eventwalk.py             # writes event_graphs.json

does the same for the twenty-six net events. An event's `unpack` is not in the
class's vtable at a fixed slot like `unpackUpdate`: it sits four words past the
`.data` word holding the class's `getClassRep`, which is how it is found here.

Twenty-four of the twenty-six walk, and every one that `t2EventTables.h`
already carried came back with the same node count -- an independent check on a
table recovered by other means. `SimTargetAudioEvent` is new. The one
disagreement worth noting is `Sim3DAudioEvent`: the walk reads three floats
where the hand-written reader reads three bytes and a position.
