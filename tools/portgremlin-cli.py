#!/usr/bin/env python3
"""PortGremlin host-side control and enumeration monitor."""

from __future__ import annotations

import argparse
import re
import sys
import threading
import time
from dataclasses import dataclass, field
from typing import Optional

try:
    import serial
    from serial.tools import list_ports
except ImportError:
    print("pyserial required: pip install pyserial", file=sys.stderr)
    sys.exit(1)

COMMANDS = {
    "help": "h",
    "status": "s",
    "auto": "a",
    "malformed": "m",
    "realvid": "r",
    "strings": "t",
    "faster": "+",
    "slower": "-",
    "cycle": "c",
    "reenum": "e",
    "kb": "1",
    "audio": "2",
    "printer": "3",
    "midi": "4",
    "gamepad": "5",
    "brain": "b",
    "persona": "p",
    "oracle": "o",
    "confusion": "d",
    "vault": "v",
    "choreo-red": "[",
    "choreo-stealth": "]",
    "choreo-blitz": "\\",
}

VID_RE = re.compile(r"VID:\s*0x([0-9A-Fa-f]{4}),\s*PID:\s*0x([0-9A-Fa-f]{4})")
SWITCH_RE = re.compile(r"Switching to (\w+)")


@dataclass
class EnumEvent:
    timestamp: float
    kind: str
    detail: str = ""


@dataclass
class SessionStats:
    events: list[EnumEvent] = field(default_factory=list)
    last_vid: Optional[str] = None
    last_pid: Optional[str] = None
    last_class: Optional[str] = None

    def record_line(self, line: str) -> None:
        now = time.time()
        vid_match = VID_RE.search(line)
        if vid_match:
            self.last_vid, self.last_pid = vid_match.groups()
            self.events.append(EnumEvent(now, "reenum", f"VID={self.last_vid} PID={self.last_pid}"))
            return

        switch_match = SWITCH_RE.search(line)
        if switch_match:
            self.last_class = switch_match.group(1)
            self.events.append(EnumEvent(now, "class", self.last_class))


class PortGremlinCLI:
    def __init__(self, port: str, baud: int = 115200) -> None:
        self.port = port
        self.baud = baud
        self.ser: Optional[serial.Serial] = None
        self.stats = SessionStats()
        self._reader_stop = threading.Event()
        self._reader_thread: Optional[threading.Thread] = None

    def connect(self) -> None:
        self.ser = serial.Serial(self.port, self.baud, timeout=0.1)
        time.sleep(0.3)
        self._reader_stop.clear()
        self._reader_thread = threading.Thread(target=self._read_loop, daemon=True)
        self._reader_thread.start()

    def close(self) -> None:
        self._reader_stop.set()
        if self._reader_thread:
            self._reader_thread.join(timeout=1.0)
        if self.ser and self.ser.is_open:
            self.ser.close()

    def send(self, cmd: str) -> None:
        if not self.ser or not self.ser.is_open:
            raise RuntimeError("Not connected")
        self.ser.write(cmd.encode("ascii"))
        self.ser.flush()

    def _read_loop(self) -> None:
        assert self.ser is not None
        while not self._reader_stop.is_set():
            try:
                raw = self.ser.readline()
            except serial.SerialException:
                break
            if not raw:
                continue
            line = raw.decode("utf-8", errors="replace").rstrip("\r\n")
            if line:
                print(line)
                self.stats.record_line(line)

    def interactive(self) -> None:
        print(f"Connected to {self.port} @ {self.baud}")
        print("Type a command name or key (help/status/malformed/cycle/...) or 'quit'")
        while True:
            try:
                user_input = input("portgremlin> ").strip()
            except (EOFError, KeyboardInterrupt):
                print()
                break
            if not user_input:
                continue
            if user_input.lower() in ("quit", "exit", "q"):
                break
            if user_input.lower() == "stats":
                self._print_stats()
                continue
            cmd = COMMANDS.get(user_input.lower(), user_input)
            if len(cmd) == 1:
                self.send(cmd)
            else:
                print(f"Unknown command: {user_input}")

    def _print_stats(self) -> None:
        print("--- Session Stats ---")
        print(f"Events: {len(self.stats.events)}")
        print(f"Last class: {self.stats.last_class or 'n/a'}")
        print(f"Last VID:PID: {self.stats.last_vid or 'n/a'}:{self.stats.last_pid or 'n/a'}")
        recent = self.stats.events[-10:]
        for ev in recent:
            ts = time.strftime("%H:%M:%S", time.localtime(ev.timestamp))
            print(f"  [{ts}] {ev.kind}: {ev.detail}")


def find_launchpad_port() -> Optional[str]:
    for port in list_ports.comports():
        desc = (port.description or "").lower()
        if any(token in desc for token in ("ti", "xds", "icdi", "launchpad", "tm4c")):
            return port.device
    ports = list(list_ports.comports())
    return ports[0].device if ports else None


def main() -> int:
    parser = argparse.ArgumentParser(description="PortGremlin USB enumeration research CLI")
    parser.add_argument("-p", "--port", help="Serial port (auto-detect if omitted)")
    parser.add_argument("-b", "--baud", type=int, default=115200)
    parser.add_argument("-c", "--command", help="Send single command and exit")
    parser.add_argument("--list-ports", action="store_true", help="List serial ports")
    args = parser.parse_args()

    if args.list_ports:
        for port in list_ports.comports():
            print(f"{port.device}\t{port.description}")
        return 0

    port = args.port or find_launchpad_port()
    if not port:
        print("No serial port found. Use -p /dev/ttyACM0", file=sys.stderr)
        return 1

    cli = PortGremlinCLI(port, args.baud)
    try:
        cli.connect()
        if args.command:
            cmd = COMMANDS.get(args.command.lower(), args.command)
            cli.send(cmd[0] if len(cmd) == 1 else cmd)
            time.sleep(1.0)
            cli._print_stats()
        else:
            cli.send("h")
            time.sleep(0.5)
            cli.interactive()
    finally:
        cli.close()
    return 0


if __name__ == "__main__":
    sys.exit(main())
