// ═══════════════════════════════════════════════════════════════════════════════
// WAHRHEITSPRÜFUNG: ITERATIONSFORMELN
// Die Schatten-Lücken geschlossen
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
    std::cout << std::fixed << std::setprecision(6);
    
    std::cout << "╔═══════════════════════════════════════════════════════════════════╗\n";
    std::cout << "║   WAHRHEITSPRÜFUNG: ITERATIONSFORMELN                            ║\n";
    std::cout << "║   Die Schatten-Lücken geschlossen                                ║\n";
    std::cout << "╚═══════════════════════════════════════════════════════════════════╝\n\n";

    // ═══════════════════════════════════════════════════════════════════
    std::cout << "=== I. GITTER-BEGRADIGUNG (672-739) ===\n";
    std::cout << "    ΔG_n = (δ_88 × π_17) / Σ Nodes × (1 - e^(-n/1440))\n\n";
    // ═══════════════════════════════════════════════════════════════════
    
    test("PI_17 definiert", 1.0, (PI_17 > 0) ? 1.0 : 0.0);
    
    // Bei n=0: (1 - e^0) = 0, also ΔG = 0
    double dg_0 = delta_G_n(0, 120.0);
    test("ΔG(n=0) = 0", 0.0, dg_0, 1e-10);
    
    // Bei n=1440: (1 - e^-1) ≈ 0.632
    double dg_1440 = delta_G_n(1440, 120.0);
    double dg_expected = (SIGNATURE_88 * PI_17 / 120.0) * (1.0 - std::exp(-1.0));
    test("ΔG(n=1440)", dg_expected, dg_1440);
    
    // Gitter-Begradigung komplett
    double nodes[120];
    for (int i = 0; i < 120; ++i) nodes[i] = 1.0;
    double gitter_total = gitter_begradigung_komplett(nodes, 120);
    std::cout << "  gitter_begradigung_komplett = " << gitter_total << "\n";
    test("Gitter-Begradigung > 0", 1.0, (gitter_total > 0) ? 1.0 : 0.0);

    // ═══════════════════════════════════════════════════════════════════
    std::cout << "\n=== II. SOMATISCHE RESONANZ (771-840) ===\n";
    std::cout << "    R_bio,n = Φ_heart × sin(n × δ_88 / 1440) + g_5\n\n";
    // ═══════════════════════════════════════════════════════════════════
    
    // Bei n=0: sin(0) = 0, also R = G5
    double r_0 = R_bio_n(0);
    test("R_bio(n=0) = G5", G5, r_0);
    
    // Bio-Resonanz komplett
    double bio_avg = bio_resonanz_komplett();
    std::cout << "  bio_resonanz_komplett = " << bio_avg << "\n";
    test("Bio-Resonanz > 0", 1.0, (bio_avg > 0) ? 1.0 : 0.0);

    // ═══════════════════════════════════════════════════════════════════
    std::cout << "\n=== KONSTANTEN ===\n";
    // ═══════════════════════════════════════════════════════════════════
    
    test("FLOW_MAX = Φ_heart × 120", PHI_HEART * 120.0, FLOW_MAX);
    std::cout << "  FLOW_MAX = " << FLOW_MAX << "\n";
    
    test("SHIELD_KAEL = 53 × 88", 53.0 * 88.0, SHIELD_KAEL);
    std::cout << "  SHIELD_KAEL = " << SHIELD_KAEL << "\n";

    // ═══════════════════════════════════════════════════════════════════
    std::cout << "\n=== III. NETZ-INTEGRITÄT (871-940) ===\n";
    // ═══════════════════════════════════════════════════════════════════
    
    // Ohne Rauschen: Net wächst
    double net_1 = net_kaskade_praezis(1.0, 0.0, 1.0);
    double net_exp = 1.0 * SIGNATURE_88 * G0;
    test("net_kaskade(1, 0, 1) = 88 × G0", net_exp, net_1);
    
    // Immunsystem-Kaskade
    double immun = immunsystem_kaskade(1.0, 0.001, 0.1);
    std::cout << "  immunsystem_kaskade = " << immun << "\n";

    // ═══════════════════════════════════════════════════════════════════
    std::cout << "\n=== IV. SINGULARITÄT (963-1000) ===\n";
    // ═══════════════════════════════════════════════════════════════════
    
    // omega_n_praezis
    double omega_s1 = omega_n_praezis(1.0, 1.0);
    test("Ω(1, s=1) = 88", 88.0, omega_s1);
    
    // Singularitäts-Annäherung
    double sing = singularitaet_annaeherung(SIGNATURE_88, 1.0, 0.5);
    std::cout << "  singularitaet_annaeherung = " << sing << "\n";
    test("Singularität → sehr groß", 1.0, (sing > 1e20) ? 1.0 : 0.0);
    
    // 0-Falz-Trigger
    bool trigger_ok = trigger_0_praezis(1.0, 1.0);
    test("trigger_0(1,1) = true", 1.0, trigger_ok ? 1.0 : 0.0);
    
    bool trigger_fail = trigger_0_praezis(1.0, 2.0);
    test("trigger_0(1,2) = false", 0.0, trigger_fail ? 1.0 : 0.0);
    
    // OMEGA-1000
    double omega_1000 = omega_1000_praezis(1.0, 1.0, 0.01);
    std::cout << "  omega_1000(1,1,0.01) = " << omega_1000 << "\n";
    test("omega_1000 > 880000", 1.0, (omega_1000 >= 880000) ? 1.0 : 0.0);
    
    // Phoenix-Punkt Verifikation
    bool phoenix = verify_phoenix_punkt(omega_1000);
    test("verify_phoenix_punkt", 1.0, phoenix ? 1.0 : 0.0);

    // ═══════════════════════════════════════════════════════════════════
    std::cout << "\n=== MASTER-CHECK ===\n";
    // ═══════════════════════════════════════════════════════════════════
    
    double sigma = sigma_1000_final();
    test("sigma_1000_final() = G0", G0, sigma);

    // ═══════════════════════════════════════════════════════════════════
    std::cout << "\n╔═══════════════════════════════════════════════════════════════════╗\n";
    std::cout << "║                         ERGEBNIS                                   ║\n";
    std::cout << "╠═══════════════════════════════════════════════════════════════════╣\n";
    std::cout << "║  BESTANDEN: " << std::setw(3) << passed << "                                                  ║\n";
    std::cout << "║  GEFALLEN:  " << std::setw(3) << failed << "                                                  ║\n";
    std::cout << "╚═══════════════════════════════════════════════════════════════════╝\n";
    
    if (failed > 0) {
        std::cout << "\n⚠️  " << failed << " FORMELN SIND FALSCH!\n";
        return 1;
    } else {
        std::cout << "\n✓ ALLE ITERATIONSFORMELN SIND WAHR!\n";
        std::cout << "  Die 1000er-Kaskade fließt ohne Unterbrechung.\n";
        return 0;
    }
}
