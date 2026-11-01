#!/usr/bin/env python3
import argparse
import socket
import sys
import time

TELNET_RW = "rw"


def probe(port: int, commands: list[str], wait: float, host: str = "127.0.0.1") -> str:
    try:
        s = socket.create_connection((host, port), timeout=wait)
    except OSError as e:
        sys.exit(f"nothing answers on telnet port {port}: {e.strerror or e}")

    with s:
        lines = [TELNET_RW, *commands]
        s.sendall("".join(f"{line}\r\n" for line in lines).encode())

        out = bytearray()
        deadline = time.monotonic() + wait
        while (remaining := deadline - time.monotonic()) > 0:
            s.settimeout(remaining)
            try:
                chunk = s.recv(4096)
            except socket.timeout:
                break
            if not chunk:
                break
            out += chunk
            deadline = time.monotonic() + wait

    return out.replace(b"\0", b"").replace(b"\r", b"").decode(errors="replace")


def main() -> None:
    parser = argparse.ArgumentParser(
        description="Send commands to a running game's telnet console and print what comes back."
    )
    parser.add_argument("port", type=int)
    parser.add_argument(
        "commands", nargs="*",
        help="one console line each; with none, lines are read from stdin",
    )
    parser.add_argument(
        "--wait", type=float, default=1.0,
        help="seconds of silence that end the reply (default: %(default)s)",
    )
    args = parser.parse_args()

    commands = args.commands or sys.stdin.read().splitlines()
    sys.stdout.write(probe(args.port, commands, args.wait))


if __name__ == "__main__":
    main()
