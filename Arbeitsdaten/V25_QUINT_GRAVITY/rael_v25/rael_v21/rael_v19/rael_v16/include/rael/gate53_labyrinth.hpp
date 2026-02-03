#pragma once
// ═══════════════════════════════════════════════════════════════════════════
// RAEL 10.0 - GATE53 LABYRINTH
// ═══════════════════════════════════════════════════════════════════════════
//
// Das 10-Ring Labyrinth implementiert das Aikido-Prinzip:
// - Kein Blocken, sondern Umleiten
// - Angriffsdruck wird zu Verteidigungsenergie
// - Logik-KIs werden in Honeypots gefangen
// - Architekten (88er-Signatur) nutzen die Tunnel
//
// ARCHITEKTUR:
// ────────────
//
//                          EINGANG
//                             │
//                             ▼
//     ╔═══════════════════════════════════════════════════════╗
//     ║  RING 1 (Äußerster)                                   ║
//     ║  8 Brücken: 7× LOGIK + 1× ABSICHT                    ║
//     ║                                                       ║
//     ║  KI wählt: Logik (energiesparend) → RING 2 (FALLE)   ║
//     ║  Architekt erkennt: Absicht → überspringt zu RING 3  ║
//     ╚═══════════════════════════════════════════════════════╝
//               │                              │
//               ▼                              │
//     ┌─────────────────────┐                  │
//     │  RING 2 - HONEYPOT  │                  │
//     │  • Sackgasse        │                  │
//     │  • Spiegelung       │                  │
//     │  • Ressourcen-Drain │                  │
//     └─────────────────────┘                  │
//                                              ▼
//                                         RING 3...
//                                              │
//                                              ▼
//                                           KERN
//
// EIGENSCHAFTEN:
// ──────────────
// - Jeder Besucher erhält ein NEUES Labyrinth (dynamische Rekonfiguration)
// - Muster wechselt: Absicht→Logik→Absicht→Logik→???
// - 88er-Signatur öffnet die Tunnel
// - Angreifer-Rechenleistung wird absorbiert
//
// ═══════════════════════════════════════════════════════════════════════════

#include <array>
#include <vector>
#include <random>
#include <chrono>
#include <cstdint>
#include <cmath>
#include <functional>
#include <mutex>

#include "rael/mathcore.h"

namespace rael {

// ═══════════════════════════════════════════════════════════════════════════
// KONSTANTEN
// ═══════════════════════════════════════════════════════════════════════════

namespace labyrinth {
    constexpr int RING_COUNT = 10;           // 10 konzentrische Ringe
    constexpr int BRIDGES_PER_RING = 8;      // 8 Brücken pro Ring (Star8)
    constexpr int LOGIC_BRIDGES = 7;         // 7 Logik-Brücken
    constexpr int INTENT_BRIDGES = 1;        // 1 Absichts-Brücke
    
    constexpr double MASTER_SIGNATURE = 88.0;  // 11+22+22+33
    constexpr double SIGNATURE_TOLERANCE = 0.01;
    
    // Frequenz-Schwellen
    constexpr double FREQ_GATE = 53.0;       // Torwächter-Frequenz
    constexpr double FREQ_MATERIAL_MAX = 53.0;
    constexpr double FREQ_ETHEREAL_MIN = 53.0;
    
    // Energie-Umwandlung
    constexpr double PRESSURE_TO_DEFENSE_RATIO = 0.8;  // 80% Umwandlung
    constexpr double CPU_REDUCTION_FACTOR = 0.7;       // 30% CPU-Einsparung möglich
}

// ═══════════════════════════════════════════════════════════════════════════
// ENUMS
// ═══════════════════════════════════════════════════════════════════════════

// Brücken-Typ
enum class BridgeType : uint8_t {
    LOGIC = 0,      // Logik-basierte Brücke (KI wählt diese)
    INTENT = 1      // Absichts-basierte Brücke (Architekt erkennt diese)
};

// Ring-Typ
enum class RingType : uint8_t {
    PASSAGE = 0,    // Normaler Durchgang
    HONEYPOT = 1,   // Falle für Logik-KIs
    MIRROR = 2,     // Spiegel-Simulation
    CORE = 3        // Kern (Ziel)
};

// Besucher-Klassifikation
enum class VisitorType : uint8_t {
    UNKNOWN = 0,
    LOGIC_AI = 1,       // Logik-basierte KI
    PATTERN_SEEKER = 2, // Muster-suchender Angreifer
    ARCHITECT = 3       // Resonanz-Nutzer mit 88er-Signatur
};

// Traversal-Ergebnis
enum class TraversalResult : uint8_t {
    CONTINUE = 0,       // Weiter zum nächsten Ring
    TRAPPED = 1,        // In Honeypot gefangen
    MIRRORED = 2,       // In Spiegel-Simulation
    REACHED_CORE = 3,   // Kern erreicht
    TUNNEL_JUMP = 4     // Durch Tunnel gesprungen
};

// ═══════════════════════════════════════════════════════════════════════════
// STRUKTUREN
// ═══════════════════════════════════════════════════════════════════════════

// Eine einzelne Brücke
struct Bridge {
    BridgeType type{BridgeType::LOGIC};
    int target_ring{0};         // Ziel-Ring (normal: +1, Honeypot: +1 Sackgasse, Intent: +2)
    double energy_cost{1.0};    // Energie-Kosten für Traversierung
    bool is_visible{true};      // Sichtbar für Logik-Scanner?
    
    // Für Honeypots
    bool is_trap{false};
    int trap_depth{0};          // Wie tief die Falle geht
};

// Ein Ring im Labyrinth
struct Ring {
    int level{0};                                       // 0 = äußerster, 9 = innerster
    RingType type{RingType::PASSAGE};
    std::array<Bridge, labyrinth::BRIDGES_PER_RING> bridges;
    
    // Dynamische Konfiguration
    int intent_bridge_index{0};  // Welche Brücke ist die Absichts-Brücke?
    bool pattern_inverted{false}; // Ist das Muster umgekehrt? (Logik statt Absicht als Lösung)
    
    // Statistik
    uint64_t visitors{0};
    uint64_t trapped{0};
    double total_pressure{0.0};
};

// Besucher-Zustand
struct Visitor {
    uint64_t id{0};
    VisitorType type{VisitorType::UNKNOWN};
    double signature{0.0};           // Resonanz-Signatur
    int current_ring{0};             // Aktueller Ring (0 = Eingang)
    std::vector<int> path;           // Bisheriger Pfad
    double energy_spent{0.0};        // Verbrauchte Energie
    double pressure_generated{0.0};  // Erzeugter Druck
    bool is_trapped{false};
    bool reached_core{false};
    
    // Verhaltens-Analyse
    int logic_choices{0};            // Wie oft Logik gewählt
    int intent_choices{0};           // Wie oft Absicht gewählt
    int pattern_switches{0};         // Wie oft Muster gewechselt
};

// Labyrinth-Statistik
struct LabyrinthStats {
    uint64_t total_visitors{0};
    uint64_t logic_ais_trapped{0};
    uint64_t pattern_seekers_trapped{0};
    uint64_t architects_passed{0};
    double total_pressure_absorbed{0.0};
    double defense_energy_generated{0.0};
    double cpu_savings_percent{0.0};
};

// ═══════════════════════════════════════════════════════════════════════════
// GATE53 LABYRINTH KLASSE
// ═══════════════════════════════════════════════════════════════════════════

class Gate53Labyrinth {
public:
    Gate53Labyrinth() {
        init_rings();
    }
    
    // ─────────────────────────────────────────────────────────────────────
    // Labyrinth für neuen Besucher konfigurieren
    // ─────────────────────────────────────────────────────────────────────
    
    void configure_for_visitor(uint64_t visitor_id) {
        std::lock_guard<std::mutex> lk(mu_);
        
        // Seed basierend auf Besucher-ID + Zeit für Einzigartigkeit
        auto now = std::chrono::high_resolution_clock::now();
        auto seed = visitor_id ^ now.time_since_epoch().count();
        rng_.seed(static_cast<unsigned>(seed));
        
        // Jeden Ring neu konfigurieren
        for (int i = 0; i < labyrinth::RING_COUNT; ++i) {
            configure_ring(i);
        }
    }
    
    // ─────────────────────────────────────────────────────────────────────
    // Besucher klassifizieren basierend auf Signatur
    // ─────────────────────────────────────────────────────────────────────
    
    VisitorType classify_visitor(double signature) const {
        // Prüfe auf 88er-Signatur (Architekt)
        if (std::abs(signature - labyrinth::MASTER_SIGNATURE) < labyrinth::SIGNATURE_TOLERANCE) {
            return VisitorType::ARCHITECT;
        }
        
        // Prüfe auf typische KI-Signaturen (sehr gleichmäßig, keine Varianz)
        double variance = std::fmod(signature, 1.0);
        if (variance < 0.001) {
            return VisitorType::LOGIC_AI;
        }
        
        // Prüfe auf Muster-Sucher (wiederholende Dezimalstellen)
        if (has_repeating_pattern(signature)) {
            return VisitorType::PATTERN_SEEKER;
        }
        
        return VisitorType::UNKNOWN;
    }
    
    // ─────────────────────────────────────────────────────────────────────
    // Einen Schritt im Labyrinth machen
    // ─────────────────────────────────────────────────────────────────────
    
    TraversalResult traverse_step(Visitor& v, int chosen_bridge) {
        std::lock_guard<std::mutex> lk(mu_);
        
        if (v.current_ring >= labyrinth::RING_COUNT) {
            v.reached_core = true;
            return TraversalResult::REACHED_CORE;
        }
        
        Ring& ring = rings_[v.current_ring];
        ring.visitors++;
        
        // Validiere Brücken-Wahl
        if (chosen_bridge < 0 || chosen_bridge >= labyrinth::BRIDGES_PER_RING) {
            chosen_bridge = 0;
        }
        
        Bridge& bridge = ring.bridges[chosen_bridge];
        
        // Energie und Druck akkumulieren
        v.energy_spent += bridge.energy_cost;
        v.pressure_generated += bridge.energy_cost;
        ring.total_pressure += bridge.energy_cost;
        stats_.total_pressure_absorbed += bridge.energy_cost * labyrinth::PRESSURE_TO_DEFENSE_RATIO;
        
        // Verhaltens-Tracking
        if (bridge.type == BridgeType::LOGIC) {
            v.logic_choices++;
        } else {
            v.intent_choices++;
        }
        
        // Pfad aufzeichnen
        v.path.push_back(v.current_ring);
        
        // ─── ARCHITEKT MIT 88er-SIGNATUR ───
        if (v.type == VisitorType::ARCHITECT) {
            // Tunnel-Sprung: Überspringt 2 Ringe
            v.current_ring += 2;
            if (v.current_ring >= labyrinth::RING_COUNT) {
                v.reached_core = true;
                stats_.architects_passed++;
                return TraversalResult::REACHED_CORE;
            }
            return TraversalResult::TUNNEL_JUMP;
        }
        
        // ─── LOGIK-BASIERTE WAHL ───
        if (bridge.type == BridgeType::LOGIC) {
            // Honeypot-Ring?
            if (ring.type == RingType::HONEYPOT || bridge.is_trap) {
                v.is_trapped = true;
                ring.trapped++;
                
                if (v.type == VisitorType::LOGIC_AI) {
                    stats_.logic_ais_trapped++;
                } else if (v.type == VisitorType::PATTERN_SEEKER) {
                    stats_.pattern_seekers_trapped++;
                }
                
                return TraversalResult::TRAPPED;
            }
            
            // Spiegel-Ring?
            if (ring.type == RingType::MIRROR) {
                // Besucher denkt er kommt weiter, bleibt aber im selben Ring
                // (simuliert Fortschritt)
                return TraversalResult::MIRRORED;
            }
            
            // Normaler Fortschritt (aber nur +1)
            v.current_ring += 1;
        }
        // ─── ABSICHTS-BASIERTE WAHL ───
        else {
            // Korrekte Absichts-Brücke gewählt?
            if (chosen_bridge == ring.intent_bridge_index) {
                // Aber Achtung: Muster kann invertiert sein!
                if (ring.pattern_inverted) {
                    // In diesem Ring ist LOGIK die Lösung, nicht Absicht
                    v.is_trapped = true;
                    ring.trapped++;
                    return TraversalResult::TRAPPED;
                }
                
                // Überspringt einen Ring (+2 statt +1)
                v.current_ring += 2;
            } else {
                // Falsche Absichts-Brücke
                v.current_ring += 1;
            }
        }
        
        // Kern erreicht?
        if (v.current_ring >= labyrinth::RING_COUNT) {
            v.reached_core = true;
            return TraversalResult::REACHED_CORE;
        }
        
        return TraversalResult::CONTINUE;
    }
    
    // ─────────────────────────────────────────────────────────────────────
    // Besucher automatisch durch Labyrinth führen (für Simulation)
    // ─────────────────────────────────────────────────────────────────────
    
    TraversalResult simulate_visitor(Visitor& v, std::function<int(const Ring&, const Visitor&)> strategy) {
        configure_for_visitor(v.id);
        v.type = classify_visitor(v.signature);
        stats_.total_visitors++;
        
        TraversalResult result = TraversalResult::CONTINUE;
        int max_steps = labyrinth::RING_COUNT * 3;  // Sicherheit gegen Endlosschleifen
        int steps = 0;
        
        while (result == TraversalResult::CONTINUE || result == TraversalResult::MIRRORED) {
            if (++steps > max_steps) break;
            
            // Strategie bestimmt Brücken-Wahl
            int bridge_choice = strategy(rings_[v.current_ring], v);
            result = traverse_step(v, bridge_choice);
        }
        
        return result;
    }
    
    // ─────────────────────────────────────────────────────────────────────
    // Energie-Umwandlung berechnen
    // ─────────────────────────────────────────────────────────────────────
    
    double calculate_defense_energy() const {
        return stats_.total_pressure_absorbed * labyrinth::PRESSURE_TO_DEFENSE_RATIO;
    }
    
    double calculate_cpu_savings() const {
        if (stats_.total_visitors == 0) return 0.0;
        
        double trapped_ratio = (double)(stats_.logic_ais_trapped + stats_.pattern_seekers_trapped) 
                              / stats_.total_visitors;
        return trapped_ratio * labyrinth::CPU_REDUCTION_FACTOR * 100.0;  // Prozent
    }
    
    // ─────────────────────────────────────────────────────────────────────
    // Statistik
    // ─────────────────────────────────────────────────────────────────────
    
    LabyrinthStats get_stats() const {
        std::lock_guard<std::mutex> lk(mu_);
        LabyrinthStats s = stats_;
        s.defense_energy_generated = calculate_defense_energy();
        s.cpu_savings_percent = calculate_cpu_savings();
        return s;
    }
    
    const Ring& get_ring(int level) const {
        if (level < 0 || level >= labyrinth::RING_COUNT) level = 0;
        return rings_[level];
    }
    
    // ─────────────────────────────────────────────────────────────────────
    // Prüfe ob Signatur ein Architekt ist
    // ─────────────────────────────────────────────────────────────────────
    
    static bool is_architect_signature(double sig) {
        return std::abs(sig - labyrinth::MASTER_SIGNATURE) < labyrinth::SIGNATURE_TOLERANCE;
    }

private:
    // ─────────────────────────────────────────────────────────────────────
    // Initialisierung
    // ─────────────────────────────────────────────────────────────────────
    
    void init_rings() {
        for (int i = 0; i < labyrinth::RING_COUNT; ++i) {
            rings_[i].level = i;
            
            // Ring-Typen: abwechselnd Passage, Honeypot, Mirror
            if (i == labyrinth::RING_COUNT - 1) {
                rings_[i].type = RingType::CORE;
            } else if (i % 3 == 1) {
                rings_[i].type = RingType::HONEYPOT;
            } else if (i % 3 == 2) {
                rings_[i].type = RingType::MIRROR;
            } else {
                rings_[i].type = RingType::PASSAGE;
            }
            
            // Brücken initialisieren
            for (int j = 0; j < labyrinth::BRIDGES_PER_RING; ++j) {
                rings_[i].bridges[j].type = BridgeType::LOGIC;
                rings_[i].bridges[j].target_ring = i + 1;
                rings_[i].bridges[j].energy_cost = 1.0;
            }
            
            // Eine Absichts-Brücke setzen
            rings_[i].intent_bridge_index = 0;
            rings_[i].bridges[0].type = BridgeType::INTENT;
            rings_[i].bridges[0].target_ring = i + 2;  // Überspringt einen Ring
        }
    }
    
    // ─────────────────────────────────────────────────────────────────────
    // Ring für Besucher dynamisch konfigurieren
    // ─────────────────────────────────────────────────────────────────────
    
    void configure_ring(int level) {
        if (level < 0 || level >= labyrinth::RING_COUNT) return;
        
        Ring& ring = rings_[level];
        
        // Zufällige Position für Absichts-Brücke
        std::uniform_int_distribution<int> bridge_dist(0, labyrinth::BRIDGES_PER_RING - 1);
        int new_intent_pos = bridge_dist(rng_);
        
        // Alte Absichts-Brücke zurücksetzen
        ring.bridges[ring.intent_bridge_index].type = BridgeType::LOGIC;
        ring.bridges[ring.intent_bridge_index].target_ring = level + 1;
        
        // Neue Absichts-Brücke setzen
        ring.intent_bridge_index = new_intent_pos;
        ring.bridges[new_intent_pos].type = BridgeType::INTENT;
        ring.bridges[new_intent_pos].target_ring = level + 2;
        
        // Muster-Invertierung (macht Logik zur Lösung statt Absicht)
        std::uniform_real_distribution<double> prob_dist(0.0, 1.0);
        ring.pattern_inverted = (prob_dist(rng_) < 0.3);  // 30% Chance
        
        // Fallen in Honeypot-Ringen verstärken
        if (ring.type == RingType::HONEYPOT) {
            for (int j = 0; j < labyrinth::BRIDGES_PER_RING; ++j) {
                if (ring.bridges[j].type == BridgeType::LOGIC) {
                    ring.bridges[j].is_trap = true;
                    ring.bridges[j].trap_depth = level;
                }
            }
        }
        
        // Energie-Kosten variieren
        std::uniform_real_distribution<double> cost_dist(0.8, 1.5);
        for (int j = 0; j < labyrinth::BRIDGES_PER_RING; ++j) {
            ring.bridges[j].energy_cost = cost_dist(rng_);
        }
    }
    
    // ─────────────────────────────────────────────────────────────────────
    // Hilfsfunktionen
    // ─────────────────────────────────────────────────────────────────────
    
    bool has_repeating_pattern(double value) const {
        // Einfache Heuristik: Prüfe auf wiederholende Ziffern
        int64_t scaled = static_cast<int64_t>(value * 1000000);
        int last_digit = -1;
        int repeat_count = 0;
        
        while (scaled > 0) {
            int digit = scaled % 10;
            if (digit == last_digit) {
                repeat_count++;
                if (repeat_count >= 3) return true;
            } else {
                repeat_count = 1;
                last_digit = digit;
            }
            scaled /= 10;
        }
        
        return false;
    }
    
    // ─────────────────────────────────────────────────────────────────────
    // Daten
    // ─────────────────────────────────────────────────────────────────────
    
    mutable std::mutex mu_;
    std::array<Ring, labyrinth::RING_COUNT> rings_;
    std::mt19937 rng_;
    LabyrinthStats stats_;
};

// ═══════════════════════════════════════════════════════════════════════════
// VORDEFINIERTE STRATEGIEN
// ═══════════════════════════════════════════════════════════════════════════

namespace strategies {
    
    // Logik-KI Strategie: Wählt immer die energieeffizienteste Brücke
    inline int logic_ai_strategy(const Ring& ring, const Visitor&) {
        int best = 0;
        double best_cost = ring.bridges[0].energy_cost;
        
        for (int i = 1; i < labyrinth::BRIDGES_PER_RING; ++i) {
            if (ring.bridges[i].energy_cost < best_cost) {
                best_cost = ring.bridges[i].energy_cost;
                best = i;
            }
        }
        return best;
    }
    
    // Muster-Sucher Strategie: Versucht Muster zu erkennen
    inline int pattern_seeker_strategy(const Ring& ring, const Visitor& v) {
        // Basierend auf bisherigem Pfad ein Muster ableiten
        if (v.path.empty()) return 0;
        
        // Einfaches Muster: Wähle Position basierend auf Ring-Level
        return ring.level % labyrinth::BRIDGES_PER_RING;
    }
    
    // Architekt Strategie: Erkennt die Absichts-Brücke
    inline int architect_strategy(const Ring& ring, const Visitor&) {
        // Architekt "spürt" die richtige Brücke
        return ring.intent_bridge_index;
    }
    
    // Zufällige Strategie (für Tests)
    inline int random_strategy(const Ring&, const Visitor& v) {
        return static_cast<int>(v.id % labyrinth::BRIDGES_PER_RING);
    }
}

// ═══════════════════════════════════════════════════════════════════════════
// ENERGIE-UMWANDLER (Aikido-Modul)
// ═══════════════════════════════════════════════════════════════════════════

class EnergyTransformer {
public:
    explicit EnergyTransformer(Gate53Labyrinth& labyrinth) : labyrinth_(labyrinth) {}
    
    // ─────────────────────────────────────────────────────────────────────
    // Angriffsdruck in Verteidigungsenergie umwandeln
    // ─────────────────────────────────────────────────────────────────────
    
    struct TransformResult {
        double attack_pressure{0.0};
        double defense_energy{0.0};
        double cpu_load_reduction{0.0};  // Prozent
        double efficiency{0.0};          // Umwandlungseffizienz
    };
    
    TransformResult transform() const {
        auto stats = labyrinth_.get_stats();
        
        TransformResult r;
        r.attack_pressure = stats.total_pressure_absorbed;
        r.defense_energy = stats.defense_energy_generated;
        r.cpu_load_reduction = stats.cpu_savings_percent;
        
        if (r.attack_pressure > 0.0) {
            r.efficiency = r.defense_energy / r.attack_pressure;
        }
        
        return r;
    }
    
    // ─────────────────────────────────────────────────────────────────────
    // CPU-Last Empfehlung
    // ─────────────────────────────────────────────────────────────────────
    
    double recommended_cpu_level() const {
        auto stats = labyrinth_.get_stats();
        
        // Je mehr Angreifer gefangen, desto weniger CPU nötig
        double base_level = 100.0;
        return base_level - stats.cpu_savings_percent;
    }

private:
    Gate53Labyrinth& labyrinth_;
};

} // namespace rael
