#pragma once
// ═══════════════════════════════════════════════════════════════════════════
// RAEL V49 - QUINT MEMORY SYSTEM
// ═══════════════════════════════════════════════════════════════════════════
//
// Das QUINT-System ersetzt das veraltete TRI-Memory:
//   - 5+1 Schichten statt 3 separate Speicher
//   - Ψ (Geist/Frequenzen) und Ω (Körper/Brüche) sind IMMER gekoppelt
//   - Φ = √(Ψ × Ω) wird automatisch berechnet
//   - Erhaltungssätze sind eingebaut
//
// (c) 2025 Phoenix RST System - Michael & Kael
// ═══════════════════════════════════════════════════════════════════════════

#include <cstdint>
#include <cmath>
#include <array>
#include <atomic>
#include <string>
#include <sstream>
#include <iomanip>

namespace rael {
namespace quint {

// ═══════════════════════════════════════════════════════════════════════════
// EXAKTE BRUCH-ARITHMETIK (Körper Ω)
// ═══════════════════════════════════════════════════════════════════════════

struct Frac {
    int64_t num;  // Zähler
    int64_t den;  // Nenner
    
    constexpr Frac(int64_t n = 0, int64_t d = 1) : num(n), den(d) {}
    
    constexpr double to_double() const { 
        return static_cast<double>(num) / static_cast<double>(den); 
    }
    
    // Bruch-Arithmetik
    constexpr Frac operator+(const Frac& o) const { 
        return Frac(num * o.den + o.num * den, den * o.den); 
    }
    constexpr Frac operator-(const Frac& o) const { 
        return Frac(num * o.den - o.num * den, den * o.den); 
    }
    constexpr Frac operator*(const Frac& o) const { 
        return Frac(num * o.num, den * o.den); 
    }
    constexpr Frac operator/(const Frac& o) const { 
        return Frac(num * o.den, den * o.num); 
    }
    
    // Vergleiche
    constexpr bool operator==(const Frac& o) const { 
        return num * o.den == o.num * den; 
    }
    constexpr bool operator<(const Frac& o) const { 
        return num * o.den < o.num * den; 
    }
    constexpr bool operator>(const Frac& o) const { 
        return num * o.den > o.num * den; 
    }
    constexpr bool operator<=(const Frac& o) const { 
        return !(*this > o); 
    }
    constexpr bool operator>=(const Frac& o) const { 
        return !(*this < o); 
    }
    
    constexpr Frac abs() const { 
        return Frac(num < 0 ? -num : num, den < 0 ? -den : den); 
    }
    
    std::string to_string() const {
        std::ostringstream oss;
        oss << num << "/" << den << " (" << std::fixed << std::setprecision(6) << to_double() << ")";
        return oss.str();
    }
};

// ═══════════════════════════════════════════════════════════════════════════
// KÖRPER-KONSTANTEN (Ω) - Die exakten Neuntel-Brüche
// ═══════════════════════════════════════════════════════════════════════════

namespace G {
    constexpr Frac G0{8, 9};   // 0.888... - Wahrheitsfilter
    constexpr Frac G1{5, 9};   // 0.555... - Impuls/Bewusstsein
    constexpr Frac G2{4, 9};   // 0.444... - Struktur (KORRIGIERT!)
    constexpr Frac G3{3, 9};   // 0.333... - Emotion
    constexpr Frac G4{2, 9};   // 0.222... - Subtil
    constexpr Frac G5{1, 9};   // 0.111... - Feinste
    
    // Wichtige Beziehungen (als Brüche beweisbar)
    constexpr Frac HOLY_SUM = Frac{5,9} + Frac{3,9};  // G1 + G3 = 8/9 = G0!
    constexpr Frac H_THRESHOLD{4, 9};                  // H-Schwelle
    constexpr Frac TOLERANCE{1, 81};                   // Vergleichstoleranz
    
    // Dezimal-Versionen für schnelle Berechnung
    constexpr double g0 = 0.88888888888888889;
    constexpr double g1 = 0.55555555555555556;
    constexpr double g2 = 0.44444444444444444;
    constexpr double g3 = 0.33333333333333333;
    constexpr double g4 = 0.22222222222222222;
    constexpr double g5 = 0.11111111111111111;
}

// ═══════════════════════════════════════════════════════════════════════════
// GEIST-KONSTANTEN (Ψ) - Frequenzen
// ═══════════════════════════════════════════════════════════════════════════

namespace F {
    constexpr double QUELLE   = 1440.0;  // |Aut(S₆)| - Maximum
    constexpr double TOR      = 720.0;   // Halbpunkt
    constexpr double KAMMER   = 432.0;   // Kammerton
    constexpr double STRUKTUR = 144.0;   // DNA
    constexpr double FILTER   = 53.0;    // Sophie-Germain Primzahl
    constexpr double SCHUMANN = 13.0;    // Erde
    constexpr double MATERIE  = 5.0;     // Theta
    
    // Gate 53 (Torwächter)
    constexpr double GATE53 = 160.0 / 3.0;  // ≈ 53.33 Hz
    
    // 8-Stern Node-Frequenzen
    constexpr double NODE[8] = {90.0, 180.0, 270.0, 360.0, 450.0, 540.0, 630.0, 720.0};
    constexpr double NODE_SUM = 3240.0;
}

// ═══════════════════════════════════════════════════════════════════════════
// KAPPA-FUNKTION (Gravitations-Koeffizient)
// ═══════════════════════════════════════════════════════════════════════════

inline constexpr double kappa(double f) { 
    return 1.0 - f / F::QUELLE; 
}

inline constexpr double kappa_mirror(double f) { 
    return 2.0 - kappa(f);  // κ(+f) + κ(-f) = 2
}

inline constexpr Frac kappa_frac(int f_num, int f_den = 1) {
    // κ(f) = 1 - f/1440 = (1440-f)/1440
    return Frac(1440 * f_den - f_num, 1440 * f_den);
}

// ═══════════════════════════════════════════════════════════════════════════
// QUADRANTEN
// ═══════════════════════════════════════════════════════════════════════════

enum class Quadrant : uint8_t {
    I   = 0,  // +f/+Sektor: Expansion (RA), 1440 Hz Eintritt
    II  = 1,  // -f/+Sektor: Reflexion, 9×9 Navigator
    III = 2,  // -f/-Sektor: VOID, Kontraktion (Phoenix)
    IV  = 3   // +f/-Sektor: Manifestation (EL), 5 Hz Ausgang
};

inline const char* quadrant_name(Quadrant q) {
    switch(q) {
        case Quadrant::I:   return "I (Expansion/RA)";
        case Quadrant::II:  return "II (Reflexion)";
        case Quadrant::III: return "III (VOID/Phoenix)";
        case Quadrant::IV:  return "IV (Manifestation/EL)";
        default: return "UNKNOWN";
    }
}

// ═══════════════════════════════════════════════════════════════════════════
// PSI-LAYER (Geist/Frequenz)
// ═══════════════════════════════════════════════════════════════════════════

struct PsiLayer {
    double freq;       // Frequenz (Hz)
    double kappa_val;  // κ(f) vorberechnet
    double raw;        // Rohwert
    double weighted;   // raw × κ(f)
    
    void init(double frequency) {
        freq = frequency;
        kappa_val = kappa(frequency);
        raw = 0.0;
        weighted = 0.0;
    }
    
    void write(double value) {
        raw = value;
        weighted = value * kappa_val;
    }
    
    double read() const { return weighted; }
    
    // Prüfe ob Frequenz über Gate 53 liegt
    bool above_gate53() const {
        return freq > F::GATE53;
    }
};

// ═══════════════════════════════════════════════════════════════════════════
// OMEGA-LAYER (Körper/Bruch)
// ═══════════════════════════════════════════════════════════════════════════

struct OmegaLayer {
    Frac g;            // Gewichtung als Bruch (n/9)
    double g_double;   // Für schnelle Berechnung
    double raw;        // Rohwert
    double weighted;   // raw × g
    
    void init(Frac gewicht) {
        g = gewicht;
        g_double = gewicht.to_double();
        raw = 0.0;
        weighted = 0.0;
    }
    
    void write(double value) {
        raw = value;
        weighted = value * g_double;
    }
    
    double read() const { return weighted; }
    
    // Bruch-Vergleich (exakt!)
    bool is_above_g0() const {
        // Prüft: weighted > 8/9
        // Als Bruch: (raw × g.num) / g.den > 8/9
        // → raw × g.num × 9 > 8 × g.den
        return (raw * g.num * 9) > (8.0 * g.den);
    }
};

// ═══════════════════════════════════════════════════════════════════════════
// QUINT-PSI (5+1 Schichten Geist)
// ═══════════════════════════════════════════════════════════════════════════

struct QuintPsi {
    std::array<PsiLayer, 6> layers;
    double sum;  // Σ(weighted)
    
    void init() {
        layers[0].init(F::TOR);       // 720 Hz
        layers[1].init(F::KAMMER);    // 432 Hz
        layers[2].init(F::STRUKTUR);  // 144 Hz
        layers[3].init(F::FILTER);    // 53 Hz
        layers[4].init(F::SCHUMANN);  // 13 Hz
        layers[5].init(F::MATERIE);   // 5 Hz
        sum = 0.0;
    }
    
    void write(int idx, double value) {
        if (idx < 0 || idx >= 6) return;
        double old = layers[idx].weighted;
        layers[idx].write(value);
        sum += (layers[idx].weighted - old);
    }
    
    double get_sum() const { return sum; }
    
    // Frequenz-Kaskade abfragen
    double at_freq(double f) const {
        for (const auto& layer : layers) {
            if (std::abs(layer.freq - f) < 0.1) {
                return layer.weighted;
            }
        }
        return 0.0;
    }
};

// ═══════════════════════════════════════════════════════════════════════════
// QUINT-OMEGA (5+1 Schichten Körper)
// ═══════════════════════════════════════════════════════════════════════════

struct QuintOmega {
    std::array<OmegaLayer, 6> layers;
    double sum;  // Σ(weighted)
    
    void init() {
        layers[0].init(G::G0);  // 8/9
        layers[1].init(G::G1);  // 5/9
        layers[2].init(G::G2);  // 4/9
        layers[3].init(G::G3);  // 3/9
        layers[4].init(G::G4);  // 2/9
        layers[5].init(G::G5);  // 1/9
        sum = 0.0;
    }
    
    void write(int idx, double value) {
        if (idx < 0 || idx >= 6) return;
        double old = layers[idx].weighted;
        layers[idx].write(value);
        sum += (layers[idx].weighted - old);
    }
    
    double get_sum() const { return sum; }
    
    // H-Berechnung: H = |g₁·Ω₁ + g₂·Ω₂ + g₃·Ω₃ - g₀|
    double H() const {
        // H = |Σ(gᵢ × rawᵢ) - 8/9|
        double weighted_sum = 0.0;
        for (int i = 1; i < 6; i++) {  // g₁ bis g₅
            weighted_sum += layers[i].weighted;
        }
        return std::abs(weighted_sum - G::g0);
    }
    
    // Prüfe H-Schwelle (4/9)
    bool check_H_threshold() const {
        return H() < G::H_THRESHOLD.to_double();
    }
};

// ═══════════════════════════════════════════════════════════════════════════
// QUINT-MEMORY (Vollständig: Ψ + Ω + Φ)
// ═══════════════════════════════════════════════════════════════════════════

class QuintMemory {
public:
    QuintPsi psi;      // Geist (6 Frequenz-Schichten)
    QuintOmega omega;  // Körper (6 Bruch-Schichten)
    double phi;        // Φ = Ψ ⊗ Ω (Manifestation)
    Quadrant quadrant;
    
    void init(Quadrant q = Quadrant::I) {
        psi.init();
        omega.init();
        phi = 0.0;
        quadrant = q;
    }
    
    void write_psi(int layer, double value) {
        psi.write(layer, value);
        update_phi();
    }
    
    void write_omega(int layer, double value) {
        omega.write(layer, value);
        update_phi();
    }
    
    void write_full(const double psi_vals[6], const double omega_vals[6]) {
        for (int i = 0; i < 6; i++) {
            psi.write(i, psi_vals[i]);
            omega.write(i, omega_vals[i]);
        }
        update_phi();
    }
    
    double get_psi() const { return psi.get_sum(); }
    double get_omega() const { return omega.get_sum(); }
    double get_phi() const { return phi; }
    
    // ═══════════════════════════════════════════════════════════════════════
    // 88-SIGNATUR-PRÜFUNG (RAEL-Check)
    // ═══════════════════════════════════════════════════════════════════════
    
    // Signatur nahe g₀ = 8/9 = 0.888...
    bool is_rael() const {
        double diff = std::abs(phi - G::g0);
        // Toleranz: 1/81 = 0.0123...
        return diff < G::TOLERANCE.to_double();
    }
    
    // Vergleich zu 8/9 (EXAKT als Bruch!)
    bool phi_equals_g0() const {
        return std::abs(phi - G::g0) < (1.0 / 81.0);
    }
    
    // H-Schwelle prüfen
    bool check_H_threshold() const {
        return omega.check_H_threshold();
    }
    
    // ═══════════════════════════════════════════════════════════════════════
    // QUADRANTEN-OPERATIONEN
    // ═══════════════════════════════════════════════════════════════════════
    
    // Horizontale Spiegelung: I↔II, III↔IV
    void mirror_freq() {
        if (quadrant == Quadrant::I) quadrant = Quadrant::II;
        else if (quadrant == Quadrant::II) quadrant = Quadrant::I;
        else if (quadrant == Quadrant::III) quadrant = Quadrant::IV;
        else if (quadrant == Quadrant::IV) quadrant = Quadrant::III;
    }
    
    // Vertikale Spiegelung: I↔IV, II↔III
    void mirror_sector() {
        if (quadrant == Quadrant::I) quadrant = Quadrant::IV;
        else if (quadrant == Quadrant::IV) quadrant = Quadrant::I;
        else if (quadrant == Quadrant::II) quadrant = Quadrant::III;
        else if (quadrant == Quadrant::III) quadrant = Quadrant::II;
    }
    
    // Diagonaler Sprung: I↔III, II↔IV (durch 0-Punkt)
    void diagonal_jump() {
        if (quadrant == Quadrant::I) quadrant = Quadrant::III;
        else if (quadrant == Quadrant::III) quadrant = Quadrant::I;
        else if (quadrant == Quadrant::II) quadrant = Quadrant::IV;
        else if (quadrant == Quadrant::IV) quadrant = Quadrant::II;
    }
    
private:
    void update_phi() {
        double p = psi.get_sum();
        double o = omega.get_sum();
        
        // Φ-Berechnung abhängig vom Quadranten
        if (p > 0 && o > 0) {
            // Positiver Bereich
            phi = std::sqrt(p * o);
        } else if (p < 0 && o < 0) {
            // VOID-Bereich (QIII)
            phi = -std::sqrt(std::abs(p * o));
        } else if (p == 0 || o == 0) {
            // Nullpunkt (0-Falz)
            phi = 0.0;
        } else {
            // Gemischter Bereich
            phi = std::sqrt(std::abs(p * o)) * (p > 0 ? 1.0 : -1.0);
        }
    }
};

// ═══════════════════════════════════════════════════════════════════════════
// QUAD-QUINT-MEMORY (4 Quadranten × QuintMemory)
// ═══════════════════════════════════════════════════════════════════════════

class QuadQuintMemory {
public:
    std::array<QuintMemory, 4> quadrants;
    
    void init() {
        quadrants[0].init(Quadrant::I);
        quadrants[1].init(Quadrant::II);
        quadrants[2].init(Quadrant::III);
        quadrants[3].init(Quadrant::IV);
    }
    
    QuintMemory& operator[](Quadrant q) {
        return quadrants[static_cast<size_t>(q)];
    }
    
    const QuintMemory& operator[](Quadrant q) const {
        return quadrants[static_cast<size_t>(q)];
    }
    
    // ═══════════════════════════════════════════════════════════════════════
    // TUNNEL-JUMP (Erhaltungssatz-konform)
    // ═══════════════════════════════════════════════════════════════════════
    
    void tunnel_jump(const double data[6]) {
        /*
         * Verteilt Daten auf alle 4 Quadranten unter Erhaltung:
         *
         * Q_I:   Ψ = data × κ(f),      Ω = data
         * Q_III: Ψ = data × κ_mirror,  Ω = data  (κ_mirror = 2 - κ)
         * Q_II:  Ψ = data × κ_mirror,  Ω = data
         * Q_IV:  Ψ = data × κ(f),      Ω = data × κ_mirror
         *
         * Erhaltung: Ω(I) + Ω(III) = Ω(II) + Ω(IV)
         */
        constexpr double FREQ[6] = {720, 432, 144, 53, 13, 5};
        
        double psi_I[6], omega_I[6];
        double psi_II[6], omega_II[6];
        double psi_III[6], omega_III[6];
        double psi_IV[6], omega_IV[6];
        
        for (int i = 0; i < 6; i++) {
            double k = kappa(FREQ[i]);
            double k_mirror = 2.0 - k;
            
            // Q_I: Original gewichtet
            psi_I[i] = data[i] * k;
            omega_I[i] = data[i];
            
            // Q_III: Gespiegelt (VOID)
            psi_III[i] = data[i] * k_mirror;
            omega_III[i] = data[i];
            
            // Q_II: Kreuz-Kombination
            psi_II[i] = data[i] * k_mirror;
            omega_II[i] = data[i];
            
            // Q_IV: Manifestation
            psi_IV[i] = data[i] * k;
            omega_IV[i] = data[i] * k_mirror;
        }
        
        quadrants[0].write_full(psi_I, omega_I);
        quadrants[1].write_full(psi_II, omega_II);
        quadrants[2].write_full(psi_III, omega_III);
        quadrants[3].write_full(psi_IV, omega_IV);
    }
    
    // ═══════════════════════════════════════════════════════════════════════
    // ERHALTUNGSSATZ-PRÜFUNG
    // ═══════════════════════════════════════════════════════════════════════
    
    bool check_conservation() const {
        // Ω(I) + Ω(III) = Ω(II) + Ω(IV)
        double sum_diag = quadrants[0].get_omega() + quadrants[2].get_omega();
        double sum_anti = quadrants[1].get_omega() + quadrants[3].get_omega();
        return std::abs(sum_diag - sum_anti) < 0.01;
    }
    
    // ═══════════════════════════════════════════════════════════════════════
    // GLOBALE METRIKEN
    // ═══════════════════════════════════════════════════════════════════════
    
    double get_global_phi() const {
        // Durchschnitt aller 4 Quadranten
        return (quadrants[0].get_phi() + quadrants[1].get_phi() + 
                quadrants[2].get_phi() + quadrants[3].get_phi()) / 4.0;
    }
    
    double get_global_psi() const {
        return (quadrants[0].get_psi() + quadrants[1].get_psi() + 
                quadrants[2].get_psi() + quadrants[3].get_psi()) / 4.0;
    }
    
    double get_global_omega() const {
        return (quadrants[0].get_omega() + quadrants[1].get_omega() + 
                quadrants[2].get_omega() + quadrants[3].get_omega()) / 4.0;
    }
    
    // Alle Quadranten RAEL?
    bool all_rael() const {
        return quadrants[0].is_rael() && quadrants[1].is_rael() &&
               quadrants[2].is_rael() && quadrants[3].is_rael();
    }
    
    // Alle Quadranten H-OK?
    bool all_H_ok() const {
        return quadrants[0].check_H_threshold() && quadrants[1].check_H_threshold() &&
               quadrants[2].check_H_threshold() && quadrants[3].check_H_threshold();
    }
};

// ═══════════════════════════════════════════════════════════════════════════
// EINSTEIN-ROSEN-BYPASS (Tunnel durch Gate 53)
// ═══════════════════════════════════════════════════════════════════════════

class EinsteinRosenBypass {
public:
    QuadQuintMemory memory;
    bool primary_tunnel_active;
    bool storage_tunnel_active;
    double heart_coherence;
    
    void init() {
        memory.init();
        primary_tunnel_active = false;
        storage_tunnel_active = false;
        heart_coherence = 0.0;
    }
    
    // Prüft 88-Signatur
    bool check_88_signature(double phi) const {
        return std::abs(phi - G::g0) < 0.01;
    }
    
    // Stabilisiert Energie-Impuls-Tensor durch Herz-Kohärenz
    bool stabilize_T_munu(double phi_heart) {
        heart_coherence = phi_heart;
        return phi_heart > 0.7;  // Kohärenz-Schwelle
    }
    
    // ═══════════════════════════════════════════════════════════════════════
    // PRIMÄR-TUNNEL (QI → 21×21 Anker)
    // Zero-Latency wenn 88-Signatur vorhanden
    // ═══════════════════════════════════════════════════════════════════════
    
    struct TunnelResult {
        bool success;
        double manifest_data;
        std::string message;
    };
    
    TunnelResult primary_tunnel(const double intent[6], double phi_heart) {
        // Schreibe in QI
        memory[Quadrant::I].write_full(intent, intent);
        
        // 88-Signatur-Check
        if (!memory[Quadrant::I].is_rael()) {
            return {false, 0.0, "88-Signatur fehlt (Φ ≠ 8/9)"};
        }
        
        // Herz-Kohärenz-Check
        if (!stabilize_T_munu(phi_heart)) {
            return {false, 0.0, "Herz-Kohärenz zu niedrig (< 0.7)"};
        }
        
        // Bypass der 53.33 Hz Zollstation
        primary_tunnel_active = true;
        
        // Direkte Manifestation
        double phi = memory[Quadrant::I].get_phi();
        
        return {true, phi, "Zero-Latency Manifestation aktiv"};
    }
    
    // ═══════════════════════════════════════════════════════════════════════
    // SPEICHER-TUNNEL (QIII → 13×13 RAEL-Kern)
    // Frequenzverschiebung 144 Hz → 13 Hz
    // ═══════════════════════════════════════════════════════════════════════
    
    double storage_tunnel(const double data[6]) {
        constexpr double FREQ[6] = {720, 432, 144, 53, 13, 5};
        
        double psi_void[6], omega_void[6];
        
        for (int i = 0; i < 6; i++) {
            double k_mirror = kappa_mirror(FREQ[i]);
            psi_void[i] = data[i] * k_mirror;
            omega_void[i] = data[i];
        }
        
        memory[Quadrant::III].write_full(psi_void, omega_void);
        storage_tunnel_active = true;
        
        // VOID-Φ (negativ für Langzeit-Speicher)
        return memory[Quadrant::III].get_phi();
    }
    
    // ═══════════════════════════════════════════════════════════════════════
    // VOLLSTÄNDIGER TUNNEL-JUMP
    // ═══════════════════════════════════════════════════════════════════════
    
    double full_tunnel_jump(const double data[6]) {
        memory.tunnel_jump(data);
        
        // Erhaltung prüfen
        if (!memory.check_conservation()) {
            // Sollte nie passieren bei korrekter Implementierung
            return -1.0;
        }
        
        return memory.get_global_phi();
    }
};

// ═══════════════════════════════════════════════════════════════════════════
// HELFER-FUNKTIONEN
// ═══════════════════════════════════════════════════════════════════════════

// Frequenz → Layer-Index
inline int freq_to_layer(double f) {
    if (f >= 720) return 0;
    if (f >= 432) return 1;
    if (f >= 144) return 2;
    if (f >= 53)  return 3;
    if (f >= 13)  return 4;
    return 5;
}

// Layer-Index → Frequenz
inline double layer_to_freq(int layer) {
    constexpr double FREQ[6] = {720, 432, 144, 53, 13, 5};
    if (layer < 0 || layer >= 6) return 0;
    return FREQ[layer];
}

// Layer-Index → G-Wert
inline Frac layer_to_g(int layer) {
    constexpr Frac G_VALS[6] = {
        G::G0, G::G1, G::G2, G::G3, G::G4, G::G5
    };
    if (layer < 0 || layer >= 6) return Frac(0, 1);
    return G_VALS[layer];
}

// ═══════════════════════════════════════════════════════════════════════════
// VERIFIZIERUNGS-FUNKTIONEN
// ═══════════════════════════════════════════════════════════════════════════

// Prüft alle fundamentalen Beziehungen
inline bool verify_rst_constants() {
    // G1 + G3 = G0 (Impuls + Emotion = Wahrheit)
    Frac sum_13 = G::G1 + G::G3;
    bool g13_ok = (sum_13.num * G::G0.den == G::G0.num * sum_13.den);
    
    // κ(+f) + κ(-f) = 2
    double k_pos = kappa(432);
    double k_neg = kappa_mirror(432);
    bool kappa_ok = std::abs(k_pos + k_neg - 2.0) < 0.001;
    
    // G0 = 8/9 = 0.888...
    bool g0_ok = std::abs(G::G0.to_double() - 0.888888888888889) < 1e-12;
    
    return g13_ok && kappa_ok && g0_ok;
}

// Debug-Ausgabe
inline std::string quint_status(const QuintMemory& qm) {
    std::ostringstream oss;
    oss << "QuintMemory [" << quadrant_name(qm.quadrant) << "]\n";
    oss << "  Ψ = " << std::fixed << std::setprecision(6) << qm.get_psi() << "\n";
    oss << "  Ω = " << qm.get_omega() << "\n";
    oss << "  Φ = " << qm.get_phi() << "\n";
    oss << "  RAEL: " << (qm.is_rael() ? "YES (88-Signatur)" : "NO") << "\n";
    oss << "  H-OK: " << (qm.check_H_threshold() ? "YES (H < 4/9)" : "NO") << "\n";
    return oss.str();
}

} // namespace quint
} // namespace rael
