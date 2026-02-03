#pragma once
// ═══════════════════════════════════════════════════════════════════════════
// RAEL CONTROL STAR + STAR RING DOCKING (Verbesserung #5)
// ═══════════════════════════════════════════════════════════════════════════
// Parallele Sterne für Hypothesen/Coding/Defense-Varianten mit Combiner
// ═══════════════════════════════════════════════════════════════════════════

#include <string>
#include <vector>
#include <array>
#include <functional>
#include <mutex>
#include <atomic>
#include <thread>
#include <future>
#include <memory>
#include <cstdint>

namespace rael {

// ═══════════════════════════════════════════════════════════════════════════
// STAR TYPE - Verschiedene Stern-Typen für unterschiedliche Aufgaben
// ═══════════════════════════════════════════════════════════════════════════

enum class StarType : uint8_t {
    HYPOTHESIS = 1,   // Hypothesen-Generierung und -Test
    CODING     = 2,   // Code-Generierung und -Analyse
    DEFENSE    = 3,   // Sicherheits-Analyse und -Schutz
    RESONANCE  = 4,   // Resonanz-Berechnung
    ANALYTICS  = 5    // Datenanalyse
};

inline const char* star_type_name(StarType t) {
    switch (t) {
        case StarType::HYPOTHESIS: return "HYPOTHESIS";
        case StarType::CODING:     return "CODING";
        case StarType::DEFENSE:    return "DEFENSE";
        case StarType::RESONANCE:  return "RESONANCE";
        case StarType::ANALYTICS:  return "ANALYTICS";
        default:                   return "UNKNOWN";
    }
}

// ═══════════════════════════════════════════════════════════════════════════
// STAR RESULT - Ergebnis einer Stern-Berechnung
// ═══════════════════════════════════════════════════════════════════════════

struct StarResult {
    uint64_t star_id = 0;
    StarType type = StarType::HYPOTHESIS;
    std::string output;
    double confidence = 0.0;
    double quality = 0.0;
    double execution_time_ms = 0.0;
    bool success = false;
    std::string error;

    // Für Combiner
    double weight = 1.0;
    std::vector<std::string> tags;
};

// ═══════════════════════════════════════════════════════════════════════════
// CONTROL STAR - Ein einzelner Stern im Ring
// ═══════════════════════════════════════════════════════════════════════════

class ControlStar {
public:
    explicit ControlStar(uint64_t id, StarType type);

    // Führe Berechnung aus
    StarResult execute(const std::string& input);

    // Konfiguration
    void set_handler(std::function<StarResult(const std::string&)> handler);
    void set_weight(double w) { weight_ = w; }
    void set_enabled(bool e) { enabled_ = e; }

    // Getters
    uint64_t id() const { return id_; }
    StarType type() const { return type_; }
    double weight() const { return weight_; }
    bool enabled() const { return enabled_; }

    // Statistiken
    uint64_t executions() const { return executions_.load(); }
    double avg_time_ms() const;
    double success_rate() const;

private:
    uint64_t id_;
    StarType type_;
    double weight_ = 1.0;
    bool enabled_ = true;

    std::function<StarResult(const std::string&)> handler_;

    // Statistiken
    std::atomic<uint64_t> executions_{0};
    std::atomic<uint64_t> successes_{0};
    std::atomic<uint64_t> total_time_ns_{0};
};

// ═══════════════════════════════════════════════════════════════════════════
// STAR RING - Ring aus mehreren parallelen Sternen
// ═══════════════════════════════════════════════════════════════════════════

class StarRing {
public:
    static constexpr size_t MAX_STARS = 8;

    StarRing(const std::string& name = "default");

    // Stern-Management
    uint64_t add_star(StarType type);
    bool remove_star(uint64_t id);
    ControlStar* get_star(uint64_t id);
    std::vector<ControlStar*> get_stars_by_type(StarType type);

    // Parallele Ausführung
    std::vector<StarResult> execute_all(const std::string& input);
    std::vector<StarResult> execute_by_type(StarType type, const std::string& input);

    // Async Ausführung
    std::future<std::vector<StarResult>> execute_async(const std::string& input);

    // Ring-Konfiguration
    const std::string& name() const { return name_; }
    size_t star_count() const { return stars_.size(); }

private:
    std::string name_;
    std::vector<std::unique_ptr<ControlStar>> stars_;
    std::mutex mutex_;
    uint64_t next_id_ = 1;
};

// ═══════════════════════════════════════════════════════════════════════════
// RESULT COMBINER - Kombiniert Ergebnisse mehrerer Sterne
// ═══════════════════════════════════════════════════════════════════════════

enum class CombineStrategy : uint8_t {
    WEIGHTED_AVERAGE = 1,   // Gewichteter Durchschnitt
    BEST_CONFIDENCE  = 2,   // Höchste Confidence gewinnt
    CONSENSUS        = 3,   // Konsens-basiert (Mehrheit)
    CASCADE          = 4    // Kaskade (erste Erfolgreiche)
};

class ResultCombiner {
public:
    explicit ResultCombiner(CombineStrategy strategy = CombineStrategy::WEIGHTED_AVERAGE);

    // Kombiniere Ergebnisse
    StarResult combine(const std::vector<StarResult>& results);

    // Strategie
    void set_strategy(CombineStrategy s) { strategy_ = s; }
    CombineStrategy strategy() const { return strategy_; }

    // Schwellwerte
    void set_min_confidence(double c) { min_confidence_ = c; }
    void set_consensus_threshold(double t) { consensus_threshold_ = t; }

private:
    StarResult combine_weighted(const std::vector<StarResult>& results);
    StarResult combine_best(const std::vector<StarResult>& results);
    StarResult combine_consensus(const std::vector<StarResult>& results);
    StarResult combine_cascade(const std::vector<StarResult>& results);

    CombineStrategy strategy_;
    double min_confidence_ = 0.3;
    double consensus_threshold_ = 0.6;  // 60% Übereinstimmung
};

// ═══════════════════════════════════════════════════════════════════════════
// STAR RING DOCKING - Verbindet mehrere Ringe
// ═══════════════════════════════════════════════════════════════════════════

class StarRingDocking {
public:
    StarRingDocking();

    // Ring-Management
    void dock_ring(std::shared_ptr<StarRing> ring);
    void undock_ring(const std::string& name);
    std::shared_ptr<StarRing> get_ring(const std::string& name);
    std::vector<std::string> list_rings() const;

    // Globale Ausführung über alle Ringe
    std::vector<StarResult> broadcast(const std::string& input);

    // Routing zu spezifischem Ring
    std::vector<StarResult> route_to(const std::string& ring_name, const std::string& input);

    // Combiner für Ring-übergreifende Ergebnisse
    StarResult combine_all(const std::string& input, CombineStrategy strategy = CombineStrategy::BEST_CONFIDENCE);

    // Statistiken
    size_t ring_count() const { return rings_.size(); }
    size_t total_star_count() const;

private:
    std::vector<std::shared_ptr<StarRing>> rings_;
    mutable std::mutex mutex_;
    ResultCombiner combiner_;
};

// Global instance
extern StarRingDocking gStarDocking;

} // namespace rael
