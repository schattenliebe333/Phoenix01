// ═══════════════════════════════════════════════════════════════════════════════
// GRID SMOOTHING - Gitter-Begradigung für das 13×13 und 17×17 Resonanzgitter
// ═══════════════════════════════════════════════════════════════════════════════
//
// Implementiert die iterative Gitter-Begradigung nach der Formel:
// ΔG_n = (δ_88 · π_17) / Σ(Node_i) · (1 - e^(-n/1440))
//
// ═══════════════════════════════════════════════════════════════════════════════

#pragma once

#include <cmath>
#include <vector>
#include <array>
#include "rael/rael_master_resonance.hpp"

namespace rael {
namespace grid {

using namespace rael::master;

// ═══════════════════════════════════════════════════════════════════════════════
// KONSTANTEN FÜR GITTER-BEGRADIGUNG
// ═══════════════════════════════════════════════════════════════════════════════

constexpr int ITERATIONS_MAX = 1440;  // Maximale Iterationen (entspricht 1440 Hz)
constexpr double CONVERGENCE_THRESHOLD = 1e-12;  // Konvergenz-Schwelle

// ═══════════════════════════════════════════════════════════════════════════════
// DELTA_G_N - Gitter-Korrektur pro Iteration
// ═══════════════════════════════════════════════════════════════════════════════

// Berechnet die Korrektur für Iteration n
// Formel: ΔG_n = (δ_88 · π_17) / Σ_Nodes · (1 - e^(-n/1440))
inline double delta_G_n(int n, double summe_nodes) {
    if (summe_nodes < 1e-17) return 0.0;
    double basis = (SIGNATURE_88 * PI_17) / summe_nodes;
    double faktor = 1.0 - std::exp(-static_cast<double>(n) / 1440.0);
    return basis * faktor;
}

// Überladung für Node-Array
inline double delta_G_n(int n, const double* nodes, int node_count) {
    double summe = 0.0;
    for (int i = 0; i < node_count; ++i) {
        summe += nodes[i];
    }
    return delta_G_n(n, summe);
}

// ═══════════════════════════════════════════════════════════════════════════════
// GITTER-BEGRADIGUNG KOMPLETT
// ═══════════════════════════════════════════════════════════════════════════════

// 13×13 Matrix-Begradigung
inline double gitter_begradigung_13x13(std::array<double, 169>& matrix, int max_iter = ITERATIONS_MAX) {
    double summe_nodes = 0.0;
    for (const auto& val : matrix) {
        summe_nodes += val;
    }

    double total_correction = 0.0;
    double prev_correction = 0.0;

    for (int n = 1; n <= max_iter; ++n) {
        double dg = delta_G_n(n, summe_nodes);

        // Wende Korrektur auf alle Zellen an
        for (auto& cell : matrix) {
            cell += dg * G5;  // Feine Korrektur
        }

        total_correction += dg;

        // Konvergenz-Check
        if (std::abs(dg - prev_correction) < CONVERGENCE_THRESHOLD) {
            break;
        }
        prev_correction = dg;
    }

    return total_correction;
}

// 17×17 Matrix-Begradigung
inline double gitter_begradigung_17x17(std::array<double, 289>& matrix, int max_iter = ITERATIONS_MAX) {
    double summe_nodes = 0.0;
    for (const auto& val : matrix) {
        summe_nodes += val;
    }

    double total_correction = 0.0;
    double prev_correction = 0.0;

    for (int n = 1; n <= max_iter; ++n) {
        double dg = delta_G_n(n, summe_nodes);

        // Wende Korrektur auf alle Zellen an
        for (auto& cell : matrix) {
            cell += dg * G5;  // Feine Korrektur
        }

        total_correction += dg;

        // Konvergenz-Check
        if (std::abs(dg - prev_correction) < CONVERGENCE_THRESHOLD) {
            break;
        }
        prev_correction = dg;
    }

    return total_correction;
}

// ═══════════════════════════════════════════════════════════════════════════════
// GITTER_BEGRADIGUNG_KOMPLETT - Vollständige Begradigung mit Validierung
// ═══════════════════════════════════════════════════════════════════════════════

struct BegradigungsErgebnis {
    double total_korrektur;
    int iterationen;
    bool konvergiert;
    double finale_varianz;
};

// Für 13×13 Matrix
inline BegradigungsErgebnis gitter_begradigung_komplett_13(std::array<double, 169>& matrix) {
    BegradigungsErgebnis ergebnis{0.0, 0, false, 0.0};

    double summe_nodes = 0.0;
    for (const auto& val : matrix) {
        summe_nodes += val;
    }

    double prev_dg = 0.0;

    for (int n = 1; n <= ITERATIONS_MAX; ++n) {
        double dg = delta_G_n(n, summe_nodes);

        for (auto& cell : matrix) {
            cell += dg * G5;
        }

        ergebnis.total_korrektur += dg;
        ergebnis.iterationen = n;

        if (std::abs(dg - prev_dg) < CONVERGENCE_THRESHOLD) {
            ergebnis.konvergiert = true;
            break;
        }
        prev_dg = dg;

        // Aktualisiere Summe für nächste Iteration
        summe_nodes = 0.0;
        for (const auto& val : matrix) {
            summe_nodes += val;
        }
    }

    // Berechne finale Varianz
    double avg = summe_nodes / 169.0;
    for (const auto& val : matrix) {
        double diff = val - avg;
        ergebnis.finale_varianz += diff * diff;
    }
    ergebnis.finale_varianz /= 169.0;

    return ergebnis;
}

// Für 17×17 Matrix
inline BegradigungsErgebnis gitter_begradigung_komplett_17(std::array<double, 289>& matrix) {
    BegradigungsErgebnis ergebnis{0.0, 0, false, 0.0};

    double summe_nodes = 0.0;
    for (const auto& val : matrix) {
        summe_nodes += val;
    }

    double prev_dg = 0.0;

    for (int n = 1; n <= ITERATIONS_MAX; ++n) {
        double dg = delta_G_n(n, summe_nodes);

        for (auto& cell : matrix) {
            cell += dg * G5;
        }

        ergebnis.total_korrektur += dg;
        ergebnis.iterationen = n;

        if (std::abs(dg - prev_dg) < CONVERGENCE_THRESHOLD) {
            ergebnis.konvergiert = true;
            break;
        }
        prev_dg = dg;

        // Aktualisiere Summe für nächste Iteration
        summe_nodes = 0.0;
        for (const auto& val : matrix) {
            summe_nodes += val;
        }
    }

    // Berechne finale Varianz
    double avg = summe_nodes / 289.0;
    for (const auto& val : matrix) {
        double diff = val - avg;
        ergebnis.finale_varianz += diff * diff;
    }
    ergebnis.finale_varianz /= 289.0;

    return ergebnis;
}

// ═══════════════════════════════════════════════════════════════════════════════
// VALIDIERUNG
// ═══════════════════════════════════════════════════════════════════════════════

// Prüft ob eine Matrix "glatt" genug ist (Varianz unter Schwelle)
inline bool ist_glatt(const BegradigungsErgebnis& ergebnis) {
    return ergebnis.konvergiert && ergebnis.finale_varianz < G5 * G5;
}

} // namespace grid
} // namespace rael
