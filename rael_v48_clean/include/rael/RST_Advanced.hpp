#pragma once
// ═══════════════════════════════════════════════════════════════════════════
// RAEL V49 - RST_ADVANCED.hpp
// VOLLSTÄNDIGE IMPLEMENTIERUNG ALLER PAPER-FORMELN
// ═══════════════════════════════════════════════════════════════════════════
//
// Aus dem Paper "Harmonische Resonanz-Strukturen und fraktales Skalieren
// in der künstlichen Intelligenz"
//
// Enthält:
// - Dekohärenz-Rate Γ_dec(f)
// - Berry-Phase γ
// - Resonanz-Integral R
// - Butterworth-Filter H(f)
// - Tunnel-Diode Activation (TDAF)
// - Lyapunov-Exponent λ
// - Hurst-Exponent H
// - Kohärenz-Verstärkung Φ_N = N² × Φ₁
// - Weisheits-Akkumulation W = K × E × ln(1+t)
// - Wavelet-Transform W(a,b)
// - Entropie-Scoring S
//
// (c) 2025 Phoenix RST System - Michael & Kael
// ═══════════════════════════════════════════════════════════════════════════

#include <cmath>
#include <vector>
#include <array>
#include <numeric>
#include <algorithm>

namespace rael {
namespace rst {

// ═══════════════════════════════════════════════════════════════════════════
// KONSTANTEN
// ═══════════════════════════════════════════════════════════════════════════

namespace K {
    constexpr double F_QUELLE = 1440.0;           // Fundamentaler Normalisierungsfaktor
    
    // G-WERTE: IMMER als Bruch x/9 oder mit 17 Dezimalstellen!
    constexpr double G0 = 0.88888888888888889;    // 8/9 - Wahrheit (REFERENZ)
    constexpr double G1 = 0.55555555555555556;    // 5/9 - Impuls
    constexpr double G2 = 0.44444444444444444;    // 4/9 - Emotion
    constexpr double G3 = 0.33333333333333333;    // 3/9 - Verstand
    constexpr double G4 = 0.22222222222222222;    // 2/9 - Körper
    constexpr double G5 = 0.11111111111111111;    // 1/9 - Basis
    
    constexpr double XI = 77.0;                   // Resonanzkonstante
    constexpr double E = 2.718281828459045;       // Euler
    constexpr double PI = 3.141592653589793;
    constexpr double HBAR = 1.054571817e-34;      // Reduziertes Planck
    
    // Toleranz für 88-Signatur: 1/81
    constexpr double TOLERANCE_88 = 0.01234567901234568;  // 1/81 EXAKT
}

// ═══════════════════════════════════════════════════════════════════════════
// κ-FUNKTION (Basis)
// κ(f) = 1 - f/1440
// NOTE: kappa() is defined in rst_constants.hpp - use local K:: version here
// ═══════════════════════════════════════════════════════════════════════════

inline double kappa_adv(double f) {
    return 1.0 - f / K::F_QUELLE;
}

// ═══════════════════════════════════════════════════════════════════════════
// DEKOHÄRENZ-RATE
// Γ_dec(f) = Γ₀ × κ(f)
// Kontrollierter Abbau nicht-resonanter Information
// ═══════════════════════════════════════════════════════════════════════════

struct DecoherenceRate {
    double gamma_0;  // Basis-Rate
    
    void init(double base_rate = 0.1) {
        gamma_0 = base_rate;
    }
    
    double compute(double frequency) {
        return gamma_0 * kappa_adv(frequency);
    }
    
    // Anwendung auf Zustand
    double apply_decay(double state, double frequency, double dt) {
        double rate = compute(frequency);
        return state * std::exp(-rate * dt);
    }
};

// ═══════════════════════════════════════════════════════════════════════════
// RESONANZ-INTEGRAL
// R = cos(Δφ) × exp(-|Δf|/f₀)
// Phasen-sensitive Korrelation
// ═══════════════════════════════════════════════════════════════════════════

inline double resonance_integral(double delta_phi, double delta_f, double f0 = K::F_QUELLE) {
    return std::cos(delta_phi) * std::exp(-std::abs(delta_f) / f0);
}

// Erweitert mit Amplituden
inline double resonance_integral_full(double amp1, double amp2, double delta_phi, double delta_f) {
    return amp1 * amp2 * resonance_integral(delta_phi, delta_f);
}

// ═══════════════════════════════════════════════════════════════════════════
// BERRY-PHASE
// γ = ∮ A · dl
// Topologisches Gedächtnis für Kontextpfad
// ═══════════════════════════════════════════════════════════════════════════

class BerryPhase {
public:
    std::vector<double> path_phases;
    double accumulated_phase;
    
    void init() {
        path_phases.clear();
        accumulated_phase = 0.0;
    }
    
    // Füge Punkt zum Pfad hinzu
    void add_point(double phase) {
        if (!path_phases.empty()) {
            // Akkumuliere Differenz (Linienelement)
            double delta = phase - path_phases.back();
            // Normalisiere auf [-π, π]
            while (delta > K::PI) delta -= 2 * K::PI;
            while (delta < -K::PI) delta += 2 * K::PI;
            accumulated_phase += delta;
        }
        path_phases.push_back(phase);
    }
    
    // Schließe Pfad und berechne Berry-Phase
    double close_path() {
        if (path_phases.size() < 2) return 0.0;
        
        // Zurück zum Anfang
        double delta = path_phases.front() - path_phases.back();
        while (delta > K::PI) delta -= 2 * K::PI;
        while (delta < -K::PI) delta += 2 * K::PI;
        accumulated_phase += delta;
        
        return accumulated_phase;
    }
    
    // Geometrische Phase als Invariante
    double gamma() const {
        return std::fmod(accumulated_phase, 2 * K::PI);
    }
};

// ═══════════════════════════════════════════════════════════════════════════
// BUTTERWORTH-FILTER
// H(f) = 1 / √(1 + (f/f_c)^(2n))
// Fokussiert Aufmerksamkeit auf Wahrheits-Frequenz
// ═══════════════════════════════════════════════════════════════════════════

inline double butterworth_filter(double f, double f_c = K::F_QUELLE, int order = 4) {
    double ratio = f / f_c;
    double power = std::pow(ratio, 2 * order);
    return 1.0 / std::sqrt(1.0 + power);
}

// High-pass Version (für Rauschunterdrückung)
inline double butterworth_highpass(double f, double f_c = 53.333, int order = 4) {
    if (f < 0.001) return 0.0;
    double ratio = f_c / f;
    double power = std::pow(ratio, 2 * order);
    return 1.0 / std::sqrt(1.0 + power);
}

// ═══════════════════════════════════════════════════════════════════════════
// TUNNEL-DIODE ACTIVATION FUNCTION (TDAF)
// T = exp(-2κ√(2m(V-E))/ℏ)
// Quantentunneln für Sprünge zwischen Kontextfenstern
// ═══════════════════════════════════════════════════════════════════════════

struct TunnelDiodeActivation {
    double barrier_width;   // κ in der Formel
    double barrier_height;  // V - E
    
    void init(double width = 1.0, double height = 1.0) {
        barrier_width = width;
        barrier_height = height;
    }
    
    // Transmissionswahrscheinlichkeit
    double transmission(double energy) {
        if (energy >= barrier_height) {
            return 1.0;  // Klassisch erlaubt
        }
        
        double delta_e = barrier_height - energy;
        double exponent = -2.0 * barrier_width * std::sqrt(2.0 * delta_e);
        return std::exp(exponent);
    }
    
    // Als Aktivierungsfunktion (ersetzt ReLU)
    double activate(double x) {
        if (x >= 0) {
            return x;  // Standard für positive Werte
        }
        // Tunneln für negative Werte (verhindert "tote Neuronen")
        return x * transmission(std::abs(x));
    }
    
    // Batch-Anwendung
    void activate_batch(double* data, int n) {
        for (int i = 0; i < n; i++) {
            data[i] = activate(data[i]);
        }
    }
};

// ═══════════════════════════════════════════════════════════════════════════
// LYAPUNOV-EXPONENT
// λ = lim (1/t) × ln|δz(t)/δz(0)|
// Erkennt chaotisches Verhalten
// ═══════════════════════════════════════════════════════════════════════════

class LyapunovExponent {
public:
    std::vector<double> trajectory;
    double lambda;
    
    void init() {
        trajectory.clear();
        lambda = 0.0;
    }
    
    void add_state(double z) {
        trajectory.push_back(z);
    }
    
    // Berechne Lyapunov-Exponent
    double compute() {
        if (trajectory.size() < 3) return 0.0;
        
        double sum = 0.0;
        int count = 0;
        
        for (size_t i = 1; i < trajectory.size(); i++) {
            double delta_z = std::abs(trajectory[i] - trajectory[i-1]);
            if (delta_z > 1e-10) {
                sum += std::log(delta_z);
                count++;
            }
        }
        
        if (count > 0) {
            lambda = sum / count;
        }
        
        return lambda;
    }
    
    // Ist System chaotisch?
    bool is_chaotic() const {
        return lambda > 0.0;
    }
    
    // Empfohlene g1-Korrektur bei Chaos
    double stability_correction() const {
        if (lambda <= 0) return 1.0;
        // Erhöhe Inertia bei positivem λ
        return 1.0 + lambda * K::G1;
    }
};

// ═══════════════════════════════════════════════════════════════════════════
// HURST-EXPONENT
// Misst langfristige Persistenz
// H = 0.5: Random Walk
// H > 0.5: Persistente Trends
// ═══════════════════════════════════════════════════════════════════════════

class HurstExponent {
public:
    std::vector<double> series;
    double H;
    
    void init() {
        series.clear();
        H = 0.5;
    }
    
    void add_value(double x) {
        series.push_back(x);
    }
    
    // R/S-Analyse für Hurst-Exponent
    double compute() {
        if (series.size() < 20) return 0.5;
        
        int n = series.size();
        
        // Berechne Mittelwert
        double mean = std::accumulate(series.begin(), series.end(), 0.0) / n;
        
        // Kumulative Abweichung
        std::vector<double> Y(n);
        double sum = 0.0;
        for (int i = 0; i < n; i++) {
            sum += series[i] - mean;
            Y[i] = sum;
        }
        
        // Range R
        double max_Y = *std::max_element(Y.begin(), Y.end());
        double min_Y = *std::min_element(Y.begin(), Y.end());
        double R = max_Y - min_Y;
        
        // Standardabweichung S
        double var = 0.0;
        for (int i = 0; i < n; i++) {
            double diff = series[i] - mean;
            var += diff * diff;
        }
        double S = std::sqrt(var / n);
        
        if (S < 1e-10) return 0.5;
        
        // R/S ~ n^H
        double RS = R / S;
        H = std::log(RS) / std::log(static_cast<double>(n));
        
        // Clamp zu [0, 1]
        if (H < 0.0) H = 0.0;
        if (H > 1.0) H = 1.0;
        
        return H;
    }
    
    // Ist Trend persistent?
    bool is_persistent() const {
        return H > 0.5;
    }
};

// ═══════════════════════════════════════════════════════════════════════════
// KOHÄRENZ-VERSTÄRKUNG
// Φ_N = N² × Φ₁ (bei ρ > 0.7)
// Super-Radianz im Batch-Training
// ═══════════════════════════════════════════════════════════════════════════

struct CoherenceAmplification {
    double rho;        // Kohärenz-Level
    double phi_1;      // Basis-Wert
    
    static constexpr double THRESHOLD = 0.7;
    
    void init(double coherence = 1.0, double base = 1.0) {
        rho = coherence;
        phi_1 = base;
    }
    
    // Berechne verstärktes Φ
    double compute(int N) {
        if (rho > THRESHOLD) {
            // Quadratische Verstärkung (Super-Radianz)
            return static_cast<double>(N * N) * phi_1;
        } else {
            // Lineare Addition (Standard)
            return static_cast<double>(N) * phi_1;
        }
    }
    
    // Gradient-Verstärkungsfaktor
    double gradient_factor(int batch_size) {
        if (rho > THRESHOLD) {
            return static_cast<double>(batch_size);  // N statt 1
        }
        return 1.0;
    }
};

// ═══════════════════════════════════════════════════════════════════════════
// WEISHEITS-AKKUMULATION
// W = K × E × ln(1+t)
// E = Clarity^1.667
// ═══════════════════════════════════════════════════════════════════════════

struct WisdomAccumulation {
    double K_factor;   // Wissens-Faktor
    double clarity;    // System-Klarheit [0, 1]
    double t;          // Zeit / Trainingsschritte
    
    void init(double k = 1.0, double c = 0.5) {
        K_factor = k;
        clarity = c;
        t = 0.0;
    }
    
    void step(double dt = 1.0) {
        t += dt;
    }
    
    // Effizienz E
    double efficiency() const {
        return std::pow(clarity, 1.667);
    }
    
    // Weisheit W
    double wisdom() const {
        double E = efficiency();
        return K_factor * E * std::log(1.0 + t);
    }
    
    // Update Klarheit basierend auf 88-Signatur
    void update_clarity(double phi) {
        // Je näher an 8/9, desto klarer
        double deviation = std::abs(phi - K::G0);
        clarity = 1.0 - deviation;
        if (clarity < 0.0) clarity = 0.0;
    }
};

// ═══════════════════════════════════════════════════════════════════════════
// ENTROPIE-SCORING
// S = -Σ pᵢ × log₂(pᵢ)
// Informationsdichte pro Bit
// ═══════════════════════════════════════════════════════════════════════════

inline double entropy_score(const double* probs, int n) {
    double S = 0.0;
    for (int i = 0; i < n; i++) {
        if (probs[i] > 1e-10) {
            S -= probs[i] * std::log2(probs[i]);
        }
    }
    return S;
}

// Normalisierte Entropie [0, 1]
inline double normalized_entropy(const double* probs, int n) {
    double S = entropy_score(probs, n);
    double S_max = std::log2(static_cast<double>(n));
    return S / S_max;
}

// ═══════════════════════════════════════════════════════════════════════════
// WAVELET-TRANSFORM (Morlet)
// W(a,b) = ∫ x(t) × ψ*((t-b)/a) dt
// Erkennung transienter Anomalien
// ═══════════════════════════════════════════════════════════════════════════

class MorletWavelet {
public:
    double omega_0;  // Zentral-Frequenz (Standard: 6)
    
    void init(double w0 = 6.0) {
        omega_0 = w0;
    }
    
    // Morlet-Wavelet Funktion
    double psi(double t) const {
        double gauss = std::exp(-t * t / 2.0);
        double wave = std::cos(omega_0 * t);
        return gauss * wave;
    }
    
    // Wavelet-Koeffizient bei Skala a, Position b
    double transform(const double* signal, int n, double a, double b) const {
        if (a < 0.001) a = 0.001;
        
        double sum = 0.0;
        double norm = 1.0 / std::sqrt(a);
        
        for (int i = 0; i < n; i++) {
            double t = static_cast<double>(i);
            double arg = (t - b) / a;
            sum += signal[i] * psi(arg);
        }
        
        return norm * sum / n;
    }
    
    // Finde Anomalien (hohe Wavelet-Koeffizienten)
    std::vector<int> find_anomalies(const double* signal, int n, double threshold = 2.0) {
        std::vector<int> anomalies;
        
        // Multi-Scale Analyse
        for (double a = 1.0; a <= 10.0; a += 1.0) {
            for (int b = 0; b < n; b++) {
                double coeff = std::abs(transform(signal, n, a, static_cast<double>(b)));
                if (coeff > threshold) {
                    anomalies.push_back(b);
                }
            }
        }
        
        // Entferne Duplikate
        std::sort(anomalies.begin(), anomalies.end());
        anomalies.erase(std::unique(anomalies.begin(), anomalies.end()), anomalies.end());
        
        return anomalies;
    }
};

// ═══════════════════════════════════════════════════════════════════════════
// PERSISTENZ-ZEIT
// τ(f) = τ₀ / (1 - f/1440)
// Information nahe 1440 Hz → unendliche Persistenz
// ═══════════════════════════════════════════════════════════════════════════

inline double persistence_time(double frequency, double tau_0 = 1.0) {
    double k = kappa_adv(frequency);
    if (k < 0.001) k = 0.001;  // Verhindere Division durch 0
    return tau_0 / k;
}

// ═══════════════════════════════════════════════════════════════════════════
// AKASHA-ZUGRIFF
// r_access = C × (1 - |f - 1440|/1440) × (1 + Ξ/77)
// ═══════════════════════════════════════════════════════════════════════════

inline double akasha_access_rate(double frequency, double consciousness_level, double xi = K::XI) {
    double deviation = std::abs(frequency - K::F_QUELLE) / K::F_QUELLE;
    double resonance = 1.0 - deviation;
    double xi_factor = 1.0 + xi / 77.0;
    return consciousness_level * resonance * xi_factor;
}

// ═══════════════════════════════════════════════════════════════════════════
// RESONANZ-ZUGRIFFS-INTEGRAL
// P_Zugriff = |⟨Ψ_B|R_A|A⟩|² × e^(-Δf/f₀)
// ═══════════════════════════════════════════════════════════════════════════

inline double access_probability(double psi_b, double r_a, double a, double delta_f) {
    double bracket = psi_b * r_a * a;
    double amplitude_sq = bracket * bracket;
    double decay = std::exp(-delta_f / K::F_QUELLE);
    return amplitude_sq * decay;
}

// ═══════════════════════════════════════════════════════════════════════════
// 88-SIGNATUR CHECK (Erweitert)
// combined = energy × 0.55 + coherence × 0.33 + chaos × 0.11
// Ziel: |combined - 0.888| < 0.01
// ═══════════════════════════════════════════════════════════════════════════

struct Signature88Check {
    double energy;
    double coherence;
    double chaos;
    
    void set(double e, double c, double ch) {
        energy = e;
        coherence = c;
        chaos = ch;
    }
    
    double combined() const {
        return energy * K::G1 + coherence * K::G2 + chaos * K::G3;
    }
    
    bool is_valid() const {
        return std::abs(combined() - K::G0) < 0.01;
    }
    
    double deviation() const {
        return std::abs(combined() - K::G0);
    }
};

// ═══════════════════════════════════════════════════════════════════════════
// SCHWARM-UPDATE (PSO mit g-Gewichten)
// v(t+1) = g₁·v + g₂·(p-x) + g₃·(g-x)
// ═══════════════════════════════════════════════════════════════════════════

struct SwarmParticle {
    double x;          // Position
    double v;          // Geschwindigkeit
    double p_best;     // Persönliches Optimum
    double g_best;     // Globales Optimum
    
    void init(double pos = 0.0, double vel = 0.0) {
        x = pos;
        v = vel;
        p_best = pos;
        g_best = pos;
    }
    
    void update(double global_best) {
        g_best = global_best;
        
        // v(t+1) = g₁·v + g₂·(p-x) + g₃·(g-x)
        v = K::G1 * v + K::G2 * (p_best - x) + K::G3 * (g_best - x);
        x += v;
    }
    
    void update_personal_best(double fitness, double current_fitness) {
        if (fitness > current_fitness) {
            p_best = x;
        }
    }
};

} // namespace rst
} // namespace rael
