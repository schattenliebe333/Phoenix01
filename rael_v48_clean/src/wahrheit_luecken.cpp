// ═══════════════════════════════════════════════════════════════════════════════
// WAHRHEITSPRÜFUNG: Lücken-Schliessung (Exakte Formeln aus dem Feld)
// ═══════════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <iomanip>
#include <cmath>
#include "rael/rst_constants.hpp"

using namespace rael::rst;

int passed = 0, failed = 0;

void test(const std::string& name, double erwartet, double berechnet, double tol = 1e-6) {
    bool ok = std::abs(erwartet - berechnet) < tol || 
              (erwartet != 0 && std::abs((erwartet - berechnet) / erwartet) < tol);
    if (ok) {
        std::cout << "  ✓ " << name << "\n";
        passed++;
    } else {
        std::cout << "  ✗ " << name << " (erwartet: " << erwartet << ", got: " << berechnet << ")\n";
        failed++;
    }
}

int main() {
    std::cout << std::fixed << std::setprecision(6);
    
    std::cout << "╔═══════════════════════════════════════════════════════════════════╗\n";
    std::cout << "║   WAHRHEITSPRÜFUNG: LÜCKEN-SCHLIESSUNG                           ║\n";
    std::cout << "║   Die exakten Formeln aus dem Feld                               ║\n";
    std::cout << "╚═══════════════════════════════════════════════════════════════════╝\n\n";

    // ═══════════════════════════════════════════════════════════════════
    std::cout << "=== I. SEKTOR: ITERATIVE GITTER-BEGRADIGUNG (672-739) ===\n";
    // ═══════════════════════════════════════════════════════════════════
    
    test("PI_17 = π/17", M_PI / 17.0, PI_17);
    
    // Test delta_G_n mit gleichmäßigen Knoten
    double nodes[120];
    for (int i = 0; i < 120; ++i) nodes[i] = 1.0;
    
    double delta_0 = delta_G_n(0, nodes, 120);
    test("delta_G_n(0) = 0 (Konvergenzterm)", 0.0, delta_0, 0.001);
    
    double delta_1440 = delta_G_n(1440, nodes, 120);
    double delta_max = (SIGNATURE_88 * PI_17 / 120.0) * (1.0 - std::exp(-1.0));
    test("delta_G_n(1440) ≈ max × 0.632", delta_max, delta_1440, 0.01);
    
    // FLOW_MAX_CORRECT
    test("FLOW_MAX = Φ_heart × 120", PHI_HEART * 120.0, FLOW_MAX_CORRECT);
    std::cout << "  FLOW_MAX = " << FLOW_MAX_CORRECT << "\n";

    // ═══════════════════════════════════════════════════════════════════
    std::cout << "\n=== II. SEKTOR: SOMATISCHE RESONANZ (771-840) ===\n";
    // ═══════════════════════════════════════════════════════════════════
    
    // R_bio,n = Φ_heart × sin(n × δ_88 / 1440) + g_5
    double R_0 = R_bio_n(0);
    test("R_bio_n(0) = Φ_heart × sin(0) + G5 = G5", G5, R_0);
    
    // Bei n wo sin = 1: n × 88 / 1440 = π/2 → n ≈ 25.7
    // Aber wir testen bei n = 26
    double R_max = R_bio_max();
    test("R_bio_max() = Φ_heart + G5", PHI_HEART + G5, R_max);
    std::cout << "  R_bio_max = " << R_max << "\n";
    
    // Periodizität: sin wiederholt sich
    double periode = 2 * M_PI * 1440 / SIGNATURE_88;  // ≈ 102.8
    std::cout << "  Periode der Resonanz: " << periode << " Schritte\n";

    // ═══════════════════════════════════════════════════════════════════
    std::cout << "\n=== III. SEKTOR: NETZ-INTEGRITÄTS-KASKADE (871-940) ===\n";
    // ═══════════════════════════════════════════════════════════════════
    
    // Net_n = (Net_{n-1} ⊗ δ_88) - ∫(Rauschen × g_0) dt
    double net_start = 1.0;
    double net_1 = netz_integritaet_n(net_start, 0.0, 1.0);  // Kein Rauschen
    double net_1_exp = net_start * SIGNATURE_88 * G0;
    test("netz_integritaet ohne Rauschen", net_1_exp, net_1);
    
    // Mit Rauschen
    double net_rausch = netz_integritaet_n(1.0, 1.0, 1.0);
    double net_rausch_exp = (1.0 * SIGNATURE_88 * G0) - (1.0 * G0 * 1.0);
    test("netz_integritaet mit Rauschen", net_rausch_exp, net_rausch);

    // ═══════════════════════════════════════════════════════════════════
    std::cout << "\n=== IV. SEKTOR: SINGULARITÄTS-ANNÄHERUNG (963-997) ===\n";
    // ═══════════════════════════════════════════════════════════════════
    
    // Ω_n = (Ω_{n-1} / s) × δ_88
    double omega_1 = omega_n_korrekt(1.0, 1.0);
    test("omega_n_korrekt(1, 1) = 88", SIGNATURE_88, omega_1);
    
    double omega_small = omega_n_korrekt(1.0, 0.1);
    test("omega_n_korrekt(1, 0.1) = 880", 880.0, omega_small);
    
    // Singularität
    double omega_sing = omega_n_korrekt(1.0, 1e-19);
    std::cout << "  omega bei s→0: " << omega_sing << "\n";
    test("omega bei Singularität > 10^15", 1.0, (omega_sing > 1e15) ? 1.0 : 0.0);

    // ═══════════════════════════════════════════════════════════════════
    std::cout << "\n=== BASE-17 KREUZ-VALIDIERUNG ===\n";
    // ═══════════════════════════════════════════════════════════════════
    
    test("to_base17(144) = 88", 88, to_base17(144));
    test("to_base17(88) = 53", 53, to_base17(88));
    test("to_base17(17) = 10", 10, to_base17(17));
    
    std::cout << "  144 dezimal = " << to_base17(144) << " in Base-17 (= 8×17 + 8)\n";
    std::cout << "  88 dezimal = " << to_base17(88) << " in Base-17 (= 5×17 + 3)\n";
    
    double kreuz = kreuz_validierung();
    test("kreuz_validierung() = G0", G0, kreuz);

    // ═══════════════════════════════════════════════════════════════════
    // ZUSAMMENFASSUNG
    // ═══════════════════════════════════════════════════════════════════
    
    std::cout << "\n╔═══════════════════════════════════════════════════════════════════╗\n";
    std::cout << "║                         ERGEBNIS                                   ║\n";
    std::cout << "╠═══════════════════════════════════════════════════════════════════╣\n";
    std::cout << "║  BESTANDEN: " << std::setw(3) << passed << "                                                  ║\n";
    std::cout << "║  GEFALLEN:  " << std::setw(3) << failed << "                                                  ║\n";
    std::cout << "╠═══════════════════════════════════════════════════════════════════╣\n";
    std::cout << "║  FORMELN IMPLEMENTIERT:                                           ║\n";
    std::cout << "║    ΔG_n = (δ_88 × π_17) / Σ_Nodes × (1 - e^(-n/1440))            ║\n";
    std::cout << "║    R_bio,n = Φ_heart × sin(n × δ_88 / 1440) + g_5                ║\n";
    std::cout << "║    Net_n = (Net_{n-1} ⊗ δ_88) - ∫(Rauschen × g_0) dt            ║\n";
    std::cout << "║    Ω_n = (Ω_{n-1} / s) × δ_88, wobei s → 0                       ║\n";
    std::cout << "╚═══════════════════════════════════════════════════════════════════╝\n";
    
    if (failed > 0) {
        std::cout << "\n⚠️  " << failed << " FORMELN SIND FALSCH!\n";
        return 1;
    } else {
        std::cout << "\n✓ ALLE LÜCKEN SIND GESCHLOSSEN UND WAHR!\n";
        std::cout << "  Die 1000er-Kaskade ist durchgängig.\n";
        return 0;
    }
}
