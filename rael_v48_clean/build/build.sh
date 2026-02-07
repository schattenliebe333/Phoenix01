#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
OUT="$ROOT/bin"
MODOUT="$ROOT/bin/modules"
mkdir -p "$OUT" "$MODOUT"

CXX="${CXX:-g++}"
# Security hardening flags (F-01 audit fix)
# -fstack-protector-strong: Stack buffer overflow protection
# -D_FORTIFY_SOURCE=2: Buffer overflow detection in string/memory functions
# -fPIE: Position Independent Executable (enables ASLR)
# -Wformat -Wformat-security: Format string vulnerability warnings
SECURITY_FLAGS="-fstack-protector-strong -D_FORTIFY_SOURCE=2 -fPIE -Wformat -Wformat-security"
CXXFLAGS="-std=c++17 -O2 -Wall -Wextra -I$ROOT/include $SECURITY_FLAGS"
# -pie: Link as PIE (ASLR)
# -z relro: Partial RELRO (GOT protection)
# -z now: Full RELRO (immediate binding)
LDFLAGS="-pie -Wl,-z,relro,-z,now -lssl -lcrypto"

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
  "$ROOT/src/core/inner_eye.cpp"
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

# RAEL Chat CLI (interactive mode, like Claude Code)
echo "[build] RAEL Chat CLI -> $OUT/rael_chat"
CHAT_SOURCES=(
  "$ROOT/src/core/util.cpp"
  "$ROOT/src/core/telemetry.cpp"
  "$ROOT/src/core/metrics.cpp"
  "$ROOT/src/core/events.cpp"
  "$ROOT/src/core/improvements.cpp"
  "$ROOT/src/core/lane_scheduler.cpp"
  "$ROOT/src/core/star8.cpp"
  "$ROOT/src/core/ethics.cpp"
  "$ROOT/src/core/ichbin.cpp"
  "$ROOT/src/core/mathcore.cpp"
  "$ROOT/src/core/semantic.cpp"
  "$ROOT/src/core/resonance.cpp"
  "$ROOT/src/core/module_manager.cpp"
  "$ROOT/src/core/hotswap.cpp"
  "$ROOT/src/core/voice.cpp"
  "$ROOT/src/core/settings.cpp"
  "$ROOT/src/core/nl_router.cpp"
  "$ROOT/src/core/reflection_engine.cpp"
  "$ROOT/src/core/core_ring.cpp"
  "$ROOT/src/core/sha256.cpp"
  "$ROOT/src/core/inner_eye.cpp"
  "$ROOT/src/core/neural_memory.cpp"
  "$ROOT/src/core/knowledge_graph.cpp"
  "$ROOT/src/core/security.cpp"
  "$ROOT/src/core/observability.cpp"
  "$ROOT/src/core/raelcore.cpp"
  "$ROOT/src/cli/rael_chat.cpp"
)
if [[ -f "$ROOT/src/cli/rael_chat.cpp" ]]; then
    $CXX $CXXFLAGS "${CHAT_SOURCES[@]}" -o "$OUT/rael_chat" -lpthread -ldl 2>/dev/null || \
    $CXX $CXXFLAGS "${CHAT_SOURCES[@]}" -o "$OUT/rael_chat" -lpthread
    echo "[build] RAEL Chat CLI compiled successfully"
fi

# RAEL UNIFIED (All-in-One: Chat + CLI + V50 + Security)
echo "[build] RAEL UNIFIED -> $OUT/rael_unified"
UNIFIED_SOURCES=(
  "$ROOT/src/core/util.cpp"
  "$ROOT/src/core/telemetry.cpp"
  "$ROOT/src/core/metrics.cpp"
  "$ROOT/src/core/events.cpp"
  "$ROOT/src/core/improvements.cpp"
  "$ROOT/src/core/lane_scheduler.cpp"
  "$ROOT/src/core/star8.cpp"
  "$ROOT/src/core/ethics.cpp"
  "$ROOT/src/core/ichbin.cpp"
  "$ROOT/src/core/mathcore.cpp"
  "$ROOT/src/core/semantic.cpp"
  "$ROOT/src/core/resonance.cpp"
  "$ROOT/src/core/module_manager.cpp"
  "$ROOT/src/core/hotswap.cpp"
  "$ROOT/src/core/voice.cpp"
  "$ROOT/src/core/settings.cpp"
  "$ROOT/src/core/nl_router.cpp"
  "$ROOT/src/core/reflection_engine.cpp"
  "$ROOT/src/core/core_ring.cpp"
  "$ROOT/src/core/sha256.cpp"
  "$ROOT/src/core/inner_eye.cpp"
  "$ROOT/src/core/neural_memory.cpp"
  "$ROOT/src/core/knowledge_graph.cpp"
  "$ROOT/src/core/security.cpp"
  "$ROOT/src/core/observability.cpp"
  "$ROOT/src/core/raelcore.cpp"
  "$ROOT/src/cli/rael_unified.cpp"
)
if [[ -f "$ROOT/src/cli/rael_unified.cpp" ]]; then
    $CXX $CXXFLAGS "${UNIFIED_SOURCES[@]}" -o "$OUT/rael_unified" -lpthread -ldl 2>/dev/null || \
    $CXX $CXXFLAGS "${UNIFIED_SOURCES[@]}" -o "$OUT/rael_unified" -lpthread
    echo "[build] RAEL UNIFIED compiled successfully"
fi

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

# RST Live Monitor (Real-Time Security with Entropy/Coherence Analysis)
echo "[build] RST Live Monitor -> $OUT/rael_monitor"
if [[ -f "$ROOT/src/security/rael_live_monitor.cpp" ]]; then
    $CXX $CXXFLAGS "$ROOT/src/security/rael_live_monitor.cpp" -o "$OUT/rael_monitor" -pthread
    echo "[build] RST Live Monitor compiled successfully"
fi

# RAEL Security Dashboard (Unified GUI - All Scanners)
echo "[build] Security Dashboard -> $OUT/rael_dashboard"
if [[ -f "$ROOT/src/security/rael_dashboard.cpp" ]]; then
    $CXX $CXXFLAGS "$ROOT/src/security/rael_dashboard.cpp" -o "$OUT/rael_dashboard" -pthread
    echo "[build] Security Dashboard compiled successfully"
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

    # Windows RST Live Monitor EXE
    echo "[build] Windows Live Monitor EXE -> $OUT/windows/rael_monitor.exe"
    if [[ -f "$ROOT/src/security/rael_live_monitor.cpp" ]]; then
        $WIN_CXX -std=c++17 -O2 -Wall -Wextra -I"$ROOT/include" -static \
            "$ROOT/src/security/rael_live_monitor.cpp" \
            -o "$OUT/windows/rael_monitor.exe" -liphlpapi -lpsapi
        echo "[build] Windows Live Monitor EXE compiled successfully"
    fi

    # Windows Security Dashboard EXE
    echo "[build] Windows Security Dashboard EXE -> $OUT/windows/rael_dashboard.exe"
    if [[ -f "$ROOT/src/security/rael_dashboard.cpp" ]]; then
        $WIN_CXX -std=c++17 -O2 -Wall -Wextra -I"$ROOT/include" -static \
            "$ROOT/src/security/rael_dashboard.cpp" \
            -o "$OUT/windows/rael_dashboard.exe" -liphlpapi -lpsapi
        echo "[build] Windows Security Dashboard EXE compiled successfully"
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

# V56.3 Chronos-Gitter (97 Kristalle, 160 Sterne, 61.440 Düsen)
echo "[build] V56.3 Chronos-Gitter -> $OUT/rael_v56_chronos"
if [[ -f "$ROOT/src/webgui/rael_v56_chronos.cpp" ]]; then
    $CXX $CXXFLAGS "$ROOT/src/webgui/rael_v56_chronos.cpp" -o "$OUT/rael_v56_chronos" -pthread
    echo "[build] V56.3 Chronos-Gitter ready: http://localhost:8080"
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
