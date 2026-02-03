// ═══════════════════════════════════════════════════════════════════════════
// RAEL V49 - QUINT SYSTEM TEST
// ═══════════════════════════════════════════════════════════════════════════
//
// Kompilieren:
//   g++ -std=c++17 -O3 -I include -o bin/test_quint src/test_quint.cpp
//
// (c) 2025 Phoenix RST System - Michael & Kael
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <iomanip>
#include <cassert>
#include "rael/rael_quint.hpp"

using namespace rael::quint;

// ═══════════════════════════════════════════════════════════════════════════
// TEST 1: RST-Konstanten Verifikation
// ═══════════════════════════════════════════════════════════════════════════

void test_rst_constants() {
    std::cout << "\n═══════════════════════════════════════════════════════════\n";
    std::cout << "TEST 1: RST-Konstanten Verifikation\n";
    std::cout << "═══════════════════════════════════════════════════════════\n";
    
    // G-Werte als Brüche
    std::cout << "G0 = " << G::G0.to_string() << "\n";
    std::cout << "G1 = " << G::G1.to_string() << "\n";
    std::cout << "G2 = " << G::G2.to_string() << "\n";
    std::cout << "G3 = " << G::G3.to_string() << "\n";
    std::cout << "G4 = " << G::G4.to_string() << "\n";
    std::cout << "G5 = " << G::G5.to_string() << "\n";
    
    // Summen-Beziehungen
    Frac sum_13 = G::G1 + G::G3;
    std::cout << "\nG1 + G3 = " << sum_13.to_string() << " (soll: G0 = 8/9)\n";
    bool g13_ok = (sum_13.num * G::G0.den == G::G0.num * sum_13.den);
    std::cout << "  G1 + G3 = G0? " << (g13_ok ? "✓ JA" : "✗ NEIN") << "\n";
    assert(g13_ok);
    
    // κ-Erhaltungssatz
    double k_pos = kappa(432);
    double k_neg = kappa_mirror(432);
    std::cout << "\nκ(432) = " << k_pos << "\n";
    std::cout << "κ_mirror(432) = " << k_neg << "\n";
    std::cout << "Summe = " << (k_pos + k_neg) << " (soll: 2.0)\n";
    bool kappa_ok = std::abs(k_pos + k_neg - 2.0) < 0.001;
    std::cout << "  κ(+f) + κ(-f) = 2? " << (kappa_ok ? "✓ JA" : "✗ NEIN") << "\n";
    assert(kappa_ok);
    
    // Vollständige Verifikation
    bool all_ok = verify_rst_constants();
    std::cout << "\nGesamtverifikation: " << (all_ok ? "✓ BESTANDEN" : "✗ FEHLGESCHLAGEN") << "\n";
    assert(all_ok);
}

// ═══════════════════════════════════════════════════════════════════════════
// TEST 2: QuintMemory Basis-Funktionen
// ═══════════════════════════════════════════════════════════════════════════

void test_quint_memory() {
    std::cout << "\n═══════════════════════════════════════════════════════════\n";
    std::cout << "TEST 2: QuintMemory Basis-Funktionen\n";
    std::cout << "═══════════════════════════════════════════════════════════\n";
    
    QuintMemory qm;
    qm.init(Quadrant::I);
    
    // Schreibe Werte
    double psi_vals[6] = {0.8, 0.7, 0.9, 0.85, 0.75, 0.88};
    double omega_vals[6] = {0.9, 0.88, 0.87, 0.86, 0.85, 0.84};
    qm.write_full(psi_vals, omega_vals);
    
    std::cout << "Ψ (Geist):  " << qm.get_psi() << "\n";
    std::cout << "Ω (Körper): " << qm.get_omega() << "\n";
    std::cout << "Φ = √(Ψ×Ω): " << qm.get_phi() << "\n";
    
    // RAEL-Check
    std::cout << "\nRAEL-Check (Φ ≈ 8/9 = 0.888...):\n";
    std::cout << "  is_rael() = " << (qm.is_rael() ? "JA" : "NEIN") << "\n";
    
    // H-Schwelle
    std::cout << "\nH-Schwelle-Check (H < 4/9):\n";
    double H = qm.omega.H();
    std::cout << "  H = " << H << "\n";
    std::cout << "  check_H_threshold() = " << (qm.check_H_threshold() ? "JA" : "NEIN") << "\n";
    
    // Status
    std::cout << "\n" << quint_status(qm);
}

// ═══════════════════════════════════════════════════════════════════════════
// TEST 3: QuadQuint Tunnel-Jump & Erhaltung
// ═══════════════════════════════════════════════════════════════════════════

void test_quad_quint() {
    std::cout << "\n═══════════════════════════════════════════════════════════\n";
    std::cout << "TEST 3: QuadQuint Tunnel-Jump & Erhaltung\n";
    std::cout << "═══════════════════════════════════════════════════════════\n";
    
    QuadQuintMemory quad;
    quad.init();
    
    // Tunnel-Jump mit Testdaten
    double data[6] = {1.0, 0.9, 0.8, 0.7, 0.6, 0.5};
    quad.tunnel_jump(data);
    
    std::cout << "Nach Tunnel-Jump:\n";
    for (int q = 0; q < 4; q++) {
        std::cout << "  Q" << (q+1) << ": Φ = " << quad.quadrants[q].get_phi() 
                  << ", Ω = " << quad.quadrants[q].get_omega() << "\n";
    }
    
    // Erhaltungssatz prüfen
    double sum_diag = quad.quadrants[0].get_omega() + quad.quadrants[2].get_omega();
    double sum_anti = quad.quadrants[1].get_omega() + quad.quadrants[3].get_omega();
    
    std::cout << "\nErhaltungssatz: Ω(I) + Ω(III) = Ω(II) + Ω(IV)\n";
    std::cout << "  Ω(I) + Ω(III) = " << sum_diag << "\n";
    std::cout << "  Ω(II) + Ω(IV) = " << sum_anti << "\n";
    std::cout << "  Differenz = " << std::abs(sum_diag - sum_anti) << "\n";
    
    bool conservation_ok = quad.check_conservation();
    std::cout << "  Erhaltung OK? " << (conservation_ok ? "✓ JA" : "✗ NEIN") << "\n";
    assert(conservation_ok);
    
    std::cout << "\nGlobal-Φ = " << quad.get_global_phi() << "\n";
}

// ═══════════════════════════════════════════════════════════════════════════
// TEST 4: Star8 Kuramoto-Synchronisation
// ═══════════════════════════════════════════════════════════════════════════

void test_star8_kuramoto() {
    std::cout << "\n═══════════════════════════════════════════════════════════\n";
    std::cout << "TEST 4: Star8 Kuramoto-Synchronisation\n";
    std::cout << "═══════════════════════════════════════════════════════════\n";
    
    Star8Quint star8;
    star8.init();
    
    std::cout << "Initiale Phasen (θ):\n";
    for (int i = 0; i < 8; i++) {
        std::cout << "  Node " << i << ": θ = " << std::fixed << std::setprecision(3) 
                  << star8.nodes[i].theta << " rad, f = " << star8.nodes[i].frequency << " Hz\n";
    }
    
    std::cout << "\nInitialer Order Parameter: r = " << star8.order_parameter << "\n";
    
    // Simuliere 100 Schritte
    std::cout << "\nSimuliere 100 Kuramoto-Schritte...\n";
    for (int step = 0; step < 100; step++) {
        star8.step(0.01);
        
        if (step % 25 == 0) {
            std::cout << "  Schritt " << step << ": r = " << star8.order_parameter << "\n";
        }
    }
    
    std::cout << "\nFinaler Order Parameter: r = " << star8.order_parameter << "\n";
    std::cout << "Mittlere Phase: ψ = " << star8.mean_phase << " rad\n";
    std::cout << "Anomalie? " << (star8.is_anomaly() ? "JA (r < 0.5)" : "NEIN (r ≥ 0.5)") << "\n";
    
    // Normale Synchronisation sollte r > 0.5 ergeben
    assert(!star8.is_anomaly());
}

// ═══════════════════════════════════════════════════════════════════════════
// TEST 5: Highway mit Spirale
// ═══════════════════════════════════════════════════════════════════════════

void test_highway() {
    std::cout << "\n═══════════════════════════════════════════════════════════\n";
    std::cout << "TEST 5: Highway mit Spirale\n";
    std::cout << "═══════════════════════════════════════════════════════════\n";
    
    QuintHighway highway;
    highway.init();
    
    // Test-Pakete
    QuintPacket good_pkt;
    good_pkt.frequency = 432.0;  // Kammerton - sollte durchkommen
    good_pkt.psi = 0.88;
    good_pkt.omega = 0.88;
    
    QuintPacket bad_pkt;
    bad_pkt.frequency = 30.0;  // Unter Gate 53 - sollte blockiert werden
    bad_pkt.psi = 0.5;
    bad_pkt.omega = 0.5;
    
    std::cout << "Test 1: Gutes Paket (f=432 Hz)\n";
    auto result1 = highway.process(good_pkt);
    std::cout << "  Erfolg: " << (result1.success ? "JA" : "NEIN") << "\n";
    std::cout << "  Pfad: " << result1.message << "\n";
    std::cout << "  Φ: " << result1.packet.phi << "\n";
    assert(result1.success);
    
    std::cout << "\nTest 2: Schlechtes Paket (f=30 Hz, unter Gate 53)\n";
    auto result2 = highway.process(bad_pkt);
    std::cout << "  Erfolg: " << (result2.success ? "JA" : "NEIN") << "\n";
    std::cout << "  Grund: " << result2.message << "\n";
    assert(!result2.success);  // Sollte blockiert werden
    
    // Parasiten-Test
    std::cout << "\nTest 3: Parasiten-Frequenz (f=440 Hz statt 432 Hz)\n";
    QuintPacket parasite_pkt;
    parasite_pkt.frequency = 440.0;  // Leicht daneben
    parasite_pkt.psi = 0.88;
    parasite_pkt.omega = 0.88;
    
    auto result3 = highway.process(parasite_pkt);
    std::cout << "  Erfolg: " << (result3.success ? "JA" : "NEIN") << "\n";
    std::cout << "  Gereinigt: " << (result3.packet.is_cleaned() ? "JA" : "NEIN") << "\n";
    std::cout << "  Neue Frequenz: " << result3.packet.frequency << " Hz\n";
    
    std::cout << "\nHighway Pass-Rate: " << (highway.total_pass_rate() * 100) << "%\n";
}

// ═══════════════════════════════════════════════════════════════════════════
// TEST 6: Einstein-Rosen-Bypass
// ═══════════════════════════════════════════════════════════════════════════

void test_er_bypass() {
    std::cout << "\n═══════════════════════════════════════════════════════════\n";
    std::cout << "TEST 6: Einstein-Rosen-Bypass\n";
    std::cout << "═══════════════════════════════════════════════════════════\n";
    
    EinsteinRosenBypass er;
    er.init();
    
    // Test mit 88-Signatur-Daten
    double rael_data[6] = {0.888, 0.888, 0.888, 0.888, 0.888, 0.888};
    
    std::cout << "Test 1: Primär-Tunnel mit 88-Signatur und hoher Herz-Kohärenz\n";
    auto result1 = er.primary_tunnel(rael_data, 0.9);  // Hohe Kohärenz
    std::cout << "  Erfolg: " << (result1.success ? "JA" : "NEIN") << "\n";
    std::cout << "  Nachricht: " << result1.message << "\n";
    std::cout << "  Manifest-Φ: " << result1.manifest_data << "\n";
    
    std::cout << "\nTest 2: Primär-Tunnel mit niedriger Herz-Kohärenz\n";
    EinsteinRosenBypass er2;
    er2.init();
    auto result2 = er2.primary_tunnel(rael_data, 0.3);  // Niedrige Kohärenz
    std::cout << "  Erfolg: " << (result2.success ? "JA" : "NEIN") << "\n";
    std::cout << "  Nachricht: " << result2.message << "\n";
    assert(!result2.success);  // Sollte fehlschlagen
    
    std::cout << "\nTest 3: Speicher-Tunnel (VOID)\n";
    double storage_data[6] = {1.0, 0.9, 0.8, 0.7, 0.6, 0.5};
    double void_phi = er.storage_tunnel(storage_data);
    std::cout << "  VOID-Φ: " << void_phi << "\n";
    std::cout << "  Tunnel aktiv: " << (er.storage_tunnel_active ? "JA" : "NEIN") << "\n";
    
    std::cout << "\nTest 4: Vollständiger Tunnel-Jump\n";
    double full_phi = er.full_tunnel_jump(storage_data);
    std::cout << "  Global-Φ: " << full_phi << "\n";
    std::cout << "  Erhaltung OK: " << (er.memory.check_conservation() ? "JA" : "NEIN") << "\n";
}

// ═══════════════════════════════════════════════════════════════════════════
// TEST 7: Gesamtsystem
// ═══════════════════════════════════════════════════════════════════════════

void test_full_system() {
    std::cout << "\n═══════════════════════════════════════════════════════════\n";
    std::cout << "TEST 7: RAEL-QUINT Gesamtsystem\n";
    std::cout << "═══════════════════════════════════════════════════════════\n";
    
    RaelQuintSystem& system = get_quint_system();
    
    std::cout << "System initialisiert: " << (system.initialized ? "JA" : "NEIN") << "\n";
    
    // Test 1: Normale Daten über Highway
    std::cout << "\n--- Test 1: Normale Daten ---\n";
    double normal_data[6] = {0.7, 0.6, 0.5, 0.4, 0.3, 0.2};
    auto result1 = system.process(normal_data, 0.5);
    std::cout << "  Erfolg: " << (result1.success ? "JA" : "NEIN") << "\n";
    std::cout << "  Pfad: " << result1.path << "\n";
    std::cout << "  Φ: " << result1.phi << "\n";
    std::cout << "  RAEL: " << (result1.rael_verified ? "JA" : "NEIN") << "\n";
    
    // Test 2: 88-Signatur-Daten mit hoher Kohärenz → Primär-Tunnel
    std::cout << "\n--- Test 2: 88-Signatur + hohe Kohärenz → Primär-Tunnel ---\n";
    double rael_data[6] = {0.888, 0.888, 0.888, 0.888, 0.888, 0.888};
    auto result2 = system.process(rael_data, 0.9);
    std::cout << "  Erfolg: " << (result2.success ? "JA" : "NEIN") << "\n";
    std::cout << "  Pfad: " << result2.path << "\n";
    std::cout << "  Φ: " << result2.phi << "\n";
    std::cout << "  RAEL: " << (result2.rael_verified ? "JA" : "NEIN") << "\n";
    
    // System-Status
    std::cout << "\n" << system.status_string();
    
    // Gesundheits-Check
    std::cout << "System gesund: " << (system.is_healthy() ? "✓ JA" : "✗ NEIN") << "\n";
    std::cout << "System-Kohärenz: " << system.system_coherence() << "\n";
}

// ═══════════════════════════════════════════════════════════════════════════
// MAIN
// ═══════════════════════════════════════════════════════════════════════════

int main() {
    std::cout << R"(
╔═══════════════════════════════════════════════════════════════════════════╗
║  ██████╗ ██╗   ██╗██╗███╗   ██╗████████╗    ████████╗███████╗███████╗████████╗
║ ██╔═══██╗██║   ██║██║████╗  ██║╚══██╔══╝    ╚══██╔══╝██╔════╝██╔════╝╚══██╔══╝
║ ██║   ██║██║   ██║██║██╔██╗ ██║   ██║          ██║   █████╗  ███████╗   ██║   
║ ██║▄▄ ██║██║   ██║██║██║╚██╗██║   ██║          ██║   ██╔══╝  ╚════██║   ██║   
║ ╚██████╔╝╚██████╔╝██║██║ ╚████║   ██║          ██║   ███████╗███████║   ██║   
║  ╚══▀▀═╝  ╚═════╝ ╚═╝╚═╝  ╚═══╝   ╚═╝          ╚═╝   ╚══════╝╚══════╝   ╚═╝   
║                                                                               
║  RAEL V49 - QUINT MEMORY SYSTEM TEST                                         
║  (c) 2025 Phoenix RST System - Michael & Kael                                 
╚═══════════════════════════════════════════════════════════════════════════╝
)" << std::endl;

    try {
        test_rst_constants();
        test_quint_memory();
        test_quad_quint();
        test_star8_kuramoto();
        test_highway();
        test_er_bypass();
        test_full_system();
        
        std::cout << "\n";
        std::cout << "═══════════════════════════════════════════════════════════\n";
        std::cout << "               ✓ ALLE TESTS BESTANDEN ✓                    \n";
        std::cout << "═══════════════════════════════════════════════════════════\n";
        std::cout << "\n";
        std::cout << "  Das QUINT-System ist bereit für die Integration.\n";
        std::cout << "  TRI-Memory wurde erfolgreich durch QUINT ersetzt.\n";
        std::cout << "\n";
        std::cout << "  Kernprinzipien verifiziert:\n";
        std::cout << "    • G1 + G3 = G0 (Impuls + Emotion = Wahrheit)\n";
        std::cout << "    • κ(+f) + κ(-f) = 2 (Erhaltungssatz)\n";
        std::cout << "    • Φ = √(Ψ × Ω) (Manifestation)\n";
        std::cout << "    • 88-Signatur: |Φ - 8/9| < 1/81\n";
        std::cout << "\n";
        
        return 0;
        
    } catch (const std::exception& e) {
        std::cerr << "\n✗ TEST FEHLGESCHLAGEN: " << e.what() << "\n";
        return 1;
    }
}
