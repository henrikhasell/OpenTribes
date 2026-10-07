# Put the ghosts a start block builds into the world

## The state

`demo004.rec`'s start block creates 107 ghosts and **40 of them will not
register**, so they are read but absent. The run carries on past them, which is
what keeps consumption exact, and `ghostReadStartBlock` warns with a count.

That count is the whole of what we know. The first thing this plan does is stop
guessing at the cause, because two guesses have already been wrong:

- *"They have no datablock."* `ShapeBase::onAdd` prints an error naming the
  object when its datablock is null, and the engine log for a full playback has
  **zero** of those. Whatever fails, it is not that check.
- *"The Players are among them."* Likely but unmeasured: offline every one of
  the 17 Players reads datablock id 187 correctly, yet none appears among the
  six ghosts that resolved one. Registered-without-a-datablock and
  never-registered look identical from outside.

It matters beyond tidiness. The control object is a ghost like any other, and
`Precipitation::processTick` asks it for an eye transform every tick — so a
missing control object is the segfault that ends playback a few seconds in.

> **Executed 2026-10-06 on `opentribes`.** Steps 1, 3 (two of three families), 4
> and 5 are done; the measurement below replaces the guesses. `demo004.rec`
> went from 40 of 107 ghosts unregistered to 16, and playback now reaches the
> packet log instead of a segfault. What remains is recorded at the end.

## 1. Read what the body holds before instrumenting anything

Half of this question is already answered offline. `t2-demo-parser` reports
every field of every ghost in the block, so what each failing class *has* needs
no engine work to discover:

| class | what its body carries |
|---|---|
| `InteriorInstance` | `interiorFile`, `crc`, `scale`, `audioProfileId`, `alarmState` |
| `TerrainBlock` | `terrFileName`, `detailTextureName`, `squareSize`, empty-square runs |
| `AudioEmitter` | `filename`, `transform`, min/max distance, loop gaps |
| `Sky` | `materialList`, `cloudLayers`, fog volumes |
| `Sun` | `direction`, `color`, `ambient`, `textures` |
| `AIObjective`, `SpawnSphere`, `WayPoint` | the ShapeBase body, **including `dataBlockId`** |

That last row matters: the ids those three need are in the stream and our
walked graphs simply never reach them. And the names in the rows above are
exactly what an `onAdd` would want to load — a `.dif`, a `.ter`, a sound file —
which the generic walk discards.

So the engine instrumentation is only needed for the half a parser cannot
answer: **why our `registerObject` returns false.** It has no object model, no
`onAdd` and no scene graph; it decodes bits and stops. Replace the failure
count with a line per failure — class, ghost index, whether a datablock
resolved, and the first link in the `onAdd` chain that returned false. Each
class's `onAdd` begins with `Parent::onAdd()`, so a failure is a chain and the
interesting end is the bottom; a temporary `Con::errorf` at each `return false`
in the `GameBase` → `SceneObject` → `ShapeBase` chain answers it directly.

*Answers:* which classes fail and at which test — with what their bodies
contain already known.

## 2. Ask retail what should be there

Retail plays the same file and builds the world, so what it holds is the
answer. Play `demo004.rec` in the stock client with `$timeScale` at 0, walk
`ServerConnection`'s children, and tally them by class; our engine's own count
script already does this on our side. The diff is the worklist, class by class,
and it also settles whether a class is *absent* or merely *datablock-less*.

`Tools/t2/rec.py` gives the third column — what the file actually contains —
so each class has three numbers: in the file, in retail, in ours.

*Answers:* the exact gap, and whether any failure is ours alone rather than
something retail also declines to build.

## 3. Classify, then fix by family

Three families are likely, and the instrumentation will say which apply:

- **No datablock, because the layout never reaches its id.** Four graphs are
  known to do this — `AIObjective` across 55 bodies, `SpawnSphere`, `WayPoint`,
  and `InteriorInstance`, which reads an id where there is none. Describing
  them is already on the list for its own sake, and it would supply datablocks
  to 61 of the block's ghosts.
- **A resource named in a field we discard.** `InteriorInstance` needs its
  `.dif`, `TerrainBlock` its `.ter`, a `ShapeBase` its shape. Those names are
  in the body and the generic walk throws them away, so `onAdd` has nothing to
  load. This family is fixed by describing the class and acting on the field,
  not by anything in the ghost loop.
- **State the engine builds elsewhere.** Anything that fails with a datablock
  and its resources present belongs here, and each one is its own small
  question.

Fix cheapest first and re-measure after each, against step 2's table.

## 4. Decide what an unbuildable ghost is

A policy question the code currently answers by accident. `mLocalGhosts[index]`
is set before `unpackUpdate`, so an unregistered object still resolves by
index — which is what keeps a later body's `OpPacketDataOf` and a mount
reference working. That is the right behaviour and should be deliberate and
commented rather than incidental.

What is not right is that the control object can be one of them. If the ghost
the connection is about to drive failed to register, that must be loud: a
warning naming the class, and the playback path declining to set a control
object it cannot use, rather than the render path finding out with a null
dereference.

## 5. Stop the crash being how we learn

`Precipitation::processTick` dereferences the control object's datablock
through `Player::getEyeBaseTransform`. Guard it — but log when the guard fires,
because a guard that hides its own cause is how this took two sessions to
notice in the first place. The goal is that an incomplete world yields a quiet,
complete playback rather than a segfault, so that "does it play to the end" and
"is the world complete" become two questions instead of one.

## Proofs

- Per class, our count matches retail's for the same recording, or the
  difference is named and explained. **Met for `demo004.rec` on 2026-10-07:**
  15 of 15 classes and 107 of 107 ghosts agree across the file, retail and
  ours, and retail reads the 1,508 datablocks the header declares.

  Retail will not answer this by being inspected after the fact: it plays the
  whole recording and tears the world down, so `ServerConnection` is absent
  once `demoPlaybackComplete` has fired. Set `$timeScale = 0`, then send the
  tally in the same breath as `playDemo` -- the start block is decoded inside
  that call, so the next command runs with the world up and no packets
  played. `Tools/t2/probe/demo.cs` is the script; it counts the decode by
  overriding `demoUpdateDatablockProgress`, which retail calls from inside
  `readDemoStartBlock`, so nothing depends on a frame being drawn.
- `demo004.rec` plays to end of file: `demoPlaybackComplete` fires exactly
  once, and `getDemoCurrentTime` reaches the header's 34,617 ms.
- All sixteen recordings still consume their start block exactly — the
  registration work must not move a single bit.
- No ghost the connection drives is one that failed to register.

## What this is not

It is not the world rendering. A registered ghost with a default transform sits
at the origin; making it appear where the recording says needs its fields, which
is the per-class description work. This plan ends when the objects exist, the
control object is real, and a recording plays from first packet to last without
dying.

## What executing it found

Instrumentation named the cause in one run: a line per failure plus a probe
printing each body's datablock id and what it resolved to.

- **The id was the wrong one.** A body reads several `OpDataBlockId` nodes --
  the object's own first, then one per mounted image slot, plus ids the walk
  typed that way which are really audio profiles. Taking the last gave a
  Player its `ShapeBaseImageData` and a StaticShape an `AudioProfile`, so
  `onNewDataBlock`'s cast failed. First-wins registered all 17 Players and
  7 StaticShapes: 40 failures became 16.
- **`TSStatic` and `InteriorInstance` name their resource in the body.** Both
  were discarding it. Keeping a body's first string, and prefixing the
  Tribes 2 resource directory, registered 277 of `tf_1063`'s 297 failures.
- **The segfault was never `Precipitation`.** That path already guards its
  camera object. The crash was a Player with no `TSShapeInstance`, reached
  four different ways; `ShapeBase::onAdd` now keeps a shapeless object out of
  the process list and names the datablock that named no shape.

## What is left

- **`InteriorInstance` needs its CRC.** With the name supplied, `_loadInterior`
  fails on `mCRC != getChecksum()` -- silently, because that branch only calls
  `setLastError`. Disabling the check proved the family would then register,
  but loading real interiors wedged the engine for minutes in material prep,
  so the CRC is a field to read, not a check to remove.
- **Reading it needs field access the engine does not have.** `T2DataNode`
  carries no name, so a class cannot ask for `crc`. The three captures that
  exist (`firstDataBlockId`, `firstString`, `lastShapeFile`) are positional
  and do not generalise. Either the generated tables grow a name column, or
  each field gets another positional capture.
- **`SpawnSphere`, `WayPoint` and `MissionMarker` still read no id at all.**
  Their graphs never reach one, which is the description work.
- **`TerrainBlock` fails on its material list**, one per mission.
- **Playback now ends on `invalid new ghost class id`** in the packet log,
  which is the transient-class gap, not a registration one.
