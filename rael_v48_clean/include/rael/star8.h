#pragma once
#include "rael/lane_scheduler.h"
#include "rael/star8_themes.h"
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
// NODE MEMORY - 5D-Speicher fuer jeden Star-Node
// ═══════════════════════════════════════════════════════════════════════════
// Jeder Node hat sein eigenes 5-Quint-Gedaechtnis:
//   G1 Reflex   (36)  - Lokale Reaktionszeiten
//   G2 Instinct (48)  - Gelernte Muster/Patterns
//   G3 Emotion  (61)  - Node-Resonanz/Auslastung
//   G4 Ratio    (130) - Analytische Historie
//   G5 Spirit   (233) - Langfristige Trends
// ═══════════════════════════════════════════════════════════════════════════

struct NodeMemory {
    // 5-Quint Ring Buffers (identisch zu AetherBus)
    SelfComputingRingBuffer reflex{36};    // G1: Reaktionszeiten (ms)
    SelfComputingRingBuffer instinct{48};  // G2: Task-Komplexitaet
    SelfComputingRingBuffer emotion{61};   // G3: Erfolgsrate
    SelfComputingRingBuffer ratio{130};    // G4: Durchsatz
    SelfComputingRingBuffer spirit{233};   // G5: Langzeit-Performance

    // Zusaetzliche Node-Statistiken
    std::atomic<uint64_t> tasks_completed{0};
    std::atomic<uint64_t> tasks_failed{0};
    std::atomic<uint64_t> total_processing_ns{0};
    std::atomic<double> coherence{0.0};

    // Thema des Nodes
    NodeTheme theme = NodeTheme::SPRACHE;

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
        return std::min(1.0, success_rate + (trend > 0 ? 0.1 : 0.0));
    }

    // Berechne durchschnittliche Reaktionszeit (ms)
    double avg_reaction_ms() const {
        return reflex.mean();
    }
};

// ═══════════════════════════════════════════════════════════════════════════
// STAR8 - Themen-Stern mit N spezialisierten Nodes
// ═══════════════════════════════════════════════════════════════════════════
//
// N0 SPRACHE       | N1 GEDAECHTNIS  | N2 SICHERHEIT
// N3 REFLEXION     | N4 MATHEMATIK   | N5 BEOBACHTUNG
// N6 LERNEN        | N7 MANIFESTATION| N8 INNERES_AUGE
//
// Jeder Node ist ein Themen-Kalkulator mit eigenen Kernen.
// Prozesse stehen sich nicht im Weg!
//
// ═══════════════════════════════════════════════════════════════════════════

class Star8 {
public:
    explicit Star8(RaelCore& core);
    ~Star8();

    void start();
    void stop();

    // Task einreihen (mit automatischer Themen-Erkennung)
    bool submit(Lane lane, const std::string& payload);

    // Task einreihen mit expliziter Domaene
    bool submit_themed(TaskDomain domain, Lane lane, const std::string& payload);

    std::array<LaneStats,5> lane_stats() const { return sched_.lane_stats(); }
    std::array<NodeStats,STAR_NODE_COUNT> node_stats() const { return sched_.node_stats(); }

    // Zugriff auf Node-Memory
    const NodeMemory& node_memory(size_t node_id) const {
        return node_memories_[node_id % STAR_NODE_COUNT];
    }
    const std::array<NodeMemory, STAR_NODE_COUNT>& all_node_memories() const {
        return node_memories_;
    }

    // Globale Stern-Statistiken
    double total_coherence() const;
    double total_energy() const;
    double average_health() const;

    // Node-Thema abfragen
    NodeTheme node_theme(size_t node_id) const {
        return node_memories_[node_id % STAR_NODE_COUNT].theme;
    }

    // Anzahl aktiver Nodes
    size_t node_count() const { return STAR_NODE_COUNT; }

    void set_max_depth(size_t d) { sched_.set_max_depth(d); }
    void set_slow_threshold(size_t n) { sched_.set_slow_threshold(n); }

private:
    void worker(size_t node_id);

    RaelCore& core_;
    LaneScheduler sched_;
    std::atomic<bool> running_{false};
    std::vector<std::thread> threads_;

    // 5D-Speicher fuer jeden der STAR_NODE_COUNT Nodes
    std::array<NodeMemory, STAR_NODE_COUNT> node_memories_;
};

} // namespace rael
