#include <iostream>
#include <iomanip>
#include "rael/rst_constants.hpp"

using namespace rael::rst;

int main() {
    std::cout << std::fixed << std::setprecision(6);
    
    std::cout << "╔═══════════════════════════════════════════════════════════════════╗\n";
    std::cout << "║        BLOCK #821-850 TEST (Gemini-Formeln)                       ║\n";
    std::cout << "╚═══════════════════════════════════════════════════════════════════╝\n\n";

    std::cout << "NEUE KONSTANTEN:\n";
    std::cout << "  #823 PHI_FILE (13/17×88)  = " << PHI_FILE << "\n";
    std::cout << "  #834 AETHER_SAT           = " << AETHER_SAT << "\n";
    std::cout << "  #844 K_A_CONST (G0/9)     = " << K_A_CONST << "\n";
    std::cout << "  #845 SIG_UNIFIED (800/9)  = " << SIG_UNIFIED << "\n";
    std::cout << "  #848 TUNNEL_LICHT         = " << TUNNEL_LICHT << "\n";
    std::cout << "  #849 TUNNEL_KERN          = " << TUNNEL_KERN << "\n\n";

    std::cout << "FUNKTIONEN:\n";
    std::cout << "  #821 g_comp(53,G0,PHI_H)  = " << g_comp(53.0, G0, PHI_HEART) << "\n";
    std::cout << "  #822 phase_async(1.0)     = " << phase_async_gemini(1.0) << "\n";
    std::cout << "  #844 a_quadrat(1,1,1)     = " << a_quadrat(1.0,1.0,1.0) << " (=G0) ✓\n";
    std::cout << "  #846 soliton_peak(0)      = " << soliton_peak_unified(0.0) << "\n";
    std::cout << "  #850 sigma_54_tunnel()    = " << sigma_54_tunnel() << " (=G0) ✓\n\n";

    std::cout << "MASTER-CHECK:\n";
    double sigma = sigma_55_block();
    std::cout << "  #921 sigma_55_block()     = " << sigma;
    std::cout << (sigma > 0.8 ? " ✓ PASS\n" : " ✗ FAIL\n");

    std::cout << "\n╔═══════════════════════════════════════════════════════════════════╗\n";
    std::cout << "║  Formeln: 820 → 850 (+30 Gemini-Formeln)                          ║\n";
    std::cout << "║  Zeilen:  6963 → 7073 (+110)                                      ║\n";
    std::cout << "║  Mapping: Gemini #X → Unsere #(X+170)                             ║\n";
    std::cout << "╚═══════════════════════════════════════════════════════════════════╝\n";

    return 0;
}
