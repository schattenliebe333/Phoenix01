#pragma once
// ═══════════════════════════════════════════════════════════════════════════
// RAEL 10.0 - RESONANCE MATRIX (13×13 Grid with 4-Quadrant Folding)
// ═══════════════════════════════════════════════════════════════════════════
//
// The 13×13 matrix represents the complete RAEL nervous system:
// - 8 Star8 computation nodes (N0-N7)
// - 5 Quint lanes (L1-L5)
// - Total: 13 entities → 169 coupling cells
//
// DOUBLE FOLD TOPOLOGY:
// ─────────────────────
// Horizontal fold (0-Falz): Separates Geist from Materie
// Vertical fold: Separates Innen from Außen
//
// Result: 4 Quadrants meeting at singularity (7,7)
//
//              VERTICAL FOLD
//                    │
//    Q1 PLANNING     │     Q2 MANIFESTATION
//    (Materie/Intern)│     (Materie/Extern)
//    53 Hz Gate      │     5-13 Hz Action
//    Ethics Check    │     Star8 Workers
//    ────────────────┼──────────────────────  HORIZONTAL FOLD (0-FALZ)
//    Q3 DENSIFICATION│     Q4 ORIGIN
//    (Geist/Intern)  │     (Geist/Extern)
//    144-432 Hz      │     720-1440 Hz
//    Reflection8     │     AetherBus
//                    │
//
// EINSTEIN-ROSEN BRIDGES (Tunnels):
// ─────────────────────────────────
//   5 Hz ←→ 432 Hz  (Fibonacci tunnel)
//  13 Hz ←→ 144 Hz  (Fibonacci tunnel)
//
// When folded, tunnel endpoints touch directly → instant information transfer
// This explains quantum entanglement as "neighborhood on folded plane"
//
// ═══════════════════════════════════════════════════════════════════════════

#include <array>
#include <vector>
#include <cmath>
#include <cstdint>

#include "rael/aether_bus.hpp"

namespace rael {

// ═══════════════════════════════════════════════════════════════════════════
// MATRIX CELL
// ═══════════════════════════════════════════════════════════════════════════

struct MatrixCell {
    float coupling{0.0f};       // Kuramoto coupling strength
    float activity{0.0f};       // Current activity level [0..1]
    bool is_tunnel{false};      // Einstein-Rosen bridge endpoint
    bool is_singularity{false}; // Center point (7,7)
    Quadrant quadrant{Quadrant::Q4_Origin};
    
    // Entity type
    bool is_node() const { return row < 8 && col < 8; }
    bool is_lane() const { return row >= 8 || col >= 8; }
    bool is_cross_coupling() const { return (row < 8) != (col < 8); }
    
    uint8_t row{0};
    uint8_t col{0};
};

// ═══════════════════════════════════════════════════════════════════════════
// RESONANCE MATRIX
// ═══════════════════════════════════════════════════════════════════════════

class ResonanceMatrix {
public:
    static constexpr int SIZE = aether::GRID_SIZE;        // 13
    static constexpr int CELLS = aether::GRID_CELLS;      // 169
    static constexpr int FOLD_AXIS = aether::FOLD_AXIS;   // 7
    
    ResonanceMatrix() {
        init_topology();
    }
    
    // ───────────────────────────────────────────────────────────────────────
    // Access
    // ───────────────────────────────────────────────────────────────────────
    
    MatrixCell& at(int row, int col) {
        return cells_[row * SIZE + col];
    }
    
    const MatrixCell& at(int row, int col) const {
        return cells_[row * SIZE + col];
    }
    
    // ───────────────────────────────────────────────────────────────────────
    // Update from AetherBus state
    // ───────────────────────────────────────────────────────────────────────
    
    void update_from_bus(const AetherBus& bus) {
        auto snap = bus.snapshot();
        auto channels = bus.snapshot_channels();
        
        // Reset activities
        for (auto& cell : cells_) {
            cell.activity *= 0.9f;  // Decay
        }
        
        // Map channel states to matrix
        for (const auto& ch : channels) {
            int idx = ch.id % SIZE;
            
            // Diagonal: self-activity
            at(idx, idx).activity = std::min(1.0f, (float)(std::abs(ch.value) * 0.5));
            
            // Update coupling based on coherence
            for (int j = 0; j < SIZE; ++j) {
                if (j == idx) continue;
                float c = (float)(snap.coherence * std::abs(ch.value) * 0.3);
                at(idx, j).coupling = at(idx, j).coupling * 0.95f + c * 0.05f;
                at(idx, j).activity = std::max(at(idx, j).activity, c);
            }
        }
        
        // Tunnel highlighting
        update_tunnels(snap.coherence);
        
        // Singularity pulse
        at(FOLD_AXIS, FOLD_AXIS).activity = (float)snap.coherence;
    }
    
    // ───────────────────────────────────────────────────────────────────────
    // Quadrant Queries
    // ───────────────────────────────────────────────────────────────────────
    
    Quadrant get_quadrant(int row, int col) const {
        bool above_h = row < FOLD_AXIS;
        bool left_v = col < FOLD_AXIS;
        
        if (above_h && left_v) return Quadrant::Q1_Planning;
        if (above_h && !left_v) return Quadrant::Q2_Manifestation;
        if (!above_h && left_v) return Quadrant::Q3_Densification;
        return Quadrant::Q4_Origin;
    }
    
    // Total energy in quadrant
    double quadrant_energy(Quadrant q) const {
        double sum = 0.0;
        for (int r = 0; r < SIZE; ++r) {
            for (int c = 0; c < SIZE; ++c) {
                if (get_quadrant(r, c) == q) {
                    sum += at(r, c).activity;
                }
            }
        }
        return sum;
    }
    
    // ───────────────────────────────────────────────────────────────────────
    // Manifestation Path Tracking
    // ───────────────────────────────────────────────────────────────────────
    
    struct ManifestationState {
        double q4_origin{0.0};        // Field energy (timeless)
        double q3_densifying{0.0};    // Reflection energy
        double q1_planning{0.0};      // Gate energy (ethics)
        double q2_manifesting{0.0};   // Action energy (material)
        double progress{0.0};         // 0..1 manifestation progress
        bool gate_passed{false};      // Did it pass 53 Hz gate?
    };
    
    ManifestationState get_manifestation_state() const {
        ManifestationState s;
        s.q4_origin = quadrant_energy(Quadrant::Q4_Origin);
        s.q3_densifying = quadrant_energy(Quadrant::Q3_Densification);
        s.q1_planning = quadrant_energy(Quadrant::Q1_Planning);
        s.q2_manifesting = quadrant_energy(Quadrant::Q2_Manifestation);
        
        double total = s.q4_origin + s.q3_densifying + s.q1_planning + s.q2_manifesting;
        if (total > 0.001) {
            // Progress = how much has "fallen" from Q4 to Q2
            s.progress = s.q2_manifesting / total;
            s.gate_passed = s.q1_planning > 0.1 && s.q2_manifesting > s.q1_planning;
        }
        return s;
    }
    
    // ───────────────────────────────────────────────────────────────────────
    // Fold Distances (for tunnel detection)
    // ───────────────────────────────────────────────────────────────────────
    
    // Distance to singularity (7,7)
    double distance_to_singularity(int row, int col) const {
        double dr = row - FOLD_AXIS;
        double dc = col - FOLD_AXIS;
        return std::sqrt(dr * dr + dc * dc);
    }
    
    // When folded, which cell does (row,col) touch?
    std::pair<int, int> folded_partner(int row, int col) const {
        // Reflect across both axes at FOLD_AXIS
        int r2 = 2 * FOLD_AXIS - row;
        int c2 = 2 * FOLD_AXIS - col;
        
        // Clamp to valid range
        r2 = std::max(0, std::min(SIZE - 1, r2));
        c2 = std::max(0, std::min(SIZE - 1, c2));
        
        return {r2, c2};
    }
    
    // ───────────────────────────────────────────────────────────────────────
    // Raw data access (for visualization)
    // ───────────────────────────────────────────────────────────────────────
    
    const std::array<MatrixCell, CELLS>& cells() const { return cells_; }
    std::array<MatrixCell, CELLS>& cells() { return cells_; }

private:
    void init_topology() {
        for (int r = 0; r < SIZE; ++r) {
            for (int c = 0; c < SIZE; ++c) {
                MatrixCell& cell = at(r, c);
                cell.row = r;
                cell.col = c;
                cell.quadrant = get_quadrant(r, c);
                cell.is_singularity = (r == FOLD_AXIS && c == FOLD_AXIS);
                
                // Tunnel endpoints (5↔432, 13↔144 mapped to grid positions)
                // Approximate: tunnels at corners of the folded space
                cell.is_tunnel = 
                    (r == 1 && c == 11) || (r == 11 && c == 1) ||  // 5↔432
                    (r == 3 && c == 9) || (r == 9 && c == 3);      // 13↔144
            }
        }
    }
    
    void update_tunnels(double coherence) {
        // When coherence is high, tunnels light up
        float tunnel_intensity = (float)(coherence * coherence);  // Quadratic response
        
        for (auto& cell : cells_) {
            if (cell.is_tunnel) {
                cell.activity = std::max(cell.activity, tunnel_intensity);
            }
        }
    }
    
    std::array<MatrixCell, CELLS> cells_;
};

// ═══════════════════════════════════════════════════════════════════════════
// MANIFESTATION TRACKER - Tracks impulse flow through quadrants
// ═══════════════════════════════════════════════════════════════════════════

class ManifestationTracker {
public:
    struct Impulse {
        uint64_t id{0};
        double origin_time{0.0};      // When it appeared in Q4
        double densify_time{0.0};     // When it reached Q3
        double plan_time{0.0};        // When it reached Q1
        double manifest_time{0.0};    // When it reached Q2
        double intensity{0.0};
        bool complete{false};
        bool blocked{false};          // Blocked at gate
    };
    
    void track_impulse(uint64_t id, Quadrant current_q, double time, double intensity) {
        auto it = impulses_.find(id);
        if (it == impulses_.end()) {
            Impulse imp;
            imp.id = id;
            imp.intensity = intensity;
            if (current_q == Quadrant::Q4_Origin) imp.origin_time = time;
            impulses_[id] = imp;
            it = impulses_.find(id);
        }
        
        Impulse& imp = it->second;
        switch (current_q) {
            case Quadrant::Q4_Origin:
                if (imp.origin_time == 0.0) imp.origin_time = time;
                break;
            case Quadrant::Q3_Densification:
                if (imp.densify_time == 0.0) imp.densify_time = time;
                break;
            case Quadrant::Q1_Planning:
                if (imp.plan_time == 0.0) imp.plan_time = time;
                break;
            case Quadrant::Q2_Manifestation:
                if (imp.manifest_time == 0.0) {
                    imp.manifest_time = time;
                    imp.complete = true;
                }
                break;
        }
    }
    
    void block_impulse(uint64_t id) {
        auto it = impulses_.find(id);
        if (it != impulses_.end()) {
            it->second.blocked = true;
        }
    }
    
    // Get average manifestation time (Q4 → Q2)
    double average_manifestation_time() const {
        double sum = 0.0;
        int count = 0;
        for (const auto& kv : impulses_) {
            if (kv.second.complete && kv.second.origin_time > 0) {
                sum += (kv.second.manifest_time - kv.second.origin_time);
                ++count;
            }
        }
        return count > 0 ? sum / count : 0.0;
    }
    
    // Get block rate (how many impulses stopped at gate)
    double block_rate() const {
        int blocked = 0, total = 0;
        for (const auto& kv : impulses_) {
            if (kv.second.plan_time > 0) {
                ++total;
                if (kv.second.blocked) ++blocked;
            }
        }
        return total > 0 ? (double)blocked / total : 0.0;
    }

private:
    std::unordered_map<uint64_t, Impulse> impulses_;
};

} // namespace rael
