# Tribes 2's recording format, as recovered

What `rec.py` implements, and where each step came from.  Addresses are in
retail `Tribes2.exe` (build 25034, image base 0x400000); the disassembly and
the lifted bodies are in `~/Documents/Repositories/Tribes2Decompiled`.

Measured against `Classic/recordings/demo004.rec`: 410,873 bytes, 0:34 of a
Classic mission with 17 players.

## The container

`NetConnection::replayDemoRecord` (0x00588000) reads, from the file stream:

| field | width | demo004 |
|---|---|---|
| `readString` | 1 + 17 | `"Tribes2 Recording"` |
| version | u32 | `0x00330004` |
| length, ms | u32 | 34,617 |
| start-block size | u32 | 103,725 |
| start block | that many bytes | uncompressed |

The version word is compared for exact equality against the constant at
0x00587c20 (`return 0x330004`), and its high half is kept as the connection's
protocol version — 51.  A file that fails the compare is refused, which is
what `RecordingsDlg` greys out.

Everything after the start block is read through a filter stream — a
0x20-byte object attached with a bool method at vtable+0x24, which is
`ZipSubRStream` — so the block log is **raw deflate** (`wbits = -15`).
Inflated it holds `[u16 type:4|size:12][size bytes]` records
(0x00588222-0x00588247), the same framing Torque uses.  In demo004 the log
inflates to 545,628 bytes and 22,872 records that tile it exactly:

| type | count | reading |
|---|---|---|
| 0 | 1,277 | received packet |
| 1 | 19,236 | send-packet marker, size 0 (one per frame) |
| 2 | 1,082 | move — 34,617 ms / 32 ms, exactly |
| 3 | 1,277 | 8 bytes, one per packet: the time base `getDemoCurrentTime` reports |

## The start block

Read in this order.  Byte reads are `Stream::read(n, ptr)`, which on a
BitStream is `readBits(n*8)` — unaligned, so the whole block is one bit
stream with no padding anywhere but the end.

| # | step | where |
|---|---|---|
| 0 | connection string table: 1024 × flag, each set one a string | 0x005898b0, called from `replayDemoRecord` **before** the connection's own block |
| 1 | u32 datablock total, then `demoUpdateDatablockProgress(0, total)` | 0x005fb5c0 |
| 2 | `while(flag) SimDataBlockEvent` | 0x005ffc90: a second flag, id(11), class(7), index(11), total(12), then the class's `packData` body |
| 3 | u8 first person (a global, not a member) | |
| 4 | 6 × u32 into the connection | +0x8384, +0x8388, +0x83f8, +0x83fc, +0x8400, +0x8434 |
| 5 | 16 × u32 | +0x83b8[16] |
| 6 | u32 move count, then that many moves | 0x00601800: 3 × (flag ? 16 bits), 3 × 6 bits, 1 flag, 6 flags |
| 7 | `while(flag) string` into `$DemoValue%d` | the globals `recordings.cs` reads back in `loadDemoSettings` |
| 8 | the target manager | 0x00670660 — see below |
| 9 | `NetConnection::readDemoStartBlock` | 0x00588260 |
| 9a | `ConnectionProtocol::readDemoStartBlock` | 0x0043d820: 32 × u32 of last-received-at-send, then last recvd, highest acked, last send, ack mask, connect sequence, last recv ack-ack, and a bool.  **Torque's, word for word.** |
| 9b | f32 round trip, f32 packet loss | |
| 9c | `PathManager::readState` | 0x00591ce0: u32 path count, then per path a u32 and a node count, then per node three u32 of position and one of time, interleaved |
| 9d | u32 notify count (allocates, reads nothing more) | |
| 9e | `eventReadStartBlock` | 0x00583ac0: u32 sequence, then `while(flag)` class(6)+`unpack` |
| 9f | `ghostReadStartBlock` | 0x00585220: u32 sequence, then `while(flag)` index(10) + class(7) + `unpackUpdate` |
| 10 | u32 control ghost index, then that object's `readPacketData` | ‑1 for none |
| 11 | string into `$MissionName` | |
| 12 | u32 | +0x8368 |
| 13 | two process lists: u8 and 4 × u32 each | 0x006021b0 over the globals at 0x009e82f8 and 0x009e5ec0 |

Three of these differ from Torque in ways that matter to the engine side:

- The **string table is read first**, from `replayDemoRecord`, not from inside
  `NetConnection::readDemoStartBlock`.
- The **datablock run is counted**: a u32 total up front, which is what feeds
  the progress bar.  Torque has no such word.
- **Ghosts are created, unpacked and registered one at a time.**  Torque
  creates every ghost first and only then unpacks any of them, so a body can
  name a ghost that comes later in the block; retail's cannot.

Class ids on the wire are table indices: 7 bits for an object or a datablock,
6 for an event.  Retail biases them into one flat table — datablocks by 0x80
(0x005ffd19), events by 0xff (0x00583afa), objects not at all — and the
widths are not literals in the binary either: it asks for
`log2(nextPow2(count))` through the two helpers at 0x0043f120 and 0x0043f150.

### The target manager (0x00670660)

Tribes 2's targeting, so nothing here came from Torque:

- one u32,
- a 32 × 32 grid of flag-gated u32 — sensor group against target type,
- 512 target slots, each a flag and, if set: two optional u32, five optional
  strings, a 5-bit field, a 9-bit field, a datablock id (11 bits, flag-gated,
  **only for slots from 32 up** — the first 32 skip the read outright), and
  7 bits of scale.

In demo004 that is 11,573 bits: 63 grid entries and 24 live targets.  The
shape was checked by emulating the retail function over the same file with
`BananaEngine/tools/t2/emu.py`, serving its reads from the real bits: it asks
for the same bits in the same order and ends on the same one.  The boundary it
lands on is confirmed from the other side too — what follows reads as
`ConnectionProtocol`'s 32 sequence words (191…253 wrapping to 187…190),
`established = 1` and a round trip of 17.4 ms.

## What a run reports now

```
datablocks  : 1508 read, header says 1508
moves       : 2
$DemoValue  : 70
targets     : 24 of 512 slots, 63 grid entries
connection  : established True, seq 253 acked 217, round trip 17.4 ms, 0 paths
ghosts      : 28 -- Camera 1, Player 16, Item 3, StaticShape 7, LinearProjectile 1
STOPPED     : LinearProjectile (ghost 27): our layout reads 46 bits, retail wrote 144
```

The stop is not a start-block question.  Ghost indices in the block run
0, 1, 2, … with no gap, so the bits after a body are known before it is read,
and exactly one body length in 8,192 lands on the next header — 144 where our
recovered `LinearProjectile` layout takes 46.

What the 98 missing bits are: a **compressed point**.  Retail reads a
projectile's position through `conn->readCompressedPoint(stream, &p, scale)`
— a virtual on the *connection*, vtable+0xc0 (`0x0062f22d`), not a method on
the BitStream — and a type-3 point is 2 + 3 × 32 = 98 bits.  46 + 98 = 144,
exactly.

Our graph has no `OpPoint` node for the class because the walk could not
resolve that indirect call and stepped over it: the table comment says
`LinearProjectile: 29 nodes from 221 explored, 3 calls stepped over`.  The
walk seats a fake BitStream so a read through the *stream's* vtable resolves;
it seats no connection, so a read through the connection's does not.

How far that generalises, counted rather than assumed — scanning each class's
`unpackUpdate` chain (direct calls into other slot-19 bodies followed, so
parents are included):

- **8 chains read through connection slot 0xc0**, and 5 of them already have
  an `OpPoint` because they were hand-described.  So the missing point costs
  exactly three classes: `LinearProjectile`, `LinearFlareProjectile` and
  `TracerProjectile`.  39 of 54 graphs do carry a "calls stepped over" note,
  but a stepped-over call only loses bits if the callee reads, and most of
  these are `this`-virtuals that do not.
- **17 chains contain the Added test** that `OpInitial` models, and 9 of those
  already have the node.  So that gap is eight classes — AIObjective,
  BeaconObject, MissionMarker, ScopeAlwaysShape, ShapeBase, SpawnSphere,
  WaterBlock, WayPoint, all of them reaching ShapeBase's per-image-slot test.
  The other **36 chains never test it at all**, which means their bodies are
  the same shape initial or not and they need no node.

So neither gap is 45 classes wide.  What is 45 classes wide is the question
itself: every graph was produced by running the function once, and nothing
says which other state-dependent branches were taken one way.

## What initial means, and how a graph gets an `OpInitial`

Retail's `unpackUpdate` takes `(conn, stream)` and nothing else — there is no
initial argument to fork on.  The predicate is the object's own state:

    ; ShapeBase::unpackUpdate, 0x005ef79a, in the eight-slot image loop
    mov  -0x58(%ebp),%eax     ; this
    mov  0x18(%eax),%eax      ; SimObject's flag word
    and  $0x8,%eax            ; Added
    je   0x005ef800           ; not added -> read one more flag

`+0x18` is the flag word `registerObject` clears the low two bits of
(0x00439d27, `andl $0xfffffffc,0x18(%esi)`), and bit 3 is `Added`.  Both
`ghostReadPacket` and `ghostReadStartBlock` call `unpackUpdate` **before**
`registerObject` (0x005852ae then 0x005852b3), so a brand-new ghost is not yet
added and an established one is.  "Initial" is `!isProperlyAdded()`.

So `OpInitial` is a fork on a value that is not in the stream, which is why
the walk never emits one: it runs the function once, with `this` reading as
zeroes, and takes whichever side that lands on.  Two ways to give a graph the
node:

1. **Fork the walk on it.**  The machinery is the one that forks on `readFlag`
   — snapshot, follow both, merge where they reconverge.  What is new is that
   the fork point is a test of `this+0x18 & 8` rather than a stream read, so
   it emits `{ OpInitial, 0, <clear>, <set> }` and consumes no bits, and the
   side taken has to be part of the memo key or the two subgraphs merge back
   into one.  Mark the cell the way a loop bound is marked, and fork where the
   marker reaches a `test`/`and`.
2. **Describe the class.**  The 12 graphs whose comment says `from 0 explored`
   came from `handlayout.py`'s DSL (`game_base()`, `shape_base()`,
   `mission_marker()`, `static_shape()`, …) and place `OpInitial` explicitly —
   that is where ShapeBase's, Player's and Item's eight each came from, one
   extra flag per image slot rejoining the update path.  Reliable, but one
   class at a time and it needs the body understood first.

Order matters: route 1 is also what fixes the stepped-over reads, because
seating a connection object whose vtable+0xc0 is a compressed-point read is
the same kind of change — give the emulator the object graph the function
reaches through, instead of letting it step over the call.  Fixing the points
is worth more than fixing initial: it is 39 graphs against 45, and a missing
point loses 98 bits where a missing initial flag loses one.

## Checking a layout against retail

Retail plays these files, so its own reader is the oracle -- and two things
make it reachable without a C++ debugger.

**The end state.** `$timeScale = 0;` before `playDemo(...)` (from an exec'd
file, not typed) leaves the world holding exactly what the start block said,
because playback only advances in the client process list per tick.  Ghosts
are children of the connection -- `conn+0xa0` is its `SimGroup` base, which is
why `ghostReadStartBlock` adds them there and why `ServerConnection.add`
appears in `commanderMap.cs` -- so `ServerConnection.getCount()` and
`.getObject(%i)` enumerate them.  What can then be compared is the script
surface: `getTransform`, `getPosition`, `getScale`, `getDataBlock`,
`getDamageLevel`, `getDamageState`, `getEnergyLevel`, `getVelocity`,
`isCloaked`, `getWorldBox`, and -- the useful ones -- the per-slot
`getMountedImage(slot)`, `getImageState/Ammo/Loaded/Target/Trigger(slot)`,
`getMountedObject(index)`, `getObjectMount()`.  Those test the one thing shape
cannot: which slot a value belongs to.

**Mid-decode, with the debugger.** Retail ships TGE's telnet script debugger:
`dbgSetParameters(port, pass)` at 0x0043a6c0, with `BRKSET file line clear
passct expr`, `CONTINUE`, `STEPIN`/`STEPOVER`/`STEPOUT` and `EVAL id frame
expr`.  Breakpoints carry a pass count and a condition.  Because
`readDemoStartBlock` calls `demoUpdateDatablockProgress(count, total)` once
per datablock (0x005fb65d), a breakpoint there steps the start block datablock
by datablock and `EVAL` reads retail's own cursor at each stop -- so a
datablock desync localises to one body instead of to the block.  The ghost run
calls no script, so it has no equivalent: there, the end state is all there is.

Limits worth knowing before trusting either: only script-visible fields can be
compared, retail's getters return the scaled and biased value where the reader
has raw bits (and a compressed point is relative to the connection's
compression point, so a position mismatch may be the base rather than the
width), and while stopped at a breakpoint the main loop is blocked -- the
telnet console answers nothing, so everything has to go through `EVAL`.
