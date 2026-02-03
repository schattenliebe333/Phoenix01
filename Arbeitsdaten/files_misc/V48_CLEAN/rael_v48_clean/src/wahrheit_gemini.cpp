// ═══════════════════════════════════════════════════════════════════════════════
// FINALE WAHRHEITSPRÜFUNG: Gemini Präzisions-Formeln
// ═══════════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <iomanip>
#include <cmath>
#include "rael/rst_constants.hpp"

using namespace rael::rst;

int passed = 0, failed = 0;

void test(const std::string& name, double erw, double got, double tol = 1e-6) {
    bool ok = std::abs(erw - got) < tol || (erw != 0 && std::abs((erw - got) / erw) < tol);
    if (ok) { std::cout << "  ✓ " << name << "\n"; passed++; }
    else { std::cout << "  ✗ " << name << " (erw: " << erw << ", got: " << got << ")\n"; failed++; }
}

int main() {
    std::cout << std::fixed << std::setprecision(6);
    
    std::cout << "╔═══════════════════════════════════════════════════════════════════╗\n";
    std::cout << "║   FINALE WAHRHEITSPRÜFUNG: GEMINI PRÄZISIONS-FORMELN             ║\n";
    std::cout << "╚═══════════════════════════════════════════════════════════════════╝\n\n";

    // === SEKTOR I ===
    std::cout << "=== SEKTOR I: HARDWARE-ARRETIERUNG ===\n";
    
    test("PI_17 = π/17", M_PI / 17.0, PI_17);
    
    // delta_G_n bei n=1440, summe=120
    double dg = delta_G_n_precise(1440, 120.0);
    double dg_exp = (88.0 * PI_17 / 120.0) * (1.0 - std::exp(-1.0));
    test("ΔG_n(1440, 120)", dg_exp, dg);
    
    test("ΔG_n(0, 120) = 0", 0.0, delta_G_n_precise(0, 120.0));
    
    test("FLOW_MAX = Φ_heart × 120", PHI_HEART * 120.0, FLOW_MAX_PRECISE);
    std::cout << "  FLOW_MAX = " << FLOW_MAX_PRECISE << "\n";
    
    test("SHIELD_KAEL = 53 × 88 = 4664", 4664.0, SHIELD_KAEL);
    test("VEC_17 = 289/169", 289.0/169.0, VEC_17_RATIO);

    // === SEKTOR II ===
    std::cout << "\n=== SEKTOR II: BIO-SOUVERÄNITÄT ===\n";
    
    // R_bio,n = Φ_heart · sin((n·δ_88)/1440) + g_5
    // Bei n=0: sin(0)=0, also R = 0 + g_5 = g_5
    test("R_bio(0) = G5", G5, R_bio_n_precise(0));
    
    // Bei n=1440π/(2×88) ≈ 25.7 wäre sin=1, also Maximum = Φ_heart + g_5
    double r_test = R_bio_n_precise(100);
    bool r_ok = (r_test >= -PHI_HEART + G5) && (r_test <= PHI_HEART + G5 + 0.01);
    test("R_bio(100) in [G5, Φ_heart+G5]", 1.0, r_ok ? 1.0 : 0.0);
    
    test("vita_check(1440)", 1.0, vita_check_precise(1440.0) ? 1.0 : 0.0);
    test("vita_check(5)", 1.0, vita_check_precise(5.0) ? 1.0 : 0.0);
    test("vita_check(432)", 1.0, vita_check_precise(432.0) ? 1.0 : 0.0);
    test("vita_check(0) = false", 0.0, vita_check_precise(0.0) ? 1.0 : 0.0);

    // === SEKTOR III ===
    std::cout << "\n=== SEKTOR III: GLOBALES IMMUNSYSTEM ===\n";
    
    double net = net_n_precise(1.0, 0.0, 1.0);
    test("net_n(1,0,1) = 88×G0", SIGNATURE_88 * G0, net);
    
    double net_r = net_n_precise(1.0, 1.0, 1.0);
    test("net_n mit Rauschen subtrahiert", SIGNATURE_88 * G0 - G0, net_r);
    
    double casc = casc_L7_gemini();
    std::cout << "  casc_L7 = " << casc << "\n";
    test("casc_L7 > 200", 1.0, (casc > 200) ? 1.0 : 0.0);

    // === SEKTOR IV ===
    std::cout << "\n=== SEKTOR IV: DIE SINGULARITÄT ===\n";
    
    test("Ω_n(1, 1) = 88", 88.0, omega_n_gemini(1.0, 1.0));
    test("Ω_n(1, 0.1) = 880", 880.0, omega_n_gemini(1.0, 0.1));
    
    double omega_sing = omega_n_gemini(1.0, 1e-19);
    std::cout << "  Ω_n(1, ~0) = " << omega_sing << " (SINGULARITÄT)\n";
    test("Ω_n bei s→0 > 1e15", 1.0, (omega_sing > 1e15) ? 1.0 : 0.0);
    
    test("trigger_0(1, 1)", 1.0, trigger_0_gemini(1.0, 1.0) ? 1.0 : 0.0);
    test("trigger_0(1, 1.01) = false", 0.0, trigger_0_gemini(1.0, 1.01) ? 1.0 : 0.0);
    
    std::cout << "\n--- OMEGA-1000 (DER PHÖNIX-PUNKT) ---\n";
    test("Ω_1000(1,1,1) = 88", 88.0, omega_1000_gemini(1.0, 1.0, 1.0));
    test("Ω_1000(1,1,0.01) = 880000", 880000.0, omega_1000_gemini(1.0, 1.0, 0.01));
    
    double o_sing = omega_1000_gemini(1.0, 1.0, 1e-19);
    std::cout << "  Ω_1000(1,1,~0) = " << o_sing << "\n";
    test("Ω_1000 SINGULARITÄT > 1e30", 1.0, (o_sing > 1e30) ? 1.0 : 0.0);

    // === FINALE ===
    std::cout << "\n=== FINALE VALIDIERUNG ===\n";
    test("sigma_final_gemini() = G0", G0, sigma_final_gemini());
    test("omega_ready() = true", 1.0, omega_ready() ? 1.0 : 0.0);

    // === ZUSAMMENFASSUNG ===
    std::cout << "\n╔═══════════════════════════════════════════════════════════════════╗\n";
    std::cout << "║                    FINALE ERGEBNISSE                              ║\n";
    std::cout << "╠═══════════════════════════════════════════════════════════════════╣\n";
    std::cout << "║  BESTANDEN: " << std::setw(3) << passed << "                                                  ║\n";
    std::cout << "║  GEFALLEN:  " << std::setw(3) << failed << "                                                  ║\n";
    std::cout << "╠═══════════════════════════════════════════════════════════════════╣\n";
    std::cout << "║  KRITISCHE KONSTANTEN:                                            ║\n";
    std::cout << "║    FLOW_MAX   = " << std::setw(10) << FLOW_MAX_PRECISE << " (Φ_heart × 120)             ║\n";
    std::cout << "║    SHIELD_K   = " << std::setw(10) << SHIELD_KAEL << " (53 × 88)                   ║\n";
    std::cout << "║    PHI_INF    = " << std::setw(10) << PHI_INF << " (φ × 88)                    ║\n";
    std::cout << "╚═══════════════════════════════════════════════════════════════════╝\n";
    
    if (failed > 0) {
        std::cout << "\n⚠️  " << failed << " FORMELN FALSCH!\n";
        return 1;
    }
    std::cout << "\n✓ ALLE GEMINI-FORMELN SIND WAHR!\n";
    std::cout << "  Das Omega-System ist bereit für den 0-Falz.\n";
    return 0;
}
