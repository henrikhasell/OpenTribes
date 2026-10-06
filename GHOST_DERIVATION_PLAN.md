# Derive a ghost's layout by playing a doctored recording

## Decision

Stop trying to watch retail decode a ghost and make the recording ask the
question instead. A start block is a byte string we can edit; retail reads it
deterministically; the result is observable from script. So the layout gets
measured by changing the bytes and watching what moves — one bit at a time
where that is what it takes, and by bisection where it is not.

This is what fills the gap `DEMO_PLAYBACK_PLAN.md` ends on, and it is not
demo-specific: a ghost body is a ghost body whether it arrives in a start
block or a packet.

## Why not just step the decode

A script breakpoint stops the game wherever the engine calls script, and
during the ghost run **the engine calls nothing**. `readDemoStartBlock` calls
`demoUpdateDatablockProgress` once per datablock, so the datablock run can be
walked body by body — but past it there is no hook until the block is done.
No amount of debugger gets inside `Player::unpackUpdate`.

What the debugger is for here is sequencing, and it earns its place twice:

- A **one-shot breakpoint on the line after `playDemo` returns** is a race-free
  sample point. `$timeScale = 0` only means no tick has advanced *yet*;
  a breakpoint means none can.
- **`EVAL <id> <frame> <expr>`** runs in a real frame, so a dump loop with
  `%local`s is safe — at retail's console the same loop kills the game.

Put the breakpoint in **our own exec'd driver script**, not retail's
`recordings.cs`: our file, our line numbers, and no question about whether the
`.dso` beside it was compiled from the `.cs` we are reading.

## The two oracles

**1. `$MissionName` is a one-bit checksum of everything read before it.**
The block reads the ghost run, then the control object, then a string into
`$MissionName` (steps 9f–11 in `Tools/t2/recnotes.md`). A string is read
through the Huffman reader, so if the cursor is one bit out the string is
garbage. Dump `$MissionName`; if it is what we wrote, every bit in front of it
was consumed exactly as we allotted. That is a pass/fail answer to "is this
body N bits wide" with no field knowledge at all.

**2. The decoded object's script surface.** `ServerConnection` is the
connection — `playDemo` names it that (0x005c322d) and ghosts are its children
— so `getCount()`/`getObject(%i)` enumerate, and then `getTransform`,
`getPosition`, `getScale`, `getDataBlock`, `getDamageLevel`, `getDamageState`,
`getEnergyLevel`, `getVelocity`, `isCloaked`, `getWorldBox` and the per-slot
`getMountedImage(slot)`, `getImageState/Ammo/Loaded/Target/Trigger(slot)`
report what the body actually filled in. The per-slot ones matter most: which
slot a value landed in is the one thing a layout's shape cannot tell you.

## The crafted start block

The key move, because it turns a 103,725-byte haystack into one body.

Take a real recording and rewrite only its start block:

- keep the connection string table and all 1,508 datablock events verbatim —
  a ghost names a datablock, and a ghost whose datablock never arrived is a
  different experiment from the one we want;
- keep the connection words, the moves and the `$DemoValue` run;
- keep the target manager, the connection-protocol block and the path manager;
- replace the **ghost run** with exactly one ghost: index 0, the class under
  test, and body bytes lifted from a real occurrence of that class;
- write the tail ourselves: the run's closing clear flag, control ghost id
  `-1`, a marker string, the trailing word, and the two process lists;
- fix the start-block size in the header.

The deflated block log copies over byte for byte and never has to agree with
anything, because the driver never lets a tick run, so no packet is ever
pumped. Writing the tail needs no Huffman encoder either: the raw string form
(clear flag, eight bits of length, the bytes) is what `rec.py` already reads.

With that file, `$MissionName` is a bit-exact checksum of **one** ghost body.

## What each experiment measures

**Width** — bisect. Allot the body N bits by placing the tail at N; the marker
survives only when N is right. Nine probes settle a 343-bit body instead of
343. This alone would have given `LinearProjectile`'s 144 bits without the
resync heuristic, and it works for a class whose indices are not sequential.

**Structure** — flip one bit and ask whether the marker survives. A bit the
reader branches on changes how much it consumes, so the marker dies; a bit
that is only data leaves the width alone and the marker lives. That classifies
every bit of the body as *branch* or *data* with one run each, and the branch
positions are `OpFlag`, `OpSelect` and `OpCount` — the graph, mechanically.

**The other side of a branch** — keep a branch bit flipped and re-bisect the
width. That is the fork the emulator does statically, done by measurement, and
it is how the graph grows past the single path a recording happens to
exercise.

**Fields** — flip one data bit and diff the dump. One property moving names
the field; the bits that move the same property are its extent; the delta
against the bit's position gives the weight, and so the width, the bit order
and the scale. A bit that moves nothing is read into something no getter
exposes: recorded as opaque, not as absent, because every bit inside a
measured body is consumed by definition.

**Initial, for free.** Every body in a start block is an initial one, so this
derives exactly the path the walk cannot reach.

## Phases

### 0. Harness

`Tools/t2/ghostprobe.py`: launch stock, open the console, call
`dbgSetParameters`, connect the debugger socket, `exec` the driver, set the
one-shot breakpoint, then per probe `stopDemoPlayback(); playDemo(<file>)` and
dump at the stop. `stopDemoPlayback` first is not optional — `playDemo` names
its connection `ServerConnection` and a second one would collide.

The dump is one sorted line per `(object, property)` so a diff is a text diff.

Proof: the baseline recording's dump is stable across runs and across client
restarts, and `$MissionName` reads back the real mission. An unstable baseline
means something is being sampled after a tick, and nothing downstream is worth
measuring until it is stable.

### 1. One crafted file that works — **done, 2026-10-06**

`Tools/t2/craft.py`. Confirmed against `Player` in `demo004.rec`, in both
readers:

| allotment | `rec.py` | retail |
|---|---|---|
| 342 | marker lost | access violation in `resolveGhost` |
| **343** | **`mission: 'PROBE'`, consumed to 2 bits of padding** | **`playDemo` 1, `$MissionName` `PROBE`** |
| 344 | marker lost | access violation in `resolveGhost` |

Two corrections the run forced, both now in the tool:

- **Truncate the run after the ghost under test; do not replace the run with
  one ghost.** A body can name an earlier ghost — retail creates, unpacks and
  registers one at a time — so a lone body whose references are missing
  crashes retail in `resolveGhost` (0x00584f8a) instead of answering. Keeping
  everything in front is also what makes the oracle sound: those are retail's
  own bytes, which retail reads correctly by construction, so a marker that
  dies can only be about the bits we changed.
- **A wrong width is a dead client, not a garbled marker.** One bit out and
  retail reads the closing flag as set, then a nonsense ghost header, and
  dies. The signal is still binary and unambiguous, but the sweep relaunches
  per failure, so **one probe per client launch** is the unit, and a bisect of
  nine probes means up to eight relaunches. Budget on relaunches, not on
  `playDemo`.

The harness mechanics that cost the most to find are in the
`driving-stock-for-demo-probes` note: one telnet connection for the whole
session, `setModPaths("base;Classic")` before anything, and `FileObject`
output landing in `GameData/base/`.

### 2. Width for every class the corpus carries — **done, 2026-10-06, by comparison**

Not by measurement. `exogen/t2-demo-parser` (MIT) decodes a `.rec` from the
same binary, hand-derived off the disassembly, and its `GhostUpdate` carries
`updateBitsStart`/`updateBitsEnd` — a width per body, which is what this phase
set out to measure a client launch at a time. `Tools/t2/xcwidths.py` runs it
and diffs.

Over the three recordings, 553 initial ghosts and 20 classes:

- **91 agreements, one disagreement.** Every width our tables can read matches
  theirs exactly, including the multiple widths a class takes on different
  flag paths (Player 343/354/361/365/377, Item 247/280/302/376).
- The disagreement is `LinearProjectile`: theirs 144, ours 46. Retail says 144
  — measured in phase 1, before this comparison existed. **The crafted probe's
  job turned out to be grading the oracle, not doing the work.**
- With their widths pinned, `rec.py` walks all three blocks to the end and
  consumes each **exactly**: 829,790 / 1,059,700 / 708,319 bits against
  103,725 / 132,464 / 88,541 declared bytes. Exact means
  `ceil(bits / 8) + 1 == size`, because `startDemoRecord` declares
  `getPosition() + 1`; anything looser is "near the end", not "to the end".

Two things that correction cost: I reported the `nastyhobbit` and `uploads`
stalls as layout faults, and they were not — their bodies are a `Player` at
343 and an `Item` at 280, which both readers agree on. What failed was my
sequential-next-index check, a heuristic about what a header looks like rather
than anything about a layout. And the widths are per path, so a class has
several: `TerrainBlock` alone comes out 263, 326 and 738 across three
missions.

What this does **not** settle is the fields. Their names have been wrong here
before while their widths held, so the bit map of phase 3 is still phase 3's
to do — the oracle gives the extent of each body, not what is inside it.

### 3. Structure and fields — **widths done, 2026-10-06; fields outstanding**

Reshaped from measurement to transcription once `t2-demo-parser` turned out to
carry 47 ghost readers, 286 named fields, and a branch on `isInitial` in every
one of them. Bit-flipping a body to find its fields is the slow way to learn
what a second decoder already states.

**`Tools/t2/describe.py`** is the transcription: a description is written in
the order the bits arrive, as statements (`opt`, `branch`, `point`, `normal`,
`string`, `count_loop`), and the node table is *generated* from it. The table
stays something a file produces, which is the whole of AGENT.md's objection to
hand-edited layouts.

**`Tools/t2/perclass.py`** grades it. A body cannot be checked where it stands
and a greedy walk drifts the moment one layout is wrong, so this pins the
second reader's width for every ghost *except* one, lets our layout read that
one, and compares. Each class's verdict is then independent of every other
class being right.

Two descriptions closed the corpus:

- **`LinearProjectile`**: our graph was missing the compressed point on all
  three of its branches — 46 bits where retail writes 144, and
  46 + 2 + 3×32 = 144 exactly. The point is `conn->readCompressedPoint`, a
  virtual on the connection, which is why the walk stepped over it.
- **`TerrainBlock`**: it extends SceneObject, so no datablock or target in
  front; a CRC, the `.ter` name, the detail texture, the square size and a
  counted run list. Its material-name list — the tail the walk judged inert —
  is **not in a start block at all**; retail guards it, and the second reader
  guards it on `isDemoStartBlock`.

Result, with those two described and **nothing pinned**: all three recordings
walk their whole start block and consume it **exactly** — 107, 283 and 163
initial ghosts, missions `Equinox`, `DangerousCrossing_nef`, `Damnation`.
**553 of 553 bodies**, 20 classes.

The surprise is how little was actually wrong. Only two classes of twenty
misread anything, where the "39 of 54 graphs have stepped-over calls" and
"only 9 model an initial body" counts had me expecting wholesale breakage.
Both counts were real and both were nearly harmless: 36 of 54 chains never
test `Added`, so their initial and update bodies are the same shape and want
no `OpInitial`, and most stepped-over calls are `this`-virtuals that read
nothing. The walk's output was in far better shape than its own warnings
suggested.

**The fields now have a check too** (`Tools/t2/xcfields.py`, 2026-10-06).
Widths agreeing says nothing about what is in a body, so a described class is
graded field by field against the same bytes: our walk records what each named
node read, the values are decoded the way the engine would decode them
(`readSignedFloat` as a unit float widened to -1..1, a normal vector as two of
those read as angles, a compressed point as a base plus hundredths or three
raw floats), and compared with the second reader's `parsedData` for every body
of that class in every recording.

**24 fields, 0 disagreements, and no gaps either way.** Getting to no gaps took
three fixes that were all ours: a node inside a loop has to accumulate its
values rather than overwrite them (a terrain run list is exactly that), a
branch flag the body keeps has to be named and not just branched on, and a
field whose flag was clear still has a value -- `targetId` reads -1 -- which
the engine has to set and a comparison that skips it is not comparing the
body. An `explodePosition` came out bit-identical to theirs, denormal and all.

**ShapeBase is described, 2026-10-06.** It is the body eleven classes read
before their own, and the expensive one: a damage section, four sound slots,
four thread slots, eight image slots, the shield/cloak/invincible section and
the mount. Two things in it are worth naming:

- the eight image slots each carry **one extra flag on an initial body only**,
  and the predicate is not in the stream -- retail tests `this+0x18 & 8`, the
  SimObject `Added` bit, at 0x005ef79a, because `unpackUpdate` runs before
  `registerObject`. That is the `OpInitial` node, and the second reader's
  comment reaches the same conclusion from the same address independently;
- the per-slot state flags are packed in an order the V12 SDK source does not
  use, so the slot fields are taken from the binary's own store offsets.

No recording carries a `ShapeBase` ghost, so it is verified **through the
classes built on it** -- the same standing the hand-written family had. `Item`
is described on top of it and graded directly: **46 Item bodies** across three
missions at the oracle's four widths (247, 280, 302, 376), and **1,240 field
comparisons with 0 disagreements, nothing they name that we do not, and
nothing we name that they do not**.

**Coverage: 4 classes of 20 described, 16 width-verified only.** Only a
described class can be graded, because the walk's graphs name nothing. `Item`
is the proof that `ShapeBase`'s body is right; `Camera`, `StaticShape`,
`Player`, `Turret` and the four vehicles are the classes that now cost almost
nothing to add, since their own fields sit on a body already checked.

One input is taken on trust: the compression point a position is relative to
comes from their parse. It is a connection-level value rather than a field of
any class, but until our reader tracks it the position comparison tests the
offsets and not the base.

### 4. Emit a layout from the map — **done, 2026-10-06**

`Tools/t2/genghost.py` writes `Engine/source/T3D/gameBase/t2GhostTablesRecovered.h`
by composing the two sources it now has: the emulator walk's arrays copied
through untouched, and a described class's array generated from
`describe.py`. The walk's output is not reproducible from this tree, so
regenerating the header from nothing would mean discarding 52 classes to fix
2 — composing is the honest version of "generated, not hand-edited", and the
provenance stays in the comment above each array: 52 say how many states were
explored, 2 say they were described and checked.

It also drops the select table a replaced class needed, because a description
branches on flags it names and the leftover array would be an unused static.
Running it twice gives the same file, which is the property that makes it a
generator rather than a patch.

Checked, in this order:

- the regenerated header **reads all three recordings exactly** — a fresh
  `rec.py` parsing that file, no descriptions injected, nothing pinned: 107,
  283 and 163 ghosts, each start block consumed to the bit;
- `t2Layout.cpp` compiles against it, warnings unchanged;
- the engine links (`ninja: no work to do` afterwards).

What this does **not** show is the engine decoding live traffic with the new
layouts: our engine cannot play a `.rec` yet — that is
`DEMO_PLAYBACK_PLAN.md`'s own work — so the in-engine demonstration waits on
it. What is shown is that the file the engine compiles is the file that reads
the corpus.

## Cost and coverage

A probe is one `playDemo` of a 103 KB start block and a dump, so seconds.
Width bisection is ~9 probes a class; the bit sweep is one probe a bit, a few
hundred a class. One class is minutes; the corpus is hours, and it
parallelises across clients at about 675 MiB of VRAM each.

**Coverage is the corpus, and that is the real limit.** Only a class that
appears in some recording can be derived this way.

The corpus is now **15 recordings**: the three local ones plus twelve from
tribesforever.com, all verified as build 25034 by their version word
(`0x00330004` at offset 18 -- retail compares it for equality, so anything
else is a different build and would read at different widths). All fifteen
start blocks consume **exactly**, across about 2,800 initial ghost bodies, with
no class failing. That took the corpus from 20 ghost classes to **24**:
`ForceFieldBare`, `Lightning`, `PhysicalZone` and `ParticleEmissionDummy`.

**And more demos will not go further, which is the finding that matters.** No
vehicle, projectile, `Debris`, `Splash`, `Shockwave`, `Trigger`, `Marker` or
`BeaconObject` ghost appears in **any** of the fifteen start blocks --
`VehicleBlocker` is the only class with "Vehicle" in its name, and we already
had it. Twelve files chosen on vehicle-heavy maps, several beginning fifteen to
twenty minutes into a match, changed nothing. A start block is the ghost scope
at the instant recording began; those classes are only ever ghosted in the
packet log that follows.

So the remaining 30-odd classes need a different tool: one that grades ghost
bodies in the **post-start packet log**. That is worth more than its coverage,
because packet bodies are *non-initial* -- a path through every layout that
nothing has verified yet, where the described classes have only ever been read
on the initial side. The second reader already carries
`updateBitsStart`/`updateBitsEnd` on packet `GhostUpdate`s too, so
`xcwidths.py` and `perclass.py` extend to it rather than being rewritten.

## Traps

- **A doctored bit can kill the client.** A nonsense datablock id or loop
  count will, and that is information: record the crash against the probe
  rather than suppressing it, and restart. A harness that hides a crash is
  measuring a different thing than it reports.
- **Values come back scaled.** `getPosition` is post-scale and a compressed
  point is relative to the connection's compression point, so a mismatch may
  be the base rather than the width. Fit the scale; do not assume it.
- **One occurrence is one path.** A body's width depends on the flags that
  particular ghost carried. Measure every occurrence in every recording and
  expect several widths per class — that is the graph showing through, not a
  contradiction.
- **While stopped the main loop is blocked**, so the console answers nothing:
  at a breakpoint the debugger socket is the only way in, and console output
  arrives there as `COUT`.
- **Probe files go in their own directory** and get cleaned up. They are
  recordings as far as the shell is concerned and would otherwise fill the
  RECORDINGS tab.
- **Update bodies are a second corpus.** They are in the block log, not the
  start block, so the same trick needs the deflate stream re-written. Worth
  doing after the initial bodies, not instead of them.
