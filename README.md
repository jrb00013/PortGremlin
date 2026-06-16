# PortGremlin

PortGremlin is a **closed-loop USB enumeration attack platform** — firmware on a TM4C123 LaunchPad that fingerprints your host, evolves attack genomes, and coordinates with a host-side orchestrator that watches kernel USB errors and escalates autonomously.

**Authorized security research only.**

## One Command

```sh
./setup.sh          # install everything
./setup.sh --run    # launch Overwatch (dashboard + orchestrator)
```

Open **http://127.0.0.1:8765** for the live attack dashboard.

## Platform Layers

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
./setup.sh --run                         # Overwatch + dashboard
./setup.sh --run --no-auto               # monitor only
python3 tools/portgremlin-cli.py         # manual serial control
python3 tools/gremlin-oracle.py         # dual-perspective monitor
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
  portgremlin-overwatch.py  Closed-loop host orchestrator + dashboard
  portgremlin-cli.py        Interactive serial control
  gremlin-oracle.py         Dual-perspective session monitor
setup.sh                    Install deps + ./setup.sh --run
```

## License

MIT — see LICENSE. Depends on TI TivaWare (separately licensed).
