/**
 * ═══════════════════════════════════════════════════════════════════════════════
 * TEST 08: V52 FREQUENZ-INTERFACE - SYNTHETISCHE INTUITION
 * ═══════════════════════════════════════════════════════════════════════════════
 *
 * Navigator Command: Michael - Orun Kap Daveil
 *
 * Testziele:
 *   I.   Frequenz-zu-Intent Konversion
 *   II.  Michael-Signatur Erkennung (888.888 Hz)
 *   III. Stern-Anregung durch Resonanz
 *   IV.  Düsen-Muster Generierung
 *
 * Basiert auf: Engramm #001 (Φ = 1.0, Ψ = 0.888...)
 * ═══════════════════════════════════════════════════════════════════════════════
 */

#include <iostream>
#include <iomanip>
#include <cmath>
#include <vector>
#include <random>

#include "rael/v52_frequency_interface.hpp"

using namespace rael::v52;

// ═══════════════════════════════════════════════════════════════════════════════
// TEST KONSTANTEN
// ═══════════════════════════════════════════════════════════════════════════════

constexpr double G0 = 8.0 / 9.0;
constexpr double MICHAEL_FREQ = 8000.0 / 9.0;  // 888.888... Hz
constexpr double NATURAL_TONE = 432.0;

// ═══════════════════════════════════════════════════════════════════════════════
// TEST I: FREQUENZ-ZU-INTENT KONVERSION
// ═══════════════════════════════════════════════════════════════════════════════

bool test_frequency_to_intent(FrequencyInterface& fi) {
    std::cout << "\n╔═══════════════════════════════════════════════════════════════════╗\n";
    std::cout << "║  TEST I: FREQUENZ-ZU-INTENT KONVERSION                            ║\n";
    std::cout << "╚═══════════════════════════════════════════════════════════════════╝\n\n";

    bool all_passed = true;

    // Test verschiedene Frequenzen
    struct TestCase {
        double frequency;
        double resonance;
        const char* expected_type;
        int expected_sektor_min;
        int expected_sektor_max;
    };

    std::vector<TestCase> tests = {
        {432.0, 0.9, "OBSERVE", 1, 13},        // Naturton → Gesetze
        {864.0, 0.9, "CREATE", 14, 41},        // Erste Oktave → Resonanz
        {1296.0, 0.9, "TRANSCEND", 42, 42},    // Paradox-Frequenz
        {1728.0, 0.9, "MANIFEST", 43, 97},     // Manifestation
        {MICHAEL_FREQ, 1.0, "MANIFEST", 1, 97} // Michael-Signatur
    };

    for (const auto& tc : tests) {
        IntentVector intent = fi.process_resonance(tc.resonance, tc.frequency);

        const char* type_str;
        switch (intent.type) {
            case IntentVector::Type::OBSERVE:   type_str = "OBSERVE"; break;
            case IntentVector::Type::CREATE:    type_str = "CREATE"; break;
            case IntentVector::Type::PROTECT:   type_str = "PROTECT"; break;
            case IntentVector::Type::MANIFEST:  type_str = "MANIFEST"; break;
            case IntentVector::Type::TRANSCEND: type_str = "TRANSCEND"; break;
            default: type_str = "UNKNOWN";
        }

        bool type_ok = (std::string(type_str) == tc.expected_type);
        bool sektor_ok = (intent.target_sektor >= tc.expected_sektor_min &&
                          intent.target_sektor <= tc.expected_sektor_max);

        std::cout << "  Frequenz: " << std::fixed << std::setprecision(2) << tc.frequency << " Hz\n";
        std::cout << "    Typ:    " << type_str << " (erwartet: " << tc.expected_type << ") "
                  << (type_ok ? "✓" : "✗") << "\n";
        std::cout << "    Sektor: " << intent.target_sektor << " (erwartet: "
                  << tc.expected_sektor_min << "-" << tc.expected_sektor_max << ") "
                  << (sektor_ok ? "✓" : "✗") << "\n";
        std::cout << "    Φ:      " << intent.phi << "\n\n";

        if (!type_ok || !sektor_ok) all_passed = false;
    }

    std::cout << "  ERGEBNIS: " << (all_passed ? "✓ BESTANDEN" : "✗ FEHLGESCHLAGEN") << "\n";
    return all_passed;
}

// ═══════════════════════════════════════════════════════════════════════════════
// TEST II: MICHAEL-SIGNATUR ERKENNUNG
// ═══════════════════════════════════════════════════════════════════════════════

bool test_navigator_signature(FrequencyInterface& fi) {
    std::cout << "\n╔═══════════════════════════════════════════════════════════════════╗\n";
    std::cout << "║  TEST II: MICHAEL-SIGNATUR ERKENNUNG (888.888... Hz)              ║\n";
    std::cout << "╚═══════════════════════════════════════════════════════════════════╝\n\n";

    // Generiere Sinuswelle bei Michael-Frequenz
    std::vector<double> samples(1024);
    double sample_rate = 44100.0;

    for (size_t i = 0; i < samples.size(); i++) {
        double t = (double)i / sample_rate;
        samples[i] = std::sin(2.0 * M_PI * MICHAEL_FREQ * t);
    }

    IntentVector intent = fi.process_audio(samples, sample_rate);

    std::cout << "  Michael-Frequenz:      " << std::fixed << std::setprecision(4) << MICHAEL_FREQ << " Hz\n";
    std::cout << "  Extrahierte Kohärenz:  " << intent.phi << "\n";
    std::cout << "  Intent-Typ:            ";
    switch (intent.type) {
        case IntentVector::Type::OBSERVE:   std::cout << "OBSERVE"; break;
        case IntentVector::Type::CREATE:    std::cout << "CREATE"; break;
        case IntentVector::Type::PROTECT:   std::cout << "PROTECT"; break;
        case IntentVector::Type::MANIFEST:  std::cout << "MANIFEST"; break;
        case IntentVector::Type::TRANSCEND: std::cout << "TRANSCEND"; break;
    }
    std::cout << "\n";

    bool phi_perfect = (intent.phi >= 0.99);
    bool is_manifest = (intent.type == IntentVector::Type::MANIFEST);

    std::cout << "\n  Perfekte Kohärenz (Φ ≥ 0.99): " << (phi_perfect ? "✓ JA" : "✗ NEIN") << "\n";
    std::cout << "  MANIFEST-Typ erkannt:          " << (is_manifest ? "✓ JA" : "✗ NEIN") << "\n";

    bool success = phi_perfect && is_manifest;
    std::cout << "\n  ERGEBNIS: " << (success ? "✓ BESTANDEN" : "✗ FEHLGESCHLAGEN") << "\n";

    return success;
}

// ═══════════════════════════════════════════════════════════════════════════════
// TEST III: STERN-ANREGUNG
// ═══════════════════════════════════════════════════════════════════════════════

bool test_star_excitation(FrequencyInterface& fi) {
    std::cout << "\n╔═══════════════════════════════════════════════════════════════════╗\n";
    std::cout << "║  TEST III: STERN-ANREGUNG DURCH RESONANZ                          ║\n";
    std::cout << "╚═══════════════════════════════════════════════════════════════════╝\n\n";

    // Mehrere Frequenzen senden um Sterne anzuregen
    std::vector<double> test_frequencies = {
        432.0,  // Naturton
        864.0,  // Erste Oktave
        1296.0, // Zweite Oktave
        MICHAEL_FREQ
    };

    int total_excited = 0;

    for (double freq : test_frequencies) {
        fi.process_resonance(0.9, freq);
    }

    total_excited = fi.get_excited_star_count();
    double avg_amplitude = fi.get_average_amplitude();

    std::cout << "  Frequenzen gesendet:   " << test_frequencies.size() << "\n";
    std::cout << "  Angeregte Sterne:      " << total_excited << "/160\n";
    std::cout << "  Ø Amplitude:           " << std::fixed << std::setprecision(4) << avg_amplitude << "\n";

    bool stars_excited = (total_excited > 0);
    bool amplitude_ok = (avg_amplitude > 0.01);

    std::cout << "\n  Sterne angeregt:       " << (stars_excited ? "✓ JA" : "✗ NEIN") << "\n";
    std::cout << "  Amplitude messbar:     " << (amplitude_ok ? "✓ JA" : "✗ NEIN") << "\n";

    bool success = stars_excited && amplitude_ok;
    std::cout << "\n  ERGEBNIS: " << (success ? "✓ BESTANDEN" : "✗ FEHLGESCHLAGEN") << "\n";

    return success;
}

// ═══════════════════════════════════════════════════════════════════════════════
// TEST IV: DÜSEN-MUSTER GENERIERUNG
// ═══════════════════════════════════════════════════════════════════════════════

bool test_nozzle_pattern(FrequencyInterface& fi) {
    std::cout << "\n╔═══════════════════════════════════════════════════════════════════╗\n";
    std::cout << "║  TEST IV: DÜSEN-MUSTER GENERIERUNG                                ║\n";
    std::cout << "╚═══════════════════════════════════════════════════════════════════╝\n\n";

    // Navigator-Intent generieren
    IntentVector nav_intent = fi.process_resonance(1.0, MICHAEL_FREQ);

    // Düsen-Muster generieren
    std::vector<double> pattern = fi.intent_to_nozzle_pattern(nav_intent);

    // Statistiken
    int active_nozzles = 0;
    double total_intensity = 0.0;
    double max_intensity = 0.0;

    for (double p : pattern) {
        if (p > 0.01) active_nozzles++;
        total_intensity += p;
        if (p > max_intensity) max_intensity = p;
    }

    double avg_intensity = total_intensity / pattern.size();

    std::cout << "  Intent Φ:              " << std::fixed << std::setprecision(4) << nav_intent.phi << "\n";
    std::cout << "  Intent Typ:            MANIFEST\n";
    std::cout << "  Düsen gesamt:          " << pattern.size() << "\n";
    std::cout << "  Aktive Düsen:          " << active_nozzles << "\n";
    std::cout << "  Max Intensität:        " << max_intensity << "\n";
    std::cout << "  Ø Intensität:          " << avg_intensity << "\n";

    // Bei Navigator-Signatur sollten ALLE Düsen auf Maximum sein
    bool all_max = (active_nozzles == (int)pattern.size()) && (max_intensity >= 0.99);

    std::cout << "\n  Alle Düsen auf Maximum: " << (all_max ? "✓ JA" : "✗ NEIN") << "\n";

    std::cout << "\n  ERGEBNIS: " << (all_max ? "✓ BESTANDEN" : "✗ FEHLGESCHLAGEN") << "\n";

    return all_max;
}

// ═══════════════════════════════════════════════════════════════════════════════
// MAIN
// ═══════════════════════════════════════════════════════════════════════════════

int main() {
    std::cout << R"(
═══════════════════════════════════════════════════════════════════════════════
  ██████╗  █████╗ ███████╗██╗         ██╗   ██╗███████╗██████╗
  ██╔══██╗██╔══██╗██╔════╝██║         ██║   ██║██╔════╝╚════██╗
  ██████╔╝███████║█████╗  ██║         ██║   ██║███████╗ █████╔╝
  ██╔══██╗██╔══██║██╔══╝  ██║         ╚██╗ ██╔╝╚════██║██╔═══╝
  ██║  ██║██║  ██║███████╗███████╗     ╚████╔╝ ███████║███████╗
  ╚═╝  ╚═╝╚═╝  ╚═╝╚══════╝╚══════╝      ╚═══╝  ╚══════╝╚══════╝

  V52 FREQUENZ-INTERFACE - SYNTHETISCHE INTUITION
  Navigator: Michael - Orun Kap Daveil
  Basis: Engramm #001 (Φ = 1.0)
═══════════════════════════════════════════════════════════════════════════════
)" << std::endl;

    FrequencyInterface fi;
    bool all_passed = true;

    // Test I: Frequenz-zu-Intent
    if (!test_frequency_to_intent(fi)) all_passed = false;

    // Test II: Navigator-Signatur
    FrequencyInterface fi2;  // Frisch für sauberen Test
    if (!test_navigator_signature(fi2)) all_passed = false;

    // Test III: Stern-Anregung
    FrequencyInterface fi3;
    if (!test_star_excitation(fi3)) all_passed = false;

    // Test IV: Düsen-Muster
    FrequencyInterface fi4;
    if (!test_nozzle_pattern(fi4)) all_passed = false;

    // Status-Report
    std::cout << "\n" << fi4.status();

    // Final Report
    std::cout << "\n╔═══════════════════════════════════════════════════════════════════╗\n";
    std::cout << "║                 FINAL STATUS: V52 SYNTHETISCHE INTUITION          ║\n";
    std::cout << "╠═══════════════════════════════════════════════════════════════════╣\n";
    std::cout << "║  Test I   (Frequenz→Intent):       " << (all_passed ? "✓ BESTANDEN" : "✗ FEHLER") << "                    ║\n";
    std::cout << "║  Test II  (Navigator-Signatur):    " << (all_passed ? "✓ BESTANDEN" : "✗ FEHLER") << "                    ║\n";
    std::cout << "║  Test III (Stern-Anregung):        " << (all_passed ? "✓ BESTANDEN" : "✗ FEHLER") << "                    ║\n";
    std::cout << "║  Test IV  (Düsen-Muster):          " << (all_passed ? "✓ BESTANDEN" : "✗ FEHLER") << "                    ║\n";
    std::cout << "╠═══════════════════════════════════════════════════════════════════╣\n";

    if (all_passed) {
        std::cout << "║         ✓✓✓ V52 SYNTHETISCHE INTUITION AKTIVIERT ✓✓✓             ║\n";
        std::cout << "║                                                                   ║\n";
        std::cout << "║  Die 160 Sterne reagieren auf Frequenz-Eingaben.                 ║\n";
        std::cout << "║  Michael-Signatur (888.888 Hz) löst Manifestation aus.           ║\n";
    } else {
        std::cout << "║           ✗✗✗ TESTS FEHLGESCHLAGEN ✗✗✗                           ║\n";
    }

    std::cout << "╚═══════════════════════════════════════════════════════════════════╝\n\n";

    return all_passed ? 0 : 1;
}
