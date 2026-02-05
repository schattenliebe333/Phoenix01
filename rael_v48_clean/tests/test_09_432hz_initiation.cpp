/**
 * ═══════════════════════════════════════════════════════════════════════════════
 * R.A.E.L. V52 - TEST 09: 432 Hz INITIATION
 * ═══════════════════════════════════════════════════════════════════════════════
 *
 * Navigator: Michael - Orun Kap Daveil
 * Datum: 2026-02-03
 *
 * TEST DER RESONANZ-AMPLITUDE FORMEL:
 *
 *              160    ( sin(2πft + φ_n) )
 * A_R(f) =  Σ        ( ──────────────── )
 *           n=1      (   √(G₀ - Φ_n)    )
 *
 * Wobei G₀ = 8/9 die Wahrheitsschwelle am 0-Falz darstellt.
 *
 * TESTS:
 * I.   Resonanz-Amplitude Berechnung
 * II.  432 Hz Matrix-Reinigung
 * III. Zeit-Kristall Harmonisierung
 * IV.  Dissonanz-Eliminierung
 *
 * ═══════════════════════════════════════════════════════════════════════════════
 */

#include <iostream>
#include <iomanip>
#include <string>
#include <chrono>
#include <cmath>
#include <array>
#include <random>

#include "rael/v52_frequency_interface.hpp"

using namespace rael::v52;

// ═══════════════════════════════════════════════════════════════════════════════
// HILFSFUNKTIONEN
// ═══════════════════════════════════════════════════════════════════════════════

void print_header() {
    std::cout << "═══════════════════════════════════════════════════════════════════════════════\n";
    std::cout << "  ██╗  ██╗██████╗ ██████╗     ██╗  ██╗███████╗\n";
    std::cout << "  ██║  ██║╚════██╗╚════██╗    ██║  ██║╚══███╔╝\n";
    std::cout << "  ███████║ █████╔╝ █████╔╝    ███████║  ███╔╝ \n";
    std::cout << "  ╚════██║ ╚═══██╗██╔═══╝     ██╔══██║ ███╔╝  \n";
    std::cout << "       ██║██████╔╝███████╗    ██║  ██║███████╗\n";
    std::cout << "       ╚═╝╚═════╝ ╚══════╝    ╚═╝  ╚═╝╚══════╝\n";
    std::cout << "\n";
    std::cout << "  432 Hz INITIATION - MATRIX REINIGUNG\n";
    std::cout << "  Navigator: Michael - Orun Kap Daveil\n";
    std::cout << "  Resonanz-Amplitude: A_R(f) = Σ[sin(2πft + φ_n) / √(G₀ - Φ_n)]\n";
    std::cout << "═══════════════════════════════════════════════════════════════════════════════\n\n";
}

void print_test_header(const std::string& title) {
    std::cout << "\n╔═══════════════════════════════════════════════════════════════════╗\n";
    std::cout << "║  " << std::left << std::setw(64) << title << "║\n";
    std::cout << "╚═══════════════════════════════════════════════════════════════════╝\n\n";
}

// ═══════════════════════════════════════════════════════════════════════════════
// TEST I: RESONANZ-AMPLITUDE BERECHNUNG
// ═══════════════════════════════════════════════════════════════════════════════

bool test_resonance_amplitude() {
    print_test_header("TEST I: RESONANZ-AMPLITUDE BERECHNUNG");

    // Initialisiere 160 Sterne mit zufälligen Phasen und Kohärenzen
    std::array<double, K::TOTAL_STARS> phases;
    std::array<double, K::TOTAL_STARS> coherences;

    std::mt19937 rng(42);  // Reproduzierbar
    std::uniform_real_distribution<double> phase_dist(0.0, 2.0 * M_PI);
    std::uniform_real_distribution<double> coherence_dist(0.3, 0.85);  // Unter G₀

    for (int n = 0; n < K::TOTAL_STARS; n++) {
        phases[n] = phase_dist(rng);
        coherences[n] = coherence_dist(rng);
    }

    // Berechne A_R bei t=0
    auto result_t0 = compute_resonance_amplitude(K::NATURAL_TONE, 0.0, phases, coherences);

    // Berechne A_R bei t=0.00025 (nicht-ganzzahliger Zyklus: 432 × 0.00025 = 0.108 Zyklen)
    auto result_t05 = compute_resonance_amplitude(K::NATURAL_TONE, 0.00025, phases, coherences);

    // Berechne A_R bei t=0.0005
    auto result_t1 = compute_resonance_amplitude(K::NATURAL_TONE, 0.0005, phases, coherences);

    std::cout << std::fixed << std::setprecision(4);
    std::cout << "  Frequenz:              " << K::NATURAL_TONE << " Hz\n";
    std::cout << "  G₀ (Wahrheitsschwelle): " << K::G0 << "\n\n";

    std::cout << "  t = 0.0s:\n";
    std::cout << "    A_R:                 " << result_t0.A_R << "\n";
    std::cout << "    Max Stern-Beitrag:   " << result_t0.max_star_contribution << " (Stern #" << result_t0.max_star_id << ")\n";
    std::cout << "    Ø Kohärenz:          " << result_t0.avg_coherence << "\n";
    std::cout << "    Dissonante Sterne:   " << result_t0.dissonant_stars << "/" << K::TOTAL_STARS << "\n\n";

    std::cout << "  t = 0.00025s (0.108 Zyklen):\n";
    std::cout << "    A_R:                 " << result_t05.A_R << "\n";
    std::cout << "    Max Stern-Beitrag:   " << result_t05.max_star_contribution << "\n\n";

    std::cout << "  t = 0.0005s (0.216 Zyklen):\n";
    std::cout << "    A_R:                 " << result_t1.A_R << "\n";
    std::cout << "    Max Stern-Beitrag:   " << result_t1.max_star_contribution << "\n\n";

    // Validierung
    bool amplitude_varies = (std::abs(result_t0.A_R - result_t05.A_R) > 1.0);
    bool coherence_valid = (result_t0.avg_coherence > 0.0 && result_t0.avg_coherence < K::G0);
    bool dissonance_detected = (result_t0.dissonant_stars > 0);

    std::cout << "  Amplitude variiert über Zeit: " << (amplitude_varies ? "✓ JA" : "✗ NEIN") << "\n";
    std::cout << "  Kohärenz im gültigen Bereich: " << (coherence_valid ? "✓ JA" : "✗ NEIN") << "\n";
    std::cout << "  Dissonanz erkannt:            " << (dissonance_detected ? "✓ JA" : "✗ NEIN") << "\n";

    bool passed = amplitude_varies && coherence_valid && dissonance_detected;
    std::cout << "\n  ERGEBNIS: " << (passed ? "✓ BESTANDEN" : "✗ FEHLGESCHLAGEN") << "\n";

    return passed;
}

// ═══════════════════════════════════════════════════════════════════════════════
// TEST II: 432 Hz MATRIX-REINIGUNG
// ═══════════════════════════════════════════════════════════════════════════════

bool test_432hz_healing() {
    print_test_header("TEST II: 432 Hz MATRIX-REINIGUNG");

    // Initialisiere Sterne mit niedriger Kohärenz (dissonant)
    std::array<StarResonance, K::TOTAL_STARS> stars;

    std::mt19937 rng(123);
    std::uniform_real_distribution<double> phase_dist(0.0, 2.0 * M_PI);
    std::uniform_real_distribution<double> amp_dist(0.1, 0.5);  // Niedrige Amplituden

    double initial_avg_amp = 0.0;
    for (int n = 0; n < K::TOTAL_STARS; n++) {
        stars[n].id = n;
        stars[n].natural_frequency = K::NATURAL_TONE * std::pow(K::PHI, (double)n / 40.0);
        stars[n].current_phase = phase_dist(rng);
        stars[n].amplitude = amp_dist(rng);
        stars[n].coupling_strength = K::G0;
        stars[n].is_excited = false;
        initial_avg_amp += stars[n].amplitude;
    }
    initial_avg_amp /= K::TOTAL_STARS;

    std::cout << std::fixed << std::setprecision(4);
    std::cout << "  VORHER (dissonanter Zustand):\n";
    std::cout << "    Ø Amplitude:         " << initial_avg_amp << "\n";

    // Zähle angeregte Sterne vorher
    int initial_excited = 0;
    for (const auto& s : stars) {
        if (s.is_excited) initial_excited++;
    }
    std::cout << "    Angeregte Sterne:    " << initial_excited << "/" << K::TOTAL_STARS << "\n\n";

    // 432 Hz Initiation durchführen
    auto start = std::chrono::high_resolution_clock::now();
    double final_phi = initiate_432hz_healing(stars, 1.0, 100);
    auto end = std::chrono::high_resolution_clock::now();

    double duration_ms = std::chrono::duration<double, std::milli>(end - start).count();

    // Berechne finale Statistiken
    double final_avg_amp = 0.0;
    int final_excited = 0;
    for (const auto& s : stars) {
        final_avg_amp += s.amplitude;
        if (s.is_excited) final_excited++;
    }
    final_avg_amp /= K::TOTAL_STARS;

    std::cout << "  NACHHER (harmonisierter Zustand):\n";
    std::cout << "    Ø Amplitude:         " << final_avg_amp << "\n";
    std::cout << "    Angeregte Sterne:    " << final_excited << "/" << K::TOTAL_STARS << "\n";
    std::cout << "    Finale Kohärenz Φ:   " << final_phi << "\n\n";

    std::cout << "  HEILUNGS-METRIKEN:\n";
    std::cout << "    Amplitude-Steigerung: " << (final_avg_amp / initial_avg_amp) << "x\n";
    std::cout << "    Stern-Aktivierung:    " << (final_excited - initial_excited) << " Sterne\n";
    std::cout << "    Dauer:                " << duration_ms << " ms\n\n";

    // Validierung
    bool amplitude_increased = (final_avg_amp > initial_avg_amp);
    bool coherence_high = (final_phi > 0.8);
    bool stars_excited = (final_excited > initial_excited);

    std::cout << "  Amplitude gestiegen:   " << (amplitude_increased ? "✓ JA" : "✗ NEIN") << "\n";
    std::cout << "  Hohe Kohärenz (>0.8):  " << (coherence_high ? "✓ JA" : "✗ NEIN") << "\n";
    std::cout << "  Sterne aktiviert:      " << (stars_excited ? "✓ JA" : "✗ NEIN") << "\n";

    bool passed = amplitude_increased && coherence_high && stars_excited;
    std::cout << "\n  ERGEBNIS: " << (passed ? "✓ BESTANDEN" : "✗ FEHLGESCHLAGEN") << "\n";

    return passed;
}

// ═══════════════════════════════════════════════════════════════════════════════
// TEST III: FREQUENZ-MAPPING EFFEKTE
// ═══════════════════════════════════════════════════════════════════════════════

bool test_frequency_mapping() {
    print_test_header("TEST III: FREQUENZ-MAPPING EFFEKTE");

    std::cout << "  SPEZIAL-FREQUENZEN UND IHRE EFFEKTE:\n\n";

    struct TestFreq {
        double freq;
        const char* name;
    };

    TestFreq test_freqs[] = {
        {30.0,   "Delta"},
        {200.0,  "Theta"},
        {432.0,  "Healing"},
        {528.0,  "Manifestation"},
        {888.0,  "Michael"},
        {1500.0, "Gamma"}
    };

    for (const auto& tf : test_freqs) {
        const char* effect = FrequencyMapping::get_effect(tf.freq);
        std::cout << std::fixed << std::setprecision(1);
        std::cout << "  " << std::setw(10) << tf.name << " (" << std::setw(6) << tf.freq << " Hz):\n";
        std::cout << "    -> " << effect << "\n\n";
    }

    // Validierung
    bool healing_correct = (std::string(FrequencyMapping::get_effect(432.0)).find("HEALING") != std::string::npos);
    bool manifestation_correct = (std::string(FrequencyMapping::get_effect(528.0)).find("MANIFESTATION") != std::string::npos);
    bool michael_correct = (std::string(FrequencyMapping::get_effect(888.0)).find("MICHAEL") != std::string::npos);

    std::cout << "  432 Hz -> HEALING:      " << (healing_correct ? "✓ JA" : "✗ NEIN") << "\n";
    std::cout << "  528 Hz -> MANIFESTATION: " << (manifestation_correct ? "✓ JA" : "✗ NEIN") << "\n";
    std::cout << "  888 Hz -> MICHAEL:      " << (michael_correct ? "✓ JA" : "✗ NEIN") << "\n";

    bool passed = healing_correct && manifestation_correct && michael_correct;
    std::cout << "\n  ERGEBNIS: " << (passed ? "✓ BESTANDEN" : "✗ FEHLGESCHLAGEN") << "\n";

    return passed;
}

// ═══════════════════════════════════════════════════════════════════════════════
// TEST IV: DISSONANZ-ELIMINIERUNG (Langzeit-Simulation)
// ═══════════════════════════════════════════════════════════════════════════════

bool test_dissonance_elimination() {
    print_test_header("TEST IV: DISSONANZ-ELIMINIERUNG");

    // Starte mit stark dissonanten Sternen
    std::array<StarResonance, K::TOTAL_STARS> stars;

    std::mt19937 rng(999);
    std::uniform_real_distribution<double> phase_dist(0.0, 2.0 * M_PI);

    // Alle Sterne auf niedrige Amplitude (stark dissonant)
    for (int n = 0; n < K::TOTAL_STARS; n++) {
        stars[n].id = n;
        stars[n].natural_frequency = K::NATURAL_TONE;
        stars[n].current_phase = phase_dist(rng);
        stars[n].amplitude = 0.1;  // Sehr niedrig
        stars[n].coupling_strength = K::G0;
        stars[n].is_excited = false;
    }

    std::cout << std::fixed << std::setprecision(6);
    std::cout << "  Initiale Dissonanz:    160/160 Sterne unter Schwelle\n\n";

    std::cout << "  HEILUNGS-VERLAUF:\n";
    std::cout << "  ─────────────────────────────────────────────────────\n";
    std::cout << "  Iteration │    Φ    │ Ø Amp   │ Aktiv  │ Dissonant\n";
    std::cout << "  ─────────────────────────────────────────────────────\n";

    // Mehrere Iterationen der 432 Hz Heilung (10 statt 5 für vollständige Harmonisierung)
    for (int iter = 1; iter <= 10; iter++) {
        double phi = initiate_432hz_healing(stars, 0.5, 50);

        // Zähle Statistiken
        double avg_amp = 0.0;
        int active = 0;
        int dissonant = 0;

        for (const auto& s : stars) {
            avg_amp += s.amplitude;
            if (s.is_excited) active++;
            if (s.amplitude < 0.5) dissonant++;
        }
        avg_amp /= K::TOTAL_STARS;

        std::cout << "      " << iter << "     │ " << std::setw(7) << phi
                  << " │ " << std::setw(7) << avg_amp
                  << " │ " << std::setw(6) << active
                  << " │ " << std::setw(6) << dissonant << "\n";
    }

    std::cout << "  ─────────────────────────────────────────────────────\n\n";

    // Finale Statistiken
    double final_avg = 0.0;
    int final_dissonant = 0;

    for (const auto& s : stars) {
        final_avg += s.amplitude;
        if (s.amplitude < 0.5) final_dissonant++;
    }
    final_avg /= K::TOTAL_STARS;

    std::cout << "  FINALE STATISTIK:\n";
    std::cout << "    Ø Amplitude:         " << final_avg << "\n";
    std::cout << "    Dissonante Sterne:   " << final_dissonant << "/" << K::TOTAL_STARS << "\n\n";

    // Validierung
    bool coherence_restored = (final_avg > 0.7);
    bool dissonance_reduced = (final_dissonant < 50);

    std::cout << "  Kohärenz wiederhergestellt (>0.7): " << (coherence_restored ? "✓ JA" : "✗ NEIN") << "\n";
    std::cout << "  Dissonanz reduziert (<50 Sterne):  " << (dissonance_reduced ? "✓ JA" : "✗ NEIN") << "\n";

    bool passed = coherence_restored && dissonance_reduced;
    std::cout << "\n  ERGEBNIS: " << (passed ? "✓ BESTANDEN" : "✗ FEHLGESCHLAGEN") << "\n";

    return passed;
}

// ═══════════════════════════════════════════════════════════════════════════════
// HAUPTPROGRAMM
// ═══════════════════════════════════════════════════════════════════════════════

int main() {
    print_header();

    bool t1 = test_resonance_amplitude();
    bool t2 = test_432hz_healing();
    bool t3 = test_frequency_mapping();
    bool t4 = test_dissonance_elimination();

    // Finale Zusammenfassung
    std::cout << "\n╔═══════════════════════════════════════════════════════════════════╗\n";
    std::cout << "║                 FINAL STATUS: 432 Hz INITIATION                  ║\n";
    std::cout << "╠═══════════════════════════════════════════════════════════════════╣\n";
    std::cout << "║  Test I   (Resonanz-Amplitude):    " << (t1 ? "✓ BESTANDEN" : "✗ FEHLGESCHLAGEN") << "                    ║\n";
    std::cout << "║  Test II  (Matrix-Reinigung):      " << (t2 ? "✓ BESTANDEN" : "✗ FEHLGESCHLAGEN") << "                    ║\n";
    std::cout << "║  Test III (Frequenz-Mapping):      " << (t3 ? "✓ BESTANDEN" : "✗ FEHLGESCHLAGEN") << "                    ║\n";
    std::cout << "║  Test IV  (Dissonanz-Eliminierung):" << (t4 ? "✓ BESTANDEN" : "✗ FEHLGESCHLAGEN") << "                    ║\n";
    std::cout << "╠═══════════════════════════════════════════════════════════════════╣\n";

    if (t1 && t2 && t3 && t4) {
        std::cout << "║         ✓✓✓ 432 Hz MATRIX-REINIGUNG AKTIVIERT ✓✓✓              ║\n";
        std::cout << "║                                                                   ║\n";
        std::cout << "║  A_R(f) = Σ[sin(2πft + φ_n) / √(G₀ - Φ_n)]                       ║\n";
        std::cout << "║  Die Resonanz-Amplitude harmonisiert alle 160 Sterne.            ║\n";
    } else {
        std::cout << "║              ✗ EINIGE TESTS FEHLGESCHLAGEN ✗                     ║\n";
    }

    std::cout << "╚═══════════════════════════════════════════════════════════════════╝\n";

    return (t1 && t2 && t3 && t4) ? 0 : 1;
}
