#pragma once
// ═══════════════════════════════════════════════════════════════════════════
// RAEL fRAM - Frequenz-RAM (Beschleunigender Ringspeicher)
// ═══════════════════════════════════════════════════════════════════════════
//
// Architekten-Vision:
//   Alle Ergebnisse der 9 Themen-Nodes fliessen in den fRAM.
//   Der fRAM beschleunigt sie im Kreis - wie ein Laser-Resonator.
//   Heisse Daten steigen in schnellere Ringe auf.
//   Kalte Daten sinken ab und zerfallen (Ebbinghaus).
//
// 8-Stern Architektur (fRAM-Star):
//   F0 BLITZ    — Ring 1: 9×36  = 324  Slots (Reflex-Cache)
//   F1 PULS     — Ring 2: 9×48  = 432  Slots (Muster-Cache) ← 432 Hz!
//   F2 WELLE    — Ring 3: 9×61  = 549  Slots (Resonanz-Speicher)
//   F3 STROM    — Ring 4: 9×130 = 1170 Slots (Analyse-Archiv)
//   F4 OZEAN    — Ring 5: 9×233 = 2097 Slots (Langzeit-Weisheit)
//   F5 TRANSFER — Tier-Migration (heiss↑ kalt↓)
//   F6 KOPPLUNG — Resonanzkopplung zwischen Themes
//   F7 FREQUENZ — Umlauf-Beschleunigung (Amplitude waechst pro Zyklus)
//
// Groesse: 4572 Zellen × ~192 Bytes ≈ 1 MB
//
// Ring-Groessen folgen der Quint-Progression (Fibonacci-verwandt):
//   R1=36 (G1 Reflex), R2=48 (G2 Instinct), R3=61 (G3 Emotion),
//   R4=130 (G4 Ratio), R5=233 (G5 Spirit)
//
// ═══════════════════════════════════════════════════════════════════════════

#include "rael/aether_ring.hpp"
#include "rael/aether_bus.hpp"
#include "rael/star8_themes.h"

#include <array>
#include <deque>
#include <vector>
#include <string>
#include <mutex>
#include <atomic>
#include <chrono>
#include <cmath>

namespace rael {

// Forward
class AetherBus;

// ═══════════════════════════════════════════════════════════════════════════
// fRAM KONSTANTEN
// ═══════════════════════════════════════════════════════════════════════════

namespace fram {
    constexpr double PI = 3.14159265358979323846;
    constexpr double TWO_PI = 2.0 * PI;

    // 5 Tier-Stufen (Quint-basiert)
    constexpr size_t TIER_COUNT = 5;

    // Ring-Groessen pro Theme (Fibonacci/Quint-Progression)
    constexpr size_t RING_BLITZ  = 36;   // G1 Reflex   — schnellster Cache
    constexpr size_t RING_PULS   = 48;   // G2 Instinct — Muster-Erkennung
    constexpr size_t RING_WELLE  = 61;   // G3 Emotion  — Resonanz-Speicher
    constexpr size_t RING_STROM  = 130;  // G4 Ratio    — Analyse-Archiv
    constexpr size_t RING_OZEAN  = 233;  // G5 Spirit   — Langzeit-Weisheit

    constexpr size_t RING_SIZES[TIER_COUNT] = {
        RING_BLITZ, RING_PULS, RING_WELLE, RING_STROM, RING_OZEAN
    };

    // Slot-Berechnung: Tier-Groesse × 9 Themes
    constexpr size_t SLOTS_BLITZ = RING_BLITZ * STAR_NODE_COUNT;   // 324
    constexpr size_t SLOTS_PULS  = RING_PULS  * STAR_NODE_COUNT;   // 432 ← Kammer!
    constexpr size_t SLOTS_WELLE = RING_WELLE * STAR_NODE_COUNT;   // 549
    constexpr size_t SLOTS_STROM = RING_STROM * STAR_NODE_COUNT;   // 1170
    constexpr size_t SLOTS_OZEAN = RING_OZEAN * STAR_NODE_COUNT;   // 2097
    constexpr size_t TOTAL_SLOTS = SLOTS_BLITZ + SLOTS_PULS +
                                   SLOTS_WELLE + SLOTS_STROM +
                                   SLOTS_OZEAN;                     // 4572

    // fRAM Star-Node Anzahl
    constexpr size_t FRAM_NODE_COUNT = 8;

    // Tier-Namen
    constexpr const char* TIER_NAMES[TIER_COUNT] = {
        "BLITZ", "PULS", "WELLE", "STROM", "OZEAN"
    };

    // fRAM Star-Node Funktionen
    enum class FramNode : uint8_t {
        F0_BLITZ    = 0,  // Ring 1 Manager
        F1_PULS     = 1,  // Ring 2 Manager
        F2_WELLE    = 2,  // Ring 3 Manager
        F3_STROM    = 3,  // Ring 4 Manager
        F4_OZEAN    = 4,  // Ring 5 Manager
        F5_TRANSFER = 5,  // Tier-Migration
        F6_KOPPLUNG = 6,  // Cross-Theme Resonanz
        F7_FREQUENZ = 7   // Umlauf-Beschleunigung
    };

    // Beschleunigungs-Faktor pro Zyklus (Laser-Resonator Prinzip)
    constexpr double ACCELERATION_FACTOR = 1.05;  // +5% pro Zyklus

    // Zerfalls-Faktor pro Zyklus (Ebbinghaus)
    constexpr double DECAY_FACTOR = 0.98;  // -2% pro Zyklus

    // Schwellwerte fuer Tier-Migration
    constexpr double PROMOTE_THRESHOLD = 0.7;  // Zugriffs-Rate > 70% → aufsteigen
    constexpr double DEMOTE_THRESHOLD  = 0.1;  // Zugriffs-Rate < 10% → absteigen

    // Max Migrationen pro Zyklus (verhindert Thrashing)
    constexpr size_t MAX_MIGRATIONS_PER_CYCLE = 3;

    // QUELLE fuer Kappa-Berechnung
    constexpr double QUELLE = 1440.0;

    // Frequenz-Toleranz fuer Resonanz-Suche (Hz)
    constexpr double DEFAULT_FREQ_TOLERANCE = 5.0;

    // Kohaerenz-Zerfallskonstante (144 Hz = STRUKTUR)
    constexpr double COHERENCE_DECAY = 144.0;
}

// ═══════════════════════════════════════════════════════════════════════════
// FREQ CELL - Eine Zelle im Frequenz-RAM
// ═══════════════════════════════════════════════════════════════════════════
//
// Jede Zelle traegt:
//   - Quelle (welcher Theme-Node hat sie erzeugt)
//   - Nutzlast (das eigentliche Ergebnis)
//   - Frequenz-Signatur (dominante Frequenz in Hz)
//   - Amplitude (Signal-Staerke, waechst mit jedem Zugriff)
//   - Phase (Position im Ring-Umlauf)
//   - κ-Wert (Gravitations-Koeffizient: κ(f) = 1 - f/QUELLE)
//
// ═══════════════════════════════════════════════════════════════════════════

struct FreqCell {
    NodeTheme source{NodeTheme::SPRACHE};  // Quell-Thema
    std::string payload;                    // Ergebnis-Daten
    double frequency{0.0};                  // Dominante Frequenz (Hz)
    double amplitude{1.0};                  // Signal-Staerke
    double phase{0.0};                      // Phase im Umlauf [0, 2π)
    double kappa{0.0};                      // κ(f) = 1 - f/QUELLE

    std::chrono::steady_clock::time_point created;
    std::chrono::steady_clock::time_point last_accessed;
    uint32_t access_count{0};               // Zugriffe seit Einfuegung
    uint32_t cycle_count{0};                // Ueberlebte Ring-Umlaeufe
    double coherence{0.0};                  // Kopplung mit Nachbarn [0,1]

    // Alter in Sekunden
    double age_seconds() const {
        auto now = std::chrono::steady_clock::now();
        return std::chrono::duration<double>(now - created).count();
    }

    // Zugriffs-Rate (Zugriffe pro Umlauf)
    double access_rate() const {
        if (cycle_count == 0) return 1.0;  // Noch kein Umlauf → heiss
        return static_cast<double>(access_count) /
               static_cast<double>(cycle_count);
    }

    // Effektive Staerke (Amplitude × logarithmischer Zugriffs-Bonus)
    double effective_strength() const {
        return amplitude * (1.0 + std::log1p(static_cast<double>(access_count)));
    }
};

// ═══════════════════════════════════════════════════════════════════════════
// FREQ RING - Ein Ring pro Tier pro Theme
// ═══════════════════════════════════════════════════════════════════════════
//
// Kreisfoermiger Puffer mit automatischer Verdraengung (FIFO).
// Traegt eigene Metriken ueber SelfComputingRingBuffer.
//
// ═══════════════════════════════════════════════════════════════════════════

class FreqRing {
public:
    explicit FreqRing(size_t capacity = 36) : capacity_(capacity) {}

    // ─── Einfuegen ─────────────────────────────────────────────────────
    void push(FreqCell cell) {
        cell.created = std::chrono::steady_clock::now();
        cell.last_accessed = cell.created;
        cell.kappa = 1.0 - cell.frequency / fram::QUELLE;

        if (cells_.size() >= capacity_) {
            cells_.pop_front();
        }
        cells_.push_back(std::move(cell));

        // Amplitude-Metrik tracken
        if (!cells_.empty()) {
            amplitude_metrics_.push(cells_.back().amplitude);
        }
    }

    // ─── Abrufen ───────────────────────────────────────────────────────

    // Letzte n Zellen (neueste zuerst)
    std::vector<const FreqCell*> recent(size_t n) const {
        std::vector<const FreqCell*> result;
        size_t count = std::min(n, cells_.size());
        for (size_t i = cells_.size() - count; i < cells_.size(); ++i) {
            result.push_back(&cells_[i]);
        }
        return result;
    }

    // Suche nach Frequenz (±tolerance Hz)
    std::vector<const FreqCell*> find_by_frequency(double freq,
                                                    double tolerance) const {
        std::vector<const FreqCell*> result;
        for (const auto& cell : cells_) {
            if (std::abs(cell.frequency - freq) <= tolerance) {
                result.push_back(&cell);
            }
        }
        return result;
    }

    // ─── Zugriff markieren (Amplitude steigt) ──────────────────────────
    void touch(size_t index) {
        if (index < cells_.size()) {
            cells_[index].access_count++;
            cells_[index].last_accessed = std::chrono::steady_clock::now();
            cells_[index].amplitude *= fram::ACCELERATION_FACTOR;
        }
    }

    // ─── Ring-Umlauf (ein Zyklus) ─────────────────────────────────────
    void cycle() {
        double phase_step = fram::TWO_PI / static_cast<double>(
            capacity_ > 0 ? capacity_ : 1);

        for (auto& cell : cells_) {
            cell.cycle_count++;
            cell.phase += phase_step;
            if (cell.phase >= fram::TWO_PI) cell.phase -= fram::TWO_PI;

            // Zerfall fuer selten genutzte Zellen
            if (cell.access_rate() < fram::DEMOTE_THRESHOLD) {
                cell.amplitude *= fram::DECAY_FACTOR;
            }
        }
    }

    // ─── Beschleunigung (F7 FREQUENZ) ─────────────────────────────────
    void accelerate(double coherence_boost, double tier_speed) {
        for (auto& cell : cells_) {
            // Nur aktiv genutzte Zellen beschleunigen
            if (cell.access_count > 0 && cell.cycle_count > 0) {
                cell.amplitude *= coherence_boost;
            }
            // Schnellere Tiers rotieren schneller
            cell.phase += tier_speed * 0.1;
            if (cell.phase >= fram::TWO_PI) {
                cell.phase -= fram::TWO_PI;
            }
        }
    }

    // ─── Migration-Kandidaten ──────────────────────────────────────────

    // Heisse Zellen → aufsteigen
    std::vector<size_t> candidates_for_promotion() const {
        std::vector<size_t> result;
        for (size_t i = 0; i < cells_.size(); ++i) {
            if (cells_[i].access_rate() > fram::PROMOTE_THRESHOLD) {
                result.push_back(i);
            }
        }
        return result;
    }

    // Kalte Zellen → absteigen
    std::vector<size_t> candidates_for_demotion() const {
        std::vector<size_t> result;
        for (size_t i = 0; i < cells_.size(); ++i) {
            if (cells_[i].access_rate() < fram::DEMOTE_THRESHOLD &&
                cells_[i].cycle_count > 3) {
                result.push_back(i);
            }
        }
        return result;
    }

    // Zelle extrahieren (entfernen + zurueckgeben)
    FreqCell extract(size_t index) {
        if (index >= cells_.size()) return FreqCell{};
        FreqCell cell = std::move(cells_[index]);
        cells_.erase(cells_.begin() + static_cast<long>(index));
        return cell;
    }

    // ─── Statistiken ───────────────────────────────────────────────────
    size_t size() const { return cells_.size(); }
    size_t capacity() const { return capacity_; }
    bool   empty() const { return cells_.empty(); }
    bool   full() const { return cells_.size() >= capacity_; }

    double mean_amplitude() const { return amplitude_metrics_.mean(); }

    double mean_frequency() const {
        if (cells_.empty()) return 0.0;
        double sum = 0.0;
        for (const auto& c : cells_) sum += c.frequency;
        return sum / static_cast<double>(cells_.size());
    }

    double total_energy() const {
        double e = 0.0;
        for (const auto& c : cells_) {
            e += c.amplitude * (1.0 - c.kappa);
        }
        return e;
    }

private:
    size_t capacity_;
    std::deque<FreqCell> cells_;
    SelfComputingRingBuffer amplitude_metrics_{64};
};

// ═══════════════════════════════════════════════════════════════════════════
// FREQ TIER - Ein kompletter Tier mit 9 Theme-Ringen
// ═══════════════════════════════════════════════════════════════════════════

class FreqTier {
public:
    explicit FreqTier(size_t ring_size = 36, const char* name = "?")
        : name_(name)
    {
        for (size_t i = 0; i < STAR_NODE_COUNT; ++i) {
            rings_[i] = FreqRing(ring_size);
        }
    }

    void store(NodeTheme theme, FreqCell cell) {
        size_t idx = static_cast<size_t>(theme);
        if (idx < STAR_NODE_COUNT) {
            rings_[idx].push(std::move(cell));
        }
    }

    std::vector<const FreqCell*> recall(NodeTheme theme, size_t n) const {
        size_t idx = static_cast<size_t>(theme);
        if (idx < STAR_NODE_COUNT) return rings_[idx].recent(n);
        return {};
    }

    std::vector<const FreqCell*> find_by_frequency(double freq,
                                                    double tolerance) const {
        std::vector<const FreqCell*> result;
        for (const auto& ring : rings_) {
            auto found = ring.find_by_frequency(freq, tolerance);
            result.insert(result.end(), found.begin(), found.end());
        }
        return result;
    }

    void cycle() {
        for (auto& ring : rings_) ring.cycle();
    }

    FreqRing&       ring(NodeTheme t)       { return rings_[static_cast<size_t>(t) % STAR_NODE_COUNT]; }
    const FreqRing& ring(NodeTheme t) const { return rings_[static_cast<size_t>(t) % STAR_NODE_COUNT]; }

    size_t total_cells() const {
        size_t n = 0;
        for (const auto& r : rings_) n += r.size();
        return n;
    }

    double total_energy() const {
        double e = 0.0;
        for (const auto& r : rings_) e += r.total_energy();
        return e;
    }

    const char* name() const { return name_; }

private:
    std::array<FreqRing, STAR_NODE_COUNT> rings_;
    const char* name_;
};

// ═══════════════════════════════════════════════════════════════════════════
// fRAM SNAPSHOT
// ═══════════════════════════════════════════════════════════════════════════

struct FreqRAMSnapshot {
    size_t   total_cells{0};
    size_t   tier_cells[fram::TIER_COUNT]{};
    double   tier_energy[fram::TIER_COUNT]{};
    double   total_energy{0.0};
    double   mean_coherence{0.0};
    uint64_t total_stores{0};
    uint64_t total_recalls{0};
    uint64_t total_promotions{0};
    uint64_t total_demotions{0};
    uint64_t cycle_count{0};
};

// ═══════════════════════════════════════════════════════════════════════════
// FREQ RAM - Der 8-Stern Frequenz-Arbeitsspeicher
// ═══════════════════════════════════════════════════════════════════════════

class FreqRAM {
public:
    FreqRAM();

    // ─── Hauptoperationen ──────────────────────────────────────────────

    // Ergebnis speichern (kommt von Theme-Node, landet immer in R1 BLITZ)
    void store(NodeTheme source, const std::string& payload,
               double frequency = 432.0);

    // Letzte n Ergebnisse eines Themes abrufen (schnellster Tier zuerst)
    std::vector<const FreqCell*> recall(NodeTheme theme, size_t n = 5);

    // Suche nach Frequenz (ueber alle Tiers und Themes)
    std::vector<const FreqCell*> recall_by_frequency(
        double freq, double tolerance = fram::DEFAULT_FREQ_TOLERANCE);

    // Suche nach Theme + Frequenz
    std::vector<const FreqCell*> recall_themed_frequency(
        NodeTheme theme, double freq,
        double tolerance = fram::DEFAULT_FREQ_TOLERANCE);

    // ─── Zyklus (wird von RaelCore getriggert) ────────────────────────

    // Ein kompletter fRAM-Zyklus:
    //   1. F0-F4: Ring-Rotation (cycle)
    //   2. F5:    Tier-Migration (promote/demote)
    //   3. F6:    Cross-Theme Kopplung
    //   4. F7:    Frequenz-Beschleunigung
    void tick();

    // ─── Status ───────────────────────────────────────────────────────
    FreqRAMSnapshot snapshot() const;
    std::string status_string() const;

    // Tier-Zugriff
    const FreqTier& tier(size_t idx) const {
        return tiers_[idx % fram::TIER_COUNT];
    }

    // Kohaerenz zwischen zwei Themes abfragen
    double coherence(NodeTheme a, NodeTheme b) const {
        return coherence_matrix_[static_cast<size_t>(a)]
                                [static_cast<size_t>(b)];
    }

    // AetherBus-Kopplung (Channels ab base_channel)
    void couple_to_bus(AetherBus& bus, int base_channel = 90);

private:
    // F5: TRANSFER - Tier-Migration (heiss↑ kalt↓)
    void run_transfer();

    // F6: KOPPLUNG - Cross-Theme Resonanz-Erkennung
    void run_coupling();

    // F7: FREQUENZ - Umlauf-Beschleunigung (Laser-Resonator)
    void run_acceleration();

    // ─── Daten ────────────────────────────────────────────────────────
    std::array<FreqTier, fram::TIER_COUNT> tiers_;

    mutable std::mutex mu_;

    // Statistiken
    std::atomic<uint64_t> total_stores_{0};
    std::atomic<uint64_t> total_recalls_{0};
    std::atomic<uint64_t> total_promotions_{0};
    std::atomic<uint64_t> total_demotions_{0};
    std::atomic<uint64_t> cycle_count_{0};

    // AetherBus-Kopplung
    AetherBus* bus_{nullptr};
    int bus_base_channel_{90};

    // Kohaerenz-Matrix: theme × theme Kopplungsstaerke [0,1]
    std::array<std::array<double, STAR_NODE_COUNT>, STAR_NODE_COUNT>
        coherence_matrix_{};
};

} // namespace rael
