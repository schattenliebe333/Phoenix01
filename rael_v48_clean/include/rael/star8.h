#pragma once
#include "rael/lane_scheduler.h"
#include "rael/aether_ring.hpp"
#include <atomic>
#include <thread>
#include <vector>
#include <string>
#include <array>
#include <chrono>

namespace rael {

class RaelCore;

// ═══════════════════════════════════════════════════════════════════════════
// NODE MEMORY - 5D-Speicher für jeden Star8-Node
// ═══════════════════════════════════════════════════════════════════════════
// Jeder Node hat sein eigenes 5-Quint-Gedächtnis:
//   G1 Reflex   (36)  - Lokale Reaktionszeiten
//   G2 Instinct (48)  - Gelernte Muster/Patterns
//   G3 Emotion  (61)  - Node-Resonanz/Auslastung
//   G4 Ratio    (130) - Analytische Historie
//   G5 Spirit   (233) - Langfristige Trends
// ═══════════════════════════════════════════════════════════════════════════

struct NodeMemory {
    // 5-Quint Ring Buffers (identisch zu AetherBus)
    SelfComputingRingBuffer reflex{36};    // G1: Reaktionszeiten (ms)
    SelfComputingRingBuffer instinct{48};  // G2: Task-Komplexität
    SelfComputingRingBuffer emotion{61};   // G3: Erfolgsrate
    SelfComputingRingBuffer ratio{130};    // G4: Durchsatz
    SelfComputingRingBuffer spirit{233};   // G5: Langzeit-Performance

    // Zusätzliche Node-Statistiken
    std::atomic<uint64_t> tasks_completed{0};
    std::atomic<uint64_t> tasks_failed{0};
    std::atomic<uint64_t> total_processing_ns{0};
    std::atomic<double> coherence{0.0};

    // Zeitstempel
    std::chrono::steady_clock::time_point last_activity;

    // Berechne Gesamtenergie des Nodes (gewichtet nach 5-Quint)
    double total_energy() const {
        constexpr double G1 = 1.0 / 169.0;
        constexpr double G2 = 8.0 / 169.0;
        constexpr double G3 = 27.0 / 169.0;
        constexpr double G4 = 64.0 / 169.0;
        constexpr double G5 = 69.0 / 169.0;

        return G1 * reflex.mean() +
               G2 * instinct.mean() +
               G3 * emotion.mean() +
               G4 * ratio.mean() +
               G5 * spirit.mean();
    }

    // Berechne Node-Gesundheit [0..1]
    double health() const {
        if (tasks_completed.load() == 0) return 1.0;
        double success_rate = (double)tasks_completed.load() /
            (double)(tasks_completed.load() + tasks_failed.load());
        double trend = spirit.trend();
        // Gesundheit = Erfolgsrate + positiver Trend-Bonus
        return std::min(1.0, success_rate + (trend > 0 ? 0.1 : 0.0));
    }

    // Berechne durchschnittliche Reaktionszeit (ms)
    double avg_reaction_ms() const {
        return reflex.mean();
    }
};

// ═══════════════════════════════════════════════════════════════════════════
// STAR8 - 8-Node Worker Pool mit 5D-Speicher
// ═══════════════════════════════════════════════════════════════════════════

class Star8 {
public:
    explicit Star8(RaelCore& core);
    ~Star8();

    void start();
    void stop();

    bool submit(Lane lane, const std::string& payload);

    std::array<LaneStats,5> lane_stats() const { return sched_.lane_stats(); }
    std::array<NodeStats,8> node_stats() const { return sched_.node_stats(); }

    // NEU: Zugriff auf Node-Memory
    const NodeMemory& node_memory(size_t node_id) const {
        return node_memories_[node_id % 8];
    }
    const std::array<NodeMemory, 8>& all_node_memories() const {
        return node_memories_;
    }

    // NEU: Globale Star8-Statistiken
    double total_coherence() const;
    double total_energy() const;
    double average_health() const;

    void set_max_depth(size_t d) { sched_.set_max_depth(d); }
    void set_slow_threshold(size_t n) { sched_.set_slow_threshold(n); }

private:
    void worker(size_t node_id);

    RaelCore& core_;
    LaneScheduler sched_;
    std::atomic<bool> running_{false};
    std::vector<std::thread> threads_;

    // NEU: 5D-Speicher für jeden der 8 Nodes
    std::array<NodeMemory, 8> node_memories_;
};

} // namespace rael
