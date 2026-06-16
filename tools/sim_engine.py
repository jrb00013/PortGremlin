"""PortGremlin firmware behavior simulation engine."""

from __future__ import annotations

import random
import time
from dataclasses import dataclass, field, replace
from enum import Enum, auto
from typing import Callable, Optional


class HostOS(Enum):
    UNKNOWN = "Unknown"
    WINDOWS = "Windows"
    LINUX = "Linux"
    MACOS = "macOS"
    EMBEDDED = "Embedded"


class DeviceClass(Enum):
    KEYBOARD = "Keyboard"
    AUDIO = "Audio"
    PRINTER = "Printer"
    MIDI = "MIDI"
    GAMEPAD = "Gamepad"

    @classmethod
    def all(cls) -> list[DeviceClass]:
        return list(cls)


class Persona(Enum):
    MANUAL = "Manual"
    CHIMERA = "Chimera"
    MIMIC = "Mimic"
    STORM = "Storm"
    HAUNTED = "Haunted"
    PHANTOM = "Phantom"
    SPECTRE = "Spectre"

    @classmethod
    def cycle(cls, current: Persona) -> Persona:
        members = [p for p in cls if p != cls.MANUAL]
        if current == cls.MANUAL:
            return members[0]
        idx = members.index(current)
        return members[(idx + 1) % len(members)]


class BrainPhase(Enum):
    IDLE = "Idle"
    PROBE = "Probe"
    ESCALATE = "Escalate"
    CORRUPT = "Corrupt"
    CHAOS = "Chaos"


@dataclass
class MimicProfile:
    vid: int
    pid: int
    device_class: DeviceClass
    manufacturer: str
    product: str


MIMIC_VAULT: list[MimicProfile] = [
    MimicProfile(0x046D, 0xC52B, DeviceClass.KEYBOARD, "Logitech", "USB Receiver"),
    MimicProfile(0x046D, 0xC077, DeviceClass.KEYBOARD, "Logitech", "USB Optical Mouse"),
    MimicProfile(0x05AC, 0x0250, DeviceClass.KEYBOARD, "Apple Inc.", "Apple Keyboard"),
    MimicProfile(0x05AC, 0x8300, DeviceClass.KEYBOARD, "Apple Inc.", "Internal Keyboard"),
    MimicProfile(0x045E, 0x07B2, DeviceClass.KEYBOARD, "Microsoft", "Wired Keyboard 600"),
    MimicProfile(0x045E, 0x028E, DeviceClass.GAMEPAD, "Microsoft", "Xbox360 Controller"),
    MimicProfile(0x0781, 0x5567, DeviceClass.PRINTER, "SanDisk", "Cruzer Blade"),
    MimicProfile(0x0951, 0x1666, DeviceClass.PRINTER, "Kingston", "DataTraveler 3.0"),
    MimicProfile(0x0BDA, 0x8152, DeviceClass.AUDIO, "Realtek", "USB Audio"),
    MimicProfile(0x1235, 0x0002, DeviceClass.MIDI, "Focusrite", "Scarlett 2i2"),
    MimicProfile(0x04F9, 0x2042, DeviceClass.PRINTER, "Brother", "HL-L2350DW"),
    MimicProfile(0x03F0, 0x094A, DeviceClass.PRINTER, "HP", "LaserJet Pro"),
]

KNOWN_VIDS = [0x046D, 0x045E, 0x05AC, 0x8087, 0x1D6B, 0x0BDA, 0x413C]


@dataclass
class Genome:
    interval: int = 5
    malformed: bool = False
    real_vid: bool = True
    contradiction: bool = False
    fitness: int = 0
    generation: int = 0


@dataclass
class HostDevice:
    vid: int
    pid: int
    device_class: DeviceClass
    label: str
    pinned: bool = False
    age: float = 0.0


@dataclass
class SimEvent:
    ts: float
    source: str
    message: str
    level: str = "info"


@dataclass
class SimulationState:
    host_os: HostOS = HostOS.LINUX
    persona: Persona = Persona.MANUAL
    brain_phase: BrainPhase = BrainPhase.IDLE
    brain_active: bool = False
    evolve_active: bool = False
    auto_cycle: bool = False
    malformed: bool = False
    contradiction: bool = False
    pinned_vid: int = 0
    pinned_pid: int = 0
    device_class: DeviceClass = DeviceClass.KEYBOARD
    vid: int = 0x1CBE
    pid: int = 0x0001
    manufacturer: str = "PortGremlin Corp"
    product: str = "Keyboard Device"
    enum_count: int = 0
    cycle_count: int = 0
    disconnects: int = 0
    tolerance: int = 50
    pain_score: float = 0.0
    host_errors: int = 0
    config_latency_ms: int = 0
    reset_count: int = 0
    genome: Genome = field(default_factory=Genome)
    host_devices: list[HostDevice] = field(default_factory=list)
    events: list[SimEvent] = field(default_factory=list)
    running: bool = False
    packet_phase: float = 0.0
    last_enum_flash: float = 0.0


class PortGremlinSimulator:
    def __init__(self, on_event: Optional[Callable[[SimEvent], None]] = None) -> None:
        self.state = SimulationState()
        self._on_event = on_event
        self._class_index = 0
        self._tick_accum = 0.0
        self._best_genome = Genome()

    def log(self, source: str, message: str, level: str = "info") -> None:
        ev = SimEvent(time.time(), source, message, level)
        self.state.events.append(ev)
        if len(self.state.events) > 500:
            self.state.events = self.state.events[-500:]
        if self._on_event:
            self._on_event(ev)

    def set_host_os(self, host: HostOS) -> None:
        self.state.host_os = host
        self.log("oracle", f"Host set to {host.value}")

    def _host_latency_profile(self) -> tuple[int, int]:
        profiles = {
            HostOS.WINDOWS: (18, 2),
            HostOS.LINUX: (95, 0),
            HostOS.MACOS: (45, 0),
            HostOS.EMBEDDED: (120, 0),
            HostOS.UNKNOWN: (60, 1),
        }
        base, resets = profiles[self.state.host_os]
        return base + random.randint(-8, 8), resets + (1 if random.random() < 0.15 else 0)

    def classify_host(self) -> None:
        lat = self.state.config_latency_ms
        rst = self.state.reset_count
        if rst >= 2:
            detected = HostOS.WINDOWS
        elif lat > 80:
            detected = HostOS.LINUX
        elif 0 < lat < 25:
            detected = HostOS.WINDOWS
        elif 25 <= lat <= 80:
            detected = HostOS.MACOS
        else:
            detected = HostOS.EMBEDDED
        self.state.host_os = detected
        self.log("oracle", f"Host classified: {detected.value} (cfg={lat}ms, resets={rst})")

    def _random_vid_pid(self) -> tuple[int, int]:
        st = self.state
        if st.contradiction and st.pinned_vid:
            return st.pinned_vid, st.pinned_pid
        if st.malformed and random.random() < 0.3:
            return random.choice([0x0000, 0xFFFF]), random.choice([0x0000, 0xFFFF])
        if st.persona in (Persona.MIMIC, Persona.PHANTOM) or st.genome.real_vid:
            vid = random.choice(KNOWN_VIDS)
        else:
            vid = random.randint(0x1000, 0xFFFF)
        pid = random.randint(0x0001, 0xFFFE)
        return vid, pid

    def _apply_persona_config(self, persona: Persona) -> None:
        st = self.state
        st.persona = persona
        if persona == Persona.CHIMERA:
            st.auto_cycle, st.malformed = True, True
            st.genome.interval = 3
        elif persona == Persona.MIMIC:
            st.auto_cycle, st.malformed = True, False
            st.genome.interval = 15
            self.deploy_mimic(random.randint(0, len(MIMIC_VAULT) - 1))
        elif persona == Persona.STORM:
            st.auto_cycle, st.malformed = True, False
            st.genome.interval = 1
        elif persona == Persona.HAUNTED:
            st.auto_cycle, st.malformed = True, True
            st.contradiction = True
            st.pinned_vid = random.randint(0x1000, 0xFFFF)
            st.pinned_pid = random.randint(0x1000, 0xFFFF)
            st.genome.interval = 4
        elif persona == Persona.PHANTOM:
            st.auto_cycle, st.malformed = True, False
            st.genome.interval = 50
        elif persona == Persona.SPECTRE:
            st.auto_cycle = True
            if st.host_os == HostOS.WINDOWS:
                self._apply_persona_config(Persona.CHIMERA)
            elif st.host_os == HostOS.LINUX:
                self._apply_persona_config(Persona.HAUNTED)
            elif st.host_os == HostOS.MACOS:
                self.deploy_mimic(2)
                st.persona = Persona.MIMIC
            else:
                self._apply_persona_config(Persona.STORM)
            return
        self.log("persona", f"{persona.value} engaged")

    def deploy_mimic(self, index: int) -> None:
        profile = MIMIC_VAULT[index % len(MIMIC_VAULT)]
        st = self.state
        st.vid, st.pid = profile.vid, profile.pid
        st.device_class = profile.device_class
        st.manufacturer = profile.manufacturer
        st.product = profile.product
        self.log("mimic", f"#{index} {profile.manufacturer} {profile.product} [{profile.vid:04X}:{profile.pid:04X}]")

    def next_persona(self) -> None:
        self._apply_persona_config(Persona.cycle(self.state.persona))

    def toggle_brain(self) -> None:
        st = self.state
        st.brain_active = not st.brain_active
        st.brain_phase = BrainPhase.IDLE if st.brain_active else BrainPhase.IDLE
        self.log("brain", f"Gremlin Brain {'ACTIVE' if st.brain_active else 'off'}")

    def toggle_evolve(self) -> None:
        st = self.state
        st.evolve_active = not st.evolve_active
        if st.evolve_active:
            st.brain_active = False
            self.log("evolve", f"Genetic engine ACTIVE (gen {st.genome.generation})")
        else:
            self.log("evolve", "Genetic engine off")

    def overdrive(self) -> None:
        st = self.state
        st.brain_active = True
        st.brain_phase = BrainPhase.IDLE
        if not st.evolve_active:
            self.toggle_evolve()
        self._apply_persona_config(Persona.STORM)
        st.auto_cycle = True
        self.log("host", "OVERDRIVE — full autonomous stack engaged", "warn")

    def toggle_contradiction(self) -> None:
        st = self.state
        st.contradiction = not st.contradiction
        if st.contradiction:
            st.pinned_vid = random.randint(0x1000, 0xFFFF)
            st.pinned_pid = random.randint(0x1000, 0xFFFF)
            self.log("device", f"Driver confusion ON {st.pinned_vid:04X}:{st.pinned_pid:04X}")
        else:
            self.log("device", "Driver confusion OFF")

    def _mutate_genome(self) -> None:
        g = self.state.genome
        g.generation += 1
        field = random.randint(0, 3)
        if field == 0:
            g.interval = random.randint(1, 20)
        elif field == 1:
            g.malformed = not g.malformed
        elif field == 2:
            g.real_vid = not g.real_vid
        else:
            g.contradiction = not g.contradiction
        if g.fitness > self._best_genome.fitness:
            self._best_genome = replace(g)
        self.log("evolve", f"Genome mutated → gen={g.generation} int={g.interval} mal={g.malformed}")

    def _brain_tick(self) -> None:
        st = self.state
        if not st.brain_active:
            return
        enums = st.enum_count
        if st.brain_phase == BrainPhase.IDLE:
            st.brain_phase = BrainPhase.PROBE
            self._apply_persona_config(Persona.PHANTOM)
            self.log("brain", "Entering PROBE")
        elif st.brain_phase == BrainPhase.PROBE and enums >= 15:
            st.brain_phase = BrainPhase.ESCALATE
            self._apply_persona_config(Persona.STORM)
            self.log("brain", "Escalating to STORM")
        elif st.brain_phase == BrainPhase.ESCALATE and enums >= 40:
            st.brain_phase = BrainPhase.CORRUPT
            self._apply_persona_config(Persona.CHIMERA)
            self.log("brain", "Escalating to CORRUPT")
        elif st.brain_phase == BrainPhase.CORRUPT and enums >= 80:
            st.brain_phase = BrainPhase.CHAOS
            self._apply_persona_config(Persona.HAUNTED)
            st.genome.interval = 1
            self.log("brain", "Maximum chaos — HAUNTED", "warn")

    def _host_react(self) -> None:
        st = self.state
        pain = 0.0
        if st.malformed:
            pain += 2.5
            if random.random() < 0.4:
                st.host_errors += 1
                self.log("kernel", "usb core: descriptor parse error", "error")
        if st.contradiction:
            for dev in st.host_devices:
                if dev.pinned and dev.vid == st.pinned_vid and dev.pid == st.pinned_pid:
                    if dev.device_class != st.device_class:
                        pain += 4.0
                        st.host_errors += 1
                        self.log(
                            "kernel",
                            f"driver cache conflict {st.pinned_vid:04X}:{st.pinned_pid:04X} "
                            f"({dev.device_class.value} vs {st.device_class.value})",
                            "error",
                        )
        if st.genome.interval <= 2:
            pain += 1.0
        if st.enum_count > 100:
            pain += 0.5
        st.pain_score = min(100.0, st.pain_score * 0.95 + pain)
        st.tolerance = max(0, min(100, st.tolerance - int(pain * 2)))
        if pain > 3 and random.random() < 0.08:
            st.disconnects += 1
            self.log("host", "Host disconnected device (stack rejection)", "warn")
            if st.evolve_active:
                self._mutate_genome()
            if st.brain_active and st.brain_phase.value not in ("Idle", "Probe"):
                st.brain_phase = BrainPhase.PROBE
                self._apply_persona_config(Persona.PHANTOM)
                self.log("brain", "De-escalating to PROBE")

    def enumerate(self) -> None:
        st = self.state
        if st.persona == Persona.MIMIC and random.random() < 0.5:
            self.deploy_mimic(random.randint(0, len(MIMIC_VAULT) - 1))
        else:
            st.vid, st.pid = self._random_vid_pid()
            if st.contradiction:
                st.vid, st.pid = st.pinned_vid, st.pinned_pid
            classes = DeviceClass.all()
            if st.auto_cycle:
                self._class_index = (self._class_index + 1) % len(classes)
                st.device_class = classes[self._class_index]
            st.product = f"{st.device_class.value} Device"
            if st.malformed:
                st.manufacturer = random.choice(["", "ZZZZ", "\xff\xfe"])

        st.enum_count += 1
        st.last_enum_flash = time.time()
        lat, rst = self._host_latency_profile()
        st.config_latency_ms = lat
        st.reset_count = rst
        if st.enum_count == 1 or st.enum_count % 7 == 0:
            self.classify_host()

        label = f"{st.manufacturer} {st.product}"[:28]
        pinned = st.contradiction
        host_dev = HostDevice(st.vid, st.pid, st.device_class, label, pinned)
        st.host_devices.insert(0, host_dev)
        st.host_devices = st.host_devices[:24]

        if st.evolve_active:
            st.genome.fitness += 1

        self._host_react()
        self._brain_tick()
        self.log(
            "enum",
            f"#{st.enum_count} {st.device_class.value} {st.vid:04X}:{st.pid:04X} "
            f"[{st.persona.value}]",
        )

    def tick(self, dt: float) -> None:
        st = self.state
        if not st.running:
            return
        st.packet_phase = (st.packet_phase + dt * 3) % 1.0
        for dev in st.host_devices:
            dev.age += dt
        if not st.auto_cycle and not st.brain_active and not st.evolve_active:
            return
        interval = st.genome.interval * 0.05
        if st.evolve_active:
            st.malformed = st.genome.malformed
            st.contradiction = st.genome.contradiction
            interval = st.genome.interval * 0.05
        self._tick_accum += dt
        if self._tick_accum >= interval:
            self._tick_accum = 0.0
            self.enumerate()
            if st.evolve_active and st.enum_count % 25 == 0:
                self._mutate_genome()

    def start(self) -> None:
        self.state.running = True
        self.log("sim", "Simulation started")

    def stop(self) -> None:
        self.state.running = False
        self.log("sim", "Simulation paused")

    def reset(self) -> None:
        self.state = SimulationState(host_os=self.state.host_os)
        self._class_index = 0
        self._tick_accum = 0.0
        self.log("sim", "Simulation reset")
