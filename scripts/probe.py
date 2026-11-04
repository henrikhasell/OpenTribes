#!/usr/bin/env python3
import argparse
import re
import socket
import sys
import time

from launch import GAMEDATA

TELNET_RW = "rw"

MOD = "Classic"

# Function and package declarations can't nest inside a function body.
TOP_LEVEL_DECL = re.compile(r"^\s*(function|package)\b", re.MULTILINE)


def clean(data: bytes) -> str:
    return data.replace(b"\0", b"").replace(b"\r", b"").decode(errors="replace")


class Console:
    def __init__(self, port: int, host: str = "127.0.0.1", timeout: float = 1.0):
        try:
            self.sock = socket.create_connection((host, port), timeout=timeout)
        except OSError as e:
            sys.exit(f"nothing answers on telnet port {port}: {e.strerror or e}")
        self.out = bytearray()
        self.send([TELNET_RW])

    def close(self) -> None:
        self.sock.close()

    def send(self, lines: list[str]) -> None:
        self.sock.sendall("".join(f"{line}\r\n" for line in lines).encode())

    def recv(self, timeout: float) -> bool:
        self.sock.settimeout(timeout)
        try:
            chunk = self.sock.recv(4096)
        except socket.timeout:
            return False
        if not chunk:
            return False
        self.out += chunk
        return True

    def read_until_silent(self, wait: float) -> None:
        while self.recv(wait):
            pass

    def text(self) -> str:
        return clean(self.out)


def script_source(code: str) -> str:
    if TOP_LEVEL_DECL.search(code):
        return code + "\n"
    # Locals at file scope have no frame to live in, so give them one.
    # Opened on the code's first line so error line numbers stay true.
    return f"function __probe() {{ {code}\n}}\n__probe();\n"


def probe_inline(port: int, commands: list[str], wait: float) -> str:
    console = Console(port, timeout=wait)
    try:
        console.send(commands)
        console.read_until_silent(wait)
    finally:
        console.close()
    return console.text()


def probe_script(port: int, code: str, wait: float) -> str:
    mod_dir = GAMEDATA / MOD
    if not mod_dir.is_dir():
        sys.exit(f"no mod directory at {mod_dir}")

    console = Console(port, timeout=wait)
    try:
        name = f"_probe_{time.time_ns()}.cs"
        path = mod_dir / name
        path.write_text(script_source(code))
        try:
            # Tribes 2 only finds files that were there when it last
            # scanned its mod paths; OpenTribes reads the directory.
            console.send([f'setModPaths("{MOD}"); exec("{name}");'])
            console.read_until_silent(wait)
        finally:
            for leftover in (path, path.with_name(name + ".dso")):
                leftover.unlink(missing_ok=True)
    finally:
        console.close()
    return console.text()


def main() -> None:
    parser = argparse.ArgumentParser(
        description="Run TorqueScript in a running game through its telnet console and print what comes back. "
        f"The code goes into a temporary .cs file in GameData/{MOD}, is exec'd, then deleted."
    )
    parser.add_argument("port", type=int)
    parser.add_argument(
        "commands", nargs="*",
        help="lines of TorqueScript; with none, they are read from stdin",
    )
    parser.add_argument(
        "--wait", type=float, default=1.0,
        help="seconds of silence that end the reply (default: %(default)s)",
    )
    parser.add_argument(
        "--inline", action="store_true",
        help="send each line straight to the console instead of exec'ing a file",
    )
    args = parser.parse_args()

    commands = args.commands or sys.stdin.read().splitlines()
    if args.inline:
        out = probe_inline(args.port, commands, args.wait)
    else:
        out = probe_script(args.port, "\n".join(commands), args.wait)
    sys.stdout.write(out)


if __name__ == "__main__":
    main()
