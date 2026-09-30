#!/usr/bin/env python3
"""Record a Tribes 2 session by sitting between a client and a server.

    capture.py LISTEN_PORT DEST_HOST DEST_PORT OUT

Every datagram the server sends the client is written to OUT as a 32-bit
length and then its bytes, which is the format tools/t2/replay.py reads.  A
live server is not a test fixture -- it cycles missions and how far a decode
gets varies between boots -- so a recorded session is what makes a change to
the reader judgeable.
"""
import socket, struct, sys, threading, time

listen_port = int(sys.argv[1])
dest = (sys.argv[2], int(sys.argv[3]))
out_path = sys.argv[4]

srv = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
srv.bind(("0.0.0.0", listen_port))

out = open(out_path, "wb")
up_out = open(out_path + ".toserver", "wb")
lock = threading.Lock()
stats = {"to_client": 0, "to_server": 0, "bytes": 0}
t0 = time.time()


def record(data):
    with lock:
        out.write(struct.pack("<I", len(data)))
        out.write(data)
        out.flush()
        stats["to_client"] += 1
        stats["bytes"] += len(data)


def pump(up, client_addr):
    """Server to client, recording as it goes."""
    while True:
        try:
            data, _ = up.recvfrom(65535)
        except OSError:
            return
        record(data)
        srv.sendto(data, client_addr)


def report():
    while True:
        time.sleep(10)
        print("[%6.1fs] %d packets to client (%d bytes), %d to server"
              % (time.time() - t0, stats["to_client"], stats["bytes"],
                 stats["to_server"]))
        sys.stdout.flush()


threading.Thread(target=report, daemon=True).start()

clients = {}
print("listening on %d, forwarding to %s:%d, writing %s"
      % (listen_port, dest[0], dest[1], out_path))
sys.stdout.flush()

while True:
    data, addr = srv.recvfrom(65535)
    if addr not in clients:
        up = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        up.connect(dest)
        clients[addr] = up
        threading.Thread(target=pump, args=(up, addr), daemon=True).start()
        print("client %s:%d" % addr)
        sys.stdout.flush()
    clients[addr].send(data)
    with lock:
        up_out.write(struct.pack("<I", len(data)))
        up_out.write(data)
        up_out.flush()
    stats["to_server"] += 1
