#!/usr/bin/env python3
"""PortGremlin Virtual Lab — GUI simulation of USB enumeration attacks."""

from __future__ import annotations

import os
import sys
import time
import tkinter as tk
from tkinter import ttk, scrolledtext

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

from sim_engine import (
    BrainPhase,
    DeviceClass,
    HostOS,
    Persona,
    PortGremlinSimulator,
    SimEvent,
)

# Colors
BG = "#0a0a0f"
PANEL = "#14141f"
BORDER = "#2a2a3a"
FG = "#e0e0e0"
MUTED = "#666666"
RED = "#ff4444"
ORANGE = "#ff8844"
GREEN = "#44ff88"
CYAN = "#44ccff"
PURPLE = "#aa44ff"

PERSONA_COLORS = {
    Persona.MANUAL: "#888888",
    Persona.CHIMERA: "#ff4444",
    Persona.MIMIC: "#44ccff",
    Persona.STORM: "#ff8844",
    Persona.HAUNTED: "#aa44ff",
    Persona.PHANTOM: "#44ff88",
    Persona.SPECTRE: "#ffffff",
}

HOST_COLORS = {
    HostOS.WINDOWS: "#0078d4",
    HostOS.LINUX: "#f57c00",
    HostOS.MACOS: "#999999",
    HostOS.EMBEDDED: "#607d8b",
    HostOS.UNKNOWN: "#444444",
}


class PortGremlinVirtualLab(tk.Tk):
    def __init__(self) -> None:
        super().__init__()
        self.title("PortGremlin Virtual Lab")
        self.configure(bg=BG)
        self.geometry("1280x820")
        self.minsize(1024, 700)

        self.sim = PortGremlinSimulator(on_event=self._on_sim_event)
        self._after_id: str | None = None

        self._build_ui()
        self._schedule_tick()
        self.sim.log("sim", "Virtual Lab ready — select host OS and press Start")

    def _build_ui(self) -> None:
        header = tk.Frame(self, bg=BG, pady=8)
        header.pack(fill=tk.X, padx=12)

        tk.Label(
            header,
            text="PortGremlin Virtual Lab",
            font=("Helvetica", 16, "bold"),
            fg=RED,
            bg=BG,
        ).pack(side=tk.LEFT)

        tk.Label(
            header,
            text="USB enumeration attack simulation — no hardware required",
            font=("Helvetica", 10),
            fg=MUTED,
            bg=BG,
        ).pack(side=tk.LEFT, padx=(12, 0))

        body = tk.Frame(self, bg=BG)
        body.pack(fill=tk.BOTH, expand=True, padx=12, pady=4)
        body.columnconfigure(1, weight=1)
        body.rowconfigure(0, weight=1)

        self._build_left_panel(body)
        self._build_canvas(body)
        self._build_right_panel(body)
        self._build_controls()
        self._build_log()

    def _panel(self, parent: tk.Widget, title: str) -> tk.Frame:
        outer = tk.Frame(parent, bg=BORDER, padx=1, pady=1)
        inner = tk.Frame(outer, bg=PANEL, padx=10, pady=8)
        tk.Label(inner, text=title, font=("Helvetica", 9, "bold"), fg=MUTED, bg=PANEL).pack(
            anchor=tk.W
        )
        inner.pack(fill=tk.BOTH, expand=True)
        return inner

    def _build_left_panel(self, parent: tk.Frame) -> None:
        left = tk.Frame(parent, bg=BG, width=260)
        left.grid(row=0, column=0, sticky="nsew", padx=(0, 6))
        left.grid_propagate(False)

        host_panel = self._panel(left, "TARGET HOST")
        host_panel.pack(fill=tk.X, pady=(0, 6))

        self.host_os_var = tk.StringVar(value=HostOS.LINUX.value)
        for os in HostOS:
            tk.Radiobutton(
                host_panel,
                text=os.value,
                variable=self.host_os_var,
                value=os.value,
                command=self._on_host_change,
                bg=PANEL,
                fg=FG,
                selectcolor=PANEL,
                activebackground=PANEL,
                activeforeground=RED,
                font=("Helvetica", 9),
            ).pack(anchor=tk.W)

        self.host_os_label = tk.Label(host_panel, text="Detected: Linux", fg=CYAN, bg=PANEL, font=("Helvetica", 10, "bold"))
        self.host_os_label.pack(anchor=tk.W, pady=(6, 0))

        metrics = self._panel(left, "HOST METRICS")
        metrics.pack(fill=tk.X, pady=6)

        self.metric_labels: dict[str, tk.Label] = {}
        for key in ("Pain", "Tolerance", "Errors", "Disconnects", "Enums"):
            row = tk.Frame(metrics, bg=PANEL)
            row.pack(fill=tk.X)
            tk.Label(row, text=f"{key}:", fg=MUTED, bg=PANEL, width=12, anchor=tk.W, font=("Helvetica", 9)).pack(side=tk.LEFT)
            lbl = tk.Label(row, text="0", fg=FG, bg=PANEL, font=("Helvetica", 9, "bold"))
            lbl.pack(side=tk.LEFT)
            self.metric_labels[key] = lbl

        self.pain_bar = ttk.Progressbar(metrics, length=200, mode="determinate", maximum=100)
        self.pain_bar.pack(fill=tk.X, pady=(8, 0))

        dev_panel = self._panel(left, "HOST DEVICE CACHE")
        dev_panel.pack(fill=tk.BOTH, expand=True)

        self.device_list = tk.Listbox(
            dev_panel,
            bg="#0d0d14",
            fg=FG,
            font=("Courier", 8),
            borderwidth=0,
            highlightthickness=0,
            selectbackground=BORDER,
            height=12,
        )
        self.device_list.pack(fill=tk.BOTH, expand=True)

    def _build_canvas(self, parent: tk.Frame) -> None:
        center = tk.Frame(parent, bg=BG)
        center.grid(row=0, column=1, sticky="nsew")

        self.canvas = tk.Canvas(center, bg="#080810", highlightthickness=0, height=380)
        self.canvas.pack(fill=tk.BOTH, expand=True)
        self.canvas.bind("<Configure>", lambda _e: self._draw_scene())

        identity = self._panel(center, "CURRENT IDENTITY")
        identity.pack(fill=tk.X, pady=(6, 0))

        self.identity_label = tk.Label(
            identity,
            text="—",
            fg=FG,
            bg=PANEL,
            font=("Courier", 10),
            justify=tk.LEFT,
        )
        self.identity_label.pack(anchor=tk.W)

    def _build_right_panel(self, parent: tk.Frame) -> None:
        right = tk.Frame(parent, bg=BG, width=260)
        right.grid(row=0, column=2, sticky="nsew", padx=(6, 0))
        right.grid_propagate(False)

        dev_panel = self._panel(right, "LAUNCHPAD (TM4C123)")
        dev_panel.pack(fill=tk.X, pady=(0, 6))

        self.persona_label = tk.Label(dev_panel, text="Persona: Manual", fg=ORANGE, bg=PANEL, font=("Helvetica", 10, "bold"))
        self.persona_label.pack(anchor=tk.W)
        self.brain_label = tk.Label(dev_panel, text="Brain: off", fg=MUTED, bg=PANEL, font=("Helvetica", 9))
        self.brain_label.pack(anchor=tk.W)
        self.evolve_label = tk.Label(dev_panel, text="Evolve: off", fg=MUTED, bg=PANEL, font=("Helvetica", 9))
        self.evolve_label.pack(anchor=tk.W)
        self.genome_label = tk.Label(dev_panel, text="Genome: —", fg=MUTED, bg=PANEL, font=("Courier", 8))
        self.genome_label.pack(anchor=tk.W, pady=(4, 0))

        flags = self._panel(right, "ATTACK FLAGS")
        flags.pack(fill=tk.X, pady=6)

        self.flag_labels: dict[str, tk.Label] = {}
        for key in ("Auto cycle", "Malformed", "Contradiction", "Interval"):
            row = tk.Frame(flags, bg=PANEL)
            row.pack(fill=tk.X)
            tk.Label(row, text=f"{key}:", fg=MUTED, bg=PANEL, width=14, anchor=tk.W, font=("Helvetica", 9)).pack(side=tk.LEFT)
            lbl = tk.Label(row, text="—", fg=FG, bg=PANEL, font=("Helvetica", 9))
            lbl.pack(side=tk.LEFT)
            self.flag_labels[key] = lbl

        mimic_panel = self._panel(right, "MIMIC VAULT")
        mimic_panel.pack(fill=tk.BOTH, expand=True)

        mimic_btns = tk.Frame(mimic_panel, bg=PANEL)
        mimic_btns.pack(fill=tk.X)
        for i in range(6):
            tk.Button(
                mimic_btns,
                text=str(i),
                width=3,
                command=lambda n=i: self.sim.deploy_mimic(n),
                bg=BORDER,
                fg=FG,
                relief=tk.FLAT,
                font=("Helvetica", 8),
            ).grid(row=i // 3, column=i % 3, padx=2, pady=2)

    def _build_controls(self) -> None:
        bar = tk.Frame(self, bg=PANEL, pady=8)
        bar.pack(fill=tk.X, padx=12, pady=4)

        buttons = [
            ("▶ Start", self._start, GREEN),
            ("⏸ Pause", self._pause, ORANGE),
            ("↺ Reset", self._reset, MUTED),
            ("⚡ Overdrive", self.sim.overdrive, RED),
            ("🧠 Brain", self.sim.toggle_brain, PURPLE),
            ("🧬 Evolve", self.sim.toggle_evolve, CYAN),
            ("🎭 Persona", self.sim.next_persona, ORANGE),
            ("👻 Confusion", self.sim.toggle_contradiction, PURPLE),
            ("↻ Enum", self.sim.enumerate, FG),
        ]
        for text, cmd, color in buttons:
            tk.Button(
                bar,
                text=text,
                command=cmd,
                bg=BORDER,
                fg=color,
                relief=tk.FLAT,
                padx=10,
                pady=4,
                font=("Helvetica", 9, "bold"),
                activebackground=BG,
                activeforeground=color,
            ).pack(side=tk.LEFT, padx=3)

    def _build_log(self) -> None:
        log_frame = tk.Frame(self, bg=BG)
        log_frame.pack(fill=tk.BOTH, expand=True, padx=12, pady=(0, 10))

        tk.Label(log_frame, text="EVENT STREAM", fg=MUTED, bg=BG, font=("Helvetica", 8, "bold")).pack(anchor=tk.W)
        self.log_text = scrolledtext.ScrolledText(
            log_frame,
            height=8,
            bg="#0d0d14",
            fg=FG,
            font=("Courier", 8),
            insertbackground=FG,
            relief=tk.FLAT,
            borderwidth=0,
        )
        self.log_text.pack(fill=tk.BOTH, expand=True, pady=(4, 0))
        self.log_text.tag_config("error", foreground=RED)
        self.log_text.tag_config("warn", foreground=ORANGE)
        self.log_text.tag_config("info", foreground=FG)

    def _on_host_change(self) -> None:
        for os in HostOS:
            if os.value == self.host_os_var.get():
                self.sim.set_host_os(os)
                break

    def _on_sim_event(self, ev: SimEvent) -> None:
        tag = ev.level if ev.level in ("error", "warn") else "info"
        line = f"[{ev.source:6}] {ev.message}\n"
        self.log_text.insert(tk.END, line, tag)
        self.log_text.see(tk.END)

    def _start(self) -> None:
        self.sim.state.auto_cycle = True
        self.sim.start()

    def _pause(self) -> None:
        self.sim.stop()

    def _reset(self) -> None:
        self.sim.stop()
        self.sim.reset()
        self.log_text.delete("1.0", tk.END)

    def _schedule_tick(self) -> None:
        self.sim.tick(0.05)
        self._refresh_ui()
        self._after_id = self.after(50, self._schedule_tick)

    def _refresh_ui(self) -> None:
        st = self.sim.state

        self.host_os_label.config(text=f"Detected: {st.host_os.value}", fg=HOST_COLORS.get(st.host_os, FG))
        self.metric_labels["Pain"].config(text=f"{st.pain_score:.1f}", fg=RED if st.pain_score > 30 else FG)
        self.metric_labels["Tolerance"].config(text=str(st.tolerance))
        self.metric_labels["Errors"].config(text=str(st.host_errors))
        self.metric_labels["Disconnects"].config(text=str(st.disconnects))
        self.metric_labels["Enums"].config(text=str(st.enum_count))
        self.pain_bar["value"] = st.pain_score

        color = PERSONA_COLORS.get(st.persona, FG)
        self.persona_label.config(text=f"Persona: {st.persona.value}", fg=color)
        self.brain_label.config(
            text=f"Brain: {'ACTIVE — ' + st.brain_phase.value if st.brain_active else 'off'}",
            fg=PURPLE if st.brain_active else MUTED,
        )
        self.evolve_label.config(
            text=f"Evolve: {'ACTIVE' if st.evolve_active else 'off'}",
            fg=CYAN if st.evolve_active else MUTED,
        )
        g = st.genome
        self.genome_label.config(
            text=f"gen={g.generation} fit={g.fitness} int={g.interval}ms mal={g.malformed}"
        )

        self.flag_labels["Auto cycle"].config(text="ON" if st.auto_cycle else "OFF", fg=GREEN if st.auto_cycle else MUTED)
        self.flag_labels["Malformed"].config(text="ON" if st.malformed else "OFF", fg=RED if st.malformed else MUTED)
        self.flag_labels["Contradiction"].config(
            text=f"ON {st.pinned_vid:04X}:{st.pinned_pid:04X}" if st.contradiction else "OFF",
            fg=PURPLE if st.contradiction else MUTED,
        )
        self.flag_labels["Interval"].config(text=f"{g.interval * 10}ms")

        self.identity_label.config(
            text=(
                f"Class:  {st.device_class.value}\n"
                f"VID:PID {st.vid:04X}:{st.pid:04X}\n"
                f"Mfg:    {st.manufacturer}\n"
                f"Product:{st.product}\n"
                f"Cfg:    {st.config_latency_ms}ms  Resets: {st.reset_count}"
            )
        )

        self.device_list.delete(0, tk.END)
        for dev in st.host_devices[:16]:
            pin = "⚡" if dev.pinned else " "
            self.device_list.insert(
                tk.END,
                f"{pin}{dev.vid:04X}:{dev.pid:04X} {dev.device_class.value[:4]} {dev.label[:18]}",
            )

        self._draw_scene()

    def _draw_scene(self) -> None:
        c = self.canvas
        c.delete("all")
        w = c.winfo_width()
        h = c.winfo_height()
        if w < 10:
            return

        st = self.sim.state
        host_x, host_y = w * 0.18, h * 0.5
        dev_x, dev_y = w * 0.82, h * 0.5

        host_color = HOST_COLORS.get(st.host_os, "#333")
        c.create_rectangle(host_x - 90, host_y - 70, host_x + 90, host_y + 70, fill=host_color, outline=BORDER, width=2)
        c.create_text(host_x, host_y - 50, text="HOST", fill="white", font=("Helvetica", 11, "bold"))
        c.create_text(host_x, host_y - 30, text=st.host_os.value, fill="white", font=("Helvetica", 9))
        c.create_text(host_x, host_y, text=f"Pain {st.pain_score:.0f}", fill=RED if st.pain_score > 20 else "white", font=("Helvetica", 9))
        c.create_text(host_x, host_y + 20, text=f"{len(st.host_devices)} cached", fill="#ccc", font=("Helvetica", 8))

        c.create_rectangle(dev_x - 70, dev_y - 55, dev_x + 70, dev_y + 55, fill="#1a2a1a", outline=GREEN, width=2)
        c.create_text(dev_x, dev_y - 35, text="LaunchPad", fill=GREEN, font=("Helvetica", 10, "bold"))
        c.create_text(dev_x, dev_y - 15, text="TM4C123", fill="#aaa", font=("Helvetica", 8))
        c.create_text(dev_x, dev_y + 5, text=st.persona.value, fill=PERSONA_COLORS.get(st.persona, FG), font=("Helvetica", 9, "bold"))
        c.create_text(dev_x, dev_y + 25, text=st.device_class.value[:8], fill=FG, font=("Helvetica", 8))

        cable_y = host_y
        c.create_line(host_x + 90, cable_y, dev_x - 70, cable_y, fill="#333", width=6)
        c.create_line(host_x + 90, cable_y, dev_x - 70, cable_y, fill=CYAN if st.running else "#222", width=2)

        if st.running:
            phase = st.packet_phase
            for i in range(3):
                t = (phase + i * 0.33) % 1.0
                px = (host_x + 90) + t * (dev_x - 70 - host_x - 90)
                persona_color = PERSONA_COLORS.get(st.persona, CYAN)
                c.create_oval(px - 5, cable_y - 5, px + 5, cable_y + 5, fill=persona_color, outline="")

        flash_age = time.time() - st.last_enum_flash if st.last_enum_flash else 999
        if flash_age < 0.4:
            alpha = 1.0 - flash_age / 0.4
            glow = int(255 * alpha)
            color = f"#{glow:02x}{glow//2:02x}44"
            c.create_oval(host_x - 20, host_y - 90, host_x + 20, host_y - 60, fill=color, outline=RED, width=2)
            c.create_text(host_x, host_y - 75, text="NEW", fill=RED, font=("Helvetica", 8, "bold"))
            c.create_text(
                host_x,
                host_y + 50,
                text=f"{st.vid:04X}:{st.pid:04X}",
                fill=RED,
                font=("Courier", 9, "bold"),
            )

        if st.malformed:
            c.create_text(w / 2, 20, text="⚠ MALFORMED DESCRIPTORS", fill=RED, font=("Helvetica", 10, "bold"))
        if st.contradiction:
            c.create_text(w / 2, 38, text="⚡ DRIVER CONFUSION ACTIVE", fill=PURPLE, font=("Helvetica", 9, "bold"))
        if st.brain_active:
            c.create_text(w / 2, h - 20, text=f"BRAIN: {st.brain_phase.value.upper()}", fill=PURPLE, font=("Helvetica", 9, "bold"))

    def on_close(self) -> None:
        if self._after_id:
            self.after_cancel(self._after_id)
        self.destroy()


def main() -> int:
    try:
        app = PortGremlinVirtualLab()
    except tk.TclError as exc:
        print(f"GUI unavailable: {exc}", file=sys.stderr)
        print("Install tkinter: sudo apt install python3-tk", file=sys.stderr)
        return 1
    app.protocol("WM_DELETE_WINDOW", app.on_close)
    app.mainloop()
    return 0


if __name__ == "__main__":
    sys.exit(main())
