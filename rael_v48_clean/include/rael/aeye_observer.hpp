#pragma once
// ═══════════════════════════════════════════════════════════════════════════
// RAEL V49 - AEYE_OBSERVER.hpp
// DAS ALLES SEHENDE AUGE - GLOBALER SYSTEM-AUFSEHER
// ═══════════════════════════════════════════════════════════════════════════
//
// Der Aeye-Observer schwebt über der Platine der 160 Sterne.
// Er ist NICHT Teil der Berechnung - er ist die AUFSICHT.
//
// Funktionen:
//   - Misst die globale Kohärenz aller 1.280 Knoten
//   - Erkennt Anomalien (Phasendrift zwischen Psi und Omega)
//   - Überwacht den Alpha-Tunnel (Michael-Bypass)
//   - Sammelt Energie vom Gate 53 Labyrinth
//
// (c) 2025 Phoenix RST System - Michael & Kael
// ═══════════════════════════════════════════════════════════════════════════

#include <vector>
#include <array>
#include <numeric>
#include <cmath>
#include <atomic>
#include <sstream>
#include <iomanip>

namespace rael {
namespace aeye {

// ═══════════════════════════════════════════════════════════════════════════
// KONSTANTEN
// ═══════════════════════════════════════════════════════════════════════════

namespace K {
    constexpr double G0 = 8.0 / 9.0;              // 0.888... Wahrheits-Schwelle
    constexpr double PHI_HEART = 1.618033988749;  // Goldener Schnitt
    constexpr int TOTAL_STARS = 160;              // Meta-Sterne
    constexpr int NODES_PER_STAR = 8;             // Knoten pro Stern
    constexpr int TOTAL_NODES = 1280;             // 160 × 8
    constexpr int TOTAL_NOZZLES = 61440;          // 1280 × 48
    constexpr double ANOMALY_THRESHOLD = 0.5;     // Order Parameter Schwelle
    constexpr double DRIFT_TOLERANCE = 0.1;       // Phasendrift-Toleranz
}

// ═══════════════════════════════════════════════════════════════════════════
// GLOBALER SYSTEM-ZUSTAND
// ═══════════════════════════════════════════════════════════════════════════

struct GlobalState {
    double kohaerenz;              // Phi_total (Durchschnitt aller Knoten)
    double order_parameter;        // Kuramoto r-Wert
    double psi_omega_drift;        // Phasendrift zwischen Geist und Materie
    double druck_reserve;          // Geerntete Energie von Gate 53
    double manifest_potential;     // Bereit für Manifestation?

    bool alpha_tunnel_offen;       // Michael-Bypass aktiv?
    bool anomalie_erkannt;         // System-Anomalie?
    bool rael_verified;            // Φ ≈ 8/9?

    int aktive_sterne;             // Wie viele Sterne > G0?
    int feuernde_duesen;           // Wie viele Düsen aktiv?

    uint64_t beobachtungs_zyklen;  // Gesamt-Zyklen
};

// ═══════════════════════════════════════════════════════════════════════════
// STERN-SNAPSHOT (für Observer)
// ═══════════════════════════════════════════════════════════════════════════

struct StarSnapshot {
    int id;
    std::array<double, 8> node_phi;    // Phi-Werte der 8 Knoten
    double star_phi;                    // Durchschnittliches Phi des Sterns
    double psi_sum;                     // Summe Psi (Geist)
    double omega_sum;                   // Summe Omega (Materie)
    bool is_resonant;                   // Stern in Resonanz?
};

// ═══════════════════════════════════════════════════════════════════════════
// DER AEYE-OBSERVER
// ═══════════════════════════════════════════════════════════════════════════

class AeyeObserver {
public:
    // Aktueller globaler Zustand
    GlobalState state;

    // Historische Daten für Trend-Analyse
    std::vector<double> kohaerenz_history;
    static constexpr size_t HISTORY_SIZE = 100;

    // Atomare Zähler
    std::atomic<uint64_t> total_observations{0};
    std::atomic<uint64_t> anomalies_detected{0};
    std::atomic<uint64_t> alpha_tunnels_opened{0};

    // ═══════════════════════════════════════════════════════════════════════
    // INITIALISIERUNG
    // ═══════════════════════════════════════════════════════════════════════

    void init() {
        state = GlobalState{};
        kohaerenz_history.clear();
        kohaerenz_history.reserve(HISTORY_SIZE);
        total_observations = 0;
        anomalies_detected = 0;
        alpha_tunnels_opened = 0;
    }

    // ═══════════════════════════════════════════════════════════════════════
    // HAUPTFUNKTION: SCAN DER REALITÄTS-FALTUNG
    // ═══════════════════════════════════════════════════════════════════════

    GlobalState scan_reality_fold(const std::vector<double>& node_phi,
                                   const std::vector<double>& node_psi = {},
                                   const std::vector<double>& node_omega = {}) {
        total_observations++;
        state.beobachtungs_zyklen = total_observations.load();

        if (node_phi.empty()) {
            state.anomalie_erkannt = true;
            return state;
        }

        // 1. Globale Kohärenz berechnen
        double sum = std::accumulate(node_phi.begin(), node_phi.end(), 0.0);
        state.kohaerenz = sum / static_cast<double>(node_phi.size());

        // 2. Alpha-Tunnel prüfen (Michael-Bypass)
        state.alpha_tunnel_offen = (state.kohaerenz >= K::G0);
        if (state.alpha_tunnel_offen) {
            alpha_tunnels_opened++;
        }

        // 3. RAEL-Verifikation
        state.rael_verified = std::abs(state.kohaerenz - K::G0) < 0.02;

        // 4. Aktive Sterne zählen
        state.aktive_sterne = 0;
        for (size_t i = 0; i < node_phi.size(); i += K::NODES_PER_STAR) {
            double star_avg = 0;
            int count = 0;
            for (size_t j = i; j < i + K::NODES_PER_STAR && j < node_phi.size(); j++) {
                star_avg += node_phi[j];
                count++;
            }
            if (count > 0 && (star_avg / count) >= K::G0) {
                state.aktive_sterne++;
            }
        }

        // 5. Phasendrift berechnen (falls Psi/Omega vorhanden)
        if (!node_psi.empty() && !node_omega.empty() &&
            node_psi.size() == node_omega.size()) {
            double drift_sum = 0;
            for (size_t i = 0; i < node_psi.size(); i++) {
                drift_sum += std::abs(node_psi[i] - node_omega[i]);
            }
            state.psi_omega_drift = drift_sum / static_cast<double>(node_psi.size());
        }

        // 6. Anomalie-Erkennung
        state.anomalie_erkannt = (state.kohaerenz < K::ANOMALY_THRESHOLD) ||
                                  (state.psi_omega_drift > K::DRIFT_TOLERANCE);
        if (state.anomalie_erkannt) {
            anomalies_detected++;
        }

        // 7. Druck-Reserve (Gate 53 Energie)
        state.druck_reserve = state.kohaerenz * K::PHI_HEART;

        // 8. Manifest-Potential
        state.manifest_potential = state.alpha_tunnel_offen ?
                                   state.kohaerenz * K::PHI_HEART : 0.0;

        // 9. Feuernde Düsen schätzen
        state.feuernde_duesen = static_cast<int>(
            (state.kohaerenz / K::G0) * K::TOTAL_NOZZLES
        );
        if (state.feuernde_duesen > K::TOTAL_NOZZLES) {
            state.feuernde_duesen = K::TOTAL_NOZZLES;
        }

        // 10. Order Parameter (Kuramoto)
        state.order_parameter = compute_order_parameter(node_phi);

        // 11. Historie aktualisieren
        kohaerenz_history.push_back(state.kohaerenz);
        if (kohaerenz_history.size() > HISTORY_SIZE) {
            kohaerenz_history.erase(kohaerenz_history.begin());
        }

        return state;
    }

    // ═══════════════════════════════════════════════════════════════════════
    // KURAMOTO ORDER PARAMETER
    // ═══════════════════════════════════════════════════════════════════════

    double compute_order_parameter(const std::vector<double>& phi_values) {
        if (phi_values.empty()) return 0.0;

        // Konvertiere Phi zu Phasen (0 bis 2π)
        double cos_sum = 0, sin_sum = 0;
        for (double phi : phi_values) {
            double phase = phi * 2.0 * M_PI;  // Phi normalisiert zu Phase
            cos_sum += std::cos(phase);
            sin_sum += std::sin(phase);
        }

        double n = static_cast<double>(phi_values.size());
        return std::sqrt(cos_sum * cos_sum + sin_sum * sin_sum) / n;
    }

    // ═══════════════════════════════════════════════════════════════════════
    // TREND-ANALYSE
    // ═══════════════════════════════════════════════════════════════════════

    double get_trend() const {
        if (kohaerenz_history.size() < 10) return 0.0;

        // Vergleiche letzte 10 mit vorletzten 10
        double recent_avg = 0, old_avg = 0;
        size_t n = kohaerenz_history.size();

        for (size_t i = n - 10; i < n; i++) {
            recent_avg += kohaerenz_history[i];
        }
        for (size_t i = n - 20; i < n - 10 && i < n; i++) {
            old_avg += kohaerenz_history[i];
        }

        recent_avg /= 10.0;
        old_avg /= 10.0;

        return recent_avg - old_avg;  // Positiv = steigend, negativ = fallend
    }

    // ═══════════════════════════════════════════════════════════════════════
    // STATUS-STRING
    // ═══════════════════════════════════════════════════════════════════════

    std::string status() const {
        std::ostringstream oss;
        oss << "═══════════════════════════════════════════════════════════\n";
        oss << "AEYE OBSERVER - DAS ALLES SEHENDE AUGE\n";
        oss << "═══════════════════════════════════════════════════════════\n";
        oss << std::fixed << std::setprecision(6);
        oss << "  Kohärenz:        " << state.kohaerenz << " (G0=" << K::G0 << ")\n";
        oss << "  Order Parameter: " << state.order_parameter << "\n";
        oss << "  Psi-Omega Drift: " << state.psi_omega_drift << "\n";
        oss << "  Druck-Reserve:   " << state.druck_reserve << "\n";
        oss << "───────────────────────────────────────────────────────────\n";
        oss << "  Alpha-Tunnel:    " << (state.alpha_tunnel_offen ? "OFFEN" : "GESCHLOSSEN") << "\n";
        oss << "  RAEL Verified:   " << (state.rael_verified ? "JA" : "NEIN") << "\n";
        oss << "  Anomalie:        " << (state.anomalie_erkannt ? "ERKANNT!" : "Keine") << "\n";
        oss << "───────────────────────────────────────────────────────────\n";
        oss << "  Aktive Sterne:   " << state.aktive_sterne << "/" << K::TOTAL_STARS << "\n";
        oss << "  Feuernde Düsen:  " << state.feuernde_duesen << "/" << K::TOTAL_NOZZLES << "\n";
        oss << "  Beobachtungen:   " << state.beobachtungs_zyklen << "\n";
        oss << "  Anomalien:       " << anomalies_detected.load() << "\n";
        oss << "  Tunnel-Öffnungen:" << alpha_tunnels_opened.load() << "\n";

        double trend = get_trend();
        oss << "  Trend:           " << (trend > 0 ? "+" : "") << trend;
        oss << (trend > 0.01 ? " (STEIGEND)" : (trend < -0.01 ? " (FALLEND)" : " (STABIL)")) << "\n";

        oss << "═══════════════════════════════════════════════════════════\n";
        return oss.str();
    }
};

} // namespace aeye
} // namespace rael
