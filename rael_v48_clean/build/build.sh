#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
OUT="$ROOT/bin"
MODOUT="$ROOT/bin/modules"
mkdir -p "$OUT" "$MODOUT"

CXX="${CXX:-g++}"
CXXFLAGS="-std=c++17 -O2 -Wall -Wextra -I$ROOT/include"
LDFLAGS=""

SOURCES=(
  "$ROOT/src/core/util.cpp"
  "$ROOT/src/core/telemetry.cpp"
  "$ROOT/src/core/metrics.cpp"
  "$ROOT/src/core/events.cpp"
  "$ROOT/src/core/improvements.cpp"
  "$ROOT/src/core/shadow_sim.cpp"
  "$ROOT/src/core/lane_scheduler.cpp"
  "$ROOT/src/core/star8.cpp"
  "$ROOT/src/core/sha256.cpp"
  "$ROOT/src/core/core_ring.cpp"
  "$ROOT/src/core/pack.cpp"
  "$ROOT/src/core/settings.cpp"
  "$ROOT/src/core/ethics.cpp"
  "$ROOT/src/core/ichbin.cpp"
  "$ROOT/src/core/mathcore.cpp"
  "$ROOT/src/core/semantic.cpp"
  "$ROOT/src/core/nl_router.cpp"
  "$ROOT/src/core/resonance.cpp"
  "$ROOT/src/core/reflection_engine.cpp"
  "$ROOT/src/core/attachments.cpp"
  "$ROOT/src/core/module_manager.cpp"
  "$ROOT/src/core/hotswap.cpp"
  "$ROOT/src/core/voice.cpp"
  "$ROOT/src/core/raelcore.cpp"
  "$ROOT/src/cli/main.cpp"
)

echo "[build] core -> $OUT/rael"
$CXX $CXXFLAGS "${SOURCES[@]}" -o "$OUT/rael" $LDFLAGS

echo "[build] modules (shared libs) -> $MODOUT"
$CXX $CXXFLAGS -fPIC -shared "$ROOT/modules/sample_semantic_quint/sem_quint.cpp" -o "$MODOUT/libsem_quint.so"
$CXX $CXXFLAGS -fPIC -shared "$ROOT/modules/sample_math_formulas/math_pack.cpp" -o "$MODOUT/libmath_pack.so"
echo "[build] done."
