/**
 * @file rael_all.hpp
 * @brief RAEL V48 SANG-REAL - Master Include File
 *
 * Inkludiert alle RAEL-Komponenten in einem einzigen Header.
 * Verwendung: #include <rael/rael_all.hpp>
 *
 * @author RAEL Team
 * @version V48.0 SANG-REAL
 */

#ifndef RAEL_ALL_HPP
#define RAEL_ALL_HPP

// ═══════════════════════════════════════════════════════════════════════════
// RST KONSTANTEN & FORMELN (Realitäts-Schwingungs-Theorie)
// ═══════════════════════════════════════════════════════════════════════════
#include "rael/rst_constants.hpp"
#include "rael/rst_formulas.hpp"
#include "rael/rst_optimized_runtime.hpp"
#include "rael/rst_ultimate_formulas.hpp"
#include "rael/RAEL_FRAC_ARITHMETIC.hpp"
#include "rael/RST_Advanced.hpp"
#include "rael/RAEL_OMEGA_FORMULAS.hpp"

// ═══════════════════════════════════════════════════════════════════════════
// CORE SYSTEM
// ═══════════════════════════════════════════════════════════════════════════
#include "rael/version.h"
#include "rael/constants.h"
#include "rael/util.h"
#include "rael/events.h"
#include "rael/metrics.h"
#include "rael/telemetry.h"
#include "rael/settings.h"
#include "rael/improvements.h"

// ═══════════════════════════════════════════════════════════════════════════
// IMMUTABLE CORES (Unveränderliche Kerne)
// ═══════════════════════════════════════════════════════════════════════════
#include "rael/ichbin.h"          // Identität/Bewusstsein
#include "rael/ethics.h"          // Ethik-Gate
#include "rael/mathcore.h"        // Mathematischer Kern

// ═══════════════════════════════════════════════════════════════════════════
// STAR8 WORKER SYSTEM
// ═══════════════════════════════════════════════════════════════════════════
#include "rael/star8.h"
#include "rael/star8_quint.hpp"
#include "rael/lane_scheduler.h"
#include "rael/control_star.h"
#include "rael/meta_star_orchestrator.hpp"

// ═══════════════════════════════════════════════════════════════════════════
// QUINT MEMORY SYSTEM (5D-Speicher)
// ═══════════════════════════════════════════════════════════════════════════
#include "rael/quint_memory.hpp"
#include "rael/rael_quint.hpp"
#include "rael/quint_integration_enhanced.hpp"
#include "rael/highway_quint.hpp"

// ═══════════════════════════════════════════════════════════════════════════
// GATE53 LABYRINTH (Sicherheitssystem)
// ═══════════════════════════════════════════════════════════════════════════
#include "rael/gate53_labyrinth.hpp"
#include "rael/gate53_simulator.hpp"
#include "rael/security.h"
#include "rael/security_core.hpp"
#include "rael/sha256.h"
#include "rael/core_ring.h"

// ═══════════════════════════════════════════════════════════════════════════
// RESONANZ & SEMANTIK
// ═══════════════════════════════════════════════════════════════════════════
#include "rael/resonance.h"
#include "rael/resonance_matrix.hpp"
#include "rael/rael_master_resonance.hpp"
#include "rael/semantic.h"
#include "rael/rst_semantic_engine.hpp"

// ═══════════════════════════════════════════════════════════════════════════
// ANKER & SCHILD SYSTEM
// ═══════════════════════════════════════════════════════════════════════════
#include "rael/A_Anker.hpp"
#include "rael/A_ANKER_ENHANCED.hpp"
#include "rael/S_Schild.hpp"
#include "rael/S_SCHILD_ENHANCED.hpp"
#include "rael/Psi_Attention.hpp"

// ═══════════════════════════════════════════════════════════════════════════
// AETHER SYSTEM
// ═══════════════════════════════════════════════════════════════════════════
#include "rael/aether_bus.hpp"
#include "rael/aether_ring.hpp"
#include "rael/aether_archive.h"

// ═══════════════════════════════════════════════════════════════════════════
// MODULE SYSTEM
// ═══════════════════════════════════════════════════════════════════════════
#include "rael/module_api.h"
#include "rael/module_manager.h"
#include "rael/hotswap.h"
#include "rael/plugin_sdk.h"

// ═══════════════════════════════════════════════════════════════════════════
// NAVIGATOR & ROUTING
// ═══════════════════════════════════════════════════════════════════════════
#include "rael/RAEL_NAVIGATOR_CORE.hpp"
#include "rael/RAEL_ERB_TOPOLOGY.hpp"
#include "rael/nl_router.h"
#include "rael/reflection_engine.h"

// ═══════════════════════════════════════════════════════════════════════════
// LLM & ML INTEGRATION
// ═══════════════════════════════════════════════════════════════════════════
#include "rael/RAEL_LLM_FORMULA_ENGINE.hpp"
#include "rael/llm_runtime.h"
#include "rael/ml_framework.h"
#include "rael/neural_memory.h"
#include "rael/knowledge_graph.h"
#include "rael/vector_store.h"

// ═══════════════════════════════════════════════════════════════════════════
// GRID & SMOOTHING
// ═══════════════════════════════════════════════════════════════════════════
#include "rael/grid_smoothing.hpp"
#include "rael/depth_scaling.h"

// ═══════════════════════════════════════════════════════════════════════════
// DISTRIBUTED & NETWORK
// ═══════════════════════════════════════════════════════════════════════════
#include "rael/distributed_task.h"
#include "rael/mesh_network.h"
#include "rael/swarm_orchestrator.h"
#include "rael/message_queue.h"

// ═══════════════════════════════════════════════════════════════════════════
// DEVELOPMENT TOOLS
// ═══════════════════════════════════════════════════════════════════════════
#include "rael/executor.h"
#include "rael/parser.h"
#include "rael/codegen.h"
#include "rael/code_review.h"
#include "rael/debug_engine.h"
#include "rael/lsp_server.h"
#include "rael/git_integration.h"
#include "rael/project_engine.h"
#include "rael/nl_shell.h"

// ═══════════════════════════════════════════════════════════════════════════
// FILE & SYSTEM
// ═══════════════════════════════════════════════════════════════════════════
#include "rael/filesystem.h"
#include "rael/pack.h"
#include "rael/attachments.h"
#include "rael/machine_binding.h"
#include "rael/observability.h"

// ═══════════════════════════════════════════════════════════════════════════
// VOICE & MEDIA
// ═══════════════════════════════════════════════════════════════════════════
#include "rael/voice.h"
#include "rael/voicepack.h"

// ═══════════════════════════════════════════════════════════════════════════
// SIMULATION & SHADOW
// ═══════════════════════════════════════════════════════════════════════════
#include "rael/shadow_sim.h"
#include "rael/sang_real.h"

// ═══════════════════════════════════════════════════════════════════════════
// V49/V50 EXTENSIONS
// ═══════════════════════════════════════════════════════════════════════════
#include "rael/rael_v50_ultimate.hpp"

// ═══════════════════════════════════════════════════════════════════════════
// API & MARKETPLACE
// ═══════════════════════════════════════════════════════════════════════════
#include "rael/api_server.h"
#include "rael/agent_marketplace.h"

// ═══════════════════════════════════════════════════════════════════════════
// MAIN ORCHESTRATOR
// ═══════════════════════════════════════════════════════════════════════════
#include "rael/raelcore.h"

#endif // RAEL_ALL_HPP
