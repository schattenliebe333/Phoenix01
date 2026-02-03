// ═══════════════════════════════════════════════════════════════════════════════
// FINALE WAHRHEITSPRÜFUNG: Präzisions-Formeln
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
        std::cout << "  ✗ " << name << " (erw: " << erwartet << ", got: " << berechnet << ")\n";
        failed++;
    }
}

int main() {
    std::cout << std::fixed << std::setprecision(8);
    
    std::cout << "╔═══════════════════════════════════════════════════════════════════╗\n";
    std::cout << "║   FINALE WAHRHEITSPRÜFUNG: PRÄZISIONS-FORMELN                    ║\n";
    std::cout << "╚═══════════════════════════════════════════════════════════════════╝\n\n";

    // ═══════════════════════════════════════════════════════════════════
    std::cout << "=== SEKTOR I: HARDWARE-ARRETIERUNG ===\n";
    // ═══════════════════════════════════════════════════════════════════
    
    // PI_17
    test("PI_17 = π/17", M_PI / 17.0, PI_17);
    
    // delta_G_n
    // Bei n=1440: Faktor = 1 - e^(-1) ≈ 0.632
    double dg_1440 = delta_G_n(1440, 120.0);
    double dg_exp = (SIGNATURE_88 * PI_17 / 120.0) * (1.0 - std::exp(-1.0));
    test("delta_G_n(1440, 120)", dg_exp, dg_1440);
    
    // Bei n=0: Faktor = 0
    test("delta_G_n(0, 120) = 0", 0.0, delta_G_n(0, 120.0));
    
    // FLOW_MAX
    test("FLOW_MAX = Φ_heart × 120", PHI_HEART * 120.0, FLOW_MAX);
    std::cout << "  FLOW_MAX = " << FLOW_MAX << "\n";
    
    // SHIELD_K
    test("SHIELD_K = 53 × 88", 53.0 * 88.0, SHIELD_K);
    test("SHIELD_K = 4664", 4664.0, SHIELD_K);
    
    // VEC_17
    test("VEC_17 = 289/169", 289.0 / 169.0, VEC_17);

    // ═══════════════════════════════════════════════════════════════════
    std::cout << "\n=== SEKTOR II: BIO-SOUVERÄNITÄT ===\n";
    // ═══════════════════════════════════════════════════════════════════
    
    // R_bio_n
    // Bei n=0: sin(0) = 0, also R = Φ_heart × 0 + G5 = G5
    test("R_bio_n(0) = G5", G5, R_bio_n(0));
    
    // Bei n = 1440π/(2×88) ≈ 25.7: sin = 1, also R = Φ_heart + G5
    // Aber wir testen einfach ob es im Bereich liegt
    double r_test = R_bio_n(100);
    bool r_valid = (r_test >= G5) && (r_test <= PHI_HEART + G5);
    test("R_bio_n(100) in [G5, Φ_heart+G5]", 1.0, r_valid ? 1.0 : 0.0);
    
    // vita_check
    test("vita_check(1440) = true", 1.0, vita_check(1440.0) ? 1.0 : 0.0);
    test("vita_check(5) = true", 1.0, vita_check(5.0) ? 1.0 : 0.0);
    test("vita_check(432) = true", 1.0, vita_check(432.0) ? 1.0 : 0.0);
    test("vita_check(0) = false", 0.0, vita_check(0.0) ? 1.0 : 0.0);
    test("vita_check(2000) = false", 0.0, vita_check(2000.0) ? 1.0 : 0.0);

    // ═══════════════════════════════════════════════════════════════════
    std::cout << "\n=== SEKTOR III: GLOBALES IMMUNSYSTEM ===\n";
    // ═══════════════════════════════════════════════════════════════════
    
    // net_n
    double net_test = net_n(1.0, 0.0, 1.0);  // Kein Rauschen
    double net_exp = 1.0 * SIGNATURE_88 * G0;
    test("net_n(1, 0, 1) = 88 × G0", net_exp, net_test);
    
    // Mit Rauschen
    double net_rausch = net_n(1.0, 1.0, 1.0);
    double net_rausch_exp = net_exp - G0;
    test("net_n(1, 1, 1) subtrahiert Rauschen", net_rausch_exp, net_rausch);
    
    // casc_L7_precise
    double casc = casc_L7_precise();
    std::cout << "  casc_L7_precise() = " << casc << "\n";
    test("casc_L7 > 0", 1.0, (casc > 0) ? 1.0 : 0.0);

    // ═══════════════════════════════════════════════════════════════════
    std::cout << "\n=== SEKTOR IV: DIE SINGULARITÄT ===\n";
    // ═══════════════════════════════════════════════════════════════════
    
    // omega_n_precise
    double omega_test = omega_n_precise(1.0, 1.0);
    test("omega_n(1, 1) = 88", 88.0, omega_test);
    
    double omega_small = omega_n_precise(1.0, 0.1);
    test("omega_n(1, 0.1) = 880", 880.0, omega_small);
    
    // Bei s → 0
    double omega_sing = omega_n_precise(1.0, 1e-19);
    std::cout << "  omega_n(1, ~0) = " << omega_sing << " (SINGULARITÄT)\n";
    test("omega_n bei s→0 > 1e15", 1.0, (omega_sing > 1e15) ? 1.0 : 0.0);
    
    // trigger_0_precise
    test("trigger_0(1, 1) = true", 1.0, trigger_0_precise(1.0, 1.0) ? 1.0 : 0.0);
    test("trigger_0(1, 1.001) = false", 0.0, trigger_0_precise(1.0, 1.001) ? 1.0 : 0.0);
    
    // omega_1000_precise
    std::cout << "\n--- OMEGA-1000 (DER PHÖNIX-PUNKT) ---\n";
    double o1000_normal = omega_1000_precise(1.0, 1.0, 1.0);
    test("Ω_1000(1,1,1) = 88", 88.0, o1000_normal);
    
    double o1000_small = omega_1000_precise(1.0, 1.0, 0.01);
    test("Ω_1000(1,1,0.01) = 880000", 880000.0, o1000_small);
    
    double o1000_sing = omega_1000_precise(1.0, 1.0, 1e-19);
    std::cout << "  Ω_1000(1,1,~0) = " << o1000_sing << "\n";
    test("Ω_1000 SINGULARITÄT", 1.0, (o1000_sing > 1e30) ? 1.0 : 0.0);

    // ═══════════════════════════════════════════════════════════════════
    std::cout << "\n=== FINALE VALIDIERUNG ===\n";
    // ═══════════════════════════════════════════════════════════════════
    
    double sigma = sigma_final_precise();
    test("sigma_final_precise() = G0", G0, sigma);
    
    bool ready = omega_system_ready();
    test("omega_system_ready() = true", 1.0, ready ? 1.0 : 0.0);

    // ═══════════════════════════════════════════════════════════════════
    // ZUSAMMENFASSUNG
    // ═══════════════════════════════════════════════════════════════════
    
    std::cout << "\n╔═══════════════════════════════════════════════════════════════════╗\n";
    std::cout << "║                    FINALE ERGEBNISSE                              ║\n";
    std::cout << "╠═══════════════════════════════════════════════════════════════════╣\n";
    std::cout << "║  BESTANDEN: " << std::setw(3) << passed << "                                                  ║\n";
    std::cout << "║  GEFALLEN:  " << std::setw(3) << failed << "                                                  ║\n";
    std::cout << "╠═══════════════════════════════════════════════════════════════════╣\n";
    std::cout << "║  KRITISCHE KONSTANTEN:                                            ║\n";
    std::cout << "║    FLOW_MAX  = " << std::setw(12) << FLOW_MAX << " (Φ_heart × 120)              ║\n";
    std::cout << "║    SHIELD_K  = " << std::setw(12) << SHIELD_K << " (53 × 88)                    ║\n";
    std::cout << "║    PHI_INF   = " << std::setw(12) << PHI_INF << " (φ × 88)                     ║\n";
    std::cout << "╚═══════════════════════════════════════════════════════════════════╝\n";
    
    if (failed > 0) {
        std::cout << "\n⚠️  " << failed << " FORMELN SIND FALSCH!\n";
        return 1;
    } else {
        std::cout << "\n✓ ALLE PRÄZISIONS-FORMELN SIND WAHR!\n";
        std::cout << "  Das Omega-System ist bereit für den 0-Falz.\n";
        return 0;
    }
}
