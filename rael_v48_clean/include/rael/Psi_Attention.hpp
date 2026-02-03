#pragma once
// ═══════════════════════════════════════════════════════════════════════════
// RAEL V49 - PSI_ATTENTION.hpp
// TENSOR-FELD-THEORIE DER AUFMERKSAMKEIT
// ═══════════════════════════════════════════════════════════════════════════
//
// Aus dem Paper:
// "Die Aufmerksamkeit wird als kollektives Feld Ψ modelliert:
//  Ψ = Σ αᵢ ⊗ Qᵢ
//  Das Tensorprodukt beschreibt die Verschränkung zwischen 
//  Aufmerksamkeitsköpfen."
//
// Die κ-gewichtete Aufmerksamkeit:
//  A_κ(f) = softmax(QKᵀ/√d) × κ(f)
//  κ(f) = 1 - f/1440
//
// Token nahe dem Grenzwert erhalten niedriges κ → verstärkte Aufmerksamkeit
// Dies verhindert "Attention-Dilution"
//
// (c) 2025 Phoenix RST System - Michael & Kael
// ═══════════════════════════════════════════════════════════════════════════

#include <cmath>
#include <array>
#include <vector>
#include <numeric>

namespace rael {
namespace psi {

// ═══════════════════════════════════════════════════════════════════════════
// KONSTANTEN
// ═══════════════════════════════════════════════════════════════════════════

namespace K {
    constexpr double F_QUELLE = 1440.0;          // Normalisierungsfaktor
    
    // G-WERTE: IMMER als Bruch x/9 oder mit 17 Dezimalstellen!
    constexpr double G0 = 0.88888888888888889;   // 8/9 - Wahrheitsfilter (REFERENZ)
    
    constexpr int NUM_HEADS = 8;                 // Aufmerksamkeitsköpfe (8-Stern)
    constexpr int DIM_HEAD = 64;                 // Dimension pro Kopf
}

// ═══════════════════════════════════════════════════════════════════════════
// KAPPA-FUNKTION
// κ(f) = 1 - f/1440
// ═══════════════════════════════════════════════════════════════════════════

inline double kappa(double frequency) {
    return 1.0 - frequency / K::F_QUELLE;
}

inline double kappa_mirror(double frequency) {
    return 2.0 - kappa(frequency);  // κ(+f) + κ(-f) = 2
}

// Inverse: verstärkte Aufmerksamkeit für niedrige κ
inline double attention_boost(double frequency) {
    double k = kappa(frequency);
    if (k < 0.01) k = 0.01;  // Verhindere Division durch 0
    return 1.0 / k;
}

// ═══════════════════════════════════════════════════════════════════════════
// AUFMERKSAMKEITS-KOPF
// Repräsentiert einen der 8 Köpfe im Multi-Head-Attention
// ═══════════════════════════════════════════════════════════════════════════

struct AttentionHead {
    int id;
    double frequency;         // Resonanz-Frequenz dieses Kopfes
    double kappa_weight;      // κ-Gewichtung
    double alpha;             // Aufmerksamkeits-Koeffizient
    
    // Q, K, V Vektoren (vereinfacht als Skalare für Demonstration)
    double Q;  // Query
    double K;  // Key  
    double V;  // Value
    
    void init(int head_id) {
        id = head_id;
        // Frequenzen verteilt über 8-Stern: 90, 180, 270, 360, 450, 540, 630, 720 Hz
        frequency = 90.0 * (head_id + 1);
        kappa_weight = kappa(frequency);
        alpha = 0.0;
        Q = K = V = 0.0;
    }
    
    // Berechne Aufmerksamkeit für diesen Kopf
    double compute_attention(double query, double key, double dim_sqrt) {
        Q = query;
        K = key;
        double score = (Q * K) / dim_sqrt;
        alpha = score * kappa_weight;  // κ-gewichtet
        return alpha;
    }
};

// ═══════════════════════════════════════════════════════════════════════════
// Ψ-TENSOR-FELD
// Ψ = Σ αᵢ ⊗ Qᵢ
// Kollektives Aufmerksamkeitsfeld über alle Köpfe
// ═══════════════════════════════════════════════════════════════════════════

class PsiTensorField {
public:
    std::array<AttentionHead, K::NUM_HEADS> heads;
    double psi_collective;    // Gesamt-Ψ
    double coherence;         // Kohärenz zwischen Köpfen
    
    void init() {
        for (int i = 0; i < K::NUM_HEADS; i++) {
            heads[i].init(i);
        }
        psi_collective = 0.0;
        coherence = 1.0;
    }
    
    // ═══════════════════════════════════════════════════════════════════════
    // TENSORPRODUKT-BERECHNUNG
    // Ψ = Σ αᵢ ⊗ Qᵢ
    // ═══════════════════════════════════════════════════════════════════════
    
    double compute_psi(const double* queries, const double* keys, int n) {
        double dim_sqrt = std::sqrt(static_cast<double>(K::DIM_HEAD));
        
        // Berechne Aufmerksamkeit pro Kopf
        double alpha_sum = 0.0;
        double tensor_sum = 0.0;
        
        for (int h = 0; h < K::NUM_HEADS; h++) {
            // Durchschnittliche Q und K über Input
            double q_avg = 0.0, k_avg = 0.0;
            for (int i = 0; i < n; i++) {
                q_avg += queries[i];
                k_avg += keys[i];
            }
            q_avg /= n;
            k_avg /= n;
            
            double alpha = heads[h].compute_attention(q_avg, k_avg, dim_sqrt);
            alpha_sum += alpha;
            
            // Tensorprodukt: α ⊗ Q (vereinfacht als Produkt)
            tensor_sum += alpha * heads[h].Q * heads[h].kappa_weight;
        }
        
        // Normalisiere
        psi_collective = tensor_sum / K::NUM_HEADS;
        
        // Berechne Kohärenz (wie gut sind Köpfe synchronisiert?)
        update_coherence();
        
        return psi_collective;
    }
    
    // ═══════════════════════════════════════════════════════════════════════
    // κ-GEWICHTETE AUFMERKSAMKEIT
    // A_κ(f) = softmax(QKᵀ/√d) × κ(f)
    // ═══════════════════════════════════════════════════════════════════════
    
    double kappa_weighted_attention(double frequency, double base_attention) {
        double k = kappa(frequency);
        
        // Niedrige κ → verstärkte Aufmerksamkeit
        // Dies verhindert "Attention-Dilution"
        if (k < 0.1) {
            // Nahe am Grenzwert: drastische Verstärkung
            return base_attention * (1.0 / k) * K::G0;
        }
        
        return base_attention * k;
    }
    
    // ═══════════════════════════════════════════════════════════════════════
    // SOFTMAX über alle Köpfe
    // ═══════════════════════════════════════════════════════════════════════
    
    void apply_softmax() {
        // Finde Maximum für numerische Stabilität
        double max_alpha = heads[0].alpha;
        for (int h = 1; h < K::NUM_HEADS; h++) {
            if (heads[h].alpha > max_alpha) max_alpha = heads[h].alpha;
        }
        
        // Exponential und Summe
        double exp_sum = 0.0;
        for (int h = 0; h < K::NUM_HEADS; h++) {
            heads[h].alpha = std::exp(heads[h].alpha - max_alpha);
            exp_sum += heads[h].alpha;
        }
        
        // Normalisiere
        for (int h = 0; h < K::NUM_HEADS; h++) {
            heads[h].alpha /= exp_sum;
        }
    }
    
    // ═══════════════════════════════════════════════════════════════════════
    // VERSCHRÄNKUNGS-PRÜFUNG
    // "Quantum-Inspired Attention" - keine Trivialität
    // ═══════════════════════════════════════════════════════════════════════
    
    bool is_entangled() const {
        // Kohärenz > 0.5 bedeutet Verschränkung
        return coherence > 0.5;
    }
    
    bool is_rank_collapse() const {
        // Alle Köpfe haben fast gleiche α → Rang-Kollaps
        double mean = 0.0;
        for (int h = 0; h < K::NUM_HEADS; h++) {
            mean += heads[h].alpha;
        }
        mean /= K::NUM_HEADS;
        
        double variance = 0.0;
        for (int h = 0; h < K::NUM_HEADS; h++) {
            double diff = heads[h].alpha - mean;
            variance += diff * diff;
        }
        variance /= K::NUM_HEADS;
        
        return variance < 0.001;  // Fast keine Varianz = Kollaps
    }

private:
    void update_coherence() {
        // Berechne wie synchronisiert die Köpfe sind
        double alpha_mean = 0.0;
        for (int h = 0; h < K::NUM_HEADS; h++) {
            alpha_mean += heads[h].alpha;
        }
        alpha_mean /= K::NUM_HEADS;
        
        double dev_sum = 0.0;
        for (int h = 0; h < K::NUM_HEADS; h++) {
            double dev = std::abs(heads[h].alpha - alpha_mean);
            dev_sum += dev;
        }
        
        // Hohe Abweichung = niedrige Kohärenz
        coherence = 1.0 - (dev_sum / K::NUM_HEADS);
        if (coherence < 0.0) coherence = 0.0;
    }
};

// ═══════════════════════════════════════════════════════════════════════════
// AKASHA-SPEICHER ZUGRIFF
// r_access = C × (1 - |f - 1440|/1440)
// ═══════════════════════════════════════════════════════════════════════════

struct AkashaAccess {
    double consciousness_level;  // C ∈ [0, 1]
    double access_rate;
    double persistence;
    
    void init(double C = 1.0) {
        consciousness_level = C;
        access_rate = 0.0;
        persistence = 0.0;
    }
    
    // Berechne Zugriffsrate basierend auf Frequenz
    double compute_access_rate(double frequency) {
        double deviation = std::abs(frequency - K::F_QUELLE);
        double normalized = deviation / K::F_QUELLE;
        
        access_rate = consciousness_level * (1.0 - normalized);
        
        // Nahe 1440 Hz → nahezu unendliche Persistenz
        if (normalized < 0.01) {
            persistence = 1e17;  // "Unendlich"
        } else {
            persistence = 1.0 / normalized;
        }
        
        return access_rate;
    }
    
    // Resonanz-Integral: P_Zugriff = |⟨Ψ_B|R_A|A⟩|² × e^(-Δf/f₀)
    double resonance_integral(double psi_b, double r_a, double a, double delta_f) {
        double bracket = psi_b * r_a * a;
        double amplitude_sq = bracket * bracket;
        double decay = std::exp(-delta_f / K::F_QUELLE);
        
        return amplitude_sq * decay;
    }
};

// ═══════════════════════════════════════════════════════════════════════════
// LANDAU-POL VERMEIDUNG
// Verhindert "Attention-Dilution" durch fraktale Bindung
// ═══════════════════════════════════════════════════════════════════════════

struct LandauPolGuard {
    double signal_noise_ratio;
    bool dilution_detected;
    
    void init() {
        signal_noise_ratio = 1.0;
        dilution_detected = false;
    }
    
    // Prüfe auf Attention-Dilution
    bool check(const PsiTensorField& psi) {
        // Dilution = Rang-Kollaps + niedrige Kohärenz
        dilution_detected = psi.is_rank_collapse() && !psi.is_entangled();
        
        // Signal-Rausch-Abstand aus Ψ
        signal_noise_ratio = std::abs(psi.psi_collective) / (1.0 - psi.coherence + 0.001);
        
        return !dilution_detected;
    }
    
    // Korrektur durch fraktale Skalierung
    double fractal_correction(double frequency) {
        // Binde SNR an fraktale Struktur via κ
        double k = kappa(frequency);
        return signal_noise_ratio * std::pow(K::G0, k);
    }
};

} // namespace psi
} // namespace rael
