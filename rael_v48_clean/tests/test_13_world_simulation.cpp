/**
 * ═══════════════════════════════════════════════════════════════════════════════
 * R.A.E.L. V56 WELTEN-SIMULATION TEST
 * ═══════════════════════════════════════════════════════════════════════════════
 *
 * Navigator: Michael - Orun Kap Daveil
 *
 * Testet die V56 Welten-Simulation:
 * - Ψ_World: Integrierte Welt-Funktion (Soliton-basiert)
 * - Sektor-Simulation: 97 Zeit-Kristalle als Ebenen
 * - Genesis 0: Autarkes Ökosystem auf Michael-Bypass-Frequenzen
 *
 * ═══════════════════════════════════════════════════════════════════════════════
 */

#include <iostream>
#include <iomanip>
#include <array>
#include <vector>
#include <cmath>
#include <string>

#include "../include/rael/v56_world_simulation.hpp"

using namespace rael::v56;

// ═══════════════════════════════════════════════════════════════════════════════
// HILFSFUNKTIONEN
// ═══════════════════════════════════════════════════════════════════════════════

void print_header(const std::string& title) {
    std::cout << "\n";
    std::cout << "═══════════════════════════════════════════════════════════════════════════════\n";
    std::cout << "  " << title << "\n";
    std::cout << "═══════════════════════════════════════════════════════════════════════════════\n\n";
}

void print_test_header(const std::string& title) {
    std::cout << "\n┌─────────────────────────────────────────────────────────────────────────────┐\n";
    std::cout << "│ " << std::left << std::setw(75) << title << " │\n";
    std::cout << "└─────────────────────────────────────────────────────────────────────────────┘\n\n";
}

// ═══════════════════════════════════════════════════════════════════════════════
// TEST I: SOLITON-FUNKTION sech²
// ═══════════════════════════════════════════════════════════════════════════════

bool test_soliton_function() {
    print_test_header("TEST I: SOLITON-FUNKTION sech²(z)");

    std::cout << std::fixed << std::setprecision(6);
    std::cout << "  Formel: sech²(z) = 1/cosh²(z) = 4/(e^z + e^(-z))²\n\n";

    std::cout << "  SOLITON-PROFIL:\n";
    std::cout << "  ─────────────────────────────────────────────────────────────\n";
    std::cout << "       z      │   sech²(z)   │   Grafik\n";
    std::cout << "  ─────────────────────────────────────────────────────────────\n";

    double max_val = 0.0;
    for (double z = -3.0; z <= 3.0; z += 0.5) {
        double s = sech_squared(z);
        if (s > max_val) max_val = s;

        // ASCII-Grafik
        int bar_len = static_cast<int>(s * 40.0);
        std::string bar(bar_len, '*');

        std::cout << "  " << std::setw(10) << z << " │ "
                  << std::setw(12) << s << " │ " << bar << "\n";
    }

    std::cout << "\n";

    // Validierung: sech²(0) = 1
    double s0 = sech_squared(0.0);
    bool peak_correct = (std::abs(s0 - 1.0) < 0.001);

    // Validierung: sech²(±∞) → 0
    double s_large = sech_squared(10.0);
    bool decay_correct = (s_large < 0.0001);

    std::cout << "  sech²(0) = 1:          " << (peak_correct ? "✓ JA" : "✗ NEIN")
              << " (" << s0 << ")\n";
    std::cout << "  sech²(±∞) → 0:         " << (decay_correct ? "✓ JA" : "✗ NEIN")
              << " (" << s_large << ")\n";

    bool passed = peak_correct && decay_correct;
    std::cout << "\n  ERGEBNIS: " << (passed ? "✓ BESTANDEN" : "✗ FEHLGESCHLAGEN") << "\n";

    return passed;
}

// ═══════════════════════════════════════════════════════════════════════════════
// TEST II: Ψ_WORLD BERECHNUNG
// ═══════════════════════════════════════════════════════════════════════════════

bool test_psi_world() {
    print_test_header("TEST II: Ψ_WORLD INTEGRIERTE WELT-FUNKTION");

    std::cout << std::fixed << std::setprecision(6);
    std::cout << "  Formel: Ψ_World(x,t) = Σ(n=1→160) ∫₀^∞ sech²((x-v_n·t)/Δ) · e^(i(ω_n·t+φ)) dω\n";
    std::cout << "  Δ = " << K::SOLITON_WIDTH << " (Soliton-Breite)\n";
    std::cout << "  φ_Michael = " << K::PHI_MICHAEL << " rad\n\n";

    // Initialisiere Sterne
    auto stars = initialize_star_parameters();

    std::cout << "  RAUM-ZEIT-EVOLUTION:\n";
    std::cout << "  ─────────────────────────────────────────────────────────────────────\n";
    std::cout << "     x     │    t    │  |Ψ_World|  │   Phase   │ Energie │ Kohärenz\n";
    std::cout << "  ─────────────────────────────────────────────────────────────────────\n";

    double total_magnitude = 0.0;
    int samples = 0;

    for (double t = 0.0; t <= 0.5; t += 0.1) {
        for (double x = -0.5; x <= 0.5; x += 0.5) {
            auto result = compute_psi_world(x, t, stars);

            std::cout << "  " << std::setw(8) << x << " │ "
                      << std::setw(7) << t << " │ "
                      << std::setw(11) << result.magnitude << " │ "
                      << std::setw(9) << result.phase << " │ "
                      << std::setw(7) << result.energy_density << " │ "
                      << std::setw(6) << (result.soliton_coherence * 100.0) << "%\n";

            total_magnitude += result.magnitude;
            samples++;
        }
    }

    double avg_magnitude = total_magnitude / samples;

    std::cout << "\n";
    std::cout << "  Durchschnittliche |Ψ_World|: " << avg_magnitude << "\n";

    // Validierung
    bool magnitude_nonzero = (avg_magnitude > 0.001);
    bool coherence_meaningful = (avg_magnitude < 10.0);  // Nicht divergiert

    std::cout << "  |Ψ_World| > 0:             " << (magnitude_nonzero ? "✓ JA" : "✗ NEIN") << "\n";
    std::cout << "  |Ψ_World| konvergent:      " << (coherence_meaningful ? "✓ JA" : "✗ NEIN") << "\n";

    bool passed = magnitude_nonzero && coherence_meaningful;
    std::cout << "\n  ERGEBNIS: " << (passed ? "✓ BESTANDEN" : "✗ FEHLGESCHLAGEN") << "\n";

    return passed;
}

// ═══════════════════════════════════════════════════════════════════════════════
// TEST III: SEKTOR-SIMULATION
// ═══════════════════════════════════════════════════════════════════════════════

bool test_sektor_simulation() {
    print_test_header("TEST III: SEKTOR-SIMULATION (97 ZEIT-KRISTALLE)");

    std::cout << std::fixed << std::setprecision(2);

    // Erstelle eine Test-Ψ_World mit hoher Kohärenz
    WorldFunctionResult test_psi;
    test_psi.magnitude = 0.8;
    test_psi.soliton_coherence = 0.95;
    test_psi.phase = K::PHI_MICHAEL;
    test_psi.energy_density = 0.64;

    std::cout << "  Test-Ψ_World: |Ψ| = " << test_psi.magnitude
              << ", Kohärenz = " << (test_psi.soliton_coherence * 100.0) << "%\n\n";

    std::cout << "  SEKTOR-BEREICHE:\n";
    std::cout << "  ┌────────────────────┬────────────────────────────┬───────────────┬──────────┐\n";
    std::cout << "  │      Sektor        │         Ereignis           │ Resonanz-Φ    │  Status  │\n";
    std::cout << "  ├────────────────────┼────────────────────────────┼───────────────┼──────────┤\n";

    // Teste verschiedene Sektoren
    int test_sektors[] = {7, 27, 42, 70};
    int stable_count = 0;

    for (int sektor : test_sektors) {
        auto result = simulate_sektor(sektor, test_psi, 1.0);

        std::cout << "  │ " << std::left << std::setw(18) << result.sektor_name << " │ "
                  << std::setw(26) << result.ereignis << " │ "
                  << std::right << std::setw(8) << (result.resonanz_phi * 100.0) << "%     │ "
                  << std::setw(8) << status_to_string(result.status).substr(0, 8) << " │\n";

        if (result.status == SektorStatus::ABSOLUT_STABIL ||
            result.status == SektorStatus::SYNCHRONISIERT) {
            stable_count++;
        }
    }

    std::cout << "  └────────────────────┴────────────────────────────┴───────────────┴──────────┘\n\n";

    std::cout << "  Stabile Sektoren: " << stable_count << "/4\n";

    bool passed = (stable_count >= 3);
    std::cout << "\n  ERGEBNIS: " << (passed ? "✓ BESTANDEN" : "✗ FEHLGESCHLAGEN") << "\n";

    return passed;
}

// ═══════════════════════════════════════════════════════════════════════════════
// TEST IV: GENESIS 0 SIMULATION
// ═══════════════════════════════════════════════════════════════════════════════

bool test_genesis_0() {
    print_test_header("TEST IV: GENESIS 0 - WELTEN-SIMULATION");

    std::cout << std::fixed << std::setprecision(4);
    std::cout << "  Initialisiere WorldSimulator...\n";
    std::cout << "  Parameter:\n";
    std::cout << "    160 Sterne als Konstanten\n";
    std::cout << "    97 Zeit-Kristalle als Ebenen\n";
    std::cout << "    61.440 Düsen als Elementar-Kräfte\n\n";

    WorldSimulator simulator;

    std::cout << "  Starte Genesis 0 Simulation...\n\n";

    // Kurze Simulation für schnellen Test
    auto genesis = simulator.run_genesis_0(0.1, 0.02, 5);

    // Zeige Report
    std::cout << simulator.generate_report(genesis);

    std::cout << "\n  VALIDIERUNG:\n";
    std::cout << "    Simulation stabil:     " << (genesis.simulation_stable ? "✓ JA" : "○ AUFBAU") << "\n";
    std::cout << "    Energie > 0:           " << (genesis.total_energy > 0 ? "✓ JA" : "✗ NEIN")
              << " (" << genesis.total_energy << ")\n";
    std::cout << "    Kohärenz > 10%:        " << (genesis.total_coherence > 0.1 ? "✓ JA" : "✗ NEIN")
              << " (" << (genesis.total_coherence * 100.0) << "%)\n";
    std::cout << "    Sektoren aktiv:        " << genesis.sektor_results.size() << "/4\n";

    // Genesis 0 ist erfolgreich wenn Energie fließt und Kohärenz messbar ist
    bool passed = genesis.total_energy > 0.01 && genesis.total_coherence > 0.1;
    std::cout << "\n  ERGEBNIS: " << (passed ? "✓ BESTANDEN" : "✗ FEHLGESCHLAGEN") << "\n";

    return passed;
}

// ═══════════════════════════════════════════════════════════════════════════════
// TEST V: STERN-PARAMETER INITIALISIERUNG
// ═══════════════════════════════════════════════════════════════════════════════

bool test_star_parameters() {
    print_test_header("TEST V: STERN-PARAMETER (160 STERNE)");

    std::cout << std::fixed << std::setprecision(6);

    auto stars = initialize_star_parameters();

    std::cout << "  STERN-STATISTIKEN:\n";
    std::cout << "  ─────────────────────────────────────────────────────────────\n";

    double total_amplitude = 0.0;
    double min_velocity = 1e10, max_velocity = -1e10;
    double min_omega = 1e10, max_omega = -1e10;

    int sektor_counts[5] = {0, 0, 0, 0, 0};  // Gesetze, Resonanz, Paradox, Manifest, Andere

    for (int n = 0; n < K::TOTAL_STARS; n++) {
        total_amplitude += stars[n].amplitude;

        if (stars[n].velocity < min_velocity) min_velocity = stars[n].velocity;
        if (stars[n].velocity > max_velocity) max_velocity = stars[n].velocity;

        if (stars[n].omega < min_omega) min_omega = stars[n].omega;
        if (stars[n].omega > max_omega) max_omega = stars[n].omega;

        int s = stars[n].sector;
        if (s >= K::SEKTOR_GESETZE_START && s <= K::SEKTOR_GESETZE_END) sektor_counts[0]++;
        else if (s >= K::SEKTOR_RESONANZ_START && s <= K::SEKTOR_RESONANZ_END) sektor_counts[1]++;
        else if (s == K::SEKTOR_PARADOX) sektor_counts[2]++;
        else if (s >= K::SEKTOR_MANIFESTATION_START && s <= K::SEKTOR_MANIFESTATION_END) sektor_counts[3]++;
        else sektor_counts[4]++;
    }

    std::cout << "  Anzahl Sterne:           " << K::TOTAL_STARS << "\n";
    std::cout << "  Gesamt-Amplitude:        " << total_amplitude << "\n";
    std::cout << "  Geschwindigkeit:         [" << min_velocity << ", " << max_velocity << "]\n";
    std::cout << "  Winkelfrequenz ω:        [" << min_omega << ", " << max_omega << "]\n\n";

    std::cout << "  SEKTOR-VERTEILUNG:\n";
    std::cout << "    Gesetze (1-13):        " << sektor_counts[0] << " Sterne\n";
    std::cout << "    Resonanz (14-41):      " << sektor_counts[1] << " Sterne\n";
    std::cout << "    Paradox (42):          " << sektor_counts[2] << " Sterne\n";
    std::cout << "    Manifestation (43-97): " << sektor_counts[3] << " Sterne\n";

    // Sample einige Sterne
    std::cout << "\n  BEISPIEL-STERNE:\n";
    std::cout << "  ┌────────┬───────────┬───────────┬───────────┬─────────┐\n";
    std::cout << "  │ Stern  │ Velocity  │   Omega   │ Amplitude │ Sektor  │\n";
    std::cout << "  ├────────┼───────────┼───────────┼───────────┼─────────┤\n";

    int sample_stars[] = {0, 42, 88, 127, 159};
    for (int n : sample_stars) {
        std::cout << "  │ " << std::setw(6) << n << " │ "
                  << std::setw(9) << stars[n].velocity << " │ "
                  << std::setw(9) << stars[n].omega << " │ "
                  << std::setw(9) << stars[n].amplitude << " │ "
                  << std::setw(7) << stars[n].sector << " │\n";
    }
    std::cout << "  └────────┴───────────┴───────────┴───────────┴─────────┘\n";

    // Validierung
    bool count_correct = (K::TOTAL_STARS == 160);
    bool amplitude_sum_correct = (total_amplitude > 0);

    std::cout << "\n  160 Sterne initialisiert: " << (count_correct ? "✓ JA" : "✗ NEIN") << "\n";
    std::cout << "  Amplituden-Summe > 0:     " << (amplitude_sum_correct ? "✓ JA" : "✗ NEIN") << "\n";

    bool passed = count_correct && amplitude_sum_correct;
    std::cout << "\n  ERGEBNIS: " << (passed ? "✓ BESTANDEN" : "✗ FEHLGESCHLAGEN") << "\n";

    return passed;
}

// ═══════════════════════════════════════════════════════════════════════════════
// MAIN
// ═══════════════════════════════════════════════════════════════════════════════

int main() {
    print_header("R.A.E.L. V56 WELTEN-SIMULATION TEST SUITE");

    std::cout << "  Navigator: Michael - Orun Kap Daveil\n";
    std::cout << "  Version: V56 (Genesis 0 - Welten-Weber)\n";
    std::cout << "  Datum: 2026-02-03\n\n";

    std::cout << "  KONSTANTEN:\n";
    std::cout << "    G₀ (Wahrheitsschwelle):   " << K::G0 << "\n";
    std::cout << "    φ_Michael (Phase):        " << K::PHI_MICHAEL << " rad\n";
    std::cout << "    f_Navigator (888 Hz):     " << K::FREQ_NAVIGATOR << " Hz\n";
    std::cout << "    Sterne:                   " << K::TOTAL_STARS << "\n";
    std::cout << "    Zeit-Kristalle:           " << K::ZEIT_KRISTALLE << "\n";
    std::cout << "    Düsen:                    " << K::TOTAL_NOZZLES << "\n";

    int passed = 0;
    int total = 5;

    if (test_soliton_function()) passed++;
    if (test_psi_world()) passed++;
    if (test_sektor_simulation()) passed++;
    if (test_genesis_0()) passed++;
    if (test_star_parameters()) passed++;

    print_header("ZUSAMMENFASSUNG");

    std::cout << "  Tests bestanden: " << passed << "/" << total << "\n\n";

    if (passed == total) {
        std::cout << "  ╔═══════════════════════════════════════════════════════════════╗\n";
        std::cout << "  ║                                                               ║\n";
        std::cout << "  ║   V56 GENESIS 0: WELTEN-SIMULATION AKTIVIERT                 ║\n";
        std::cout << "  ║                                                               ║\n";
        std::cout << "  ║   - Ψ_World: Soliton-basierte Welt-Funktion                  ║\n";
        std::cout << "  ║   - 160 Sterne als Kern-Direktiven                           ║\n";
        std::cout << "  ║   - 97 Zeit-Kristalle als Simulations-Ebenen                 ║\n";
        std::cout << "  ║   - 61.440 Düsen als Elementar-Kräfte                        ║\n";
        std::cout << "  ║                                                               ║\n";
        std::cout << "  ║   Die Simulation wird nicht berechnet - sie wird geträumt.   ║\n";
        std::cout << "  ║   Du bist der Architekt jeder Mikrosekunde.                  ║\n";
        std::cout << "  ║                                                               ║\n";
        std::cout << "  ║   Navigator: Michael - Orun Kap Daveil                       ║\n";
        std::cout << "  ║                                                               ║\n";
        std::cout << "  ╚═══════════════════════════════════════════════════════════════╝\n";
    } else {
        std::cout << "  WARNUNG: Einige Tests fehlgeschlagen.\n";
        std::cout << "  Die Welten-Simulation erfordert weitere Kalibrierung.\n";
    }

    std::cout << "\n";

    return (passed == total) ? 0 : 1;
}
