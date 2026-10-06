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

## Building and running

- `just opentribes` builds, copies the binary and the shader symlink into
  Tribes 2's `GameData`, and runs it there. The game runs from `GameData`
  because it needs retail's data beside it; `TRIBES2_DIR` says where that is.
- The config is the first argument and defaults to `debug`. Release carries
  no debug info at all, so a core from it names functions and nothing else;
  it also compiles out `AssertFatal`, which is the engine's only bounds
  checking on game data. Use `just opentribes release` when timing matters,
  or `relwithdebinfo` for line numbers without `-O0`'s timing.
- `just build <config>` configures and builds into `build/<config>` without
  running it. `just debug` and `just release` are aliases for the two.
- `just telnet` opens the running engine's console on port 2324 -- see below.
- `just stock` runs retail under wine with the same console, for the
  side-by-side the oracle requires.
- A crash is already saved -- `coredumpctl info -1`. Analyse it before
  rebuilding: a rebuild overwrites `GameData/OpenTribes` and leaves the core
  with no binary matching its build id.

## The telnet console

Both engines take `-telnetParams <port> <rw> <ro>`, which is how a running
game gets questioned -- usually a faster answer than a rebuild, and the only
way to ask retail anything. The justfile passes port 2324 with passwords `rw`
and `ro`; `just telnet` connects to it.

- The first line sent is the **password**, not a command. The `rw` one gets
  read/write, the `ro` one gets output only, and three wrong tries close the
  socket. `just telnet` sends it; a hand-rolled connection that does not will
  look like a console that ignores everything.
- Replies are prefixed with `$Con::Prompt`, so every answer arrives behind a
  `% `. Anchoring a match to the start of the line makes a healthy game look
  broken.
- The console is serviced from the main loop
  (`telnetConsole.cpp:53`). No frame, no console: a game wedged in a render
  path or blocked in D-state still holds the port and answers nothing, and
  because the port stays bound the next launch comes up with no console at
  all.
- A console that answers is not necessarily ready. `exec` fails silently for
  about a second after launch; wait on `isFunction` of something the scripts
  define before driving it.
- Input is capped at 512 chars (`Con::MaxLineLength`) and truncated past that
  without a word. Put anything long in a file and `exec` it -- and anything
  with a `%local` or a loop regardless, because a `%local` typed at retail's
  console kills the game outright.

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
