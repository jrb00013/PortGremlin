#!/usr/bin/env python3
"""
PortGremlin Overwatch — closed-loop host-side orchestrator.

Reads @PG{...} JSON telemetry from the LaunchPad, monitors kernel USB errors
(dmesg/journalctl), correlates both perspectives, and autonomously drives
escalation when the host shows pain signals.

Live dashboard: http://127.0.0.1:8765
"""

from __future__ import annotations

import argparse
import json
import os
import re
import subprocess
import sys
import threading
import time
import webbrowser
from collections import deque
from dataclasses import dataclass, field
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from typing import Any, Optional
from urllib.parse import urlparse

try:
    import serial
    from serial.tools import list_ports
except ImportError:
    print("Run ./setup.sh first to install dependencies.", file=sys.stderr)
    sys.exit(1)

PG_JSON_RE = re.compile(r"@PG(\{.*\})")
USB_ERROR_RE = re.compile(
    r"(usb|USB|xhci|ehci|ohci|udev).*(error|fail|reject|stall|timeout|unable|warn)",
    re.IGNORECASE,
)

ESCALATION_LADDER = [
    ("[", "RedTeam choreography"),
    ("b", "Gremlin Brain"),
    ("p", "next persona"),
    ("g", "genetic evolution"),
    ("d", "driver confusion"),
    ("m", "malformed mode"),
]


@dataclass
class OverwatchState:
    host_os: str = "unknown"
    persona: str = "unknown"
    brain_phase: str = "idle"
    tolerance: int = 0
    enums: int = 0
    disconnects: int = 0
    evolve_gen: int = 0
    evolve_fit: int = 0
    host_errors: int = 0
    usb_devices: int = 0
    last_vid: str = ""
    last_pid: str = ""
    last_class: str = ""
    autonomous: bool = True
    escalation_level: int = 0
    events: deque = field(default_factory=lambda: deque(maxlen=200))
    pain_score: float = 0.0

    def to_dict(self) -> dict[str, Any]:
        return {
            "host_os": self.host_os,
            "persona": self.persona,
            "brain_phase": self.brain_phase,
            "tolerance": self.tolerance,
            "enums": self.enums,
            "disconnects": self.disconnects,
            "evolve_gen": self.evolve_gen,
            "evolve_fit": self.evolve_fit,
            "host_errors": self.host_errors,
            "usb_devices": self.usb_devices,
            "last_vid": self.last_vid,
            "last_pid": self.last_pid,
            "last_class": self.last_class,
            "autonomous": self.autonomous,
            "escalation_level": self.escalation_level,
            "pain_score": round(self.pain_score, 2),
            "events": list(self.events)[-30:],
        }


STATE = OverwatchState()
STATE_LOCK = threading.Lock()


def log_event(source: str, message: str) -> None:
    entry = {"ts": time.time(), "source": source, "msg": message}
    with STATE_LOCK:
        STATE.events.append(entry)
    print(f"[{source:5}] {message}")


def find_serial_port(hint: Optional[str]) -> Optional[str]:
    if hint:
        return hint
    for port in list_ports.comports():
        desc = (port.description or "").lower()
        if any(t in desc for t in ("ti", "xds", "icdi", "launchpad", "tm4c")):
            return port.device
    ports = list(list_ports.comports())
    return ports[0].device if ports else None


def parse_pg_event(payload: dict[str, Any]) -> None:
    etype = payload.get("e", "")
    with STATE_LOCK:
        if etype == "host":
            STATE.host_os = payload.get("os", "unknown")
        elif etype == "enum":
            STATE.enums = int(payload.get("n", STATE.enums))
            STATE.last_vid = payload.get("vid", "")
            STATE.last_pid = payload.get("pid", "")
            STATE.last_class = payload.get("cls", "")
        elif etype == "persona":
            STATE.persona = payload.get("name", "")
        elif etype == "brain":
            STATE.brain_phase = payload.get("phase", "")
            STATE.tolerance = int(payload.get("tol", 0))
        elif etype == "disconnect":
            STATE.disconnects = int(payload.get("total", 0))
        elif etype == "evolve":
            STATE.evolve_gen = int(payload.get("gen", 0))
            STATE.evolve_fit = int(payload.get("fit", 0))


def handle_device_line(line: str, ser: Optional[serial.Serial]) -> None:
    log_event("dev", line)

    m = PG_JSON_RE.search(line)
    if m:
        try:
            payload = json.loads(m.group(1))
            parse_pg_event(payload)
            log_event("json", json.dumps(payload))
            maybe_autonomous_escalate(ser, payload)
        except json.JSONDecodeError:
            pass


def maybe_autonomous_escalate(ser: Optional[serial.Serial], payload: dict[str, Any]) -> None:
    if not ser or not STATE.autonomous:
        return

    etype = payload.get("e", "")
    with STATE_LOCK:
        if etype == "host" and STATE.escalation_level == 0:
            cmd = "p"
            STATE.escalation_level = 1
            reason = f"host classified {STATE.host_os}"
        elif etype == "disconnect" and STATE.escalation_level < len(ESCALATION_LADDER):
            cmd, reason = ESCALATION_LADDER[STATE.escalation_level]
            STATE.escalation_level += 1
        elif etype == "enum" and STATE.enums > 0 and STATE.enums % 50 == 0:
            cmd = "e"
            reason = f"{STATE.enums} enumerations"
        else:
            return

    ser.write(cmd.encode("ascii"))
    ser.flush()
    log_event("auto", f"CMD '{cmd}' ({reason})")


def serial_loop(port: str, baud: int, stop: threading.Event) -> None:
    ser = serial.Serial(port, baud, timeout=0.15)
    time.sleep(0.4)
    ser.write(b"x")
    ser.flush()
    log_event("host", f"Serial {port} @ {baud}, sent overdrive engage (x)")

    while not stop.is_set():
        try:
            raw = ser.readline()
        except serial.SerialException as exc:
            log_event("host", f"Serial error: {exc}")
            break
        if raw:
            line = raw.decode("utf-8", errors="replace").rstrip("\r\n")
            if line:
                handle_device_line(line, ser)
    ser.close()


def dmesg_loop(stop: threading.Event) -> None:
    if not shutil_which("dmesg"):
        return
    proc = subprocess.Popen(
        ["dmesg", "-w"],
        stdout=subprocess.PIPE,
        stderr=subprocess.DEVNULL,
        text=True,
    )
    while not stop.is_set() and proc.stdout:
        line = proc.stdout.readline()
        if not line:
            break
        if USB_ERROR_RE.search(line):
            with STATE_LOCK:
                STATE.host_errors += 1
                STATE.pain_score += 1.0
            log_event("kernel", line.strip()[:120])


def journal_loop(stop: threading.Event) -> None:
    if not shutil_which("journalctl"):
        return
    proc = subprocess.Popen(
        ["journalctl", "-kf", "-n", "0", "--grep=usb"],
        stdout=subprocess.PIPE,
        stderr=subprocess.DEVNULL,
        text=True,
    )
    while not stop.is_set() and proc.stdout:
        line = proc.stdout.readline()
        if not line:
            break
        if USB_ERROR_RE.search(line):
            with STATE_LOCK:
                STATE.host_errors += 1
                STATE.pain_score += 0.5
            log_event("journal", line.strip()[:120])


def lsusb_loop(interval: float, stop: threading.Event) -> None:
    last = -1
    while not stop.is_set():
        try:
            out = subprocess.check_output(["lsusb"], text=True, stderr=subprocess.DEVNULL)
            count = len([ln for ln in out.splitlines() if ln.strip()])
            if count != last:
                last = count
                with STATE_LOCK:
                    STATE.usb_devices = count
                log_event("host", f"USB topology: {count} devices")
        except (FileNotFoundError, subprocess.CalledProcessError):
            pass
        stop.wait(interval)


def shutil_which(cmd: str) -> Optional[str]:
    from shutil import which
    return which(cmd)


DASHBOARD_HTML = """<!DOCTYPE html>
<html><head>
<meta charset="utf-8"><title>PortGremlin Overwatch</title>
<style>
  *{box-sizing:border-box;margin:0;padding:0}
  body{background:#0a0a0f;color:#e0e0e0;font-family:ui-monospace,monospace;padding:20px}
  h1{color:#ff4444;font-size:1.4em;margin-bottom:4px}
  .sub{color:#666;margin-bottom:20px}
  .grid{display:grid;grid-template-columns:repeat(auto-fit,minmax(200px,1fr));gap:12px;margin-bottom:20px}
  .card{background:#14141f;border:1px solid #2a2a3a;border-radius:8px;padding:14px}
  .card h3{color:#888;font-size:.75em;text-transform:uppercase;margin-bottom:6px}
  .card .val{font-size:1.5em;color:#fff}
  .pain{color:#ff4444}
  .events{background:#14141f;border:1px solid #2a2a3a;border-radius:8px;padding:14px;max-height:400px;overflow-y:auto}
  .events h3{margin-bottom:10px;color:#888}
  .ev{padding:3px 0;border-bottom:1px solid #1a1a2a;font-size:.85em}
  .ev .src{color:#ff8844;margin-right:8px}
  .ev .ts{color:#555;margin-right:8px}
</style></head><body>
<h1>PortGremlin Overwatch</h1>
<p class="sub">Closed-loop USB enumeration attack — live dual-perspective</p>
<div class="grid" id="metrics"></div>
<div class="events"><h3>Event Stream</h3><div id="log"></div></div>
<script>
async function tick(){
  const r=await fetch('/api/state');const s=await r.json();
  const m=document.getElementById('metrics');
  const cards=[
    ['Host OS',s.host_os],['Persona',s.persona],['Brain',s.brain_phase],
    ['Enums',s.enums],['Disconnects',s.disconnects],['Host Errors',s.host_errors],
    ['Pain Score',s.pain_score],['Evolve Gen',s.evolve_gen],['VID:PID',s.last_vid+':'+s.last_pid],
    ['Class',s.last_class],['USB Devs',s.usb_devices],['Escalation',s.escalation_level]
  ];
  m.innerHTML=cards.map(([k,v])=>'<div class="card"><h3>'+k+'</h3><div class="val'+
    (k==='Pain Score'?' pain':'')+'">'+v+'</div></div>').join('');
  const log=document.getElementById('log');
  log.innerHTML=(s.events||[]).slice().reverse().map(e=>{
    const t=new Date(e.ts*1000).toLocaleTimeString();
    return '<div class="ev"><span class="ts">'+t+'</span><span class="src">'+e.source+'</span>'+e.msg+'</div>';
  }).join('');
}
setInterval(tick,1000);tick();
</script></body></html>"""


class OverwatchHandler(BaseHTTPRequestHandler):
    def log_message(self, *_args: Any) -> None:
        pass

    def do_GET(self) -> None:
        path = urlparse(self.path).path
        if path == "/api/state":
            with STATE_LOCK:
                body = json.dumps(STATE.to_dict()).encode()
            self.send_response(200)
            self.send_header("Content-Type", "application/json")
            self.send_header("Access-Control-Allow-Origin", "*")
            self.end_headers()
            self.wfile.write(body)
        else:
            self.send_response(200)
            self.send_header("Content-Type", "text/html")
            self.end_headers()
            self.wfile.write(DASHBOARD_HTML.encode())


def serve_dashboard(port: int, stop: threading.Event) -> None:
    server = ThreadingHTTPServer(("127.0.0.1", port), OverwatchHandler)
    server.timeout = 1
    log_event("host", f"Dashboard http://127.0.0.1:{port}")
    while not stop.is_set():
        server.handle_request()
    server.server_close()


def write_report(path: str) -> None:
    os.makedirs(os.path.dirname(path) or ".", exist_ok=True)
    with STATE_LOCK:
        data = STATE.to_dict()
    data["generated_at"] = time.strftime("%Y-%m-%d %H:%M:%S")
    with open(path, "w", encoding="utf-8") as f:
        json.dump(data, f, indent=2)
    log_event("host", f"Report written to {path}")


def main() -> int:
    parser = argparse.ArgumentParser(description="PortGremlin Overwatch orchestrator")
    parser.add_argument("-p", "--port", help="Serial port")
    parser.add_argument("-b", "--baud", type=int, default=115200)
    parser.add_argument("--web-port", type=int, default=8765)
    parser.add_argument("--no-browser", action="store_true")
    parser.add_argument("--no-auto", action="store_true", help="Disable autonomous commands")
    parser.add_argument("--duration", type=float, default=0)
    parser.add_argument("--report", default="reports/overwatch-session.json")
    args = parser.parse_args()

    serial_port = find_serial_port(args.port)
    if not serial_port:
        print("No serial port found. Connect LaunchPad ICDI port.", file=sys.stderr)
        return 1

    STATE.autonomous = not args.no_auto
    stop = threading.Event()

    threads = [
        threading.Thread(target=serial_loop, args=(serial_port, args.baud, stop), daemon=True),
        threading.Thread(target=lsusb_loop, args=(2.0, stop), daemon=True),
        threading.Thread(target=serve_dashboard, args=(args.web_port, stop), daemon=True),
    ]
    if shutil_which("dmesg"):
        threads.append(threading.Thread(target=dmesg_loop, args=(stop,), daemon=True))
    if shutil_which("journalctl"):
        threads.append(threading.Thread(target=journal_loop, args=(stop,), daemon=True))

    for t in threads:
        t.start()

    if not args.no_browser:
        threading.Timer(1.5, lambda: webbrowser.open(f"http://127.0.0.1:{args.web_port}")).start()

    print("\n  PortGremlin Overwatch running. Ctrl+C to stop.\n")
    try:
        if args.duration > 0:
            time.sleep(args.duration)
        else:
            while True:
                time.sleep(1)
    except KeyboardInterrupt:
        print("\nShutting down...")
    finally:
        stop.set()
        write_report(args.report)
    return 0


if __name__ == "__main__":
    sys.exit(main())
