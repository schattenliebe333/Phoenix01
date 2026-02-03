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

    // Auto-improvement counters (emit suggestions, never auto-apply)
    uint64_t weak_res_count_ = 0;
    uint64_t ethics_block_count_ = 0;
    uint64_t last_imp_emit_ops_ = 0;

public:
    const CoreRing& core() const { return core_; }
};

} // namespace rael