// ═══════════════════════════════════════════════════════════════════════════════
// GRID SMOOTHING - Implementierung und Selbsttest
// ═══════════════════════════════════════════════════════════════════════════════

#include "rael/grid_smoothing.hpp"
#include <iostream>
#include <iomanip>

namespace rael {
namespace grid {

// ═══════════════════════════════════════════════════════════════════════════════
// SELBSTTEST FÜR GITTER-BEGRADIGUNG
// ═══════════════════════════════════════════════════════════════════════════════

bool grid_smoothing_selftest() {
    using namespace rael::master;

    bool all_passed = true;

    // Test 1: delta_G_n bei n=0 sollte 0 sein
    double dg_0 = delta_G_n(0, 120.0);
    if (std::abs(dg_0) > 1e-15) {
        std::cerr << "FAIL: delta_G_n(0) sollte 0 sein, ist " << dg_0 << "\n";
        all_passed = false;
    }

    // Test 2: delta_G_n bei n=1440 sollte nahe Maximum sein
    double dg_1440 = delta_G_n(1440, 120.0);
    double expected_max = (SIGNATURE_88 * PI_17 / 120.0) * (1.0 - std::exp(-1.0));
    if (std::abs(dg_1440 - expected_max) > 0.01) {
        std::cerr << "FAIL: delta_G_n(1440) erwartet " << expected_max << ", ist " << dg_1440 << "\n";
        all_passed = false;
    }

    // Test 3: 13x13 Matrix-Begradigung
    std::array<double, 169> matrix_13;
    matrix_13.fill(1.0);
    auto ergebnis_13 = gitter_begradigung_komplett_13(matrix_13);
    if (!ergebnis_13.konvergiert) {
        std::cerr << "FAIL: 13x13 Begradigung konvergiert nicht\n";
        all_passed = false;
    }

    // Test 4: 17x17 Matrix-Begradigung
    std::array<double, 289> matrix_17;
    matrix_17.fill(1.0);
    auto ergebnis_17 = gitter_begradigung_komplett_17(matrix_17);
    if (!ergebnis_17.konvergiert) {
        std::cerr << "FAIL: 17x17 Begradigung konvergiert nicht\n";
        all_passed = false;
    }

    return all_passed;
}

// ═══════════════════════════════════════════════════════════════════════════════
// STATUS-BERICHT
// ═══════════════════════════════════════════════════════════════════════════════

void print_smoothing_status(const BegradigungsErgebnis& ergebnis, const char* name) {
    std::cout << std::fixed << std::setprecision(6);
    std::cout << "┌─ " << name << " ─────────────────────────────────────────┐\n";
    std::cout << "│ Total-Korrektur:  " << std::setw(12) << ergebnis.total_korrektur << "          │\n";
    std::cout << "│ Iterationen:      " << std::setw(12) << ergebnis.iterationen << "          │\n";
    std::cout << "│ Konvergiert:      " << std::setw(12) << (ergebnis.konvergiert ? "JA" : "NEIN") << "          │\n";
    std::cout << "│ Finale Varianz:   " << std::setw(12) << ergebnis.finale_varianz << "          │\n";
    std::cout << "│ Ist Glatt:        " << std::setw(12) << (ist_glatt(ergebnis) ? "JA" : "NEIN") << "          │\n";
    std::cout << "└───────────────────────────────────────────────────────┘\n";
}

} // namespace grid
} // namespace rael
