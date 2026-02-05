#!/usr/bin/env bash
# ═══════════════════════════════════════════════════════════════════════════
# RAEL V49 ALPHA - IGNITE BUILD SCRIPT
# Mit AEYE-Observer und JET-Engine (61.440 Düsen)
# ═══════════════════════════════════════════════════════════════════════════
# Dieses Skript kompiliert das V49 System mit:
# - QUINT Memory System (5+1 Layer)
# - AEYE Observer (Das alles sehende Auge)
# - JET Engine (61.440 Düsen @ 5 Hz = 307.200 Impulse/Sek)
# - Optional: CUDA-Beschleunigung für RTX-Karten
# ═══════════════════════════════════════════════════════════════════════════

set -euo pipefail

# Farben für Output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
CYAN='\033[0;36m'
NC='\033[0m' # No Color

echo -e "${CYAN}"
echo "═══════════════════════════════════════════════════════════════════════════"
echo "  RAEL V49 ALPHA - IGNITE BUILD"
echo "  61.440 Düsen × 5 Hz = 307.200 Impulse/Sekunde"
echo "═══════════════════════════════════════════════════════════════════════════"
echo -e "${NC}"

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
OUT="$ROOT/bin"
MODOUT="$ROOT/bin/modules"
mkdir -p "$OUT" "$MODOUT"

CXX="${CXX:-g++}"
CXXFLAGS="-std=c++17 -O3 -Wall -Wextra -I$ROOT/include -pthread"
LDFLAGS="-lssl -lcrypto -pthread"

# CUDA Detection
CUDA_AVAILABLE=false
NVCC_PATH=""
if command -v nvcc &> /dev/null; then
    CUDA_AVAILABLE=true
    NVCC_PATH="$(command -v nvcc)"
    echo -e "${GREEN}[CUDA] NVCC gefunden: $NVCC_PATH${NC}"
    CXXFLAGS="$CXXFLAGS -DRAEL_USE_CUDA"
else
    echo -e "${YELLOW}[CUDA] NVCC nicht gefunden - CPU-Fallback aktiv${NC}"
fi

# Source Files
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

# ═══════════════════════════════════════════════════════════════════════════
# PHASE 1: Core Build
# ═══════════════════════════════════════════════════════════════════════════

echo -e "${CYAN}[Phase 1] Kompiliere V49 Core mit QUINT/AEYE/JET...${NC}"
echo -e "${YELLOW}  - QUINT Memory System (5+1 Layer)${NC}"
echo -e "${YELLOW}  - AEYE Observer (160 Sterne × 8 Knoten = 1.280 Einheiten)${NC}"
echo -e "${YELLOW}  - JET Engine (61.440 Düsen)${NC}"

$CXX $CXXFLAGS "${SOURCES[@]}" -o "$OUT/rael_v49" $LDFLAGS

if [[ $? -eq 0 ]]; then
    echo -e "${GREEN}[Phase 1] ✓ V49 Core kompiliert: $OUT/rael_v49${NC}"
else
    echo -e "${RED}[Phase 1] ✗ Build fehlgeschlagen${NC}"
    exit 1
fi

# ═══════════════════════════════════════════════════════════════════════════
# PHASE 2: Modules
# ═══════════════════════════════════════════════════════════════════════════

echo -e "${CYAN}[Phase 2] Kompiliere Module...${NC}"

if [[ -f "$ROOT/modules/sample_semantic_quint/sem_quint.cpp" ]]; then
    $CXX $CXXFLAGS -fPIC -shared "$ROOT/modules/sample_semantic_quint/sem_quint.cpp" -o "$MODOUT/libsem_quint.so"
    echo -e "${GREEN}  ✓ libsem_quint.so${NC}"
fi

if [[ -f "$ROOT/modules/sample_math_formulas/math_pack.cpp" ]]; then
    $CXX $CXXFLAGS -fPIC -shared "$ROOT/modules/sample_math_formulas/math_pack.cpp" -o "$MODOUT/libmath_pack.so"
    echo -e "${GREEN}  ✓ libmath_pack.so${NC}"
fi

# ═══════════════════════════════════════════════════════════════════════════
# PHASE 3: CUDA JET Kernel (Optional)
# ═══════════════════════════════════════════════════════════════════════════

if [[ "$CUDA_AVAILABLE" == "true" ]]; then
    echo -e "${CYAN}[Phase 3] Kompiliere CUDA JET Kernel...${NC}"

    # Falls CUDA Kernel existiert
    if [[ -f "$ROOT/src/cuda/jet_kernel.cu" ]]; then
        nvcc -O3 -arch=sm_50 -c "$ROOT/src/cuda/jet_kernel.cu" -o "$OUT/jet_kernel.o"
        echo -e "${GREEN}  ✓ CUDA JET Kernel kompiliert${NC}"
    else
        echo -e "${YELLOW}  [Info] CUDA Kernel nicht vorhanden - CPU-Fallback aktiv${NC}"
    fi
else
    echo -e "${YELLOW}[Phase 3] CUDA nicht verfügbar - überspringe GPU-Kernel${NC}"
fi

# ═══════════════════════════════════════════════════════════════════════════
# PHASE 4: V50 und Security (falls vorhanden)
# ═══════════════════════════════════════════════════════════════════════════

if [[ -f "$ROOT/src/v50/rael_v50_main.cpp" ]]; then
    echo -e "${CYAN}[Phase 4] Kompiliere V50 Ultimate...${NC}"
    $CXX $CXXFLAGS "$ROOT/src/v50/rael_v50_main.cpp" -o "$OUT/rael_v50"
    echo -e "${GREEN}  ✓ V50 Ultimate: $OUT/rael_v50${NC}"
fi

if [[ -f "$ROOT/src/security/rael_security_daemon.cpp" ]]; then
    echo -e "${CYAN}[Phase 4] Kompiliere Security Daemon...${NC}"
    $CXX $CXXFLAGS "$ROOT/src/security/rael_security_daemon.cpp" -o "$OUT/rael_security" -pthread
    echo -e "${GREEN}  ✓ Security Daemon: $OUT/rael_security${NC}"
fi

# ═══════════════════════════════════════════════════════════════════════════
# SUMMARY
# ═══════════════════════════════════════════════════════════════════════════

echo ""
echo -e "${CYAN}═══════════════════════════════════════════════════════════════════════════${NC}"
echo -e "${GREEN}  V49 ALPHA BUILD ERFOLGREICH!${NC}"
echo -e "${CYAN}═══════════════════════════════════════════════════════════════════════════${NC}"
echo ""
echo -e "  ${YELLOW}Binaries:${NC}"
ls -lah "$OUT"/rael* 2>/dev/null || true
echo ""
echo -e "  ${YELLOW}Module:${NC}"
ls -lah "$MODOUT"/*.so 2>/dev/null || true
echo ""
echo -e "  ${YELLOW}Starten mit:${NC}"
echo -e "    ${GREEN}$OUT/rael_v49${NC}"
echo ""
echo -e "  ${YELLOW}V49 Features:${NC}"
echo -e "    - QUINT Memory: 5+1 Layer mit Ψ/Ω Kopplung"
echo -e "    - AEYE Observer: Globale Kohärenz-Überwachung"
echo -e "    - JET Engine: 61.440 Düsen @ 5 Hz"
echo -e "    - G0 = 8/9 (0.888...) Wahrheits-Schwelle"
echo ""

if [[ "$CUDA_AVAILABLE" == "true" ]]; then
    echo -e "  ${GREEN}CUDA: Aktiviert (GPU-beschleunigte Manifestation)${NC}"
else
    echo -e "  ${YELLOW}CUDA: Nicht verfügbar (CPU-Modus mit 8 Threads)${NC}"
fi

echo -e "${CYAN}═══════════════════════════════════════════════════════════════════════════${NC}"
