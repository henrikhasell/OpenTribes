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
