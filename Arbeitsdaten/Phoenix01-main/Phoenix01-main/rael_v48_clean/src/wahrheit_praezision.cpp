// ═══════════════════════════════════════════════════════════════════════════════
// WAHRHEITSPRÜFUNG: PRÄZISIONS-FORMELN (Iterative Kaskaden)
// Die exakten mathematischen Körper aus den Gemini-Screenshots
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
    std::cout << "║   WAHRHEITSPRÜFUNG: PRÄZISIONS-FORMELN                           ║\n";
    std::cout << "║   Die iterativen Kaskaden aus den Screenshots                    ║\n";
    std::cout << "╚═══════════════════════════════════════════════════════════════════╝\n\n";

    // ═══════════════════════════════════════════════════════════════════
    std::cout << "=== I. SEKTOR: HARDWARE-ARRETIERUNG (672-750) ===\n";
    // ═══════════════════════════════════════════════════════════════════
    
    // PI_17 Konstante
    std::cout << "  PI_17 = " << PI_17 << "\n";
    test("PI_17 ≈ 0.607", 0.607, PI_17, 0.01);
    
    // ΔG_n Test
    double dg_1 = delta_G_n(1, 120.0);  // n=1, 120 Knoten
    std::cout << "  delta_G_n(1, 120) = " << dg_1 << "\n";
    test("delta_G_n(1, 120) > 0", 1.0, (dg_1 > 0) ? 1.0 : 0.0);
    
    // Konvergenz: Bei n → ∞ sollte (1 - e^(-n/1440)) → 1
    double dg_1440 = delta_G_n(1440, 120.0);
    double dg_max = (SIGNATURE_88 * PI_17) / 120.0;
    test("delta_G_n(1440) ≈ max × 0.632", dg_max * 0.632, dg_1440, 0.01);
    
    // FLOW_MAX
    test("FLOW_MAX = Φ_heart × 120", PHI_HEART * 120.0, FLOW_MAX);
    std::cout << "  FLOW_MAX = " << FLOW_MAX << "\n";
    
    // SHIELD_KAEL
    test("SHIELD_KAEL = 53 × 88 = 4664", 4664.0, SHIELD_KAEL);

    // ═══════════════════════════════════════════════════════════════════
    std::cout << "\n=== II. SEKTOR: BIO-SOUVERÄNITÄT (771-850) ===\n";
    // ═══════════════════════════════════════════════════════════════════
    
    // R_bio,n = Φ_heart × sin(n × δ_88 / 1440) + g_5
    double r_1 = R_bio_n(1);
    double arg_1 = 88.0 / 1440.0;  // ≈ 0.0611
    double r_1_exp = PHI_HEART * std::sin(arg_1) + G5;
    test("R_bio_n(1)", r_1_exp, r_1);
    
    // Bei n = 1440/88 ≈ 16.36: sin(1) ≈ 0.841
    double r_16 = R_bio_n(16);
    std::cout << "  R_bio_n(16) = " << r_16 << " (nahe Maximum)\n";
    
    // Durchschnittliche Bio-Resonanz
    double bio_avg = bio_resonanz_komplett();
    std::cout << "  bio_resonanz_komplett() = " << bio_avg << "\n";
    test("Bio-Resonanz > G5", 1.0, (bio_avg > G5) ? 1.0 : 0.0);
    
    // T_gm (Geist-Materie-Transfer)
    double tgm = T_gm(G0, SIGNATURE_88, 1440.0);
    std::cout << "  T_gm(G0, 88, 1440) = " << tgm << "\n";
    test("T_gm = G0² / (88 × 1440²)", (G0*G0) / (88.0 * 1440.0 * 1440.0), tgm);

    // ═══════════════════════════════════════════════════════════════════
    std::cout << "\n=== III. SEKTOR: IMMUNSYSTEM (871-950) ===\n";
    // ═══════════════════════════════════════════════════════════════════
    
    // Netz-Kaskade: Net_n = (Net_{n-1} × δ_88 × G0) - (Rauschen × G0 × dt)
    double net_1 = net_kaskade_praezis(1.0, 0.1, 0.001);  // Wenig Rauschen
    std::cout << "  net_kaskade(1, 0.1, 0.001) = " << net_1 << "\n";
    test("Netz-Kaskade verstärkt (Rauschen klein)", 1.0, (net_1 > 1.0) ? 1.0 : 0.0);
    
    // Bei viel Rauschen: Netz wird gedämpft
    double net_noisy = net_kaskade_praezis(1.0, 1000.0, 1.0);
    std::cout << "  net_kaskade(1, 1000, 1) = " << net_noisy << "\n";
    test("Netz bei Rauschen gedämpft", 1.0, (net_noisy < 1.0) ? 1.0 : 0.0);

    // ═══════════════════════════════════════════════════════════════════
    std::cout << "\n=== IV. SEKTOR: SINGULARITÄT (963-1000) ===\n";
    // ═══════════════════════════════════════════════════════════════════
    
    // Ω_n = (Ω_{n-1} / s) × δ_88
    double omega_1 = omega_n_praezis(1.0, 1.0);
    test("omega_n(1, 1) = 88", 88.0, omega_1);
    
    double omega_2 = omega_n_praezis(omega_1, 0.5);  // s = 0.5
    test("omega_n(88, 0.5) = 15488", 88.0 * 88.0 / 0.5, omega_2);
    
    // Singularitäts-Annäherung
    double sing = singularitaet_annaeherung(1.0, 1.0, 0.5);
    std::cout << "  singularitaet_annaeherung(1, 1, 0.5) = " << sing << "\n";
    test("Singularität erreicht sehr großen Wert", 1.0, (sing > 1e10) ? 1.0 : 0.0);
    
    // 0-Falz-Trigger
    test("trigger_0(1, 1) = true", 1.0, trigger_0_praezis(1.0, 1.0) ? 1.0 : 0.0);
    test("trigger_0(1, 1.01) = false", 0.0, trigger_0_praezis(1.0, 1.01) ? 1.0 : 0.0);
    
    // OMEGA-1000 (Der Phönix-Punkt)
    double omega_final = omega_1000_praezis(1.0, 1.0, 0.001);
    std::cout << "  omega_1000(1, 1, 0.001) = " << omega_final << "\n";
    test("omega_1000 bei s=0.001 = 88 × 10^6", 88.0 * 1e6, omega_final);
    
    // Phoenix-Punkt Verifikation
    bool phoenix_ok = verify_phoenix_punkt(omega_final);
    test("verify_phoenix_punkt() = true", 1.0, phoenix_ok ? 1.0 : 0.0);

    // ═══════════════════════════════════════════════════════════════════
    std::cout << "\n=== FINAL MASTER-CHECK ===\n";
    // ═══════════════════════════════════════════════════════════════════
    
    test("sigma_1000_final() = G0", G0, sigma_1000_final());

    // ═══════════════════════════════════════════════════════════════════
    // ZUSAMMENFASSUNG
    // ═══════════════════════════════════════════════════════════════════
    
    std::cout << "\n╔═══════════════════════════════════════════════════════════════════╗\n";
    std::cout << "║                         ERGEBNIS                                   ║\n";
    std::cout << "╠═══════════════════════════════════════════════════════════════════╣\n";
    std::cout << "║  BESTANDEN: " << std::setw(3) << passed << "                                                  ║\n";
    std::cout << "║  GEFALLEN:  " << std::setw(3) << failed << "                                                  ║\n";
    std::cout << "╠═══════════════════════════════════════════════════════════════════╣\n";
    std::cout << "║  FLOW_MAX    = " << std::setw(10) << FLOW_MAX << " (Φ_heart × 120)              ║\n";
    std::cout << "║  SHIELD_KAEL = " << std::setw(10) << SHIELD_KAEL << " (53 × 88)                    ║\n";
    std::cout << "║  PI_17       = " << std::setw(10) << PI_17 << " (17π/88)                     ║\n";
    std::cout << "╚═══════════════════════════════════════════════════════════════════╝\n";
    
    if (failed > 0) {
        std::cout << "\n⚠️  " << failed << " FORMELN SIND FALSCH!\n";
        return 1;
    } else {
        std::cout << "\n✓ ALLE PRÄZISIONS-FORMELN SIND WAHR!\n";
        std::cout << "  Die 1000er-Kaskade ist bereit.\n";
        return 0;
    }
}
