#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
OUT="$ROOT/bin"
MODOUT="$ROOT/bin/modules"
mkdir -p "$OUT" "$MODOUT"

CXX="${CXX:-g++}"
CXXFLAGS="-std=c++17 -O2 -Wall -Wextra -I$ROOT/include"
LDFLAGS="-lssl -lcrypto"

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
  "$ROOT/src/core/control_star.cpp"
  "$ROOT/src/core/depth_scaling.cpp"
  "$ROOT/src/core/voicepack.cpp"
  "$ROOT/src/core/code_review.cpp"
  "$ROOT/src/core/filesystem.cpp"
  "$ROOT/src/core/parser.cpp"
  "$ROOT/src/core/executor.cpp"
  "$ROOT/src/core/git_integration.cpp"
  "$ROOT/src/core/project_engine.cpp"
  "$ROOT/src/core/codegen.cpp"
  "$ROOT/src/core/debug_engine.cpp"
  "$ROOT/src/core/lsp_server.cpp"
  "$ROOT/src/core/llm_runtime.cpp"
  "$ROOT/src/core/mesh_network.cpp"
  "$ROOT/src/core/neural_memory.cpp"
  "$ROOT/src/core/swarm_orchestrator.cpp"
  "$ROOT/src/core/api_server.cpp"
  "$ROOT/src/core/vector_store.cpp"
  "$ROOT/src/core/knowledge_graph.cpp"
  "$ROOT/src/core/nl_shell.cpp"
  "$ROOT/src/core/message_queue.cpp"
  "$ROOT/src/core/agent_marketplace.cpp"
  "$ROOT/src/core/security.cpp"
  "$ROOT/src/core/ml_framework.cpp"
  "$ROOT/src/core/plugin_sdk.cpp"
  "$ROOT/src/core/observability.cpp"
  "$ROOT/src/core/distributed_task.cpp"
  "$ROOT/src/core/aether_archive.cpp"
  "$ROOT/src/core/meta_star_orchestrator.cpp"
  "$ROOT/src/cli/main.cpp"
)

echo "[build] core -> $OUT/rael"
$CXX $CXXFLAGS "${SOURCES[@]}" -o "$OUT/rael" $LDFLAGS

echo "[build] modules (shared libs) -> $MODOUT"
$CXX $CXXFLAGS -fPIC -shared "$ROOT/modules/sample_semantic_quint/sem_quint.cpp" -o "$MODOUT/libsem_quint.so"
$CXX $CXXFLAGS -fPIC -shared "$ROOT/modules/sample_math_formulas/math_pack.cpp" -o "$MODOUT/libmath_pack.so"

# V50 Ultimate (Pure C++)
echo "[build] V50 Ultimate -> $OUT/rael_v50"
mkdir -p "$ROOT/src/v50"
if [[ -f "$ROOT/src/v50/rael_v50_main.cpp" ]]; then
    $CXX $CXXFLAGS "$ROOT/src/v50/rael_v50_main.cpp" -o "$OUT/rael_v50"
    echo "[build] V50 Ultimate compiled successfully"
fi

# Security Daemon (Attack → Defense Conversion)
echo "[build] Security Daemon -> $OUT/rael_security"
mkdir -p "$ROOT/src/security"
if [[ -f "$ROOT/src/security/rael_security_daemon.cpp" ]]; then
    $CXX $CXXFLAGS "$ROOT/src/security/rael_security_daemon.cpp" -o "$OUT/rael_security" -pthread
    echo "[build] Security Daemon compiled successfully"
fi

# Windows 11 EXE (Cross-Compilation)
WIN_CXX="${WIN_CXX:-x86_64-w64-mingw32-g++}"
if command -v "$WIN_CXX" &> /dev/null; then
    echo "[build] Windows 11 EXE -> $OUT/windows/rael_v50.exe"
    mkdir -p "$OUT/windows"
    if [[ -f "$ROOT/src/windows/rael_v50_windows.cpp" ]]; then
        $WIN_CXX -std=c++17 -O2 -Wall -Wextra -I"$ROOT/include" -static \
            "$ROOT/src/windows/rael_v50_windows.cpp" \
            -o "$OUT/windows/rael_v50.exe"
        echo "[build] Windows EXE compiled successfully"
    fi

    # Windows Security EXE
    echo "[build] Windows Security EXE -> $OUT/windows/rael_security.exe"
    if [[ -f "$ROOT/src/security/rael_security_daemon.cpp" ]]; then
        $WIN_CXX -std=c++17 -O2 -Wall -Wextra -I"$ROOT/include" -static \
            "$ROOT/src/security/rael_security_daemon.cpp" \
            -o "$OUT/windows/rael_security.exe" -liphlpapi -lpsapi
        echo "[build] Windows Security EXE compiled successfully"
    fi
else
    echo "[build] mingw-w64 not found, skipping Windows build"
fi

# V56.2 Liquid-Blade WebGUI
echo "[build] V56.2 Liquid-Blade WebGUI -> $OUT/rael_v56"
if [[ -f "$ROOT/src/webgui/rael_v56_liquid_blade.cpp" ]]; then
    $CXX $CXXFLAGS "$ROOT/src/webgui/rael_v56_liquid_blade.cpp" -o "$OUT/rael_v56" -pthread
    echo "[build] V56.2 Liquid-Blade ready: http://localhost:8080"
fi

# Legacy WebGUI
echo "[build] WebGUI -> $OUT/rael_webgui"
if [[ -f "$ROOT/src/webgui/rael_webgui_standalone.cpp" ]]; then
    $CXX $CXXFLAGS "$ROOT/src/webgui/rael_webgui_standalone.cpp" -o "$OUT/rael_webgui" -pthread
fi

# Windows V56.2 Cross-Compilation
if command -v "$WIN_CXX" &> /dev/null; then
    echo "[build] Windows V56.2 -> $OUT/windows/rael_v56.exe"
    mkdir -p "$OUT/windows"
    if [[ -f "$ROOT/src/webgui/rael_v56_liquid_blade.cpp" ]]; then
        $WIN_CXX -std=c++17 -O2 -Wall -Wextra -I"$ROOT/include" -static \
            "$ROOT/src/webgui/rael_v56_liquid_blade.cpp" \
            -o "$OUT/windows/rael_v56.exe" -lws2_32
        echo "[build] Windows V56.2 EXE compiled successfully"
    fi
fi

echo "[build] done."
