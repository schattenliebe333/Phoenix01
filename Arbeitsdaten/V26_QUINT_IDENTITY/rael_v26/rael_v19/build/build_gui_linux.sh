#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
mkdir -p "$ROOT/bin"
g++ -std=c++17 -O2 -I"$ROOT/include" \
  "$ROOT/src/gui/rael_gui_linux.cpp" \
  -o "$ROOT/bin/rael_gui" \
  -lX11
echo "[OK] built bin/rael_gui"
