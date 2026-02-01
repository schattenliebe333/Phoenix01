#!/bin/bash
# ═══════════════════════════════════════════════════════════════════════════════
# RAEL V49 — Windows GUI Cross-Compilation (MinGW)
# Erstellt: rael_gui.exe mit Machine-Binding
# ═══════════════════════════════════════════════════════════════════════════════

set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
ROOT="$(dirname "$SCRIPT_DIR")"
BIN_DIR="$ROOT/bin"

# MinGW Compiler
CXX="x86_64-w64-mingw32-g++"

# Prüfen ob MinGW installiert ist
if ! command -v $CXX &> /dev/null; then
    echo "ERROR: MinGW not found. Install with: apt install g++-mingw-w64-x86-64"
    exit 1
fi

mkdir -p "$BIN_DIR"

echo "═══════════════════════════════════════════════════════════════════════════════"
echo "  RAEL V49 — Windows GUI Cross-Compilation"
echo "═══════════════════════════════════════════════════════════════════════════════"
echo ""

# Compiler-Flags
CXXFLAGS="-std=c++17 -O2 -Wall -Wextra -Wno-unused-parameter"
CXXFLAGS="$CXXFLAGS -I$ROOT/include"
CXXFLAGS="$CXXFLAGS -DWIN32 -D_WIN32 -DUNICODE -D_UNICODE"
CXXFLAGS="$CXXFLAGS -static -mwindows -municode"

# Linker-Flags (Windows-Bibliotheken)
LDFLAGS="-static"
LDFLAGS="$LDFLAGS -lcomctl32"       # Common Controls (Tabs, etc.)
LDFLAGS="$LDFLAGS -luser32"         # Windows User Interface
LDFLAGS="$LDFLAGS -lgdi32"          # Graphics Device Interface
LDFLAGS="$LDFLAGS -liphlpapi"       # IP Helper API (MAC Address)
LDFLAGS="$LDFLAGS -lshell32"        # Shell API (Folder Paths)
LDFLAGS="$LDFLAGS -ladvapi32"       # Advanced API (Registry)
LDFLAGS="$LDFLAGS -lole32"          # OLE/COM
LDFLAGS="$LDFLAGS -lws2_32"         # Winsock
LDFLAGS="$LDFLAGS -lcomdlg32"       # Common Dialogs
LDFLAGS="$LDFLAGS -lwinmm"          # Windows Multimedia

# GUI Source
GUI_SRC="$ROOT/src/gui/rael_gui_win.cpp"
GUI_OUT="$BIN_DIR/rael_gui.exe"

echo "[build] Compiling Windows GUI..."
echo "  Source: $GUI_SRC"
echo "  Output: $GUI_OUT"
echo ""

$CXX $CXXFLAGS "$GUI_SRC" -o "$GUI_OUT" $LDFLAGS

if [ $? -eq 0 ]; then
    echo ""
    echo "═══════════════════════════════════════════════════════════════════════════════"
    echo "  BUILD SUCCESSFUL"
    echo "═══════════════════════════════════════════════════════════════════════════════"
    echo ""
    ls -lh "$GUI_OUT"
    echo ""
    echo "Die GUI-EXE enthält:"
    echo "  - Machine-Binding (Hardware-Lizenzierung)"
    echo "  - RAEL Cockpit Interface"
    echo "  - Builder/Improvements Tabs"
    echo "  - Face Animation Panel"
    echo ""
    echo "HINWEIS: Die GUI erwartet 'rael.exe' im selben Verzeichnis."
    echo "         Die rael.exe ist der Core (separates Build erforderlich)."
    echo ""
else
    echo ""
    echo "═══════════════════════════════════════════════════════════════════════════════"
    echo "  BUILD FAILED"
    echo "═══════════════════════════════════════════════════════════════════════════════"
    exit 1
fi
