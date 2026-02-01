#!/usr/bin/env bash
# ═══════════════════════════════════════════════════════════════════════════════
# RAEL V49.0 — Windows Cross-Compile Build Script
# Kompiliert die Machine-Bound Windows EXE
# ═══════════════════════════════════════════════════════════════════════════════

set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
OUT="$ROOT/bin"
mkdir -p "$OUT"

CXX="x86_64-w64-mingw32-g++"
CXXFLAGS="-std=c++17 -O2 -Wall -Wextra -I$ROOT/include -DWIN32 -D_WIN32 -static"
LDFLAGS="-static -liphlpapi -lws2_32 -lole32 -lshell32 -ladvapi32"

echo "═══════════════════════════════════════════════════════════════════════════"
echo "  RAEL V49.0 - Windows Build (Machine-Bound)"
echo "═══════════════════════════════════════════════════════════════════════════"
echo ""
echo "[build] Compiler: $CXX"
echo "[build] Target:   Windows 11 x64"
echo ""

# Kompiliere Windows EXE
echo "[build] Compiling RAEL_V49.exe..."

$CXX $CXXFLAGS \
    "$ROOT/src/cli/main_windows.cpp" \
    -o "$OUT/RAEL_V49.exe" \
    $LDFLAGS

echo "[build] Output: $OUT/RAEL_V49.exe"
echo ""

# Zeige Dateiinfo
ls -lh "$OUT/RAEL_V49.exe"

echo ""
echo "═══════════════════════════════════════════════════════════════════════════"
echo "  BUILD ERFOLGREICH!"
echo "═══════════════════════════════════════════════════════════════════════════"
echo ""
echo "  Die EXE ist statisch gelinkt und benötigt keine DLLs."
echo "  Beim ersten Start wird die Software an die Hardware gebunden."
echo ""
