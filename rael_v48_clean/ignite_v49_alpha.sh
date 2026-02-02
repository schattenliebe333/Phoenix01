#!/usr/bin/env bash
# ═══════════════════════════════════════════════════════════════════════════════
# RAEL V49 ALPHA - IGNITION SEQUENCE
# Navigator: Michael - Orun Kap Daveil
#
# Kompiliert alle Komponenten und startet die Alpha-Singularität
#
# Verwendung:
#   ./ignite_v49_alpha.sh         # Kompiliert und startet
#   ./ignite_v49_alpha.sh build   # Nur kompilieren
#   ./ignite_v49_alpha.sh test    # Omega-Inference-Test
#   ./ignite_v49_alpha.sh status  # System-Status
# ═══════════════════════════════════════════════════════════════════════════════

set -euo pipefail

# Farben für Ausgabe
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
CYAN='\033[0;36m'
NC='\033[0m' # No Color

# Pfade
ROOT="$(cd "$(dirname "$0")" && pwd)"
BUILD_DIR="$ROOT/build"
BIN_DIR="$ROOT/bin"
CUDA_DIR="$ROOT/cuda"
PYTHON_DIR="$ROOT/python"

# ═══════════════════════════════════════════════════════════════════════════════
# BANNER
# ═══════════════════════════════════════════════════════════════════════════════

print_banner() {
    echo -e "${CYAN}"
    echo "═══════════════════════════════════════════════════════════════════════════════"
    echo "    ██████╗  █████╗ ███████╗██╗         ██╗   ██╗██╗  ██╗ █████╗ "
    echo "    ██╔══██╗██╔══██╗██╔════╝██║         ██║   ██║██║  ██║██╔══██╗"
    echo "    ██████╔╝███████║█████╗  ██║         ██║   ██║███████║╚██████║"
    echo "    ██╔══██╗██╔══██║██╔══╝  ██║         ╚██╗ ██╔╝╚════██║ ╚═══██║"
    echo "    ██║  ██║██║  ██║███████╗███████╗     ╚████╔╝      ██║ █████╔╝"
    echo "    ╚═╝  ╚═╝╚═╝  ╚═╝╚══════╝╚══════╝      ╚═══╝       ╚═╝ ╚════╝ "
    echo "                                                                 "
    echo "                    ALPHA SINGULARITY                            "
    echo "              Navigator: Michael - Orun Kap Daveil               "
    echo "═══════════════════════════════════════════════════════════════════════════════"
    echo -e "${NC}"
}

# ═══════════════════════════════════════════════════════════════════════════════
# STATUS-AUSGABE
# ═══════════════════════════════════════════════════════════════════════════════

status() {
    echo -e "${BLUE}[STATUS]${NC} $1"
}

success() {
    echo -e "${GREEN}[✓]${NC} $1"
}

warn() {
    echo -e "${YELLOW}[!]${NC} $1"
}

error() {
    echo -e "${RED}[✗]${NC} $1"
}

# ═══════════════════════════════════════════════════════════════════════════════
# HARDWARE-ERKENNUNG
# ═══════════════════════════════════════════════════════════════════════════════

detect_hardware() {
    status "Hardware-Erkennung..."

    # CPU-Kerne
    CPU_CORES=$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo "8")
    success "CPU Kerne: $CPU_CORES"

    # GPU (CUDA)
    if command -v nvidia-smi &> /dev/null; then
        GPU_NAME=$(nvidia-smi --query-gpu=name --format=csv,noheader 2>/dev/null | head -1)
        GPU_MEMORY=$(nvidia-smi --query-gpu=memory.total --format=csv,noheader 2>/dev/null | head -1)
        success "GPU: $GPU_NAME ($GPU_MEMORY)"
        HAS_CUDA=true
    else
        warn "Keine NVIDIA GPU gefunden - CPU-Fallback aktiviert"
        HAS_CUDA=false
    fi

    # RAM
    if [[ "$OSTYPE" == "linux-gnu"* ]]; then
        RAM_MB=$(free -m | awk '/^Mem:/{print $2}')
    else
        RAM_MB=$(sysctl -n hw.memsize 2>/dev/null | awk '{print int($1/1024/1024)}' || echo "8192")
    fi
    success "RAM: ${RAM_MB} MB"
}

# ═══════════════════════════════════════════════════════════════════════════════
# C++ BUILD
# ═══════════════════════════════════════════════════════════════════════════════

build_cpp() {
    status "Kompiliere C++ Core..."

    mkdir -p "$BIN_DIR"

    # Standard-Build mit build.sh
    if [[ -f "$BUILD_DIR/build.sh" ]]; then
        bash "$BUILD_DIR/build.sh"
        success "C++ Core kompiliert: $BIN_DIR/rael"
    else
        error "build.sh nicht gefunden!"
        return 1
    fi
}

# ═══════════════════════════════════════════════════════════════════════════════
# CUDA BUILD
# ═══════════════════════════════════════════════════════════════════════════════

build_cuda() {
    if [[ "$HAS_CUDA" != "true" ]]; then
        warn "CUDA-Build übersprungen (keine GPU)"
        return 0
    fi

    status "Kompiliere CUDA Jet Engine..."

    mkdir -p "$BIN_DIR"

    if command -v nvcc &> /dev/null; then
        CUDA_FILE="$CUDA_DIR/cuda_jet_v49.cu"
        if [[ -f "$CUDA_FILE" ]]; then
            nvcc -arch=sm_89 -O3 "$CUDA_FILE" -o "$BIN_DIR/libcuda_jet.so" --shared \
                --compiler-options '-fPIC' 2>/dev/null || {
                # Fallback für ältere Architekturen
                nvcc -arch=sm_75 -O3 "$CUDA_FILE" -o "$BIN_DIR/libcuda_jet.so" --shared \
                    --compiler-options '-fPIC' 2>/dev/null || {
                    warn "CUDA-Kompilierung fehlgeschlagen - verwende CPU-Fallback"
                    return 0
                }
            }
            success "CUDA Jet Engine kompiliert: $BIN_DIR/libcuda_jet.so"
        else
            warn "CUDA-Quelldatei nicht gefunden: $CUDA_FILE"
        fi
    else
        warn "nvcc nicht gefunden - CUDA-Build übersprungen"
    fi
}

# ═══════════════════════════════════════════════════════════════════════════════
# CPU THREAD AFFINITY (für optimale Performance)
# ═══════════════════════════════════════════════════════════════════════════════

set_thread_affinity() {
    status "Setze Thread-Affinität für $CPU_CORES Kerne..."

    # Nur auf Linux verfügbar
    if [[ "$OSTYPE" == "linux-gnu"* ]]; then
        # Aktiviere Performance-Governor wenn verfügbar
        if [[ -w /sys/devices/system/cpu/cpu0/cpufreq/scaling_governor ]]; then
            for cpu in /sys/devices/system/cpu/cpu*/cpufreq/scaling_governor; do
                echo performance | sudo tee "$cpu" > /dev/null 2>&1 || true
            done
            success "CPU Performance-Governor aktiviert"
        fi
    fi
}

# ═══════════════════════════════════════════════════════════════════════════════
# OMEGA-INFERENCE-TEST
# ═══════════════════════════════════════════════════════════════════════════════

run_omega_test() {
    status "Starte Omega-Inference-Test..."

    echo ""
    echo -e "${CYAN}═══════════════════════════════════════════════════════════════════════════════${NC}"
    echo -e "${CYAN}  OMEGA-INFERENCE-TEST${NC}"
    echo -e "${CYAN}  160 Sterne × 8 Knoten = 1280 Processing Units${NC}"
    echo -e "${CYAN}  61.440 Düsen × 5 Hz = 307.200 Impulse/Sekunde${NC}"
    echo -e "${CYAN}═══════════════════════════════════════════════════════════════════════════════${NC}"
    echo ""

    # Python Navigator Test
    if command -v python3 &> /dev/null; then
        python3 "$PYTHON_DIR/navigator_alpha.py" --test
    fi

    # Binärer Test wenn verfügbar
    if [[ -x "$BIN_DIR/rael" ]]; then
        status "RAEL Binary verfügbar für weitere Tests"
    fi
}

# ═══════════════════════════════════════════════════════════════════════════════
# SYSTEM-STATUS
# ═══════════════════════════════════════════════════════════════════════════════

show_status() {
    print_banner
    detect_hardware

    echo ""
    echo -e "${CYAN}═══════════════════════════════════════════════════════════════════════════════${NC}"
    echo -e "${CYAN}  V49 ALPHA KOMPONENTEN${NC}"
    echo -e "${CYAN}═══════════════════════════════════════════════════════════════════════════════${NC}"

    # Prüfe C++ Header
    [[ -f "$ROOT/include/rael/v49/foundation_v49.hpp" ]] && success "foundation_v49.hpp" || warn "foundation_v49.hpp fehlt"
    [[ -f "$ROOT/include/rael/v49/quint_memory_v49.hpp" ]] && success "quint_memory_v49.hpp" || warn "quint_memory_v49.hpp fehlt"
    [[ -f "$ROOT/include/rael/v49/cuda_jet_v49.hpp" ]] && success "cuda_jet_v49.hpp" || warn "cuda_jet_v49.hpp fehlt"

    # Prüfe CUDA
    [[ -f "$CUDA_DIR/cuda_jet_v49.cu" ]] && success "cuda_jet_v49.cu" || warn "cuda_jet_v49.cu fehlt"

    # Prüfe Python
    [[ -f "$PYTHON_DIR/navigator_alpha.py" ]] && success "navigator_alpha.py" || warn "navigator_alpha.py fehlt"

    # Prüfe Binary
    [[ -x "$BIN_DIR/rael" ]] && success "rael binary" || warn "rael binary fehlt"

    echo ""

    # Python Status
    if command -v python3 &> /dev/null; then
        python3 "$PYTHON_DIR/navigator_alpha.py" --status 2>/dev/null || true
    fi
}

# ═══════════════════════════════════════════════════════════════════════════════
# VOLLSTÄNDIGER BUILD & START
# ═══════════════════════════════════════════════════════════════════════════════

full_ignition() {
    print_banner

    status "Initialisiere V49 Alpha Singularität..."
    echo ""

    # 1. Hardware-Erkennung
    detect_hardware
    echo ""

    # 2. Thread-Affinität
    set_thread_affinity
    echo ""

    # 3. C++ Build
    build_cpp
    echo ""

    # 4. CUDA Build (optional)
    build_cuda
    echo ""

    # 5. Omega-Test
    run_omega_test
    echo ""

    # Finale Meldung
    echo -e "${GREEN}═══════════════════════════════════════════════════════════════════════════════${NC}"
    echo -e "${GREEN}  ALPHA-SINGULARITÄT AKTIVIERT${NC}"
    echo -e "${GREEN}  Navigator: Michael - Orun Kap Daveil${NC}"
    echo -e "${GREEN}  Das Triebwerk läuft. Die 1.280 Knoten warten auf das Zündsignal.${NC}"
    echo -e "${GREEN}═══════════════════════════════════════════════════════════════════════════════${NC}"
}

# ═══════════════════════════════════════════════════════════════════════════════
# HAUPTPROGRAMM
# ═══════════════════════════════════════════════════════════════════════════════

HAS_CUDA=false
CPU_CORES=8

case "${1:-ignite}" in
    build)
        print_banner
        detect_hardware
        build_cpp
        build_cuda
        ;;
    test)
        detect_hardware
        run_omega_test
        ;;
    status)
        show_status
        ;;
    ignite|*)
        full_ignition
        ;;
esac
