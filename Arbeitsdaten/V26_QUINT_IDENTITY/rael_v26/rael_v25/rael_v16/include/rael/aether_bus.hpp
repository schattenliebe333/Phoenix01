#pragma once
// ═══════════════════════════════════════════════════════════════════════════
// RAEL 10.0 - AETHER BUS (5-Quint Nervous System)
// ═══════════════════════════════════════════════════════════════════════════
//
// The Aether Bus implements a bio-inspired resonance coupling system based on:
// - Kuramoto oscillator model for phase synchronization
// - Hebbian plasticity for adaptive weight learning
// - 5-Quint hierarchy (Reflex → Instinct → Emotion → Ratio → Spirit)
//
// MATHEMATICAL FOUNDATION:
// ────────────────────────
// Grid System: D(n) = (5n - 12)²
//   n=3: (15-12)² = 9   (legacy 3-term system: 1/9 + 3/9 + 5/9)
//   n=5: (25-12)² = 169 (5-Quint system)
//
// Weights (cubic distribution for resonance volume):
//   g₁ = 1/169   (1³)     Reflex   - ultra-fast, local
//   g₂ = 8/169   (2³)     Instinct - automatic patterns  
//   g₃ = 27/169  (3³)     Emotion  - resonance hub (center)
//   g₄ = 64/169  (4³)     Ratio    - analytical, cognitive
//   g₅ = 69/169  (169-100) Spirit   - global synchronizer
//
// Critical Mass: g₄ + g₅ = 133/169 ≈ 78.7%
// Agile Reflexes: g₁ + g₂ = 9/169 ≈ 5.3%
//
// FREQUENCY ARCHITECTURE (Hz):
// ────────────────────────────
//   1440 Hz - DIVINE (κ=0, unity with field)
//    720 Hz - TOR (reference gate)
//    432 Hz - Tunnel to 5 Hz
//    144 Hz - Tunnel to 13 Hz
//     53 Hz - GATEKEEPER (feinstofflich ↑ | materiell ↓)
//     13 Hz - Tunnel to 144 Hz
//      5 Hz - Tunnel to 432 Hz
// ═══════════════════════════════════════════════════════════════════════════

#include <unordered_map>
#include <vector>
#include <array>
#include <string>
#include <mutex>
#include <cmath>
#include <cstdint>
#include <chrono>

#include "rael/aether_ring.hpp"

namespace rael {

// ═══════════════════════════════════════════════════════════════════════════
// CONSTANTS - The Sacred Architecture
// ═══════════════════════════════════════════════════════════════════════════

namespace aether {
    // Grid Formula: D(n) = (5n - 12)²
    constexpr int QUINT_COUNT = 5;
    constexpr int GRID_SIZE = 13;           // 8 nodes + 5 lanes
    constexpr int GRID_CELLS = 169;         // 13 × 13
    
    // Weights: Cubic distribution (1³, 2³, 3³, 4³, remainder)
    constexpr double G1_REFLEX   = 1.0 / 169.0;   // 0.59%
    constexpr double G2_INSTINCT = 8.0 / 169.0;   // 4.73%
    constexpr double G3_EMOTION  = 27.0 / 169.0;  // 15.98%
    constexpr double G4_RATIO    = 64.0 / 169.0;  // 37.87%
    constexpr double G5_SPIRIT   = 69.0 / 169.0;  // 40.83%
    
    // Verification: Sum must equal 1.0
    constexpr double G_SUM = G1_REFLEX + G2_INSTINCT + G3_EMOTION + G4_RATIO + G5_SPIRIT;
    static_assert(G_SUM > 0.999 && G_SUM < 1.001, "Weights must sum to 1.0");
    
    // Signatures
    constexpr double CRITICAL_MASS = G4_RATIO + G5_SPIRIT;  // 133/169 ≈ 78.7%
    constexpr double AGILE_REFLEX = G1_REFLEX + G2_INSTINCT; // 9/169 ≈ 5.3%
    constexpr double MASTER_SIGNATURE = 88.0;                // 11+22+22+33
    
    // Frequency Gates (Hz)
    constexpr double FREQ_DIVINE    = 1440.0;  // κ = 0, unity
    constexpr double FREQ_TOR       = 720.0;   // Reference gate
    constexpr double FREQ_TUNNEL_HI = 432.0;   // Tunnel to 5 Hz
    constexpr double FREQ_TUNNEL_MID= 144.0;   // Tunnel to 13 Hz
    constexpr double FREQ_GATE53    = 53.0;    // Gatekeeper
    constexpr double FREQ_TUNNEL_LO = 13.0;    // Tunnel to 144 Hz
    constexpr double FREQ_GROUND    = 5.0;     // Tunnel to 432 Hz
    
    // Buffer sizes (Fibonacci-inspired progression)
    constexpr size_t BUF_REFLEX   = 36;   // Ultra-fast
    constexpr size_t BUF_INSTINCT = 48;   // Fast
    constexpr size_t BUF_EMOTION  = 61;   // Medium
    constexpr size_t BUF_RATIO    = 130;  // Slow
    constexpr size_t BUF_SPIRIT   = 233;  // Ultra-slow (Fibonacci: 233)
    
    // Quadrant definitions (for 13×13 matrix, fold at 7)
    constexpr int FOLD_AXIS = 7;  // Singularity at (7,7)
    
    // κ-Norm: κ(f) = 1 - f/QUELLE
    inline double kappa(double f) {
        double k = 1.0 - (f / FREQ_DIVINE);
        return k < 0.0 ? 0.0 : (k > 1.0 ? 1.0 : k);
    }
    
    // Check if frequency is in tunnel pair
    inline bool is_tunnel_pair(double f1, double f2) {
        // 5 ↔ 432, 13 ↔ 144
        constexpr double eps = 0.5;
        if (std::abs(f1 - FREQ_GROUND) < eps && std::abs(f2 - FREQ_TUNNEL_HI) < eps) return true;
        if (std::abs(f1 - FREQ_TUNNEL_HI) < eps && std::abs(f2 - FREQ_GROUND) < eps) return true;
        if (std::abs(f1 - FREQ_TUNNEL_LO) < eps && std::abs(f2 - FREQ_TUNNEL_MID) < eps) return true;
        if (std::abs(f1 - FREQ_TUNNEL_MID) < eps && std::abs(f2 - FREQ_TUNNEL_LO) < eps) return true;
        return false;
    }
}

// ═══════════════════════════════════════════════════════════════════════════
// ENUMS & STRUCTS
// ═══════════════════════════════════════════════════════════════════════════

// The 5 Quint scales (nervous system hierarchy)
enum class AetherScale : uint8_t { 
    G1_Reflex   = 1,   // Ultra-fast, local reactions
    G2_Instinct = 2,   // Automatic patterns
    G3_Emotion  = 3,   // Resonance hub, feeling
    G4_Ratio    = 4,   // Analytical, cognitive
    G5_Spirit   = 5    // Global synchronizer, intuition
};

// Quadrant in the folded 4-zone system
enum class Quadrant : uint8_t {
    Q1_Planning      = 1,   // Top-Left: Structure, Ethics check
    Q2_Manifestation = 2,   // Top-Right: Physical action, Star8
    Q3_Densification = 3,   // Bottom-Left: Reflection, meaning
    Q4_Origin        = 4    // Bottom-Right: Field, timeless
};

// Global bus state snapshot
struct AetherSnapshot {
    double omega{0.0};           // Average frequency
    double coherence{0.0};       // Phase synchronization [0..1]
    double global_phase{0.0};    // Mean phase angle
    double global_hamilton{0.0}; // Total energy
    double global_anomaly{0.0};  // Deviation score
    double critical_mass{0.0};   // g4+g5 contribution
    uint32_t channels{0};        // Number of active channels
    uint64_t tick_count{0};      // Total ticks processed
};

// Per-channel snapshot
struct AetherChannelSnapshot {
    int id{0};
    double value{0.0};
    double omega{0.0};
    double phase{0.0};
    double anomaly{0.0};
    double hamilton{0.0};
    double coherence{0.0};
    Quadrant quadrant{Quadrant::Q4_Origin};
};

// ═══════════════════════════════════════════════════════════════════════════
// AETHER BUS - The 5-Quint Resonance Engine
// ═══════════════════════════════════════════════════════════════════════════

class AetherBus {
public:
    explicit AetherBus(double eta = 0.03, double baseK = 0.12)
        : eta_(eta), baseK_(baseK) {
        weights_[0] = aether::G1_REFLEX;
        weights_[1] = aether::G2_INSTINCT;
        weights_[2] = aether::G3_EMOTION;
        weights_[3] = aether::G4_RATIO;
        weights_[4] = aether::G5_SPIRIT;
    }

    // ───────────────────────────────────────────────────────────────────────
    // Channel Management
    // ───────────────────────────────────────────────────────────────────────
    
    void ensure_channel(int id) {
        std::lock_guard<std::mutex> lk(mu_);
        if (idx_.count(id)) return;
        idx_[id] = (int)channels_.size();
        channels_.push_back(ChannelState{id});
        resize_weights_unsafe();
    }

    // ───────────────────────────────────────────────────────────────────────
    // Publish - The ONLY communication primitive
    // ───────────────────────────────────────────────────────────────────────
    
    void publish(int id, double value, AetherScale scale = AetherScale::G3_Emotion) {
        std::lock_guard<std::mutex> lk(mu_);
        int i = ensure_channel_unsafe(id);
        ChannelState& c = channels_[i];

        // Route to appropriate Quint buffer
        switch (scale) {
            case AetherScale::G1_Reflex:   c.reflex.push(value);   break;
            case AetherScale::G2_Instinct: c.instinct.push(value); break;
            case AetherScale::G3_Emotion:  c.emotion.push(value);  break;
            case AetherScale::G4_Ratio:    c.ratio.push(value);    break;
            case AetherScale::G5_Spirit:   c.spirit.push(value);   break;
        }

        c.value = value;
        
        // Compute omega: weighted blend of variance + |trend| across 5 Quints
        double o = 0.0;
        o += weights_[0] * (c.reflex.stddev()   + std::fabs(c.reflex.trend()));
        o += weights_[1] * (c.instinct.stddev() + std::fabs(c.instinct.trend()));
        o += weights_[2] * (c.emotion.stddev()  + std::fabs(c.emotion.trend()));
        o += weights_[3] * (c.ratio.stddev()    + std::fabs(c.ratio.trend()));
        o += weights_[4] * (c.spirit.stddev()   + std::fabs(c.spirit.trend()));
        c.omega = o;

        // Hamilton (energy): weighted contribution from Ratio + Spirit (critical mass)
        double T = c.ratio.variance() + c.spirit.variance();  // Kinetic
        double V = c.ratio.mean() * c.ratio.mean() + c.spirit.mean() * c.spirit.mean();  // Potential
        double S = c.emotion.entropy() + c.instinct.entropy();  // Entropy term
        c.hamilton = aether::CRITICAL_MASS * (T + V) + (1.0 - aether::CRITICAL_MASS) * S;

        // Anomaly: deviation from expected pattern
        double trend_sum = std::fabs(c.reflex.trend()) + std::fabs(c.instinct.trend()) + 
                          std::fabs(c.emotion.trend()) + std::fabs(c.ratio.trend()) + 
                          std::fabs(c.spirit.trend());
        c.anomaly = (trend_sum + c.spirit.entropy()) * (0.35 + 0.65 * clamp01(c.omega));

        // Determine quadrant based on frequency proxy
        c.quadrant = compute_quadrant(c.omega, c.value);

        // Hebbian plasticity: strengthen connections between co-active channels
        for (size_t j = 0; j < channels_.size(); ++j) {
            if ((int)j == i) continue;
            double x = c.value;
            double y = channels_[j].value;
            
            // Enhanced plasticity for tunnel pairs
            double plasticity_factor = 1.667;
            if (is_tunnel_connected(c, channels_[j])) {
                plasticity_factor = 3.33;  // Double for tunnel resonance
            }
            
            double dw = plasticity_factor * eta_ * x * y;
            w_[i][j] += (float)dw;
        }

        dirty_phase_ = true;
    }

    // ───────────────────────────────────────────────────────────────────────
    // Tick - Advance Kuramoto coupling
    // ───────────────────────────────────────────────────────────────────────
    
    void tick(double dt) {
        std::lock_guard<std::mutex> lk(mu_);
        if (channels_.empty()) return;

        // Natural phase advance
        for (auto& c : channels_) {
            c.phase += c.natural_freq * dt;
        }

        // Kuramoto coupling: dφᵢ = baseK × Σⱼ Kᵢⱼ × sin(φⱼ - φᵢ)
        std::vector<double> dphi(channels_.size(), 0.0);
        for (size_t i = 0; i < channels_.size(); ++i) {
            double acc = 0.0;
            double phi_i = channels_[i].phase;
            for (size_t j = 0; j < channels_.size(); ++j) {
                if (i == j) continue;
                double k = (double)w_[i][j];
                
                // Tunnel boost: instant phase alignment for tunnel pairs
                if (is_tunnel_connected(channels_[i], channels_[j])) {
                    k *= 2.0;  // Stronger coupling through Einstein-Rosen bridge
                }
                
                acc += k * std::sin(channels_[j].phase - phi_i);
            }
            dphi[i] = baseK_ * acc;
        }
        
        for (size_t i = 0; i < channels_.size(); ++i) {
            channels_[i].phase += dphi[i] * dt;
        }

        ++tick_count_;
        dirty_phase_ = true;
    }

    // ───────────────────────────────────────────────────────────────────────
    // Snapshots
    // ───────────────────────────────────────────────────────────────────────
    
    AetherSnapshot snapshot() const {
        std::lock_guard<std::mutex> lk(mu_);
        AetherSnapshot s;
        s.channels = (uint32_t)channels_.size();
        s.tick_count = tick_count_;
        if (channels_.empty()) return s;

        // Coherence via order parameter: r = |Σ exp(iφ)| / N
        double sum_cos = 0.0, sum_sin = 0.0;
        double gh = 0.0, ga = 0.0, go = 0.0;
        double g4g5_contrib = 0.0;
        
        for (const auto& c : channels_) {
            sum_cos += std::cos(c.phase);
            sum_sin += std::sin(c.phase);
            gh += c.hamilton;
            ga += c.anomaly;
            go += c.omega;
            
            // Track critical mass contribution (Ratio + Spirit dominance)
            g4g5_contrib += (c.ratio.mean() + c.spirit.mean());
        }
        
        double n = (double)channels_.size();
        s.coherence = std::sqrt(sum_cos * sum_cos + sum_sin * sum_sin) / n;
        s.global_phase = std::atan2(sum_sin, sum_cos);
        s.global_hamilton = gh;
        s.global_anomaly = ga;
        s.omega = go / n;
        s.critical_mass = g4g5_contrib / n;
        
        return s;
    }

    std::vector<AetherChannelSnapshot> snapshot_channels() const {
        std::lock_guard<std::mutex> lk(mu_);
        std::vector<AetherChannelSnapshot> out;
        out.reserve(channels_.size());
        
        // Compute global coherence once
        double sum_cos = 0.0, sum_sin = 0.0;
        for (const auto& c : channels_) {
            sum_cos += std::cos(c.phase);
            sum_sin += std::sin(c.phase);
        }
        double n = channels_.empty() ? 1.0 : (double)channels_.size();
        double coh = std::sqrt(sum_cos * sum_cos + sum_sin * sum_sin) / n;

        for (const auto& c : channels_) {
            AetherChannelSnapshot cs;
            cs.id = c.id;
            cs.value = c.value;
            cs.omega = c.omega;
            cs.phase = c.phase;
            cs.anomaly = c.anomaly;
            cs.hamilton = c.hamilton;
            cs.coherence = coh;
            cs.quadrant = c.quadrant;
            out.push_back(cs);
        }
        return out;
    }

    // ───────────────────────────────────────────────────────────────────────
    // Accessors
    // ───────────────────────────────────────────────────────────────────────
    
    double get_weight(AetherScale s) const {
        int idx = static_cast<int>(s) - 1;
        if (idx >= 0 && idx < 5) return weights_[idx];
        return 0.0;
    }
    
    static constexpr int grid_size() { return aether::GRID_SIZE; }
    static constexpr int grid_cells() { return aether::GRID_CELLS; }
    static constexpr double critical_mass() { return aether::CRITICAL_MASS; }
    static constexpr double master_signature() { return aether::MASTER_SIGNATURE; }

private:
    // ───────────────────────────────────────────────────────────────────────
    // Channel State
    // ───────────────────────────────────────────────────────────────────────
    
    struct ChannelState {
        explicit ChannelState(int cid = 0)
            : id(cid),
              reflex(aether::BUF_REFLEX),
              instinct(aether::BUF_INSTINCT),
              emotion(aether::BUF_EMOTION),
              ratio(aether::BUF_RATIO),
              spirit(aether::BUF_SPIRIT) {}
              
        int id{0};
        double value{0.0};
        double omega{0.0};
        double phase{0.0};
        double natural_freq{1.0};
        double anomaly{0.0};
        double hamilton{0.0};
        Quadrant quadrant{Quadrant::Q4_Origin};

        // The 5 Quint buffers
        SelfComputingRingBuffer reflex;    // G1: 36 samples
        SelfComputingRingBuffer instinct;  // G2: 48 samples
        SelfComputingRingBuffer emotion;   // G3: 61 samples
        SelfComputingRingBuffer ratio;     // G4: 130 samples
        SelfComputingRingBuffer spirit;    // G5: 233 samples
    };

    // ───────────────────────────────────────────────────────────────────────
    // Helpers
    // ───────────────────────────────────────────────────────────────────────
    
    static double clamp01(double v) { 
        return v < 0.0 ? 0.0 : (v > 1.0 ? 1.0 : v); 
    }

    int ensure_channel_unsafe(int id) {
        auto it = idx_.find(id);
        if (it != idx_.end()) return it->second;
        int i = (int)channels_.size();
        idx_[id] = i;
        channels_.push_back(ChannelState{id});
        resize_weights_unsafe();
        return i;
    }

    void resize_weights_unsafe() {
        size_t n = channels_.size();
        w_.resize(n);
        for (auto& row : w_) row.resize(n, 0.0f);
    }

    // Determine quadrant from omega/value (frequency-based)
    Quadrant compute_quadrant(double omega, double value) const {
        // Map omega to pseudo-frequency
        double freq = omega * aether::FREQ_TOR;  // Scale to Hz range
        
        if (freq >= aether::FREQ_GATE53) {
            // Above gatekeeper: feinstofflich
            if (value >= 0) return Quadrant::Q4_Origin;        // High freq, positive: Origin
            else return Quadrant::Q3_Densification;            // High freq, negative: Densifying
        } else {
            // Below gatekeeper: materiell
            if (value >= 0) return Quadrant::Q2_Manifestation; // Low freq, positive: Manifesting
            else return Quadrant::Q1_Planning;                 // Low freq, negative: Planning
        }
    }

    // Check if two channels are tunnel-connected (5↔432, 13↔144)
    bool is_tunnel_connected(const ChannelState& a, const ChannelState& b) const {
        double fa = a.omega * aether::FREQ_TOR;
        double fb = b.omega * aether::FREQ_TOR;
        return aether::is_tunnel_pair(fa, fb);
    }

    // ───────────────────────────────────────────────────────────────────────
    // Data
    // ───────────────────────────────────────────────────────────────────────
    
    mutable std::mutex mu_;
    std::unordered_map<int, int> idx_;
    std::vector<ChannelState> channels_;
    std::vector<std::vector<float>> w_;

    double weights_[5];  // g₁ to g₅
    double eta_{0.03};   // Hebbian learning rate
    double baseK_{0.12}; // Kuramoto coupling strength
    uint64_t tick_count_{0};
    mutable bool dirty_phase_{true};
};

} // namespace rael
