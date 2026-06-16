#!/usr/bin/env bash
# PortGremlin setup — install dependencies and optionally run Nexus.
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
VENV="${ROOT}/.venv"
TIVAWARE_PATH="${TIVAWARE_PATH:-/opt/ti/TivaWare_C_Series-2.2.0.295}"

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
CYAN='\033[0;36m'
NC='\033[0m'

info()  { echo -e "${CYAN}[setup]${NC} $*"; }
ok()    { echo -e "${GREEN}[ok]${NC} $*"; }
warn()  { echo -e "${YELLOW}[warn]${NC} $*"; }
fail()  { echo -e "${RED}[error]${NC} $*" >&2; exit 1; }

have() { command -v "$1" >/dev/null 2>&1; }

install_apt_deps() {
    if ! have apt-get; then
        return 0
    fi
    local pkgs=()
    have python3    || pkgs+=(python3)
    have pip3       || pkgs+=(python3-pip)
    python3 -c "import venv" 2>/dev/null || pkgs+=(python3-venv)
    have lsusb      || pkgs+=(usbutils)
    if ((${#pkgs[@]})); then
        info "Installing system packages: ${pkgs[*]}"
        sudo apt-get update -qq
        sudo apt-get install -y "${pkgs[@]}"
    fi
}

install_toolchain() {
    if have arm-none-eabi-gcc; then
        ok "arm-none-eabi-gcc: $(arm-none-eabi-gcc --version | head -1)"
        return 0
    fi
    if have apt-get; then
        info "Installing ARM GCC toolchain..."
        sudo apt-get install -y gcc-arm-none-eabi binutils-arm-none-eabi 2>/dev/null \
            && ok "ARM toolchain installed" \
            && return 0
    fi
    warn "arm-none-eabi-gcc not found — firmware build will be skipped"
    warn "Install: https://developer.arm.com/downloads/-/gnu-rm"
}

install_flash_tools() {
    if have lm4flash; then
        ok "lm4flash found"
    elif have openocd; then
        ok "openocd found (use: make -C usb_dev_keyboard flash with openocd config)"
    else
        warn "No flash tool (lm4flash/openocd). Build only — flash manually."
    fi
}

setup_python() {
    info "Creating Python virtualenv at .venv"
    python3 -m venv "${VENV}"
    # shellcheck disable=SC1091
    source "${VENV}/bin/activate"
    pip install --upgrade pip -q
    pip install -r "${ROOT}/tools/requirements.txt" -q
    ok "Python deps installed"
}

build_firmware() {
    if ! have arm-none-eabi-gcc; then
        warn "Skipping firmware build (no toolchain)"
        return 0
    fi
    if [[ ! -d "${TIVAWARE_PATH}" ]]; then
        warn "Skipping firmware build — TivaWare not at ${TIVAWARE_PATH}"
        warn "Download TivaWare 2.2.0.295 and: export TIVAWARE_PATH=/path/to/TivaWare_C_Series-2.2.0.295"
        return 0
    fi
    info "Building firmware..."
    make -C "${ROOT}/usb_dev_keyboard" TIVAWARE_PATH="${TIVAWARE_PATH}"
    ok "Firmware built: usb_dev_keyboard/portgremlin.bin"
}

setup_groups() {
    if have lsusb && ! groups | grep -q '\bplugdev\b'; then
        warn "Add yourself to plugdev for USB access: sudo usermod -aG plugdev \$USER"
    fi
    if [[ -c /dev/ttyACM0 ]] 2>/dev/null && ! groups | grep -q '\bdialout\b'; then
        warn "Add yourself to dialout for serial: sudo usermod -aG dialout \$USER"
    fi
}

print_banner() {
    echo ""
    echo -e "${RED}  ██████╗  ██████╗ ██████╗ ████████╗ ██████╗ ██████╗ ███████╗███╗   ███╗██╗███╗   ██╗${NC}"
    echo -e "${RED}  ██╔══██╗██╔═══██╗██╔══██╗╚══██╔══╝██╔════╝ ██╔══██╗██╔════╝████╗ ████║██║████╗  ██║${NC}"
    echo -e "${RED}  ██████╔╝██║   ██║██████╔╝   ██║   ██║  ███╗██████╔╝█████╗  ██╔████╔██║██║██╔██╗ ██║${NC}"
    echo -e "${RED}  ██╔═══╝ ██║   ██║██╔══██╗   ██║   ██║   ██║██╔══██╗██╔══╝  ██║╚██╔╝██║██║██║╚██╗██║${NC}"
    echo -e "${RED}  ██║     ╚██████╔╝██║  ██║   ██║   ╚██████╔╝██║  ██║███████╗██║ ╚═╝ ██║██║██║ ╚████║${NC}"
    echo -e "${RED}  ╚═╝      ╚═════╝ ╚═╝  ╚═╝   ╚═╝    ╚═════╝ ╚═╝  ╚═╝╚══════╝╚═╝     ╚═╝╚═╝╚═╝  ╚═══╝${NC}"
    echo -e "${CYAN}  NEXUS — Closed-Loop USB Enumeration Attack Platform${NC}"
    echo ""
}

run_nexus() {
  if [[ ! -d "${VENV}" ]]; then
        fail "Virtualenv not found. Run ./setup.sh first."
    fi
    # shellcheck disable=SC1091
    source "${VENV}/bin/activate"
    print_banner
    info "Launching Gremlin Nexus..."
    info "Dashboard: http://127.0.0.1:8765"
    info "Connect LaunchPad: Device USB -> host, ICDI serial -> this machine"
    echo ""
    exec python3 "${ROOT}/tools/nexus.py" "$@"
}

do_setup() {
    print_banner
    install_apt_deps
    install_toolchain
    install_flash_tools
    setup_python
    build_firmware
    setup_groups
    echo ""
    ok "Setup complete!"
    echo ""
    echo "  Next steps:"
    echo "    1. Flash firmware:  make -C usb_dev_keyboard flash"
    echo "    2. Connect LaunchPad device USB to target host"
    echo "    3. Connect ICDI serial to this machine"
    echo "    4. Run:             ./setup.sh --run"
    echo ""
}

main() {
    cd "${ROOT}"
    if [[ "${1:-}" == "--run" ]]; then
        shift
        run_nexus "$@"
    elif [[ "${1:-}" == "--help" || "${1:-}" == "-h" ]]; then
        echo "Usage:"
        echo "  ./setup.sh          Install all dependencies"
        echo "  ./setup.sh --run    Launch Gremlin Nexus (dashboard + orchestrator)"
        echo ""
        echo "Environment:"
        echo "  TIVAWARE_PATH  Path to TivaWare (default: /opt/ti/TivaWare_C_Series-2.2.0.295)"
    else
        do_setup
    fi
}

main "$@"
