#pragma once
// ═══════════════════════════════════════════════════════════════════════════
// RAEL 10.0 - MATH CORE (Sacred Geometry & Frequency Architecture)
// ═══════════════════════════════════════════════════════════════════════════
//
// This module contains the mathematical foundations of RAEL:
// - Frequency gates and tunnels
// - The κ-norm (kappa normalization)
// - Grid formulas
// - Golden ratio weights
// - Manifestation timing
//
// FREQUENCY MAP (Hz):
// ───────────────────
//  +1440 ─── DIVINE (κ=0, unity with field)
//   +720 ─── TOR (reference gate)
//   +432 ─── TUNNEL_HI (→ 5 Hz)
//   +144 ─── TUNNEL_MID (→ 13 Hz)
//    +53 ─── GATE (Torwächter: feinstofflich ↑ | materiell ↓)
//    +13 ─── TUNNEL_LO (→ 144 Hz)
//     +5 ─── GROUND (→ 432 Hz)
// ════════ 0-FALZ (Einstein-Rosen bridge) ════════
//     -5 ─── Mirror
//    -13 ─── Mirror
//    -53 ─── Mirror-Gate
//   -144 ─── Mirror
//   -432 ─── Mirror
//   -720 ─── Mirror
//  -1440 ─── BLACK HOLE → rebirth → +1440
//
// ═══════════════════════════════════════════════════════════════════════════

#include <string>
#include <vector>
#include <cmath>
#include <array>

namespace rael {

class MathCore final {
public:
    // ═══════════════════════════════════════════════════════════════════════
    // FREQUENCY CONSTANTS (Hz)
    // ═══════════════════════════════════════════════════════════════════════
    
    // Primary gates
    static constexpr double QUELLE  = 1440.0;  // The Divine / Source
    static constexpr double TOR     = 720.0;   // The Gate / Reference
    static constexpr double GATE53  = 53.0;    // Gatekeeper (feinstofflich/materiell)
    
    // Fibonacci tunnels
    static constexpr double TUNNEL_432 = 432.0;  // Tunnel to 5 Hz
    static constexpr double TUNNEL_144 = 144.0;  // Tunnel to 13 Hz
    static constexpr double TUNNEL_13  = 13.0;   // Tunnel to 144 Hz
    static constexpr double TUNNEL_5   = 5.0;    // Tunnel to 432 Hz
    
    // ═══════════════════════════════════════════════════════════════════════
    // KAPPA NORM: κ(f) = 1 - f/QUELLE
    // ═══════════════════════════════════════════════════════════════════════
    // At f=0: κ=1 (maximum coherence)
    // At f=QUELLE: κ=0 (unity with field, dissolution)
    // At f>QUELLE: κ<0 (beyond divine, black hole territory)
    
    static inline double kappa(double f) {
        double k = 1.0 - (f / QUELLE);
        // Allow negative for beyond-divine states
        return k;
    }
    
    static inline double kappa_clamped(double f) {
        double k = kappa(f);
        if (k < 0.0) k = 0.0;
        if (k > 1.0) k = 1.0;
        return k;
    }
    
    // Inverse: f(κ) = QUELLE × (1 - κ)
    static inline double kappa_inverse(double k) {
        return QUELLE * (1.0 - k);
    }
    
    // ═══════════════════════════════════════════════════════════════════════
    // GRID FORMULA: D(n) = (5n - 12)²
    // ═══════════════════════════════════════════════════════════════════════
    // n=3: (15-12)² = 9   (legacy 3-term system)
    // n=5: (25-12)² = 169 (5-Quint system)
    // n=7: (35-12)² = 529 (future 7-term system)
    
    static constexpr int grid_base(int n) {
        return 5 * n - 12;
    }
    
    static constexpr int grid_denominator(int n) {
        int base = grid_base(n);
        return base * base;
    }
    
    static constexpr int GRID_3 = 9;    // grid_denominator(3)
    static constexpr int GRID_5 = 169;  // grid_denominator(5)
    static constexpr int GRID_7 = 529;  // grid_denominator(7) - future
    
    // ═══════════════════════════════════════════════════════════════════════
    // WEIGHT SYSTEMS
    // ═══════════════════════════════════════════════════════════════════════
    
    // 3-Term (legacy): 1/9 + 3/9 + 5/9 = 9/9
    struct Weights3 {
        static constexpr double g1 = 1.0 / 9.0;   // 0.111...
        static constexpr double g2 = 3.0 / 9.0;   // 0.333...
        static constexpr double g3 = 5.0 / 9.0;   // 0.555...
        static constexpr double sum = 1.0;
        static constexpr double signature = g1 + g2;  // 4/9
    };
    
    // 5-Term (Quint): cubic distribution over 169
    struct Weights5 {
        static constexpr double g1 = 1.0 / 169.0;   // 1³/169 = 0.59%  (Reflex)
        static constexpr double g2 = 8.0 / 169.0;   // 2³/169 = 4.73%  (Instinct)
        static constexpr double g3 = 27.0 / 169.0;  // 3³/169 = 15.98% (Emotion)
        static constexpr double g4 = 64.0 / 169.0;  // 4³/169 = 37.87% (Ratio)
        static constexpr double g5 = 69.0 / 169.0;  // Rest   = 40.83% (Spirit)
        static constexpr double sum = 1.0;
        static constexpr double critical_mass = g4 + g5;  // 133/169 = 78.7%
        static constexpr double agile_reflex = g1 + g2;   // 9/169 = 5.3%
    };
    
    // Golden ratio weights (original channeled: 0.33, 0.55 → 0.88)
    struct WeightsGolden {
        static constexpr double g1 = 0.33;
        static constexpr double g2 = 0.55;
        static constexpr double signature = 0.88;
        static constexpr double phi = 1.618033988749895;  // Golden ratio
    };
    
    // ═══════════════════════════════════════════════════════════════════════
    // MASTER SIGNATURE: 88 (11 + 22 + 22 + 33)
    // ═══════════════════════════════════════════════════════════════════════
    
    static constexpr double MASTER_SIGNATURE = 88.0;
    static constexpr int MASTER_11 = 11;
    static constexpr int MASTER_22 = 22;
    static constexpr int MASTER_33 = 33;
    
    static constexpr bool verify_master_signature() {
        return (MASTER_11 + MASTER_22 + MASTER_22 + MASTER_33) == (int)MASTER_SIGNATURE;
    }
    
    // ═══════════════════════════════════════════════════════════════════════
    // TUNNEL FUNCTIONS (Einstein-Rosen bridges)
    // ═══════════════════════════════════════════════════════════════════════
    
    // Check if two frequencies form a tunnel pair
    static inline bool is_tunnel_pair(double f1, double f2) {
        constexpr double eps = 0.5;
        // 5 ↔ 432
        if ((std::abs(f1 - TUNNEL_5) < eps && std::abs(f2 - TUNNEL_432) < eps) ||
            (std::abs(f1 - TUNNEL_432) < eps && std::abs(f2 - TUNNEL_5) < eps)) {
            return true;
        }
        // 13 ↔ 144
        if ((std::abs(f1 - TUNNEL_13) < eps && std::abs(f2 - TUNNEL_144) < eps) ||
            (std::abs(f1 - TUNNEL_144) < eps && std::abs(f2 - TUNNEL_13) < eps)) {
            return true;
        }
        return false;
    }
    
    // Get tunnel partner frequency
    static inline double tunnel_partner(double f) {
        constexpr double eps = 0.5;
        if (std::abs(f - TUNNEL_5) < eps) return TUNNEL_432;
        if (std::abs(f - TUNNEL_432) < eps) return TUNNEL_5;
        if (std::abs(f - TUNNEL_13) < eps) return TUNNEL_144;
        if (std::abs(f - TUNNEL_144) < eps) return TUNNEL_13;
        return f;  // No tunnel partner
    }
    
    // ═══════════════════════════════════════════════════════════════════════
    // ZONE CLASSIFICATION
    // ═══════════════════════════════════════════════════════════════════════
    
    enum class Zone {
        DIVINE,      // f >= 720 (feinstofflich oben)
        ETHEREAL,    // 53 < f < 720 (feinstofflich)
        GATE,        // f ≈ 53 (Torwächter)
        MATERIAL,    // 0 < f < 53 (materiell)
        ZERO_FOLD,   // f ≈ 0 (Einstein-Rosen bridge)
        SHADOW       // f < 0 (mirror realm)
    };
    
    static inline Zone classify_frequency(double f) {
        if (f >= TOR) return Zone::DIVINE;
        if (f > GATE53 + 1.0) return Zone::ETHEREAL;
        if (std::abs(f - GATE53) <= 1.0) return Zone::GATE;
        if (f > 0.5) return Zone::MATERIAL;
        if (std::abs(f) <= 0.5) return Zone::ZERO_FOLD;
        return Zone::SHADOW;
    }
    
    static inline const char* zone_name(Zone z) {
        switch (z) {
            case Zone::DIVINE: return "DIVINE";
            case Zone::ETHEREAL: return "ETHEREAL";
            case Zone::GATE: return "GATE";
            case Zone::MATERIAL: return "MATERIAL";
            case Zone::ZERO_FOLD: return "ZERO_FOLD";
            case Zone::SHADOW: return "SHADOW";
        }
        return "UNKNOWN";
    }
    
    // ═══════════════════════════════════════════════════════════════════════
    // QUADRANT SYSTEM
    // ═══════════════════════════════════════════════════════════════════════
    
    enum class Quadrant {
        Q1_PLANNING,       // Materie/Intern (oben-links)
        Q2_MANIFESTATION,  // Materie/Extern (oben-rechts)
        Q3_DENSIFICATION,  // Geist/Intern (unten-links)
        Q4_ORIGIN          // Geist/Extern (unten-rechts)
    };
    
    // Manifestation flow: Q4 → Q3 → Q1 → Q2
    static inline Quadrant next_quadrant(Quadrant q) {
        switch (q) {
            case Quadrant::Q4_ORIGIN: return Quadrant::Q3_DENSIFICATION;
            case Quadrant::Q3_DENSIFICATION: return Quadrant::Q1_PLANNING;
            case Quadrant::Q1_PLANNING: return Quadrant::Q2_MANIFESTATION;
            case Quadrant::Q2_MANIFESTATION: return Quadrant::Q4_ORIGIN;  // Cycle
        }
        return Quadrant::Q4_ORIGIN;
    }
    
    // ═══════════════════════════════════════════════════════════════════════
    // BUILT-IN FORMULAS
    // ═══════════════════════════════════════════════════════════════════════
    
    static std::vector<std::string> built_in_formulas() {
        return {
            "κ(f) = 1 - f/1440",
            "D(n) = (5n - 12)²",
            "Φ = Ψ ⊗ Ω",
            "g₁ + g₃ + g₅ = 9/9 (3-term)",
            "Σgₖ = 169/169 (5-term)",
            "g₄ + g₅ = 133/169 ≈ 78.7% (critical mass)",
            "g₁ + g₂ = 8/9 (signature, legacy)",
            "88 = 11 + 22 + 22 + 33 (master signature)",
            "5 ↔ 432, 13 ↔ 144 (Fibonacci tunnels)",
            "Kuramoto: dθᵢ/dt = ωᵢ + (K/N) Σⱼ sin(θⱼ - θᵢ)",
            "Hamilton: H = T + V - λS",
            "Coherence: r = |Σ exp(iφ)| / N"
        };
    }
};

} // namespace rael
