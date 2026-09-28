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
| **Doctor** | `./setup.sh --doctor` | Report which prerequisites are present, change nothing |

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
| `v` | Print the mimic vault |
| `0` `6`–`9` | Deploy mimic vault profile N (slots 1–5 share digits with class toggles) |
| `n` | Next mimic vault profile (walks the full vault) |
| `[` `]` `\` | Choreography: RedTeam / Stealth / Blitz |
| `1`–`5` | Toggle device class: keyboard / audio / printer / MIDI / gamepad |
| `+` / `-` | Cycle interval faster / slower |
| `a` | Toggle auto cycle |
| `m` | Toggle malformed mode |
| `r` | Toggle real VID database |
| `t` | Randomise descriptor strings |
| `c` | Force a cycle |
| `e` | Force re-enumeration |
| `s` | Status report |
| `h` | Help |

## Host Tools

```sh
./setup.sh --run --virtual             # GUI Virtual Lab
./setup.sh --run --flash               # Overwatch + dashboard
python3 tools/portgremlin-cli.py       # manual serial control
python3 tools/gremlin-oracle.py        # dual-perspective monitor
```

## Build & Flash

The firmware build needs TI's TivaWare C Series SDK. It is not
redistributable, so it is neither vendored nor downloaded by `setup.sh` —
download it from TI and point the build at it:

```sh
export TIVAWARE_PATH=/opt/ti/TivaWare_C_Series-2.2.0.295
./setup.sh --doctor      # confirm the toolchain and SDK are visible
./setup.sh --flash
make -C usb_dev_keyboard flash
```

Run `./setup.sh --doctor` first if a build is skipped: it reports exactly
which prerequisite is missing instead of warning and continuing.

## Testing

The host tools and the Virtual Lab engine are covered by a test suite that
needs no hardware:

```sh
pip install -r tools/requirements.txt pytest
python3 -m pytest tests -q
```

The suite pins the contracts that are easy to break silently:

- every key the host CLI can send is a key the firmware implements
- the on-device help and this README only advertise keys that exist
- the Virtual Lab engine round-trips every host fingerprint it simulates
- driver confusion never emits the reserved `0x0000` identity
- the `@PG{...}` telemetry payloads stay flat JSON the host parser accepts
- Makefile source lists, the linker script and declared dependencies resolve

CI (`.github/workflows/ci.yml`) runs these on every push and pull request.
The firmware itself is not compiled in CI, because the TivaWare SDK is
license-gated; that job is opt-in via the `TIVAWARE_AVAILABLE` repository
variable.

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
tests/                      Host tool + Virtual Lab test suite
setup.sh                    Interactive setup + run
```

## License

MIT — see LICENSE. Depends on TI TivaWare (separately licensed).
