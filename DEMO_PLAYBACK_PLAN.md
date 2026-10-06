# Play Tribes 2's own recordings

## Decision

OpenTribes plays `Classic/recordings/*.rec` — the files retail Tribes 2
writes — through the engine's existing demo machinery, with retail's
container and retail's start block. There is no OpenTribes recording format
alongside it, for the same reason there is no second wire protocol: a demo is
a log of Tribes 2 packets, and Tribes 2's packets are the only ones this
engine speaks.

The shell UI for this is already retail's and needs no work:
`base/scripts.vl2` ships `gui/RecordingsDlg.gui`,
`gui/DemoPlaybackDlg.gui`, `gui/DemoLoadProgressDlg.gui` and
`scripts/recordings.cs`, reached from the launch shell's RECORDINGS tab
(`scripts/LaunchLanGui.cs`). What is missing is entirely engine side: ten
console functions, retail's file container, and retail's start block.

## Why this is worth doing before it looks finishable

A recording is a better oracle than a join. `demo004.rec` holds 1,277
server→client packets from a real retail server, replayable offline, byte
identical every run, with no server to crash, no mission cycle, no
shader-compile stall dropping the connection, and no screenshot of a frame
the game chose not to draw. It is the thing
`captures-answer-connection-questions` argues for, except it also carries
the client's own moves and a full world snapshot, so it exercises ghost
decode, datablock decode, events, moves and the control object in one run.

Which means: playback lands structurally long before it renders a world.
That is not a reason to defer it — it is the harness the per-class decode
work has been missing.

## The container, measured

Fully determined, and checked against the real file rather than reasoned
about. `demo004.rec` is 410,873 bytes:

| field | bytes | value in `demo004.rec` |
|---|---|---|
| length-prefixed string | 1 + 17 | `"Tribes2 Recording"` |
| `U32` version | 4 | `0x00330004` |
| `U32` length, ms | 4 | 34,617 (00:34) |
| `U32` start-block size | 4 | 103,725 |
| start block | 103,725 | raw, uncompressed bit stream |
| block log | 307,118 | **raw deflate** |

The version word is compared for exact equality, and its high half is kept
as the protocol version — retail does `mProtocolVersion = version >> 16`, so
51 (`0x33`), with `4` the demo format revision. A file whose word differs is
rejected outright, which is what `RecordingsDlg` greys out by styling rows
whose `getDemoVersionLength` version does not match `getDemoVersion`.

The block log is **not** read from the file stream directly. Retail wraps it
in a filter stream — a 0x20-byte object whose `attachStream`-shaped method
sits at vtable+0x24 — and reads every record through that. Inflating from
offset 103,755 with `wbits = -15` yields 545,628 bytes which tile *exactly*,
to the byte, as 22,872 records of

    [U16: type:4 | size:12][size bytes]

That is Torque's own framing (`netConnection.cpp:1040`), and the filter is
Torque's own `ZipSubRStream`, which already inits as raw deflate
(`zipSubStream.cpp:80`, `inflateInit2(m_pZipStream, -MAX_WBITS)`). The record
types in that file:

| type | count | size | reading |
|---|---|---|---|
| 0 | 1,277 | ~317 B | received packet |
| 1 | 19,236 | 0 | send-packet marker (one per frame: 19,236 / 34.6 s ≈ 556 fps) |
| 2 | 1,082 | varies | move — 34,617 ms / 32 ms = 1,082 ticks, exactly |
| 3 | 1,277 | 8 | one per received packet; the time base `getDemoCurrentTime` reports |

Types 0 and 1 are `BlockTypePacket` and `BlockTypeSendPacket` at the same
ordinals Torque uses (`netConnection.h:1038`). Type 2 lines up with
`GameConnection::BlockTypeMove` (`gameConnection.h:112`). Type 3 has no
Torque equivalent and has to be read out of the binary.

Retail anchors, all in `~/Documents/Repositories/Tribes2Decompiled`:
`startDemoRecord` at `0x00587c30` (`src/lifted/netConnection.cc:3111`),
`replayDemoRecord` at `0x00588000` (`:3945`), the version constant at
`0x00587c20` (`:3104`, `return 0x330004`), the block framing read at
`:4319-4347`, `playDemo` at `0x005c31c0`
(`src/lifted/netDispatch.cc:3368`), and the whole console surface in
`metadata/console_commands.tsv`.

## The start block, recovered

Also settled now. The order is retail's `GameConnection::readDemoStartBlock`
(0x005fb5c0) and the chain under it, recovered from the disassembly and
implemented in `Tools/t2/rec.py`; the full table of steps and their addresses
is in `Tools/t2/recnotes.md`. On `demo004.rec` the reader gets through the
connection string table, all **1,508 datablocks** (the count the block's own
header gives), the connection words, the moves, 70 `$DemoValue` strings, the
target manager, the connection protocol block, the path manager and 28 ghosts
before stopping on a ghost layout — see below.

Three steps differ from Torque in ways the engine has to follow:

- The **connection string table is read first**, by `replayDemoRecord` itself
  (0x005898b0), not from inside `NetConnection::readDemoStartBlock` where
  Torque calls it (`netConnection.cpp:932`).
- The **datablock run is counted**: a u32 total before the run, which is what
  feeds `demoUpdateDatablockProgress`. Torque has no such word, and reading
  the run without it is what made the first attempt here look like a format
  mismatch rather than a missing field.
- **Ghosts are created, unpacked and registered one at a time**
  (0x00585220). Torque's `ghostReadStartBlock` (`netGhost.cpp:1308`) creates
  every ghost first and unpacks afterwards, which lets a body name a ghost
  from later in the block; retail's cannot, and the engine's two passes have
  to become one.

Two more findings worth keeping:

- `ConnectionProtocol::readDemoStartBlock` is **Torque's, word for word**
  (0x0043d820 against `dnet.cpp:276`): 32 sequence words, six more, a bool.
  It needs no work at all.
- The demo vars are `$DemoValue%d` **globals**, written by index with no name
  on the wire — not the `demo`-prefixed connection fields Torque writes
  (`gameConnection.cpp:919`). `recordings.cs`'s `getDemoValue` reads them
  back in that order, and the first three decode as the HUD state, the player
  count and a player row, in exactly the field order `getState(PLAYERLIST)`
  writes.

**The whole start block now reads.** With ghost body widths pinned from
`t2-demo-parser` (see `GHOST_DERIVATION_PLAN.md` phase 2), `Tools/t2/rec.py`
consumes all three recordings' start blocks exactly — 107, 283 and 163 initial
ghosts, missions `Equinox`, `DangerousCrossing_nef` and `Damnation`. Retail
confirms the mission independently: played against the client it loaded
`lighting/Equinox_743b8106.ml`, so the block carries enough to resolve the
lighting cache.

Still open: whether the mission CRC is in there. `sceneLighting.cpp:487`
resolves the lighting cache as `"%s_%x.ml"` from the mission name and CRC,
and the CRC normally arrives in a `SetMissionCRCEvent` from before the
recording started. The block does carry `$MissionName` and one more u32
(step 12) after it, which is the candidate — unread so far, because the
ghost run stops first.

## What already exists on our side

More than the state of the port suggests:

- The container machinery, needing a different header and a filter stream:
  `netConnection.cpp:977` `startDemoRecord`, `:996` `replayDemoRecord`,
  `:1040` `recordBlock`, `:1053` `handleRecordedBlock`, `:1080`
  `processNextBlock`.
- The playback driver, tick-accurate and already written:
  `stdGameProcess.cpp:164-178` pumps blocks until a move arrives, once per
  client tick.
- Sends already suppressed during playback (`netConnection.cpp:868`).
- `ghostReadStartBlock` (`netGhost.cpp:1308`) already calls the unified
  `unpackUpdate(stream, isInitial)` with `isInitial = true`, which is the
  distinction `shapebase-initial-update-differs` is about.
- `readDemoStartBlock` already reads the control and camera objects through
  `T2Layout::readPacketData` (`gameConnection.cpp:1008-1026`).
- `SimDataBlockEvent::unpack` (`gameConnectionEvents.cpp:165`) drives the
  layout walk and carries `mIndex`/`mTotal`, so the progress callback has
  its numbers.
- T2's Huffman weighting, so strings in the start block read correctly
  (`bitStream.cpp:704-730`).
- `$timeScale` (`mainLoop.cpp:268`), which is the only mechanism retail's
  playback speed control uses — `updateDemoPlaybackStatus` just assigns it.

Not one of the ten console functions exists. Verified: `getDemoVersion`,
`getDemoVersionLength`, `playDemo`, `startRecord`, `stopRecord`,
`isRecordingDemo`, `isPlayingDemo`, `stopDemoPlayback`,
`getDemoCurrentTime`, `isDemo` are all absent from `Engine/source`. Note they
are **global** functions in retail, not `NetConnection` methods — Torque's
`GameConnection::playDemo` method (`gameConnection.cpp:2150`) is the wrong
signature and the wrong name for these scripts.

## Implementation

The research is finished: the container is measured, the start-block order is
recovered and implemented in `Tools/t2/rec.py`, and the ghost tables now read
every initial body in a sixteen-recording corpus exactly. What is left is
engine work, and none of it is started. Each step below says what to change,
and how it is proven — every one is checkable against `rec.py` on the same
file, which is the point of having built it first.

### 1. The container

`netConnection.cpp:977` `startDemoRecord` and `:996` `replayDemoRecord` carry
Torque's header; they need retail's. Read the length-prefixed signature,
compare the version word for **exact** equality (`0x00330004`), keep its high
half as `mProtocolVersion`, read the length and the start-block size, then
read the block itself. Wrap everything after it in `ZipSubRStream`, which
already initialises as raw deflate (`zipSubStream.cpp:80`) — but note
`attachStream` leaves `m_uncompressedSize` at 0 (`:63`), so end-of-stream has
to come from `Z_STREAM_END` and not from a size compare.

The record framing is already ours (`:1040` `recordBlock`), so only the two
extra block types are new: type 2 is the move, which `GameConnection`'s
`handleRecordedBlock` (`gameConnection.cpp:874`) already understands, and
type 3 is an eight-byte record written once per packet that nothing in Torque
has an equivalent for. It is the clock — see step 4.

*Proven by:* `replayDemoRecord` on each of the sixteen recordings reaching EOF
with the same block and packet counts `rec.py` reports.

### 2. The console surface and the script contract

Ten **global** console functions, not `NetConnection` methods — Torque's
`GameConnection::playDemo` (`gameConnection.cpp:2150`) is the wrong name and
the wrong signature. `getDemoVersion`, `getDemoVersionLength`, `playDemo`,
`startRecord`, `stopRecord`, `isRecordingDemo`, `isPlayingDemo`,
`stopDemoPlayback`, `getDemoCurrentTime`, `isDemo`. `getDemoVersionLength`
answers `version TAB lengthMs` and `-1` for a file it cannot read, which is
what the dialog renders as `"???"`.

Four callbacks the scripts expect: `demoPlaybackComplete`,
`demoRecordComplete`, `demoPlaybackUpdate(curTime)` after each processed step,
and `demoUpdateDatablockProgress(count, total)` during the start-block load.

The demo vars are `$DemoValue%d` **globals** written by index with no name on
the wire, not the `demo`-prefixed connection fields Torque writes
(`gameConnection.cpp:919`). `playDemo` must also name its connection
`ServerConnection`, as retail does (0x005c322d) — which is why
`stopDemoPlayback` has to come first between plays, or the second one collides
on the name.

*Proven by:* the shell's RECORDINGS tab listing the corpus with the right
lengths and no greyed rows, driven from the console with no GUI interaction.

### 3. The start block's three departures from Torque

All three are in `readDemoStartBlock`, and all three are ordering rather than
layout:

- the **connection string table is read first**, by `replayDemoRecord` itself
  (0x005898b0), not from inside `NetConnection::readDemoStartBlock` where
  Torque calls it (`netConnection.cpp:932`);
- a **u32 datablock count** precedes the run, and is what feeds
  `demoUpdateDatablockProgress`;
- **ghosts are created, unpacked and registered one at a time** (0x00585220),
  where `ghostReadStartBlock` (`netGhost.cpp:1308`) creates all of them before
  unpacking any. That path is demo-only — the live packet path is
  `:625`/`:681` — so collapsing it to one pass touches nothing else. It
  matters because a body may name an earlier ghost and must not resolve one
  that has not arrived.

The rest of the chain is in `Tools/t2/recnotes.md`, step by step with the
address each came from. `ConnectionProtocol::readDemoStartBlock` needs no work
at all: it is Torque's, word for word.

*Proven by:* the engine consuming each start block exactly — `ceil(bits/8) + 1
== size`, because `startDemoRecord` declares `getPosition() + 1`. Anything
looser also accepts a reader that is two bits short.

### 4. Driving it, and the clock

`stdGameProcess.cpp:164-178` already pumps blocks until a move arrives, once
per client tick, and `sendPacket` already returns early while reading
(`netConnection.cpp:868`). What is missing is the clock: `getDemoCurrentTime`
reads the type-3 record, `demoPlaybackUpdate` is called with it, and
`$timeScale` (`mainLoop.cpp:268`) is the only speed control retail's UI uses —
`updateDemoPlaybackStatus` just assigns it, with index 0 meaning zero.

One thing to check rather than assume: a demo connection receives no real
datagrams, so `NetInterface`'s timeout and ping paths must not reap it
mid-playback.

*Proven by:* `getDemoCurrentTime` advancing to the header's length;
`$timeScale` at 0 freezing it and at 16 finishing in seconds;
`demoPlaybackComplete` firing once.

### 5. The world

This is where playback stops being parsing. The start block creates every
ghost and the layouts now read them all to the right width — but a width is
not a field, and only four classes are described well enough for the engine
to act on their contents (`GHOST_DERIVATION_PLAN.md`). Expect a correct,
complete, silent parse of a world that renders as very little, and treat that
as the honest intermediate result: "playback works" means the file is
consumed exactly and the connection never desyncs, which is a different claim
from "the match is visible".

The mission comes from `$MissionName` plus the CRC the block carries, which is
how retail resolved `lighting/Equinox_743b8106.ml` from a demo on this
machine. Confirming which word is the CRC is still open.

### 6. Recording, last

`startRecord`/`stopRecord` close the loop, and a `.rec` we write should play
in retail — the strictest oracle available. The container half is
straightforward (header, deflate, framing, the length patched in at
`stopRecord`); the start block is not, because `ghostWriteStartBlock` would
have to encode bodies and `T2_UNIFICATION.md` defers encoders. Worth doing for
the container; not worth chasing a retail-accepted start block yet.

## How this is proven at every step

Three oracles, in increasing cost:

1. **`Tools/t2/rec.py`** on the same file. Offline, deterministic, and it
   reports exact consumption — the engine should agree with it bit for bit.
2. **The second reader**, through `xcwidths.py`, `perclass.py` and
   `xcfields.py`, for anything about widths or fields.
3. **Retail itself**, over telnet: play the same recording in the stock client
   and compare `$MissionName`, `ServerConnection.getCount()` and the per-slot
   getters. `Tools/t2/craft.py` builds a doctored recording when a single
   body needs pinning down.

And a corpus that can now be manufactured: a stock client recording while
objects are spawned on the local server produces a recording with whatever
classes are wanted in it, which is how the vehicles were got on the wire.

## Risks

- **The start block may carry per-mod state.** `demo004.rec` was recorded
  under Classic with TribesNext loaded (`tribesnext-patches-exe-in-memory`),
  and the demo vars come from whatever scripts were running. Record a second
  demo from a plain Classic session before concluding a field is part of the
  format.
- **`stock-runs-patched-scripts`**: retail loads `.dso` our engine cannot
  read, so any script-visible difference during playback may be the scripts,
  not the engine. Compare at the engine boundary.
- **One file is one sample.** Record several demos of different lengths and
  missions, including a deliberately unclean stop (`recordings.cs` handles a
  zero length, so retail does produce those), before trusting a field that
  only varies between files.
- **`a-shrinking-reproducer-is-not-a-cause`**: when a start-block width gets
  fixed, re-run the full file, not the trimmed case that found it.
