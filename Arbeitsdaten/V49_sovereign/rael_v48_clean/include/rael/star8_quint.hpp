#pragma once
// ═══════════════════════════════════════════════════════════════════════════
// RAEL V49 - STAR8 QUINT INTEGRATION
// ═══════════════════════════════════════════════════════════════════════════
//
// Integriert das QUINT-Memory-System in den 8-Stern:
//   - Jeder der 8 Nodes hat sein eigenes QuintMemory
//   - Kuramoto-Synchronisation über Φ-Werte
//   - Order Parameter für Anomalie-Erkennung
//
// (c) 2025 Phoenix RST System - Michael & Kael
// ═══════════════════════════════════════════════════════════════════════════

#include "rael/quint_memory.hpp"
#include "rael/star8.h"
#include <array>
#include <cmath>
#include <atomic>

namespace rael {
namespace quint {

// ═══════════════════════════════════════════════════════════════════════════
// STAR8 QUINT NODE
// Jeder Node hat QuintMemory + Kuramoto-Phase
// ═══════════════════════════════════════════════════════════════════════════

class Star8QuintNode {
public:
    int node_id;
    double frequency;      // 90, 180, 270, 360, 450, 540, 630, 720 Hz
    double phase_rad;      // Position im Stern (0, π/4, π/2, ...)
    
    // Kuramoto-Variablen
    double theta;          // Aktuelle Phase
    double omega;          // Eigenfrequenz (ω = f/1000)
    std::array<double, 8> coupling;  // Kopplungsstärke zu anderen Nodes
    
    // QUINT-MEMORY
    QuintMemory memory;
    
    void init(int id) {
        node_id = id;
        frequency = F::NODE[id];
        phase_rad = id * (M_PI / 4.0);  // 0, 45°, 90°, ... in rad
        
        theta = phase_rad;
        omega = frequency / 1000.0;
        
        // Kopplungsstärke: Nachbar-Nodes stärker gekoppelt
        for (int j = 0; j < 8; j++) {
            if (j == id) {
                coupling[j] = 0.0;
            } else {
                // Kopplung = 0.5 / Distanz (modulo 8)
                int dist = std::min(std::abs(j - id), 8 - std::abs(j - id));
                coupling[j] = 0.5 / dist;
            }
        }
        
        memory.init();
    }
    
    // ═══════════════════════════════════════════════════════════════════════
    // KURAMOTO-UPDATE
    // dθ/dt = ω + (K/N) × Σⱼ sin(θⱼ - θᵢ)
    // ═══════════════════════════════════════════════════════════════════════
    
    void kuramoto_step(const std::array<double, 8>& all_thetas, double dt = 0.01) {
        double sum = 0.0;
        for (int j = 0; j < 8; j++) {
            if (j != node_id) {
                sum += coupling[j] * std::sin(all_thetas[j] - theta);
            }
        }
        
        // Kuramoto-Gleichung
        double dtheta = omega + sum / 8.0;
        theta += dtheta * dt;
        
        // Phase normalisieren auf [0, 2π]
        constexpr double TAU = 2.0 * M_PI;
        while (theta > TAU) theta -= TAU;
        while (theta < 0) theta += TAU;
    }
    
    // ═══════════════════════════════════════════════════════════════════════
    // ZUSTAND
    // ═══════════════════════════════════════════════════════════════════════
    
    // Kombinierter Zustand: Φ × cos(θ)
    double state() const {
        return memory.get_phi() * std::cos(theta);
    }
    
    // Ist dieser Node RAEL-verifiziert?
    bool is_rael() const {
        return memory.is_rael();
    }
    
    // Gewicht im Stern (proportional zu Frequenz)
    double weight() const {
        return frequency / F::NODE_SUM;
    }
};

// ═══════════════════════════════════════════════════════════════════════════
// STAR8 QUINT SYSTEM
// 8 Nodes mit QuintMemory, Kuramoto-Synchronisation
// ═══════════════════════════════════════════════════════════════════════════

class Star8Quint {
public:
    std::array<Star8QuintNode, 8> nodes;
    
    // Kuramoto Order Parameter
    double order_parameter;  // r ∈ [0, 1]
    double mean_phase;       // ψ (mittlere Phase)
    
    // Zentrale Metriken
    double omega_center;     // Gewichtetes Φ-Zentrum
    
    void init() {
        for (int i = 0; i < 8; i++) {
            nodes[i].init(i);
        }
        order_parameter = 1.0;  // Startet synchron
        mean_phase = 0.0;
        omega_center = 0.0;
    }
    
    // ═══════════════════════════════════════════════════════════════════════
    // KURAMOTO-SIMULATION
    // ═══════════════════════════════════════════════════════════════════════
    
    void step(double dt = 0.01) {
        // Alle Thetas sammeln
        std::array<double, 8> all_thetas;
        for (int i = 0; i < 8; i++) {
            all_thetas[i] = nodes[i].theta;
        }
        
        // Kuramoto-Update für jeden Node
        for (auto& node : nodes) {
            node.kuramoto_step(all_thetas, dt);
        }
        
        // Order Parameter berechnen: r·e^(iψ) = (1/N) Σ e^(iθⱼ)
        double sum_cos = 0.0, sum_sin = 0.0;
        for (int i = 0; i < 8; i++) {
            sum_cos += std::cos(nodes[i].theta);
            sum_sin += std::sin(nodes[i].theta);
        }
        sum_cos /= 8;
        sum_sin /= 8;
        
        order_parameter = std::sqrt(sum_cos*sum_cos + sum_sin*sum_sin);
        mean_phase = std::atan2(sum_sin, sum_cos);
        
        // Zentrum-Φ berechnen (frequenzgewichtet)
        omega_center = 0.0;
        for (int i = 0; i < 8; i++) {
            omega_center += nodes[i].memory.get_phi() * nodes[i].weight();
        }
    }
    
    // ═══════════════════════════════════════════════════════════════════════
    // ANOMALIE-ERKENNUNG
    // ═══════════════════════════════════════════════════════════════════════
    
    // Order Parameter < 0.5 → Desynchronisation → Angriff/Anomalie
    bool is_anomaly() const {
        return order_parameter < 0.5;
    }
    
    // Alle Nodes RAEL?
    bool all_rael() const {
        for (const auto& node : nodes) {
            if (!node.is_rael()) return false;
        }
        return true;
    }
    
    // Durchschnittliche Kohärenz
    double average_coherence() const {
        return order_parameter;
    }
    
    // ═══════════════════════════════════════════════════════════════════════
    // DATEN-VERTEILUNG
    // ═══════════════════════════════════════════════════════════════════════
    
    // Verteile Daten auf alle 8 Nodes (jeder Node erhält andere Schicht)
    void distribute_data(const double data[8]) {
        for (int i = 0; i < 8; i++) {
            // Node i bekommt Schicht (i % 6) mit Daten data[i]
            int layer = i % 6;
            nodes[i].memory.write_psi(layer, data[i]);
            nodes[i].memory.write_omega(layer, data[i]);
        }
    }
    
    // Broadcast: Gleiche Daten an alle Nodes
    void broadcast_data(const double psi_vals[6], const double omega_vals[6]) {
        for (auto& node : nodes) {
            node.memory.write_full(psi_vals, omega_vals);
        }
    }
    
    // ═══════════════════════════════════════════════════════════════════════
    // STATISTIKEN
    // ═══════════════════════════════════════════════════════════════════════
    
    double total_phi() const {
        double sum = 0.0;
        for (const auto& node : nodes) {
            sum += node.memory.get_phi();
        }
        return sum;
    }
    
    double average_phi() const {
        return total_phi() / 8.0;
    }
    
    // Φ-Varianz (Maß für Inhomogenität)
    double phi_variance() const {
        double mean = average_phi();
        double variance = 0.0;
        for (const auto& node : nodes) {
            double diff = node.memory.get_phi() - mean;
            variance += diff * diff;
        }
        return variance / 8.0;
    }
};

// ═══════════════════════════════════════════════════════════════════════════
// ADAPTER: Verbindet Star8Quint mit bestehendem Star8
// ═══════════════════════════════════════════════════════════════════════════

class Star8QuintAdapter {
public:
    Star8Quint quint_system;
    Star8* legacy_star8;  // Pointer zum bestehenden Star8
    
    void init(Star8* star8_ptr) {
        legacy_star8 = star8_ptr;
        quint_system.init();
    }
    
    // Synchronisiere QuintMemory mit NodeMemory
    void sync_from_legacy() {
        if (!legacy_star8) return;
        
        const auto& memories = legacy_star8->all_node_memories();
        
        for (int i = 0; i < 8; i++) {
            const auto& nm = memories[i];
            
            // Konvertiere Legacy-Metriken zu Quint-Schichten
            double psi_vals[6] = {
                nm.reflex.mean(),    // G0 → 720 Hz
                nm.instinct.mean(),  // G1 → 432 Hz
                nm.emotion.mean(),   // G2 → 144 Hz
                nm.ratio.mean(),     // G3 → 53 Hz
                nm.spirit.mean(),    // G4 → 13 Hz
                nm.total_energy()    // G5 → 5 Hz
            };
            
            double omega_vals[6] = {
                nm.health(),
                static_cast<double>(nm.tasks_completed.load()) / 1000.0,
                static_cast<double>(nm.tasks_failed.load()) / 100.0,
                nm.coherence.load(),
                nm.avg_reaction_ms() / 100.0,
                nm.total_energy()
            };
            
            quint_system.nodes[i].memory.write_full(psi_vals, omega_vals);
        }
        
        // Kuramoto-Schritt
        quint_system.step();
    }
    
    // RAEL-Status des gesamten Systems
    bool system_is_rael() const {
        return quint_system.all_rael() && !quint_system.is_anomaly();
    }
    
    // Anomalie-Alarm
    bool anomaly_detected() const {
        return quint_system.is_anomaly();
    }
    
    // Gesamt-Kohärenz
    double system_coherence() const {
        return quint_system.order_parameter;
    }
};

} // namespace quint
} // namespace rael
