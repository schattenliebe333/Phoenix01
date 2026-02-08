// ═══════════════════════════════════════════════════════════════════════════
// RAEL fRAM - Frequenz-RAM Implementation
// ═══════════════════════════════════════════════════════════════════════════
//
// 8-Stern Architektur:
//   F0-F4: Ring-Manager (BLITZ, PULS, WELLE, STROM, OZEAN)
//   F5:    TRANSFER - Tier-Migration (heiss↑ kalt↓)
//   F6:    KOPPLUNG - Cross-Theme Resonanz
//   F7:    FREQUENZ - Laser-Resonator Beschleunigung
//
// Jeder tick() durchlaeuft alle 8 Nodes sequentiell.
// Die Ringe rotieren, Daten beschleunigen, und die Kohaerenz
// zwischen den Themes wird kontinuierlich berechnet.
//
// ═══════════════════════════════════════════════════════════════════════════

#include "rael/freq_ram.hpp"

#include <algorithm>
#include <sstream>
#include <iomanip>
#include <cmath>

namespace rael {

// ═══════════════════════════════════════════════════════════════════════════
// KONSTRUKTOR
// ═══════════════════════════════════════════════════════════════════════════

FreqRAM::FreqRAM()
    : tiers_{{
        FreqTier(fram::RING_BLITZ, "BLITZ"),
        FreqTier(fram::RING_PULS,  "PULS"),
        FreqTier(fram::RING_WELLE, "WELLE"),
        FreqTier(fram::RING_STROM, "STROM"),
        FreqTier(fram::RING_OZEAN, "OZEAN")
    }}
{
    // Kohaerenz-Matrix initialisieren
    for (auto& row : coherence_matrix_) {
        row.fill(0.0);
    }
}

// ═══════════════════════════════════════════════════════════════════════════
// STORE - Ergebnis in fRAM ablegen
// ═══════════════════════════════════════════════════════════════════════════
//
// Neue Ergebnisse landen IMMER in Ring 1 (BLITZ) - dem schnellsten Cache.
// Von dort aus werden sie durch den Transfer-Mechanismus (F5) in
// langsamere Ringe verschoben, wenn sie nicht mehr aktiv genutzt werden.
//
// ═══════════════════════════════════════════════════════════════════════════

void FreqRAM::store(NodeTheme source, const std::string& payload,
                     double frequency)
{
    std::lock_guard<std::mutex> lk(mu_);

    FreqCell cell;
    cell.source = source;
    cell.payload = payload;
    cell.frequency = frequency;
    cell.amplitude = 1.0;
    cell.phase = 0.0;
    cell.kappa = 1.0 - frequency / fram::QUELLE;
    cell.access_count = 0;
    cell.cycle_count = 0;
    cell.coherence = 0.0;

    // Immer in Ring 1 (BLITZ) - schnellster Zugriff
    tiers_[0].store(source, std::move(cell));

    total_stores_.fetch_add(1, std::memory_order_relaxed);

    // AetherBus: Input-Frequenz melden
    if (bus_) {
        bus_->publish(bus_base_channel_, frequency, AetherScale::G1_Reflex);
    }
}

// ═══════════════════════════════════════════════════════════════════════════
// RECALL - Ergebnisse abrufen
// ═══════════════════════════════════════════════════════════════════════════
//
// Sucht von schnellstem zu langsamstem Tier.
// Jeder Zugriff markiert die Zellen als "heiss" → Amplitude steigt.
// Heisse Zellen werden beim naechsten Transfer in schnellere Ringe befördert.
//
// ═══════════════════════════════════════════════════════════════════════════

std::vector<const FreqCell*> FreqRAM::recall(NodeTheme theme, size_t n) {
    std::lock_guard<std::mutex> lk(mu_);

    std::vector<const FreqCell*> result;

    // Von schnellstem zu langsamstem Tier suchen
    for (size_t t = 0; t < fram::TIER_COUNT && result.size() < n; ++t) {
        auto found = tiers_[t].recall(theme, n - result.size());
        result.insert(result.end(), found.begin(), found.end());
    }

    total_recalls_.fetch_add(1, std::memory_order_relaxed);

    // Touch: Zugriff markieren → Amplitude steigt
    // (nur im schnellsten Tier, der Rest wird durch Migration gehandhabt)
    auto& ring = tiers_[0].ring(theme);
    size_t touch_count = std::min(n, ring.size());
    for (size_t i = 0; i < touch_count; ++i) {
        ring.touch(ring.size() - 1 - i);
    }

    return result;
}

std::vector<const FreqCell*> FreqRAM::recall_by_frequency(
    double freq, double tolerance)
{
    std::lock_guard<std::mutex> lk(mu_);

    std::vector<const FreqCell*> result;
    for (size_t t = 0; t < fram::TIER_COUNT; ++t) {
        auto found = tiers_[t].find_by_frequency(freq, tolerance);
        result.insert(result.end(), found.begin(), found.end());
    }

    total_recalls_.fetch_add(1, std::memory_order_relaxed);
    return result;
}

std::vector<const FreqCell*> FreqRAM::recall_themed_frequency(
    NodeTheme theme, double freq, double tolerance)
{
    std::lock_guard<std::mutex> lk(mu_);

    std::vector<const FreqCell*> result;
    for (size_t t = 0; t < fram::TIER_COUNT; ++t) {
        auto found = tiers_[t].ring(theme).find_by_frequency(freq, tolerance);
        result.insert(result.end(), found.begin(), found.end());
    }

    total_recalls_.fetch_add(1, std::memory_order_relaxed);
    return result;
}

// ═══════════════════════════════════════════════════════════════════════════
// TICK - Ein kompletter fRAM-Zyklus
// ═══════════════════════════════════════════════════════════════════════════
//
// Durchlaeuft alle 8 fRAM-Star Nodes:
//   F0-F4: Ringe rotieren (jede Zelle altert um einen Schritt)
//   F5:    Heisse Zellen aufsteigen, kalte Zellen absteigen
//   F6:    Frequenz-Kohaerenz zwischen Themes berechnen
//   F7:    Amplitude beschleunigen (Laser-Resonator Effekt)
//
// ═══════════════════════════════════════════════════════════════════════════

void FreqRAM::tick() {
    std::lock_guard<std::mutex> lk(mu_);

    // F0-F4: Ring-Rotation
    for (size_t t = 0; t < fram::TIER_COUNT; ++t) {
        tiers_[t].cycle();
    }

    // F5: Tier-Migration
    run_transfer();

    // F6: Cross-Theme Resonanz-Kopplung
    run_coupling();

    // F7: Frequenz-Beschleunigung
    run_acceleration();

    cycle_count_.fetch_add(1, std::memory_order_relaxed);

    // AetherBus: Gesamt-Energie melden
    if (bus_) {
        double total_e = 0.0;
        for (const auto& tier : tiers_) {
            total_e += tier.total_energy();
        }
        bus_->publish(bus_base_channel_ + 1, total_e,
                      AetherScale::G3_Emotion);
    }
}

// ═══════════════════════════════════════════════════════════════════════════
// F5: TRANSFER - Tier-Migration
// ═══════════════════════════════════════════════════════════════════════════
//
// Promote: Zellen mit hoher Zugriffs-Rate → schnellerer Ring
// Demote:  Zellen mit niedriger Zugriffs-Rate → langsamerer Ring
//
// Max 3 Migrationen pro Theme pro Zyklus (verhindert Thrashing)
//
// ═══════════════════════════════════════════════════════════════════════════

void FreqRAM::run_transfer() {
    // ─── Promote: heiss → schnellerer Ring ─────────────────────────────
    for (size_t t = 1; t < fram::TIER_COUNT; ++t) {
        for (size_t theme = 0; theme < STAR_NODE_COUNT; ++theme) {
            auto nt = static_cast<NodeTheme>(theme);
            auto& ring = tiers_[t].ring(nt);

            auto candidates = ring.candidates_for_promotion();
            size_t promoted = 0;

            // Von hinten nach vorne extrahieren (Indices bleiben stabil)
            for (auto it = candidates.rbegin();
                 it != candidates.rend() &&
                 promoted < fram::MAX_MIGRATIONS_PER_CYCLE;
                 ++it)
            {
                FreqCell cell = ring.extract(*it);
                cell.cycle_count = 0;  // Reset im neuen Ring
                tiers_[t - 1].store(nt, std::move(cell));
                total_promotions_.fetch_add(1, std::memory_order_relaxed);
                promoted++;
            }
        }
    }

    // ─── Demote: kalt → langsamerer Ring ───────────────────────────────
    for (size_t t = 0; t + 1 < fram::TIER_COUNT; ++t) {
        for (size_t theme = 0; theme < STAR_NODE_COUNT; ++theme) {
            auto nt = static_cast<NodeTheme>(theme);
            auto& ring = tiers_[t].ring(nt);

            auto candidates = ring.candidates_for_demotion();
            size_t demoted = 0;

            for (auto it = candidates.rbegin();
                 it != candidates.rend() &&
                 demoted < fram::MAX_MIGRATIONS_PER_CYCLE;
                 ++it)
            {
                FreqCell cell = ring.extract(*it);
                cell.cycle_count = 0;
                tiers_[t + 1].store(nt, std::move(cell));
                total_demotions_.fetch_add(1, std::memory_order_relaxed);
                demoted++;
            }
        }
    }
}

// ═══════════════════════════════════════════════════════════════════════════
// F6: KOPPLUNG - Cross-Theme Resonanz-Erkennung
// ═══════════════════════════════════════════════════════════════════════════
//
// Berechnet die Frequenz-Kohaerenz zwischen allen Theme-Paaren.
// Zwei Themes resonieren, wenn sie Zellen bei aehnlichen Frequenzen haben.
//
// Kohaerenz = exp(-|f_i - f_j| / 144.0)
//   → 1.0 bei identischen Frequenzen
//   → ~0.05 bei 432 Hz Differenz
//
// Tunnel-Paare (5↔432 Hz, 13↔144 Hz) sind IMMER perfekt gekoppelt.
//
// ═══════════════════════════════════════════════════════════════════════════

void FreqRAM::run_coupling() {
    // Nur den schnellsten Tier (BLITZ) fuer Performance analysieren
    const auto& blitz = tiers_[0];

    for (size_t i = 0; i < STAR_NODE_COUNT; ++i) {
        for (size_t j = i + 1; j < STAR_NODE_COUNT; ++j) {
            auto ni = static_cast<NodeTheme>(i);
            auto nj = static_cast<NodeTheme>(j);

            const auto& ring_i = blitz.ring(ni);
            const auto& ring_j = blitz.ring(nj);

            if (ring_i.empty() || ring_j.empty()) {
                coherence_matrix_[i][j] = 0.0;
                coherence_matrix_[j][i] = 0.0;
                continue;
            }

            double fi = ring_i.mean_frequency();
            double fj = ring_j.mean_frequency();
            double diff = std::abs(fi - fj);

            // Kohaerenz: exponentieller Zerfall mit Distanz
            double coh = std::exp(-diff / fram::COHERENCE_DECAY);

            // Tunnel-Paare: perfekte Kopplung
            if (aether::is_tunnel_pair(fi, fj)) {
                coh = 1.0;
            }

            coherence_matrix_[i][j] = coh;
            coherence_matrix_[j][i] = coh;
        }
    }

    // Per-Theme Kohaerenz an AetherBus melden
    if (bus_) {
        for (size_t theme = 0; theme < STAR_NODE_COUNT; ++theme) {
            double total_coh = 0.0;
            for (size_t other = 0; other < STAR_NODE_COUNT; ++other) {
                if (other != theme) {
                    total_coh += coherence_matrix_[theme][other];
                }
            }
            double avg_coh = total_coh /
                             static_cast<double>(STAR_NODE_COUNT - 1);
            bus_->publish(bus_base_channel_ + 2 + static_cast<int>(theme),
                          avg_coh, AetherScale::G4_Ratio);
        }
    }
}

// ═══════════════════════════════════════════════════════════════════════════
// F7: FREQUENZ - Umlauf-Beschleunigung
// ═══════════════════════════════════════════════════════════════════════════
//
// Laser-Resonator Prinzip:
//   Jeder Umlauf verstaerkt die Amplitude von aktiven Zellen.
//   Die Verstaerkung wird durch die Cross-Theme Kohaerenz moduliert:
//     Hohe Kohaerenz → mehr Verstaerkung (Resonanz-Feedback)
//     Niedrige Kohaerenz → nur Standard-Verstaerkung
//
//   Schnellere Tiers rotieren schneller:
//     R1 BLITZ: 3.0× Geschwindigkeit
//     R2 PULS:  2.5×
//     R3 WELLE: 2.0×
//     R4 STROM: 1.5×
//     R5 OZEAN: 1.0× (Basis)
//
// ═══════════════════════════════════════════════════════════════════════════

void FreqRAM::run_acceleration() {
    for (size_t t = 0; t < fram::TIER_COUNT; ++t) {
        // Schnellere Tiers drehen schneller
        double tier_speed = 1.0 + static_cast<double>(
            fram::TIER_COUNT - 1 - t) * 0.5;

        for (size_t theme = 0; theme < STAR_NODE_COUNT; ++theme) {
            auto nt = static_cast<NodeTheme>(theme);

            // Kohaerenz-Boost: wie stark resoniert dieses Theme mit anderen?
            double coh_boost = 1.0;
            for (size_t other = 0; other < STAR_NODE_COUNT; ++other) {
                if (other != theme) {
                    coh_boost += coherence_matrix_[theme][other] * 0.01;
                }
            }

            tiers_[t].ring(nt).accelerate(coh_boost, tier_speed);
        }
    }
}

// ═══════════════════════════════════════════════════════════════════════════
// STATUS
// ═══════════════════════════════════════════════════════════════════════════

FreqRAMSnapshot FreqRAM::snapshot() const {
    std::lock_guard<std::mutex> lk(mu_);

    FreqRAMSnapshot snap;
    snap.total_stores = total_stores_.load(std::memory_order_relaxed);
    snap.total_recalls = total_recalls_.load(std::memory_order_relaxed);
    snap.total_promotions = total_promotions_.load(std::memory_order_relaxed);
    snap.total_demotions = total_demotions_.load(std::memory_order_relaxed);
    snap.cycle_count = cycle_count_.load(std::memory_order_relaxed);

    snap.total_energy = 0.0;
    for (size_t t = 0; t < fram::TIER_COUNT; ++t) {
        snap.tier_cells[t] = tiers_[t].total_cells();
        snap.tier_energy[t] = tiers_[t].total_energy();
        snap.total_cells += snap.tier_cells[t];
        snap.total_energy += snap.tier_energy[t];
    }

    // Mittlere Kohaerenz ueber alle Theme-Paare
    double coh_sum = 0.0;
    size_t coh_count = 0;
    for (size_t i = 0; i < STAR_NODE_COUNT; ++i) {
        for (size_t j = i + 1; j < STAR_NODE_COUNT; ++j) {
            coh_sum += coherence_matrix_[i][j];
            coh_count++;
        }
    }
    snap.mean_coherence = coh_count > 0
        ? coh_sum / static_cast<double>(coh_count)
        : 0.0;

    return snap;
}

std::string FreqRAM::status_string() const {
    auto snap = snapshot();

    std::ostringstream oss;
    oss << "═══════ fRAM - Frequenz-RAM Status ═══════\n";

    for (size_t t = 0; t < fram::TIER_COUNT; ++t) {
        oss << "  R" << (t + 1) << " " << std::left << std::setw(6)
            << fram::TIER_NAMES[t]
            << " " << std::right << std::setw(5) << snap.tier_cells[t]
            << " Zellen  E=" << std::fixed << std::setprecision(2)
            << snap.tier_energy[t] << "\n";
    }

    oss << "  ─────────────────────────────────────\n";
    oss << "  Total:     " << snap.total_cells << " / "
        << fram::TOTAL_SLOTS << " Zellen\n";
    oss << "  Energie:   " << std::fixed << std::setprecision(2)
        << snap.total_energy << "\n";
    oss << "  Kohaerenz: " << std::fixed << std::setprecision(3)
        << snap.mean_coherence << "\n";
    oss << "  Zyklen:    " << snap.cycle_count << "\n";
    oss << "  Store/Recall: " << snap.total_stores << " / "
        << snap.total_recalls << "\n";
    oss << "  Promote/Demote: " << snap.total_promotions << " / "
        << snap.total_demotions << "\n";
    oss << "═════════════════════════════════════════\n";

    return oss.str();
}

// ═══════════════════════════════════════════════════════════════════════════
// AETHER-BUS KOPPLUNG
// ═══════════════════════════════════════════════════════════════════════════
//
// fRAM registriert sich auf dem AetherBus:
//   Channel base+0:  Input-Frequenz (G1 Reflex)
//   Channel base+1:  Gesamt-Energie (G3 Emotion)
//   Channel base+2..base+10: Per-Theme Kohaerenz (G4 Ratio)
//
// ═══════════════════════════════════════════════════════════════════════════

void FreqRAM::couple_to_bus(AetherBus& bus, int base_channel) {
    bus_ = &bus;
    bus_base_channel_ = base_channel;

    // fRAM-Channels auf dem AetherBus registrieren
    bus.ensure_channel(base_channel);      // Input-Frequenz
    bus.ensure_channel(base_channel + 1);  // Gesamt-Energie
    for (size_t i = 0; i < STAR_NODE_COUNT; ++i) {
        bus.ensure_channel(base_channel + 2 + static_cast<int>(i));
    }
}

} // namespace rael
