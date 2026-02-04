#!/bin/bash
# ═══════════════════════════════════════════════════════════════════════════════
# RAEL Installation Script (Linux/macOS)
# Installs RAEL CLI to system PATH
# ═══════════════════════════════════════════════════════════════════════════════

set -e

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
CYAN='\033[0;36m'
NC='\033[0m'

echo -e "${CYAN}"
echo "╔═══════════════════════════════════════════════════════════════════╗"
echo "║  RAEL INSTALLATION                                                ║"
echo "╚═══════════════════════════════════════════════════════════════════╝"
echo -e "${NC}"

# Determine script directory
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(dirname "$SCRIPT_DIR")"
BIN_DIR="$ROOT_DIR/bin"

# Check if binary exists
if [ ! -f "$BIN_DIR/rael_chat" ]; then
    echo -e "${YELLOW}[BUILD] rael_chat nicht gefunden, kompiliere...${NC}"
    cd "$SCRIPT_DIR"

    # Build rael_chat
    g++ -std=c++17 -O2 -Wall -Wextra \
        -I"$ROOT_DIR/include" \
        "$ROOT_DIR/src/cli/rael_chat.cpp" \
        "$ROOT_DIR/src/core/raelcore.cpp" \
        "$ROOT_DIR/src/core/ichbin.cpp" \
        "$ROOT_DIR/src/core/ethics.cpp" \
        "$ROOT_DIR/src/core/mathcore.cpp" \
        "$ROOT_DIR/src/core/semantic.cpp" \
        "$ROOT_DIR/src/core/resonance.cpp" \
        "$ROOT_DIR/src/core/telemetry.cpp" \
        "$ROOT_DIR/src/core/util.cpp" \
        "$ROOT_DIR/src/core/module_manager.cpp" \
        "$ROOT_DIR/src/core/events.cpp" \
        "$ROOT_DIR/src/core/metrics.cpp" \
        "$ROOT_DIR/src/core/star8.cpp" \
        "$ROOT_DIR/src/core/lane_scheduler.cpp" \
        -lpthread -ldl \
        -o "$BIN_DIR/rael_chat" 2>/dev/null || {
            echo -e "${RED}[ERROR] Build fehlgeschlagen. Führe zuerst build.sh aus.${NC}"
            exit 1
        }

    echo -e "${GREEN}[OK] rael_chat kompiliert${NC}"
fi

# Determine installation directory
INSTALL_DIR=""
SHELL_RC=""

if [ "$EUID" -eq 0 ]; then
    # Running as root - system-wide installation
    INSTALL_DIR="/usr/local/bin"
    echo -e "${CYAN}[INFO] System-weite Installation nach $INSTALL_DIR${NC}"
else
    # User installation
    INSTALL_DIR="$HOME/.local/bin"
    mkdir -p "$INSTALL_DIR"

    # Determine shell config file
    if [ -n "$ZSH_VERSION" ] || [ -f "$HOME/.zshrc" ]; then
        SHELL_RC="$HOME/.zshrc"
    elif [ -f "$HOME/.bashrc" ]; then
        SHELL_RC="$HOME/.bashrc"
    elif [ -f "$HOME/.bash_profile" ]; then
        SHELL_RC="$HOME/.bash_profile"
    fi

    echo -e "${CYAN}[INFO] Benutzer-Installation nach $INSTALL_DIR${NC}"
fi

# Copy binary
echo -e "${YELLOW}[COPY] Kopiere rael_chat nach $INSTALL_DIR/rael${NC}"
cp "$BIN_DIR/rael_chat" "$INSTALL_DIR/rael"
chmod +x "$INSTALL_DIR/rael"

# Also create symlink for 'rael_chat' if desired
if [ ! -f "$INSTALL_DIR/rael_chat" ]; then
    ln -sf "$INSTALL_DIR/rael" "$INSTALL_DIR/rael_chat" 2>/dev/null || true
fi

# Update PATH if needed (user installation only)
if [ -n "$SHELL_RC" ] && [ "$EUID" -ne 0 ]; then
    if ! grep -q "$INSTALL_DIR" "$SHELL_RC" 2>/dev/null; then
        echo "" >> "$SHELL_RC"
        echo "# RAEL CLI" >> "$SHELL_RC"
        echo "export PATH=\"\$PATH:$INSTALL_DIR\"" >> "$SHELL_RC"
        echo -e "${GREEN}[OK] PATH aktualisiert in $SHELL_RC${NC}"
        echo -e "${YELLOW}[INFO] Führe 'source $SHELL_RC' aus oder öffne ein neues Terminal${NC}"
    else
        echo -e "${GREEN}[OK] $INSTALL_DIR ist bereits in PATH${NC}"
    fi
fi

# Verify installation
if command -v rael &> /dev/null || [ -f "$INSTALL_DIR/rael" ]; then
    echo ""
    echo -e "${GREEN}╔═══════════════════════════════════════════════════════════════════╗${NC}"
    echo -e "${GREEN}║  INSTALLATION ERFOLGREICH!                                        ║${NC}"
    echo -e "${GREEN}╚═══════════════════════════════════════════════════════════════════╝${NC}"
    echo ""
    echo -e "  Starte RAEL mit: ${CYAN}rael${NC}"
    echo -e "  Hilfe anzeigen:  ${CYAN}rael --help${NC}"
    echo ""

    # Test if executable works
    "$INSTALL_DIR/rael" --version 2>/dev/null || true
else
    echo -e "${RED}[ERROR] Installation fehlgeschlagen${NC}"
    exit 1
fi
