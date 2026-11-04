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

Retail is that oracle for behaviour, not for structure. "The exe does it
this way" is not on its own a reason to do the same: what has to match is
what a player or a measurement can tell. Where retail's shape is an
artefact of its own engine -- an order its loop happens to run in, a global
it happens to keep -- copying the artefact buys nothing and costs every
later reader. Match the observable; write the code this engine wants. When
a disassembly is the only evidence for a change, say which observable it
is there to fix; if there is none, the change is not needed.

## The port

- Torque 3D's engine, Tribes 2's game data, build 25034.
- The game's scripts are retail, from 2001, and not ours to edit.
- They know nothing of Torque 3D. What Torque 3D expects a script to set up,
  set up in the engine and name the script that would have.

## Building and running

Everything goes through the justfile -- building, launching and talking to a
running game. Do not call cmake, wine or the binary by hand; `just --list`
has every recipe and its arguments.

- `just dedicated` starts a retail dedicated server under wine, console on
  port 2325. Start it first: both clients below join it.
- `just opentribes` builds, installs into Tribes 2's `GameData` and joins the
  local dedicated server, console on port 2323. The game runs from `GameData`
  because it needs retail's data beside it; `TRIBES2_DIR` says where that is.
- `just retail` joins the same server with the retail client under wine,
  console on port 2324, for the side-by-side the oracle requires.
- Each recipe takes its console port as an argument, and refuses to launch
  if that port is already bound -- usually a previous instance that is still
  running, or wedged.
- `just opentribes` takes the config first, defaulting to `debug`. Release
  carries no debug info at all, so a core from it names functions and nothing
  else; it also compiles out `AssertFatal`, which is the engine's only bounds
  checking on game data. Use `just opentribes release` when timing matters,
  or `relwithdebinfo` for line numbers without `-O0`'s timing.
- `just build <config>` configures and builds into `build/<config>` without
  running it; `just install <config>` also copies the binary and the shader
  symlink into `GameData`. Both default to `release`.
- `just telnet <port> <command>...` questions a running game -- see below.
- A crash is already saved -- `coredumpctl info -1`. Analyse it before
  rebuilding: an install overwrites `GameData/OpenTribes` and leaves the core
  with no binary matching its build id.

## The telnet console

Both engines take `-telnetParams <port> <rw> <ro>`, which is how a running
game gets questioned -- usually a faster answer than a rebuild, and the only
way to ask retail anything. The justfile passes the port of each recipe above
with passwords `rw` and `ro`.

- All telnet communication goes through `scripts/probe.py`, by way of
  `just telnet` -- never `nc`, `telnet` or a socket of your own.
- `just telnet <port> <command>...` sends each argument as one console line
  and prints what comes back until the console has been quiet for a second;
  `--wait <seconds>` stretches that for a slow frame. Quote each command for
  the shell: `just telnet 2323 'echo($pref::Video::fov);'`.
- With no commands it reads stdin to the end and sends every line in one go,
  so `just telnet 2323 < cmds.txt` runs a script of console lines. It is not
  a live session: nothing is sent until stdin closes.
- The first line sent is the **password**, not a command. The `rw` one gets
  read/write, the `ro` one gets output only, and three wrong tries close the
  socket. `just telnet` sends it; a connection that does not will look like a
  console that ignores everything.
- Everything the console logs while connected comes back too, not only the
  answer to the command, so match on what you asked for.
- Replies are prefixed with `$Con::Prompt`, so every answer arrives behind a
  `% `. Anchoring a match to the start of the line makes a healthy game look
  broken.
- The console is serviced from the main loop
  (`telnetConsole.cpp:53`). No frame, no console: a game wedged in a render
  path or blocked in D-state still holds the port and answers nothing, and
  because the port stays bound the next launch on that port refuses to
  start. Kill the wedged one first.

## The script debugger

Retail ships TGE's telnet script debugger -- breakpoints with conditions,
stepping, a call stack and expression evaluation in a stopped frame. It is a
second socket, separate from the console, and it is off until something turns
it on. Our engine has the same debugger, so the same session works against
both, which is what makes it a comparison and not just a probe.

Why it earns its keep over the console: a script breakpoint stops the game
wherever the **engine calls script**, which includes points inside a C++
decode. `GameConnection::readDemoStartBlock` calls
`demoUpdateDatablockProgress(count, total)` once per datablock, so a
breakpoint there walks a demo's start block one body at a time; a desync
localises to one datablock instead of to the whole block. Anything that can
be reached mid-read this way is worth reaching that way.

- Turn it on from the console: `dbgSetParameters(<port>, <pass>)`. Pick a port
  that is not the console's. Ours takes a third argument retail's does not,
  `waitForClient`, which holds the engine before it runs anything -- the only
  way to breakpoint something that happens during startup.
- Connect and send the password as the **first line** -- the shipped client
  notes its own `onConnected` never fires and sends it immediately. The reply
  is `PASS Connected.`, or `PASS WrongPassword.` and the socket closes.
- Ask `FILELIST` before setting anything. `BRKSET` wants the file name the
  engine knows, and any other spelling is `DBGERR No Such file!`.
  `BREAKLIST <file>` answers which of its lines can carry a breakpoint at
  all, which is the other half of getting one to fire.
- `BRKSET <file> <line> <clear> <passct> <expr>` -- `clear` makes it one-shot,
  `passct` skips that many hits and `expr` is a condition, so "stop on the
  nine hundredth" needs no scripting. `BRKCLR <file> <line>` and `BRKCLRALL`
  undo it.
- A stop arrives as `BREAK <file> <line> <function>` followed by the rest of
  the call stack, innermost first; `CONTINUE`, `STEPIN`, `STEPOVER` and
  `STEPOUT` resume, and each resume answers `RUNNING`.
- `EVAL <id> <frame> <expr>` answers `EVALOUT <id> <value>`; frame 0 is the
  innermost. It evaluates in a real frame, so `%local`s work here -- which the
  console cannot do without killing the game. `CEVAL <expr>` is the same
  without an id.
- **While stopped, the main loop is blocked**, so the console port answers
  nothing at all: for the duration, the debugger socket is the only way in.
  Console output is forwarded to it as `COUT <text>`, so nothing is lost.
- Breakpoints are set on a script's own name and line, but retail runs `.dso`
  compiled from scripts that may not be the `.cs` beside them. If a breakpoint
  never fires, confirm with `BREAKLIST` before concluding the code did not
  run.

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
  still understand it. `just retail` beside `just opentribes`, and the
  recordings under `Classic/recordings`, are how that gets answered --
  never by reasoning about what the format ought to be.

## Style

- British spelling.
- Match the file you are in.
- Comments say why. Reasoning goes in the commit message.
