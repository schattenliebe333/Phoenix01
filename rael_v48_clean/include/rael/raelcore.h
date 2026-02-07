#pragma once
#include <string>
#include <array>
#include <memory>

#include "rael/semantic.h"
#include "rael/resonance.h"
#include "rael/attachments.h"
#include "rael/module_manager.h"
#include "rael/lane_scheduler.h"
#include "rael/star8.h"
#include "rael/star8_themes.h"
#include "rael/core_ring.h"
#include "rael/pack.h"
#include "rael/aether_bus.hpp"
#include "rael/gate53_labyrinth.hpp"
#include "rael/voice.h"
// V49 QUINT System
#include "rael/rael_quint.hpp"
// V49 Observer & Manifestation Engine
#include "rael/aeye_observer.hpp"
#include "rael/jet_nozzle.hpp"
// Bisher abgeklemmte Kerne - jetzt verdrahtet
#include "rael/inner_eye.hpp"
#include "rael/neural_memory.h"
#include "rael/knowledge_graph.h"
#include "rael/security.h"
#include "rael/observability.h"

namespace rael {

// ═══════════════════════════════════════════════════════════════════════════
// RAELCORE - Haupt-Orchestrator mit allen Kernen
// ═══════════════════════════════════════════════════════════════════════════
//
// Steht ueber allem. Versteht. Reflektiert. Verbessert.
// Alle 9 Themen-Nodes haben Zugriff auf ihre spezifischen Kerne.
//
// ═══════════════════════════════════════════════════════════════════════════

class RaelCore {
public:
    RaelCore();
    ~RaelCore();

    // Haupt-Pipeline (synchron, CLI)
    std::string process(const std::string& input);
    void speak(const std::string& text);

    // Themen-spezifische Verarbeitung (von Star8 Workern aufgerufen)
    std::string process_themed(NodeTheme theme, const std::string& payload);

    // Legacy: Generische Payload-Verarbeitung (fuer Abwaertskompatibilitaet)
    std::string process_payload(const std::string& payload);

    // ─── Subsystem-Zugriff ───────────────────────────────────────────────

    AttachmentStore& attachments() { return store; }
    ModuleManager& modules() { return mm; }

    AetherBus& aether() { return aether_; }
    const AetherBus& aether() const { return aether_; }

    Gate53Labyrinth& labyrinth() { return labyrinth_; }
    const Gate53Labyrinth& labyrinth() const { return labyrinth_; }

    // V49 QUINT System
    quint::RaelQuintSystem& quint() { return quint_; }
    const quint::RaelQuintSystem& quint() const { return quint_; }
    std::string quint_status() const { return quint_.status_string(); }

    // V49 AEYE Observer (das alles sehende Auge)
    aeye::AeyeObserver& observer() { return observer_; }
    const aeye::AeyeObserver& observer() const { return observer_; }
    aeye::GlobalState observe_reality() { return observer_.state; }

    // V49 JET Engine (61.440 Duesen)
    jet::JetEngine& jet() { return jet_; }
    const jet::JetEngine& jet() const { return jet_; }
    jet::ManifestResult manifest() { return jet_.manifest(); }

    // N8: Inneres Auge (Cyber-Verteidigung)
    inner_eye::InnerEye& eye() { return eye_; }
    const inner_eye::InnerEye& eye() const { return eye_; }

    // Star8 Themen-Stern
    bool enqueue(Lane lane, const std::string& payload);
    bool enqueue_themed(TaskDomain domain, Lane lane, const std::string& payload);
    std::array<LaneStats,5> lane_stats() const;
    std::array<NodeStats,STAR_NODE_COUNT> node_stats() const;

    const CoreRing& core() const { return core_; }

private:
    // ─── Immutable Kerne ─────────────────────────────────────────────────
    CoreRing core_;

    // ─── Themen-Stern ────────────────────────────────────────────────────
    std::unique_ptr<Star8> star8_;

    // ─── N0: SPRACHE Kerne ───────────────────────────────────────────────
    SemanticEngine sem;
    ResonanceEngine res;

    // ─── N1: GEDAECHTNIS Kerne ───────────────────────────────────────────
    // (Singletons - Zugriff ueber ::instance())

    // ─── N2: SICHERHEIT Kerne ────────────────────────────────────────────
    Gate53Labyrinth labyrinth_;

    // ─── N7: MANIFESTATION Kerne ─────────────────────────────────────────
    ModuleManager mm;
    AttachmentStore store;
    std::unique_ptr<VoiceTTS> voice_;

    // ─── N8: INNERES AUGE ────────────────────────────────────────────────
    inner_eye::InnerEye eye_;

    // ─── Resonanz-Backbone ───────────────────────────────────────────────
    AetherBus aether_;

    // ─── V49 Systeme ─────────────────────────────────────────────────────
    quint::RaelQuintSystem quint_;
    aeye::AeyeObserver observer_;
    jet::JetEngine jet_;

    // ─── Auto-Improvement Zaehler ────────────────────────────────────────
    uint64_t weak_res_count_ = 0;
    uint64_t ethics_block_count_ = 0;
    uint64_t last_imp_emit_ops_ = 0;

    // ─── Themen-Prozessoren (intern) ─────────────────────────────────────
    std::string process_sprache(const std::string& payload);
    std::string process_gedaechtnis(const std::string& payload);
    std::string process_sicherheit(const std::string& payload);
    std::string process_reflexion(const std::string& payload);
    std::string process_mathematik(const std::string& payload);
    std::string process_beobachtung(const std::string& payload);
    std::string process_lernen(const std::string& payload);
    std::string process_manifestation(const std::string& payload);
    std::string process_inneres_auge(const std::string& payload);
};

} // namespace rael
