#!/usr/bin/env python3
"""
Gremlin Oracle — dual-perspective USB enumeration forensics.

Correlates what the host OS sees (USB device arrivals) with what PortGremlin
reports over serial (personas, mimic deploys, host fingerprinting). This is
the novel bit: attack telemetry from both sides of the cable.
"""

from __future__ import annotations

import argparse
import re
import subprocess
import sys
import threading
import time
from collections import deque
from dataclasses import dataclass, field
from typing import Optional

try:
    import serial
    from serial.tools import list_ports
except ImportError:
    print("pip install pyserial", file=sys.stderr)
    sys.exit(1)

ORACLE_HOST_RE = re.compile(r"Host classified:\s+(\w+)")
PERSONA_RE = re.compile(r"\[PERSONA\]\s+(\w+)")
MIMIC_RE = re.compile(r"\[MIMIC\]\s+#(\d+)")
BRAIN_RE = re.compile(r"\[BRAIN\]\s+(.+)")
VID_RE = re.compile(r"VID:\s*0x([0-9A-Fa-f]{4}),\s*PID:\s*0x([0-9A-Fa-f]{4})")
CHOREO_RE = re.compile(r"\[CHOREO\]\s+(.+)")


@dataclass
class CorrelatedEvent:
    ts: float
    source: str
    message: str


@dataclass
class OracleSession:
    events: deque = field(default_factory=lambda: deque(maxlen=500))
    host_profile: Optional[str] = None
    persona: Optional[str] = None
    usb_snapshots: int = 0

    def add(self, source: str, message: str) -> None:
        self.events.append(CorrelatedEvent(time.time(), source, message))


def find_serial_port(hint: Optional[str]) -> Optional[str]:
    if hint:
        return hint
    for port in list_ports.comports():
        desc = (port.description or "").lower()
        if any(t in desc for t in ("ti", "xds", "icdi", "launchpad", "tm4c")):
            return port.device
    ports = list(list_ports.comports())
    return ports[0].device if ports else None


def snapshot_lsusb(session: OracleSession) -> None:
    try:
        out = subprocess.check_output(["lsusb"], text=True, stderr=subprocess.DEVNULL)
        lines = [ln.strip() for ln in out.splitlines() if ln.strip()]
        session.usb_snapshots += 1
        session.add("host", f"lsusb snapshot ({len(lines)} devices)")
        for ln in lines[-5:]:
            session.add("host", f"  {ln}")
    except (FileNotFoundError, subprocess.CalledProcessError):
        pass


def parse_device_line(line: str, session: OracleSession) -> None:
    session.add("device", line)

    m = ORACLE_HOST_RE.search(line)
    if m:
        session.host_profile = m.group(1)
        session.add("correlate", f"ORACLE fingerprint -> {session.host_profile}")

    m = PERSONA_RE.search(line)
    if m:
        session.persona = m.group(1)
        session.add("correlate", f"Persona engaged: {session.persona}")

    m = MIMIC_RE.search(line)
    if m:
        session.add("correlate", f"Mimic vault deploy #{m.group(1)}")

    m = BRAIN_RE.search(line)
    if m:
        session.add("correlate", f"Brain: {m.group(1)}")

    m = CHOREO_RE.search(line)
    if m:
        session.add("correlate", f"Choreography: {m.group(1)}")

    m = VID_RE.search(line)
    if m:
        session.add("correlate", f"Identity swap VID:PID = {m.group(1)}:{m.group(2)}")


def serial_reader(ser: serial.Serial, session: OracleSession, stop: threading.Event) -> None:
    while not stop.is_set():
        try:
            raw = ser.readline()
        except serial.SerialException:
            break
        if not raw:
            continue
        line = raw.decode("utf-8", errors="replace").rstrip("\r\n")
        if line:
            print(f"[DEV] {line}")
            parse_device_line(line, session)


def host_watcher(session: OracleSession, interval: float, stop: threading.Event) -> None:
    last_count = -1
    while not stop.is_set():
        try:
            out = subprocess.check_output(["lsusb"], text=True, stderr=subprocess.DEVNULL)
            count = len(out.splitlines())
            if count != last_count:
                last_count = count
                session.add("host", f"USB topology change: {count} devices")
                print(f"[HOST] USB device count changed -> {count}")
        except (FileNotFoundError, subprocess.CalledProcessError):
            pass
        stop.wait(interval)


def print_report(session: OracleSession) -> None:
    print("\n=== GREMLIN ORACLE SESSION REPORT ===")
    print(f"Host profile (device-side): {session.host_profile or 'not yet classified'}")
    print(f"Active persona:             {session.persona or 'n/a'}")
    print(f"Host USB snapshots:         {session.usb_snapshots}")
    print(f"Correlated events:          {len(session.events)}")
    print("\nRecent timeline:")
    for ev in list(session.events)[-20:]:
        ts = time.strftime("%H:%M:%S", time.localtime(ev.ts))
        print(f"  {ts} [{ev.source:9}] {ev.message}")
    print("=====================================\n")


def main() -> int:
    parser = argparse.ArgumentParser(description="Dual-perspective PortGremlin oracle monitor")
    parser.add_argument("-p", "--port", help="Serial port")
    parser.add_argument("-b", "--baud", type=int, default=115200)
    parser.add_argument("--host-poll", type=float, default=2.0, help="lsusb poll interval (s)")
    parser.add_argument("--duration", type=float, default=0, help="Run N seconds then report (0=forever)")
    args = parser.parse_args()

    port = find_serial_port(args.port)
    if not port:
        print("No serial port found", file=sys.stderr)
        return 1

    session = OracleSession()
    stop = threading.Event()

    ser = serial.Serial(port, args.baud, timeout=0.2)
    time.sleep(0.3)
    ser.write(b"o")  # request oracle report on connect

    threads = [
        threading.Thread(target=serial_reader, args=(ser, session, stop), daemon=True),
        threading.Thread(target=host_watcher, args=(session, args.host_poll, stop), daemon=True),
    ]
    for t in threads:
        t.start()

    print(f"Gremlin Oracle monitoring {port} (Ctrl+C to stop)")
    snapshot_lsusb(session)

    try:
        if args.duration > 0:
            time.sleep(args.duration)
            stop.set()
        else:
            while True:
                time.sleep(1)
    except KeyboardInterrupt:
        print("\nStopping...")
        stop.set()

    ser.close()
    print_report(session)
    return 0


if __name__ == "__main__":
    sys.exit(main())
