# PortGremlin NEXUS

PortGremlin is a **closed-loop USB enumeration attack platform** — firmware on a $15 LaunchPad that fingerprints your host, evolves attack genomes, and coordinates with a host-side AI orchestrator that watches kernel panic signals and escalates autonomously.

**Authorized security research only.**

## One Command

```sh
./setup.sh          # install everything
./setup.sh --run    # launch Nexus (dashboard + orchestrator)
```

Open **http://127.0.0.1:8765** for the live attack dashboard.

## What's New in NEXUS

| Layer | Capability |
|---|---|
| **Genetic Evolution** (`g`) | On-device genetic algorithm mutates attack genomes (interval, malformed, VID mode, contradiction) — survives disconnects, mutates on rejection |
| **JSON Telemetry** (`@PG{...}`) | Machine-readable event stream for closed-loop host control |
| **Gremlin Nexus** | Host orchestrator reads telemetry + `dmesg`/`journalctl` USB errors + `lsusb` — **auto-sends escalation commands** |
| **Nexus Mode** (`x`) | One key: Brain + Evolution + RedTeam choreography + telemetry |
| **Live Dashboard** | Real-time web UI: host OS, persona, pain score, event stream |

## Architecture

```
┌─────────────────────┐         serial @PG{json}         ┌──────────────────────┐
│  TM4C123 LaunchPad  │ ──────────────────────────────► │   Gremlin Nexus      │
│  • Oracle fingerprint│                                 │   • dmesg watcher    │
│  • Gremlin Brain     │ ◄────────────────────────────── │   • lsusb monitor    │
│  • Genetic evolution │         auto-cmd (b,p,g,d...)   │   • auto-escalation  │
│  • Attack personas   │                                 │   • web dashboard    │
└──────────┬──────────┘                                 └──────────────────────┘
           │ USB
           ▼
    ┌──────────────┐
    │  Target Host │  ← kernel USB errors = pain signals
    └──────────────┘
```

## Quick Reference

### Firmware UART
| Key | Action |
|-----|--------|
| `x` | **NEXUS MODE** — full autonomous stack |
| `b` | Gremlin Brain |
| `g` | Genetic evolution engine |
| `p` | Next persona |
| `o` | Oracle report |
| `d` | Driver confusion |
| `l` | Toggle JSON telemetry |
| `[` `]` `\` | Choreography scripts |

### Host
```sh
./setup.sh --run                    # full nexus + dashboard
./setup.sh --run --no-auto          # monitor only, no auto-cmd
./setup.sh --run --no-browser       # skip opening browser
python3 tools/portgremlin-cli.py    # manual serial control
```

## Setup Details

`./setup.sh` installs:
- Python venv + pyserial
- `usbutils` (lsusb)
- `gcc-arm-none-eabi` (if apt available)
- Builds firmware if `TIVAWARE_PATH` is set

```sh
export TIVAWARE_PATH=/opt/ti/TivaWare_C_Series-2.2.0.295
./setup.sh
make -C usb_dev_keyboard flash
```

## Hardware

- **EK-TM4C123GXL** LaunchPad
- Device USB → target host
- ICDI serial → control machine running Nexus

## License

MIT — see LICENSE. Depends on TI TivaWare (separately licensed).
