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

`T2_KEEP=1` carries on past a packet that will not read and lists every
failure rather than the first. `T2_TRACE=<class>` prints every node of that
class's layout as it is walked, with the bit each one took, which is how a
reader that disagrees with its layout is tracked down. `T2_VERBOSE=1` names
each packet, event and ghost as it goes.

`TRIBES2_EXE` points at the retail binary, which is read for the Huffman
frequency table; the default is the usual Wine install.

Nothing from the game is kept here -- neither the capture nor the binary --
only the code that reads them.
