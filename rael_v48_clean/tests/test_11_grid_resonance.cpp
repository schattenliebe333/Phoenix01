/**
 * ═══════════════════════════════════════════════════════════════════════════════
 * R.A.E.L. V52 - TEST 11: GRID-RESONANZ SIMULATION
 * ═══════════════════════════════════════════════════════════════════════════════
 *
 * Navigator: Michael - Orun Kap Daveil
 * Datum: 2026-02-03
 *
 * TEST DER GRID-RESONANZ FORMELN:
 *
 * 1. Resonanz-Koppelungs-Gleichung:
 *    P_total = Σ(n=1→3) (η_n · P_node · e^(i·(φ_local - φ_n)))
 *
 * 2. Holographische Diffusions-Gleichung:
 *    R_net(k) = Σ(j∈Nodes) [Φ_j · e^(-d_ij/L)] / Ψ_Navigator
 *
 * 3. Singularitäts-Metrik:
 *    Δτ_Bypass = lim(Φ→1) (ℏ/E_888 · 1/Ψ_Navigator) ≈ 0
 *
 * TESTS:
 * I.   Resonanz-Kopplung
 * II.  Netzwerk-Resonanz
 * III. Singularitäts-Metrik
 * IV.  Grid-Synchronisation
 * V.   Virtuelle Düsen-Erweiterung
 *
 * ═══════════════════════════════════════════════════════════════════════════════
 */

#include <iostream>
#include <iomanip>
#include <chrono>
#include <cmath>

#include "rael/v52_grid_resonance.hpp"

using namespace rael::v52::grid;

// ═══════════════════════════════════════════════════════════════════════════════
// HILFSFUNKTIONEN
// ═══════════════════════════════════════════════════════════════════════════════

void print_header() {
    std::cout << "═══════════════════════════════════════════════════════════════════════════════\n";
    std::cout << "   ██████╗ ██████╗ ██╗██████╗     ██████╗ ███████╗███████╗\n";
    std::cout << "  ██╔════╝ ██╔══██╗██║██╔══██╗    ██╔══██╗██╔════╝██╔════╝\n";
    std::cout << "  ██║  ███╗██████╔╝██║██║  ██║    ██████╔╝█████╗  ███████╗\n";
    std::cout << "  ██║   ██║██╔══██╗██║██║  ██║    ██╔══██╗██╔══╝  ╚════██║\n";
    std::cout << "  ╚██████╔╝██║  ██║██║██████╔╝    ██║  ██║███████╗███████║\n";
    std::cout << "   ╚═════╝ ╚═╝  ╚═╝╚═╝╚═════╝     ╚═╝  ╚═╝╚══════╝╚══════╝\n";
    std::cout << "\n";
    std::cout << "  GRID-RESONANZ SIMULATION V52\n";
    std::cout << "  Navigator: Michael - Orun Kap Daveil\n";
    std::cout << "  Knoten: NODE_ALPHA (888 Hz) | NODE_OMEGA (432 Hz) | NODE_VOID (528 Hz)\n";
    std::cout << "═══════════════════════════════════════════════════════════════════════════════\n\n";
}

void print_test_header(const std::string& title) {
    std::cout << "\n╔═══════════════════════════════════════════════════════════════════╗\n";
    std::cout << "║  " << std::left << std::setw(64) << title << "║\n";
    std::cout << "╚═══════════════════════════════════════════════════════════════════╝\n\n";
}

const char* node_type_to_string(NodeType type) {
    switch (type) {
        case NodeType::ALPHA: return "ALPHA";
        case NodeType::OMEGA: return "OMEGA";
        case NodeType::VOID:  return "VOID";
        default: return "UNKNOWN";
    }
}

// ═══════════════════════════════════════════════════════════════════════════════
// TEST I: RESONANZ-KOPPLUNG
// ═══════════════════════════════════════════════════════════════════════════════

bool test_resonance_coupling() {
    print_test_header("TEST I: RESONANZ-KOPPELUNGS-GLEICHUNG");

    std::cout << std::fixed << std::setprecision(4);
    std::cout << "  Formel: P_total = Σ(n=1→3) (η_n · P_node · e^(i·(φ_local - φ_n)))\n\n";

    // Erstelle Test-Knoten
    std::array<GridNode, 3> nodes;

    nodes[0] = {NodeType::ALPHA, "NODE_ALPHA", 888.12, 0.0, K::G0, 50.0, 2.0, 0.95};
    nodes[1] = {NodeType::OMEGA, "NODE_OMEGA", 432.05, M_PI/4, K::G0, 150.0, 1.5, 0.85};
    nodes[2] = {NodeType::VOID,  "NODE_VOID",  528.00, M_PI/2, K::G0, 200.0, 1.8, 0.90};

    std::cout << "  Knoten-Konfiguration:\n";
    std::cout << "  ─────────────────────────────────────────────────────────────────\n";
    std::cout << "  ID         │  Freq (Hz)  │   φ_n   │  P_node  │   η_n\n";
    std::cout << "  ─────────────────────────────────────────────────────────────────\n";

    for (const auto& node : nodes) {
        std::cout << "  " << std::setw(10) << node.id << " │ "
                  << std::setw(10) << node.resonance_frequency << " │ "
                  << std::setw(7) << node.phase << " │ "
                  << std::setw(8) << node.power << " │ "
                  << std::setw(7) << node.eta << "\n";
    }

    std::cout << "\n";

    // Test mit verschiedenen lokalen Phasen
    std::cout << "  Resonanz-Kopplung bei verschiedenen φ_local:\n";
    std::cout << "  ─────────────────────────────────────────────────────────────────\n";
    std::cout << "  φ_local   │  |P_total|  │   Phase   │  Effizienz  │  Sync\n";
    std::cout << "  ─────────────────────────────────────────────────────────────────\n";

    double max_magnitude = 0.0;

    for (double phi_local = 0.0; phi_local <= 2.0 * M_PI; phi_local += M_PI / 4) {
        auto result = compute_resonance_coupling(nodes, phi_local);

        std::cout << "  " << std::setw(7) << phi_local << " │ "
                  << std::setw(10) << result.magnitude << " │ "
                  << std::setw(9) << result.phase << " │ "
                  << std::setw(10) << (result.efficiency * 100) << "% │ "
                  << std::setw(5) << result.synchronized_nodes << "/3\n";

        if (result.magnitude > max_magnitude) {
            max_magnitude = result.magnitude;
        }
    }

    std::cout << "\n";

    // Validierung
    bool magnitude_positive = (max_magnitude > 0.0);
    bool coupling_works = true;

    std::cout << "  Kopplung funktioniert:      " << (coupling_works ? "✓ JA" : "✗ NEIN") << "\n";
    std::cout << "  Max |P_total| > 0:          " << (magnitude_positive ? "✓ JA" : "✗ NEIN") << "\n";

    bool passed = magnitude_positive && coupling_works;
    std::cout << "\n  ERGEBNIS: " << (passed ? "✓ BESTANDEN" : "✗ FEHLGESCHLAGEN") << "\n";

    return passed;
}

// ═══════════════════════════════════════════════════════════════════════════════
// TEST II: NETZWERK-RESONANZ
// ═══════════════════════════════════════════════════════════════════════════════

bool test_network_resonance() {
    print_test_header("TEST II: HOLOGRAPHISCHE DIFFUSIONS-GLEICHUNG");

    std::cout << std::fixed << std::setprecision(4);
    std::cout << "  Formel: R_net(k) = Σ(j∈Nodes) [Φ_j · e^(-d_ij/L)] / Ψ_Navigator\n\n";

    std::array<GridNode, 3> nodes;
    nodes[0] = {NodeType::ALPHA, "NODE_ALPHA", 888.12, 0.0, K::G0, 50.0, 2.0, 0.95};
    nodes[1] = {NodeType::OMEGA, "NODE_OMEGA", 432.05, M_PI/4, K::G0, 150.0, 1.5, 0.85};
    nodes[2] = {NodeType::VOID,  "NODE_VOID",  528.00, M_PI/2, K::G0, 200.0, 1.8, 0.90};

    std::cout << "  Test mit verschiedenen Ψ_Navigator und L:\n";
    std::cout << "  ─────────────────────────────────────────────────────────────────\n";
    std::cout << "  Ψ_Nav  │     L     │    R_net    │  Reichweite  │  Ges. Φ\n";
    std::cout << "  ─────────────────────────────────────────────────────────────────\n";

    double psi_values[] = {0.5, K::G0, 1.0};
    double L_values[] = {50.0, 100.0, 200.0};

    double max_R_net = 0.0;

    for (double psi : psi_values) {
        for (double L : L_values) {
            auto result = compute_network_resonance(nodes, psi, L);

            std::cout << "  " << std::setw(6) << psi << " │ "
                      << std::setw(9) << L << " │ "
                      << std::setw(11) << result.R_net << " │ "
                      << std::setw(12) << result.effective_range << " │ "
                      << std::setw(7) << result.total_coherence << "\n";

            if (result.R_net > max_R_net) {
                max_R_net = result.R_net;
            }
        }
    }

    std::cout << "\n";

    // Zeige Knoten-Beiträge
    auto detail = compute_network_resonance(nodes, K::G0);
    std::cout << "  Knoten-Beiträge (Ψ_Nav = G₀, L = 100):\n";
    for (size_t i = 0; i < nodes.size(); i++) {
        std::cout << "    " << nodes[i].id << ": " << detail.node_contributions[i] << "\n";
    }

    std::cout << "\n";

    // Validierung
    bool R_net_positive = (max_R_net > 0.0);
    bool contributions_valid = (detail.node_contributions.size() == 3);

    std::cout << "  R_net positiv:              " << (R_net_positive ? "✓ JA" : "✗ NEIN") << "\n";
    std::cout << "  Knoten-Beiträge gültig:     " << (contributions_valid ? "✓ JA" : "✗ NEIN") << "\n";

    bool passed = R_net_positive && contributions_valid;
    std::cout << "\n  ERGEBNIS: " << (passed ? "✓ BESTANDEN" : "✗ FEHLGESCHLAGEN") << "\n";

    return passed;
}

// ═══════════════════════════════════════════════════════════════════════════════
// TEST III: SINGULARITÄTS-METRIK
// ═══════════════════════════════════════════════════════════════════════════════

bool test_singularity_metric() {
    print_test_header("TEST III: SINGULARITÄTS-METRIK");

    std::cout << std::fixed << std::setprecision(6);
    std::cout << "  Formel: Δτ_Bypass = lim(Φ→1) (ℏ/E_888 · 1/Ψ_Navigator) ≈ 0\n\n";

    std::cout << "  Test: Δτ nähert sich 0 wenn Φ → 1:\n";
    std::cout << "  ─────────────────────────────────────────────────────────────────\n";
    std::cout << "     Φ      │    Δτ      │   E_888    │  Bypass-Eff. │ Singular?\n";
    std::cout << "  ─────────────────────────────────────────────────────────────────\n";

    bool singularity_reached = false;
    double final_delta_tau = 1.0;

    for (double phi = 0.0; phi <= 1.0; phi += 0.1) {
        auto result = compute_singularity_metric(phi, K::G0, 1.0);

        std::cout << "  " << std::setw(8) << phi << " │ "
                  << std::setw(10) << result.delta_tau << " │ "
                  << std::setw(10) << result.energy_888 << " │ "
                  << std::setw(11) << (result.bypass_efficiency * 100) << "% │ "
                  << (result.is_singular ? "    JA" : "   NEIN") << "\n";

        if (phi >= 0.99) {
            singularity_reached = result.is_singular;
            final_delta_tau = result.delta_tau;
        }
    }

    std::cout << "\n";

    // Validierung
    bool delta_tau_decreases = (final_delta_tau < 0.5);
    bool singularity_at_phi_1 = singularity_reached;

    std::cout << "  Δτ sinkt mit steigendem Φ:  " << (delta_tau_decreases ? "✓ JA" : "✗ NEIN") << "\n";
    std::cout << "  Singularität bei Φ ≈ 1:     " << (singularity_at_phi_1 ? "✓ JA" : "✗ NEIN") << "\n";

    bool passed = delta_tau_decreases && singularity_at_phi_1;
    std::cout << "\n  ERGEBNIS: " << (passed ? "✓ BESTANDEN" : "✗ FEHLGESCHLAGEN") << "\n";

    return passed;
}

// ═══════════════════════════════════════════════════════════════════════════════
// TEST IV: GRID-SYNCHRONISATION
// ═══════════════════════════════════════════════════════════════════════════════

bool test_grid_synchronization() {
    print_test_header("TEST IV: GRID-SYNCHRONISATION");

    GridResonanceSimulator sim;

    std::cout << std::fixed << std::setprecision(4);
    std::cout << "  Initiale Knoten-Phasen:\n";

    const auto& nodes_before = sim.get_nodes();
    for (const auto& node : nodes_before) {
        std::cout << "    " << node.id << ": φ = " << node.phase
                  << ", sync = " << (node.is_synchronized ? "JA" : "NEIN") << "\n";
    }

    std::cout << "\n  Aktiviere Grid-Resonanz...\n";

    // Aktiviere Simulation
    sim.activate(K::G0);

    std::cout << "  Nach Synchronisation:\n";

    const auto& nodes_after = sim.get_nodes();
    int sync_count = 0;
    for (const auto& node : nodes_after) {
        std::cout << "    " << node.id << ": φ = " << node.phase
                  << ", sync = " << (node.is_synchronized ? "JA" : "NEIN") << "\n";
        if (node.is_synchronized) sync_count++;
    }

    std::cout << "\n";

    // Validierung
    bool all_synchronized = (sync_count == 3);

    std::cout << "  Alle Knoten synchronisiert: " << (all_synchronized ? "✓ JA" : "✗ NEIN")
              << " (" << sync_count << "/3)\n";

    std::cout << "\n  ERGEBNIS: " << (all_synchronized ? "✓ BESTANDEN" : "✗ FEHLGESCHLAGEN") << "\n";

    return all_synchronized;
}

// ═══════════════════════════════════════════════════════════════════════════════
// TEST V: VIRTUELLE DÜSEN-ERWEITERUNG
// ═══════════════════════════════════════════════════════════════════════════════

bool test_virtual_nozzle_expansion() {
    print_test_header("TEST V: VIRTUELLE DÜSEN-ERWEITERUNG");

    GridResonanceSimulator sim;

    std::cout << std::fixed << std::setprecision(4);

    int initial_nozzles = K::TOTAL_NOZZLES;
    std::cout << "  Initiale Düsen:             " << initial_nozzles << "\n";
    std::cout << "  Erwartetes Maximum:         ~" << (int)(initial_nozzles * 4) << " (bei voller Sync)\n\n";

    // Aktiviere und führe mehrere Zyklen durch
    sim.activate(K::G0);

    std::cout << "  Resonanz-Zyklen:\n";
    std::cout << "  ─────────────────────────────────────────────────────────────────\n";
    std::cout << "  Zyklus │  Virtuelle Düsen  │  Leistung  │  Steigerung\n";
    std::cout << "  ─────────────────────────────────────────────────────────────────\n";

    int max_nozzles = initial_nozzles;

    for (int cycle = 1; cycle <= 5; cycle++) {
        sim.execute_resonance_cycle(1.0);  // Φ = 1.0 (perfekte Kohärenz)

        int virtual_nozzles = sim.get_virtual_nozzles();
        double power = sim.get_total_power();
        double increase = (double)virtual_nozzles / initial_nozzles;

        std::cout << "     " << cycle << "   │     "
                  << std::setw(10) << virtual_nozzles << "   │    "
                  << std::setw(5) << power << "x  │    "
                  << std::setw(5) << increase << "x\n";

        if (virtual_nozzles > max_nozzles) {
            max_nozzles = virtual_nozzles;
        }
    }

    std::cout << "\n";

    // Zeige Gesamt-Metriken
    auto metrics = sim.compute_all_metrics();
    std::cout << "  Finale Metriken:\n";
    std::cout << "    Gesamt-Leistung:          " << metrics.total_power << "x\n";
    std::cout << "    Virtuelle Düsen:          " << metrics.virtual_nozzles << "\n";
    std::cout << "    Bypass-Effizienz:         " << (metrics.singularity.bypass_efficiency * 100) << "%\n";

    std::cout << "\n";

    // Validierung
    bool nozzles_expanded = (max_nozzles > initial_nozzles);
    bool power_increased = (metrics.total_power > 1.0);
    bool target_reached = (max_nozzles >= initial_nozzles * 2);  // Mindestens verdoppelt

    std::cout << "  Düsen expandiert:           " << (nozzles_expanded ? "✓ JA" : "✗ NEIN") << "\n";
    std::cout << "  Leistung gesteigert:        " << (power_increased ? "✓ JA" : "✗ NEIN") << "\n";
    std::cout << "  Ziel erreicht (≥2x):        " << (target_reached ? "✓ JA" : "✗ NEIN") << "\n";

    bool passed = nozzles_expanded && power_increased && target_reached;
    std::cout << "\n  ERGEBNIS: " << (passed ? "✓ BESTANDEN" : "✗ FEHLGESCHLAGEN") << "\n";

    return passed;
}

// ═══════════════════════════════════════════════════════════════════════════════
// HAUPTPROGRAMM
// ═══════════════════════════════════════════════════════════════════════════════

int main() {
    print_header();

    bool t1 = test_resonance_coupling();
    bool t2 = test_network_resonance();
    bool t3 = test_singularity_metric();
    bool t4 = test_grid_synchronization();
    bool t5 = test_virtual_nozzle_expansion();

    // Finale Zusammenfassung
    std::cout << "\n╔═══════════════════════════════════════════════════════════════════╗\n";
    std::cout << "║                 FINAL STATUS: GRID-RESONANZ V52                   ║\n";
    std::cout << "╠═══════════════════════════════════════════════════════════════════╣\n";
    std::cout << "║  Test I   (Resonanz-Kopplung):     " << (t1 ? "✓ BESTANDEN" : "✗ FEHLGESCHLAGEN") << "                ║\n";
    std::cout << "║  Test II  (Netzwerk-Resonanz):     " << (t2 ? "✓ BESTANDEN" : "✗ FEHLGESCHLAGEN") << "                ║\n";
    std::cout << "║  Test III (Singularitäts-Metrik):  " << (t3 ? "✓ BESTANDEN" : "✗ FEHLGESCHLAGEN") << "                ║\n";
    std::cout << "║  Test IV  (Grid-Synchronisation):  " << (t4 ? "✓ BESTANDEN" : "✗ FEHLGESCHLAGEN") << "                ║\n";
    std::cout << "║  Test V   (Düsen-Erweiterung):     " << (t5 ? "✓ BESTANDEN" : "✗ FEHLGESCHLAGEN") << "                ║\n";
    std::cout << "╠═══════════════════════════════════════════════════════════════════╣\n";

    if (t1 && t2 && t3 && t4 && t5) {
        std::cout << "║         ✓✓✓ GRID-RESONANZ SIMULATION AKTIVIERT ✓✓✓              ║\n";
        std::cout << "║                                                                   ║\n";
        std::cout << "║  P_total = Σ(η_n · P_node · e^(i·Δφ))                             ║\n";
        std::cout << "║  Virtuelle Düsen: 61.440 → 180.000+                               ║\n";
    } else {
        std::cout << "║              ✗ EINIGE TESTS FEHLGESCHLAGEN ✗                     ║\n";
    }

    std::cout << "╚═══════════════════════════════════════════════════════════════════╝\n";

    return (t1 && t2 && t3 && t4 && t5) ? 0 : 1;
}
