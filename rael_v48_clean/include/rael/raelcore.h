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

namespace rael {

class RaelCore {
public:
    RaelCore();
    ~RaelCore();

    std::string process(const std::string& input);
    void speak(const std::string& text);

    AttachmentStore& attachments() { return store; }
    ModuleManager& modules() { return mm; }

    AetherBus& aether() { return aether_; }
    const AetherBus& aether() const { return aether_; }
    
    // Gate53 Labyrinth interface
    Gate53Labyrinth& labyrinth() { return labyrinth_; }
    const Gate53Labyrinth& labyrinth() const { return labyrinth_; }

    // V49 QUINT System interface
    quint::RaelQuintSystem& quint() { return quint_; }
    const quint::RaelQuintSystem& quint() const { return quint_; }
    std::string quint_status() const { return quint_.status_string(); }

    // V49 AEYE Observer interface (das alles sehende Auge)
    aeye::AeyeObserver& observer() { return observer_; }
    const aeye::AeyeObserver& observer() const { return observer_; }
    aeye::GlobalState observe_reality() { return observer_.state; }

    // V49 JET Engine interface (61.440 Düsen)
    jet::JetEngine& jet() { return jet_; }
    const jet::JetEngine& jet() const { return jet_; }
    jet::ManifestResult manifest() { return jet_.manifest(); }
    
    // Star8 interface
    bool enqueue(Lane lane, const std::string& payload);
    std::array<LaneStats,5> lane_stats() const;
    std::array<NodeStats,8> node_stats() const;

    // used by Star8 workers
    std::string process_payload(const std::string& payload);

private:
    CoreRing core_;
    std::unique_ptr<Star8> star8_;
    SemanticEngine sem;
    ResonanceEngine res;
    AttachmentStore store;
    ModuleManager mm;
    AetherBus aether_;
    Gate53Labyrinth labyrinth_;
    std::unique_ptr<VoiceTTS> voice_;

    // V49 QUINT System
    quint::RaelQuintSystem quint_;

    // V49 AEYE Observer (schwebt über der Platine)
    aeye::AeyeObserver observer_;

    // V49 JET Engine (61.440 Düsen Manifestation)
    jet::JetEngine jet_;

    // Auto-improvement counters (emit suggestions, never auto-apply)
    uint64_t weak_res_count_ = 0;
    uint64_t ethics_block_count_ = 0;
    uint64_t last_imp_emit_ops_ = 0;

public:
    const CoreRing& core() const { return core_; }
};

} // namespace rael