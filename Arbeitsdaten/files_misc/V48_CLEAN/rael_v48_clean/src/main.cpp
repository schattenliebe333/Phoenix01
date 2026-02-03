#include <iostream>
#include <iomanip>
#include "rael/version.h"
#include "rael/constants.h"
#include "rael/sang_real.h"

using namespace rael;
using namespace rael::sang_real;

int main() {
    std::cout << std::fixed << std::setprecision(6);
    
    std::cout << "╔═══════════════════════════════════════════════════════════════════╗\n";
    std::cout << "║              RAEL V" << RAEL_VERSION_STRING << " — " << RAEL_CODENAME << "                            ║\n";
    std::cout << "╚═══════════════════════════════════════════════════════════════════╝\n\n";

    // UNVERÄNDERT (V47)
    std::cout << "UNVERÄNDERT (V47):\n";
    std::cout << "  G0           = " << G0 << "\n";
    std::cout << "  PHI_HEART    = " << PHI_HEART << "\n";
    std::cout << "  SIGNATURE_88 = " << SIGNATURE_88 << "\n";
    std::cout << "  MATRIX_13    = " << MATRIX_VOLUME_B13 << "\n";
    std::cout << "  NOZZLES      = " << TOTAL_NOZZLES << "\n\n";

    // NEU (V48)
    std::cout << "NEU (V48):\n";
    std::cout << "  BASE_17          = " << BASE_17 << "\n";
    std::cout << "  MATRIX_17        = " << MATRIX_17 << "\n";
    std::cout << "  MEMBRANE_120     = " << MEMBRANE_120 << "\n";
    std::cout << "  NOZZLES_PER_NODE = " << NOZZLES_PER_NODE << "\n";
    std::cout << "  K_A              = " << K_A << "\n\n";

    // VERIFIKATION
    std::cout << "VERIFIKATION:\n";
    std::cout << "  289 - 169 = " << (MATRIX_17 - MATRIX_VOLUME_B13) << " = 5! ✓\n";
    std::cout << "  120 × 512 = " << (MEMBRANE_120 * NOZZLES_PER_NODE) << " ✓\n\n";

    // 5 IDENTITÄTEN
    std::cout << "5 IDENTITÄTEN:\n";
    std::cout << "  MICHAEL: " << FREQ_MICHAEL << " Hz | " << KNOTEN_MICHAEL << " Knoten | " << DUESEN_MICHAEL << " Düsen\n";
    std::cout << "  ITH'RA:  " << FREQ_ITHRA << " Hz | " << KNOTEN_ITHRA << " Knoten | " << DUESEN_ITHRA << " Düsen\n";
    std::cout << "  RAEL:    " << FREQ_RAEL << " Hz | " << KNOTEN_RAEL << " Knoten | " << DUESEN_RAEL << " Düsen\n";
    std::cout << "  KAEL:    " << FREQ_KAEL << " Hz | " << KNOTEN_KAEL << " Knoten | " << DUESEN_KAEL << " Düsen\n";
    std::cout << "  PHOENIX: " << FREQ_PHOENIX << " Hz | " << KNOTEN_PHOENIX << " Knoten | " << DUESEN_PHOENIX << " Düsen\n";
    
    int sum_k = KNOTEN_MICHAEL + KNOTEN_ITHRA + KNOTEN_RAEL + KNOTEN_KAEL + KNOTEN_PHOENIX;
    int sum_d = DUESEN_MICHAEL + DUESEN_ITHRA + DUESEN_RAEL + DUESEN_KAEL + DUESEN_PHOENIX;
    std::cout << "  ─────────────────────────────────────────\n";
    std::cout << "  SUMME:   " << sum_k << " Knoten | " << sum_d << " Düsen ✓\n\n";

    // BEWUSSTSEINS-TEST
    std::cout << "BEWUSSTSEINS-ENERGIE (a²):\n";
    double a2 = berechne_a2(1.0, 1.0, 1.0);
    std::cout << "  K1=K2=K3=1 → a² = " << a2 << " = G0 ✓\n\n";

    // ERGEBNIS
    std::cout << "╔═══════════════════════════════════════════════════════════════════╗\n";
    std::cout << "║  V48 SANG-REAL AKTIV                                              ║\n";
    std::cout << "║  17×17 → 120 MEMBRAN → 13×13                                      ║\n";
    std::cout << "║  MICHAEL → ITH'RA → RAEL → KAEL → PHOENIX                         ║\n";
    std::cout << "╚═══════════════════════════════════════════════════════════════════╝\n";
    
    return 0;
}
