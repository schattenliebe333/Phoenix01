// ═══════════════════════════════════════════════════════════════════════════════
// WAHRHEITSPRÜFUNG BLOCK #922-1021 (Bio-Digitale Schnittstelle)
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
        std::cout << "  ✗ " << name << " (erwartet: " << erwartet << ", bekommen: " << berechnet << ")\n";
        failed++;
    }
}

int main() {
    std::cout << std::fixed << std::setprecision(6);
    
    std::cout << "╔═══════════════════════════════════════════════════════════════════╗\n";
    std::cout << "║   WAHRHEITSPRÜFUNG: BIO-DIGITALE SCHNITTSTELLE (#922-1021)       ║\n";
    std::cout << "╚═══════════════════════════════════════════════════════════════════╝\n\n";

    // ═══════════════════════════════════════════════════════════════════
    std::cout << "=== #922: Neural-Mapping-Sync ===\n";
    // ═══════════════════════════════════════════════════════════════════
    
    // sync_neu = (phi_heart * G1) / (latenz + 1)
    double sync_test = sync_neu(PHI_HEART, 0.0);  // Latenz = 0
    double sync_expected = PHI_HEART * G1;  // 112.64 × 0.555... ≈ 62.58
    test("sync_neu(PHI_HEART, 0) = PHI_HEART × G1", sync_expected, sync_test);
    
    // Mit Latenz = 1: Halbierung
    double sync_lat1 = sync_neu(PHI_HEART, 1.0);
    test("sync_neu mit Latenz=1 halbiert", sync_expected / 2.0, sync_lat1);

    // ═══════════════════════════════════════════════════════════════════
    std::cout << "\n=== #923: Bio-Feedback-Schleife ===\n";
    // ═══════════════════════════════════════════════════════════════════
    
    // loop_bio = sin(alpha × f_gate) × G3
    double loop_0 = loop_bio(0.0);  // sin(0) = 0
    test("loop_bio(0) = 0", 0.0, loop_0);
    
    // Maximum bei sin = 1 → G3
    // sin(x) = 1 wenn x = π/2, also alpha = π/(2 × f_gate)
    double alpha_max = M_PI / (2.0 * F_GATE53);
    double loop_max = loop_bio(alpha_max);
    test("loop_bio(max) = G3", G3, loop_max, 0.001);

    // ═══════════════════════════════════════════════════════════════════
    std::cout << "\n=== #924: Herz-Kohärenz-Trigger (HCT) ===\n";
    // ═══════════════════════════════════════════════════════════════════
    
    // HCT = (Signatur × Φ_heart) / G0
    double hct_test = HCT(SIGNATURE_88, PHI_HEART);
    double hct_expected = (SIGNATURE_88 * PHI_HEART) / G0;
    test("HCT(88, PHI_HEART)", hct_expected, hct_test);
    test("HCT_STABLE Konstante", hct_expected, HCT_STABLE);
    
    // HCT ≈ 88 × 112.64 / 0.888... ≈ 11177.6
    test("HCT_STABLE ≈ 11177.6", 11177.6, HCT_STABLE, 1.0);

    // ═══════════════════════════════════════════════════════════════════
    std::cout << "\n=== #925: Dopamin-Resonanz-Filter ===\n";
    // ═══════════════════════════════════════════════════════════════════
    
    // dop_res = (M × G5) / (stress + 1)
    double dop_0 = dop_res(1.0, 0.0);  // Kein Stress
    test("dop_res(1, 0) = G5", G5, dop_0);
    
    double dop_stress = dop_res(1.0, 1.0);  // Stress = 1
    test("dop_res(1, 1) = G5/2", G5/2.0, dop_stress);

    // ═══════════════════════════════════════════════════════════════════
    std::cout << "\n=== #928: Vagus-Kopplung ===\n";
    // ═══════════════════════════════════════════════════════════════════
    
    // Optimales Verhältnis Herz:Atem = 5:1
    double vagus_optimal = vagus_kopplung(60.0, 12.0);  // 60bpm / 12 Atemzüge = 5
    test("vagus_kopplung(60, 12) = G0", G0, vagus_optimal, 0.01);
    
    // Suboptimales Verhältnis
    double vagus_sub = vagus_kopplung(80.0, 20.0);  // Ratio = 4
    bool vagus_less = vagus_sub < G0;
    test("vagus bei Ratio≠5 < G0", 1.0, vagus_less ? 1.0 : 0.0);

    // ═══════════════════════════════════════════════════════════════════
    std::cout << "\n=== #930: Physischer Resonanz-Anker ===\n";
    // ═══════════════════════════════════════════════════════════════════
    
    test("PHOENIX_FREQ = 5 Hz", 5.0, PHOENIX_FREQ);
    
    double anker = phys_anker(1.0);  // Intent = 1
    double anker_exp = G0 * (5.0 / F_GATE53);  // ≈ 0.0833
    test("phys_anker(1)", anker_exp, anker);

    // ═══════════════════════════════════════════════════════════════════
    std::cout << "\n=== #1017: Herz-Fokus ===\n";
    // ═══════════════════════════════════════════════════════════════════
    
    // herz_fokus = energie × PHI_HEART / 88
    double fokus = herz_fokus(SIGNATURE_88);  // Input = 88
    test("herz_fokus(88) = PHI_HEART", PHI_HEART, fokus);

    // ═══════════════════════════════════════════════════════════════════
    std::cout << "\n=== #1019: Lebenskraft-Check ===\n";
    // ═══════════════════════════════════════════════════════════════════
    
    // Bei allen Werten = 1: Durchschnitt = 1, Return = G0
    double leben = lebenskraft_check(1.0, 1.0, 1.0);
    test("lebenskraft_check(1,1,1) = G0", G0, leben);
    
    // Bei Tod (alle 0): Return = 0
    double tod = lebenskraft_check(0.0, 0.0, 0.0);
    test("lebenskraft_check(0,0,0) = 0", 0.0, tod);

    // ═══════════════════════════════════════════════════════════════════
    std::cout << "\n=== #1021: SIGMA_60 (Bio-Souveränität) ===\n";
    // ═══════════════════════════════════════════════════════════════════
    
    // Sigma_60 = (sync × HCT × inf_int) / (G1 × G3 × G5)
    double nenner = G1 * G3 * G5;
    test("G1 × G3 × G5 ≈ 0.0206", 0.020576, nenner, 0.0001);
    
    // Bei perfekten Werten: sehr großer Wert
    double sigma60 = sigma_60_bio_souveraenitaet(sync_expected, HCT_STABLE, 100.0);
    bool souveraen = verify_sigma_60(sigma60);
    std::cout << "  sigma_60 = " << sigma60 << "\n";
    test("verify_sigma_60 bei hohen Werten", 1.0, souveraen ? 1.0 : 0.0);

    // ═══════════════════════════════════════════════════════════════════
    std::cout << "\n=== FREQUENZ-KONSTANTEN (5 Identitäten) ===\n";
    // ═══════════════════════════════════════════════════════════════════
    
    test("FREQ_MICHAEL = 1440", 1440.0, FREQ_MICHAEL);
    test("FREQ_ITHRA = 720", 720.0, FREQ_ITHRA);
    test("FREQ_RAEL = 432", 432.0, FREQ_RAEL);
    test("FREQ_KAEL = 53", 53.0, FREQ_KAEL);
    test("FREQ_PHOENIX_ID = 5", 5.0, FREQ_PHOENIX_ID);
    
    // Verhältnisse
    test("MICHAEL/ITHRA = 2", 2.0, FREQ_MICHAEL / FREQ_ITHRA);
    test("ITHRA/RAEL = 5/3", 5.0/3.0, FREQ_ITHRA / FREQ_RAEL, 0.001);

    // ═══════════════════════════════════════════════════════════════════
    std::cout << "\n=== MASTER-CHECKS ===\n";
    // ═══════════════════════════════════════════════════════════════════
    
    test("sigma_57_bio_block() = G0", G0, sigma_57_bio_block());

    // ═══════════════════════════════════════════════════════════════════
    // ZUSAMMENFASSUNG
    // ═══════════════════════════════════════════════════════════════════
    
    std::cout << "\n╔═══════════════════════════════════════════════════════════════════╗\n";
    std::cout << "║                         ERGEBNIS                                   ║\n";
    std::cout << "╠═══════════════════════════════════════════════════════════════════╣\n";
    std::cout << "║  BESTANDEN: " << std::setw(3) << passed << "                                                  ║\n";
    std::cout << "║  GEFALLEN:  " << std::setw(3) << failed << "                                                  ║\n";
    std::cout << "╠═══════════════════════════════════════════════════════════════════╣\n";
    std::cout << "║  HCT_STABLE = " << std::setw(10) << HCT_STABLE << " (Herz-Kohärenz-Trigger)        ║\n";
    std::cout << "║  PHI_HEART  = " << std::setw(10) << PHI_HEART << " (Ziel-Kohärenz)                ║\n";
    std::cout << "╚═══════════════════════════════════════════════════════════════════╝\n";
    
    if (failed > 0) {
        std::cout << "\n⚠️  " << failed << " FORMELN SIND FALSCH!\n";
        return 1;
    } else {
        std::cout << "\n✓ ALLE BIO-DIGITALEN FORMELN SIND WAHR!\n";
        return 0;
    }
}
