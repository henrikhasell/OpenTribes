# Agents

Read `CONTRIBUTING.md` and follow it. The rest is what it does not cover.

## The goal

OpenTribes re-implements Tribes 2's engine, a proprietary one, on top of
Torque 3D, which is MIT licensed and open source. The exe's behaviour is
what gets re-implemented. Tribes 2's game data and its retail scripts are
not ours and are not reimplemented: the scripts are stock content this
engine parses and runs through Torque 3D's TorqueScript interpreter, so
whatever they expect of the engine beneath them, the engine has to
provide. Retail Tribes 2 is the oracle every answer is checked against.

## The port

- Torque 3D's engine, Tribes 2's game data, build 25034.
- The game's scripts are retail, from 2001, and not ours to edit.
- They know nothing of Torque 3D. What Torque 3D expects a script to set up,
  set up in the engine and name the script that would have.

## The wire

- Tribes 2's protocol is the only network format this engine speaks. There
  is no Torque-native wire format to fall back to, pick between, or keep in
  sync. Every peer is a Tribes 2 peer, including our own server over
  loopback.
- So every class has **one** `packUpdate`/`unpackUpdate`, one
  `packData`/`unpackData`, one event pack/unpack, one packet-data pair, and
  that one pair speaks Tribes 2's layout. A `*T2`-suffixed sibling is not a
  thing any more; do not add one back.
- The layouts are the recovered, table-driven ones: `t2Layout.cpp` walking
  `t2*TablesRecovered.h`. A wrong-looking layout is a recovery-tooling bug,
  never a hand-edit target -- a hand-edit is unreproducible.
- Decoding is where the work is. A class with no real decoder yet runs the
  generic walk, which discard-reads every node, so the cursor still lands
  where the sender put it.
- Encoding is deferred class by class. Until a class has a real encoder,
  `packUpdate`/`packData` run the mirror-image default-write walk, which
  writes a default value at each node's own width. That is what keeps the
  singleplayer loopback and the native demo start block from desyncing;
  they round-trip to default field values, which is the same fidelity the
  read side already has for an unported class.
- Judge every change by whether retail Tribes 2 still understands us and we
  still understand it. `just stock` beside `just opentribes`, and the
  recordings under `Classic/recordings`, are how that gets answered --
  never by reasoning about what the format ought to be.

## Style

- British spelling.
- Match the file you are in.
- Comments say why. Reasoning goes in the commit message.
