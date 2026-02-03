#pragma once
#include <array>
#include "rael/settings.h"
#include "rael/rst_constants.hpp"
#include <cstdint>
#include <deque>
#include <mutex>
#include <string>
#include <cmath>

namespace rael {

// ═══════════════════════════════════════════════════════════════════════════
// LANE-DEFINITION (Frequenzbänder)
// ═══════════════════════════════════════════════════════════════════════════
// 
// Lane 0 (L1):    0-143 Hz  → Materie/Reflex
// Lane 1 (L2):  144-287 Hz  → Emotion
// Lane 2 (L3):  288-431 Hz  → Ratio
// Lane 3 (L4):  432-575 Hz  → Intuition
// Lane 4 (L5):  576-720 Hz  → Spirit
//
// ═══════════════════════════════════════════════════════════════════════════

enum class Lane : uint8_t { L1=0, L2=1, L3=2, L4=3, L5=4 };

// Lane-Frequenz-Bänder
constexpr double LANE_FREQ_MIN[] = {0.0, 144.0, 288.0, 432.0, 576.0};
constexpr double LANE_FREQ_MAX[] = {143.0, 287.0, 431.0, 575.0, 720.0};
constexpr double LANE_CENTER[] = {72.0, 216.0, 360.0, 504.0, 648.0};

// Lane-Namen
constexpr const char* LANE_NAMES[] = {"Materie", "Emotion", "Ratio", "Intuition", "Spirit"};

struct LaneStats {
    uint64_t enqueued = 0;
    uint64_t dequeued = 0;
    uint64_t dropped  = 0;
    size_t depth = 0;
    
    // RST-Erweiterung
    double total_energy = 0.0;
    double frequency_drift = 0.0;
    uint64_t corrected = 0;
};

struct NodeStats {
    uint64_t taken = 0;
    uint64_t done  = 0;
    Lane last_lane = Lane::L1;
    
    // RST-Erweiterung
    double processing_power = 1.0;
    double resonance = 1.0;
};

struct Task {
    uint64_t id = 0;
    Lane lane = Lane::L1;
    bool slow = false;
    std::string payload;
    
    // RST-Erweiterung
    double frequency = 72.0;     // Aktuelle Frequenz
    double energy = 1.0;         // Energie-Inhalt
    double coherence = 1.0;      // Kohärenz (0-1)
    double signature = 0.0;      // Optional: 88er-Signatur
};

// ═══════════════════════════════════════════════════════════════════════════
// BRL-PHYSIK (Buffer-Router-Lane)
// ═══════════════════════════════════════════════════════════════════════════

class BRLPhysics {
public:
    // #44: Doppelkolben-Pumpfaktor
    static double pump_effizienz(double taktfrequenz) {
        return rst::brl_pump_effizienz(taktfrequenz);
    }
    
    // #45: Frequenz-Korrektur (Gegen-Frequenz-Vorhang)
    static double frequenz_korrektur(double f_ist, double f_ziel) {
        return rst::lane_frequenz_korrektur(f_ist, f_ziel);
    }
    
    // #46: Lane-Zuordnung basierend auf Frequenz
    static Lane frequenz_zu_lane(double frequenz) {
        int idx = rst::lane_index(frequenz);
        return static_cast<Lane>(idx);
    }
    
    // Lane-Zentralfrequenz
    static double lane_zentrum(Lane l) {
        return LANE_CENTER[static_cast<int>(l)];
    }
    
    // Frequenz-Drift berechnen
    static double drift(double f_ist, Lane l) {
        double f_zentrum = lane_zentrum(l);
        return std::abs(f_ist - f_zentrum);
    }
    
    // Korrigierte Frequenz
    static double korrigiere(double f_ist, Lane l) {
        double f_zentrum = lane_zentrum(l);
        return frequenz_korrektur(f_ist, f_zentrum);
    }
};

class LaneScheduler {
public:
    bool enqueue(Task t);
    bool try_dequeue(Task& out);

    std::array<LaneStats,5> lane_stats() const;
    std::array<NodeStats,8> node_stats() const;

    void mark_taken(size_t node_id, Lane lane);
    void mark_done(size_t node_id);

    void set_max_depth(size_t per_lane);
    void set_slow_threshold(size_t n);
    
    // ─────────────────────────────────────────────────────────────────────────
    // RST-ERWEITERUNGEN
    // ─────────────────────────────────────────────────────────────────────────
    
    // Frequenz-basiertes Enqueue
    bool enqueue_by_frequency(Task t) {
        // Lane automatisch nach Frequenz zuweisen
        t.lane = BRLPhysics::frequenz_zu_lane(t.frequency);
        
        // Frequenz korrigieren
        double f_korr = BRLPhysics::korrigiere(t.frequency, t.lane);
        double drift = std::abs(t.frequency - f_korr);
        
        std::lock_guard<std::mutex> lock(mtx_);
        ls_[static_cast<int>(t.lane)].frequency_drift += drift;
        if (drift > 1.0) {
            ls_[static_cast<int>(t.lane)].corrected++;
            t.frequency = f_korr;
        }
        
        return enqueue_internal(t);
    }
    
    // SIMD-Batch-Verarbeitung (#48)
    template<size_t N>
    std::array<bool, N> enqueue_batch(std::array<Task, N>& tasks) {
        std::array<bool, N> results;
        std::lock_guard<std::mutex> lock(mtx_);
        
        for (size_t i = 0; i < N; ++i) {
            tasks[i].lane = BRLPhysics::frequenz_zu_lane(tasks[i].frequency);
            results[i] = enqueue_internal(tasks[i]);
        }
        
        return results;
    }
    
    // Verdichtungs-Statistik (#47)
    double verdichtungs_faktor() const {
        std::lock_guard<std::mutex> lock(mtx_);
        uint64_t total = 0;
        for (const auto& ls : ls_) total += ls.enqueued;
        if (total == 0) return 0.0;
        return rst::VERDICHTUNGS_FAKTOR * (double)total / max_depth_;
    }
    
    // Theoretischer Speedup (#49)
    double theoretischer_speedup() const {
        return rst::S_MAX_DUESE;
    }

private:
    mutable std::mutex mtx_;
    std::array<std::deque<Task>,5> q_;
    std::array<LaneStats,5> ls_{};
    std::array<NodeStats,8> ns_{};

    size_t max_depth_ = 1024;
    size_t slow_threshold_ = 512;
    uint64_t next_id_ = 0;
    uint8_t rr_ = 0;

    bool pop_lane(Lane l, Task& out);
    
    bool enqueue_internal(Task& t) {
        int lane_idx = static_cast<int>(t.lane);
        if (q_[lane_idx].size() >= max_depth_) {
            ls_[lane_idx].dropped++;
            return false;
        }
        t.id = next_id_++;
        q_[lane_idx].push_back(t);
        ls_[lane_idx].enqueued++;
        ls_[lane_idx].depth = q_[lane_idx].size();
        ls_[lane_idx].total_energy += t.energy;
        return true;
    }
};

const char* lane_name(Lane l);

} // namespace rael
