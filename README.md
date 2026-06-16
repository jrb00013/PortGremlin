# PortGremlin

PortGremlin is a **closed-loop USB enumeration attack platform** — firmware on a TM4C123 LaunchPad that fingerprints your host, evolves attack genomes, and coordinates with a host-side orchestrator. **No hardware?** Use the **Virtual Lab** GUI simulator.

**Authorized security research only.**

## One Command

```sh
./setup.sh          # interactive — choose Virtual Lab or Hardware Flash
./setup.sh --run    # launch (prompts which mode)
```

### Modes

| Mode | Command | What you get |
|------|---------|--------------|
| **Virtual Lab** | `./setup.sh --virtual` | GUI simulation — host, LaunchPad, USB bus, personas, evolution |
| **Hardware Flash** | `./setup.sh --flash` | ARM toolchain, firmware build, optional flash |
| **Run Virtual** | `./setup.sh --run --virtual` | Open the GUI simulator |
| **Run Hardware** | `./setup.sh --run --flash` | Overwatch monitor + dashboard at `:8765` |

## Virtual Lab (GUI)

Interactive visualization with no LaunchPad required:

- Animated USB bus between host and LaunchPad
- Live enumeration storm with VID/PID/class changes
- Oracle host fingerprinting (Windows/Linux/macOS)
- Gremlin Brain escalation, genetic evolution, personas
- Host pain meter, device cache, kernel error simulation
- One-click Overdrive, Brain, Evolve, Mimic deploy

```sh
./setup.sh --virtual    # installs python3-tk + deps, launches GUI
```

## Hardware Platform

| Layer | Capability |
|---|---|
| **Oracle** | Fingerprints Windows/Linux/macOS from enumeration timing and reset patterns |
| **Attack Personas** | Chimera, Mimic, Storm, Haunted, Phantom, Spectre — full behavioral profiles |
| **Gremlin Brain** (`b`) | Autonomous PROBE → ESCALATE → CORRUPT → CHAOS escalation |
| **Genetic Evolution** (`g`) | On-device genome mutation — interval, malformed, VID mode, contradiction |
| **JSON Telemetry** (`@PG{...}`) | Machine-readable event stream for closed-loop host control |
| **Overwatch** | Host orchestrator: telemetry + dmesg + lsusb + auto-escalation + dashboard |
| **Overdrive** (`x`) | One key: Brain + Evolution + RedTeam choreography + telemetry |

## Architecture

```
┌─────────────────────┐         serial @PG{json}         ┌──────────────────────┐
│  TM4C123 LaunchPad  │ ──────────────────────────────► │  PortGremlin Overwatch│
│  • Oracle fingerprint│                                 │  • dmesg watcher      │
│  • Gremlin Brain     │ ◄────────────────────────────── │  • lsusb monitor      │
│  • Genetic evolution │         auto-cmd (b,p,g,d...)   │  • auto-escalation    │
│  • Attack personas   │                                 │  • web dashboard      │
└──────────┬──────────┘                                 └──────────────────────┘
           │ USB
           ▼
    ┌──────────────┐
    │  Target Host │  ← kernel USB errors = pain signals
    └──────────────┘
```

## UART Commands

| Key | Action |
|-----|--------|
| `x` | **Overdrive** — brain + evolution + RedTeam + telemetry |
| `b` | Gremlin Brain |
| `g` | Genetic evolution |
| `p` | Next persona |
| `o` | Oracle report |
| `d` | Driver confusion (same VID, different class) |
| `l` | Toggle JSON telemetry |
| `0`–`9` | Deploy mimic profile |
| `[` `]` `\` | Choreography: RedTeam / Stealth / Blitz |
| `h` | Help |

## Host Tools

```sh
./setup.sh --run --virtual             # GUI Virtual Lab
./setup.sh --run --flash               # Overwatch + dashboard
python3 tools/portgremlin-cli.py       # manual serial control
python3 tools/gremlin-oracle.py        # dual-perspective monitor
```

## Build & Flash

```sh
export TIVAWARE_PATH=/opt/ti/TivaWare_C_Series-2.2.0.295
./setup.sh
make -C usb_dev_keyboard flash
```

## Hardware

- **EK-TM4C123GXL** LaunchPad (TM4C123GH6PM)
- Device USB port → target host under test
- ICDI serial port → control machine at 115200 baud

## Project Layout

```
usb_dev_keyboard/
  portgremlin_oracle.c      Host fingerprinting + Gremlin Brain
  portgremlin_persona.c     Attack personas + choreography
  portgremlin_evolve.c      Genetic attack genome engine
  portgremlin_telemetry.c   JSON @PG{...} event stream
  portgremlin_mimic.c       Real device identity vault
  portgremlin_uart.c        Command interface
tools/
  portgremlin-simulator.py  Virtual Lab GUI
  sim_engine.py             Firmware behavior simulation
  portgremlin-overwatch.py  Hardware orchestrator + dashboard
  portgremlin-cli.py        Interactive serial control
  gremlin-oracle.py         Dual-perspective session monitor
setup.sh                    Interactive setup + run
```

## License

MIT — see LICENSE. Depends on TI TivaWare (separately licensed).
