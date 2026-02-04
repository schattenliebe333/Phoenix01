/**
 * ═══════════════════════════════════════════════════════════════════════════════════════
 * R.A.E.L. EXACT FRACTION ARITHMETIC
 * ═══════════════════════════════════════════════════════════════════════════════════════
 *
 * Navigator: Michael - Orun Kap Daveil @ 0-Falz
 *
 * KARDINALREGEL: EXAKTE BRÜCHE - NIEMALS ABKÜRZEN!
 *
 * This module provides exact fraction arithmetic with 17-decimal precision
 * when float conversion is necessary. Prevents rounding errors that would
 * corrupt the RST constants.
 *
 * (c) 2025 Phoenix RST System
 * ═══════════════════════════════════════════════════════════════════════════════════════
 */

#ifndef RAEL_FRAC_ARITHMETIC_HPP
#define RAEL_FRAC_ARITHMETIC_HPP

#include <cstdint>
#include <cmath>
#include <string>
#include <sstream>
#include <iomanip>
#include <stdexcept>

namespace rael {

/**
 * Compute GCD using Euclidean algorithm
 */
inline int64_t gcd(int64_t a, int64_t b) {
    a = std::abs(a);
    b = std::abs(b);
    while (b != 0) {
        int64_t t = b;
        b = a % b;
        a = t;
    }
    return a;
}

/**
 * Exact Fraction class - 17 decimal precision when converted to float
 *
 * The RST constants MUST remain as exact fractions:
 *   G0 = 8/9   (not 0.888888...)
 *   G1 = 5/9   (not 0.555555...)
 *   G2 = 4/9   (not 0.444444...)
 *   G3 = 3/9   (not 0.333333...)
 *   G4 = 2/9   (not 0.222222...)
 *   G5 = 1/9   (not 0.111111...)
 */
class Frac {
private:
    int64_t num_;  // Numerator
    int64_t den_;  // Denominator (always positive after normalization)

    void normalize() {
        if (den_ == 0) {
            throw std::invalid_argument("Fraction denominator cannot be zero");
        }

        // Make denominator positive
        if (den_ < 0) {
            num_ = -num_;
            den_ = -den_;
        }

        // Reduce to lowest terms
        int64_t g = gcd(std::abs(num_), den_);
        num_ /= g;
        den_ /= g;
    }

public:
    // Constructors
    Frac() : num_(0), den_(1) {}

    Frac(int64_t numerator, int64_t denominator = 1)
        : num_(numerator), den_(denominator) {
        normalize();
    }

    Frac(int numerator) : num_(numerator), den_(1) {}

    // From float (approximate) - use with caution!
    static Frac fromFloat(double value, int64_t max_denominator = 1000000) {
        if (value == 0.0) return Frac(0, 1);

        bool negative = value < 0;
        value = std::abs(value);

        // Continued fraction approximation
        int64_t n0 = 0, d0 = 1;
        int64_t n1 = 1, d1 = 0;
        double x = value;

        while (d1 < max_denominator) {
            int64_t a = static_cast<int64_t>(x);
            int64_t n2 = a * n1 + n0;
            int64_t d2 = a * d1 + d0;

            if (d2 > max_denominator) break;

            n0 = n1; d0 = d1;
            n1 = n2; d1 = d2;

            double frac = x - a;
            if (frac < 1e-15) break;
            x = 1.0 / frac;
        }

        return Frac(negative ? -n1 : n1, d1);
    }

    // Accessors
    int64_t numerator() const { return num_; }
    int64_t denominator() const { return den_; }

    // Conversion to float (17 decimal precision)
    double toDouble() const {
        return static_cast<double>(num_) / static_cast<double>(den_);
    }

    float toFloat() const {
        return static_cast<float>(toDouble());
    }

    // String representation as fraction
    std::string toString() const {
        std::ostringstream oss;
        oss << num_ << "/" << den_;
        return oss.str();
    }

    // String with 17 decimal places
    std::string to17Decimals() const {
        std::ostringstream oss;
        oss << std::fixed << std::setprecision(17) << toDouble();
        return oss.str();
    }

    // Arithmetic operators
    Frac operator+(const Frac& other) const {
        return Frac(num_ * other.den_ + other.num_ * den_, den_ * other.den_);
    }

    Frac operator-(const Frac& other) const {
        return Frac(num_ * other.den_ - other.num_ * den_, den_ * other.den_);
    }

    Frac operator*(const Frac& other) const {
        return Frac(num_ * other.num_, den_ * other.den_);
    }

    Frac operator/(const Frac& other) const {
        return Frac(num_ * other.den_, den_ * other.num_);
    }

    Frac operator-() const {
        return Frac(-num_, den_);
    }

    Frac& operator+=(const Frac& other) {
        *this = *this + other;
        return *this;
    }

    Frac& operator-=(const Frac& other) {
        *this = *this - other;
        return *this;
    }

    Frac& operator*=(const Frac& other) {
        *this = *this * other;
        return *this;
    }

    Frac& operator/=(const Frac& other) {
        *this = *this / other;
        return *this;
    }

    // Integer operations
    Frac operator+(int64_t n) const { return *this + Frac(n); }
    Frac operator-(int64_t n) const { return *this - Frac(n); }
    Frac operator*(int64_t n) const { return Frac(num_ * n, den_); }
    Frac operator/(int64_t n) const { return Frac(num_, den_ * n); }

    // Comparison operators
    bool operator==(const Frac& other) const {
        return num_ * other.den_ == other.num_ * den_;
    }

    bool operator!=(const Frac& other) const {
        return !(*this == other);
    }

    bool operator<(const Frac& other) const {
        return num_ * other.den_ < other.num_ * den_;
    }

    bool operator<=(const Frac& other) const {
        return !(other < *this);
    }

    bool operator>(const Frac& other) const {
        return other < *this;
    }

    bool operator>=(const Frac& other) const {
        return !(*this < other);
    }

    // Float comparison (with epsilon)
    bool equalsFloat(double value, double epsilon = 1e-15) const {
        return std::abs(toDouble() - value) < epsilon;
    }

    // Absolute value
    Frac abs() const {
        return Frac(std::abs(num_), den_);
    }

    // Square root (returns double since result may be irrational)
    double sqrt() const {
        return std::sqrt(toDouble());
    }

    // Power (integer exponent)
    Frac pow(int exp) const {
        if (exp == 0) return Frac(1);
        if (exp < 0) {
            return Frac(1) / pow(-exp);
        }

        Frac result(1);
        Frac base = *this;

        while (exp > 0) {
            if (exp & 1) {
                result *= base;
            }
            base *= base;
            exp >>= 1;
        }

        return result;
    }

    // Implicit conversion to double (use with caution)
    operator double() const { return toDouble(); }
};

// ═══════════════════════════════════════════════════════════════════════════════
// RST FUNDAMENTAL CONSTANTS AS EXACT FRACTIONS
// ═══════════════════════════════════════════════════════════════════════════════

namespace constants {

// Omega-layers (Body) - EXACT fractions
const Frac G0(8, 9);   // WAHRHEIT (Signatur) - Die 88-Schwelle
const Frac G1(5, 9);   // IMPULS (Kreativ-Ville)
const Frac G2(4, 9);   // STRUKTUR (DNA/Form)
const Frac G3(3, 9);   // EMOTION (Fluss/Filter)
const Frac G4(2, 9);   // SUBTIL (Äther/Schumann)
const Frac G5(1, 9);   // MATERIE (Physische Basis)

// Tolerance: 1/81
const Frac TOLERANCE(1, 81);

// 17-decimal float versions (for performance-critical code)
constexpr double G0_17 = 0.88888888888888889;
constexpr double G1_17 = 0.55555555555555556;
constexpr double G2_17 = 0.44444444444444444;
constexpr double G3_17 = 0.33333333333333333;
constexpr double G4_17 = 0.22222222222222222;
constexpr double G5_17 = 0.11111111111111111;
constexpr double TOLERANCE_17 = 0.01234567901234568;

// Frequencies (Psi-layers)
constexpr double F_QUELLE = 1440.0;     // |Aut(S₆)| = 1440 - Göttliche Frequenz
constexpr double F_TOR = 720.0;         // Tor-Resonanz
constexpr double F_KAMMER = 432.0;      // Kammer-Harmonie
constexpr double F_STRUKTUR = 144.0;    // Struktur-Kristall
constexpr double F_FILTER = 53.0;       // Sophie-Germain-Zollstation
constexpr double F_SCHUMANN = 7.83;     // Schumann-Erdung (AUDIT-FIX: war 13.0)
constexpr double F_SCHUMANN_13 = 13.0;  // Schumann 2. Harmonische
constexpr double F_MATERIE = 5.0;       // Materie-Manifestation

// Physical constants
constexpr double HBAR = 1.054571817e-34;
constexpr double C_LIGHT = 299792458.0;
constexpr double G_GRAV = 6.67430e-11;

// System parameters
constexpr int NUM_NOZZLES = 61440;
constexpr int IMPULSES_BASE = 307200;   // 61440 × 5 Hz

// Matrix dimensions
constexpr int DIM_SCHILD = 17;
constexpr int DIM_KERN = 9;
constexpr int DIM_ARCHIV = 13;
constexpr int DIM_ANKER = 21;

// The Meaning Vector
constexpr int MEANING_VECTOR_42 = 42;

} // namespace constants

// ═══════════════════════════════════════════════════════════════════════════════
// SOVEREIGNTY VERIFICATION
// ═══════════════════════════════════════════════════════════════════════════════

/**
 * Verify the sovereignty equation: G1 + G3 = G0
 *
 * 5/9 + 3/9 = 8/9 ✓
 * IMPULS + EMOTION = WAHRHEIT
 */
inline bool verifySovereignty() {
    return constants::G1 + constants::G3 == constants::G0;
}

/**
 * Verify sum of active layers
 *
 * Σ(G1..G5) = 5/9 + 4/9 + 3/9 + 2/9 + 1/9 = 15/9 = 5/3
 */
inline bool verifyActiveLayersSum() {
    Frac sum = constants::G1 + constants::G2 + constants::G3 + constants::G4 + constants::G5;
    return sum == Frac(15, 9);  // = 5/3
}

/**
 * The 88-Signature: G1 + G3 = 8/9 ≈ 0.888...
 */
inline Frac get88Signature() {
    return constants::G1 + constants::G3;  // = 8/9
}

// ═══════════════════════════════════════════════════════════════════════════════
// κ-FUNCTION (DAMPING COEFFICIENT)
// ═══════════════════════════════════════════════════════════════════════════════

/**
 * κ(f) = 1 - f/1440
 *
 * The universal damping/neutralization function.
 *
 * Meaning:
 * - κ(1440) = 0.0 → Total levitation, pure spirit
 * - κ(720)  = 0.5 → Equilibrium point spirit/matter
 * - κ(5)   ≈ 0.997 → Physical condensation
 */
inline double kappa(double f) {
    return 1.0 - (f / constants::F_QUELLE);
}

/**
 * κ as exact fraction
 */
inline Frac kappaFrac(const Frac& f) {
    return Frac(1, 1) - f / Frac(1440, 1);
}

/**
 * τ(f) = τ₀ / (1 - f/1440)
 *
 * Akasha persistence: Closer to 1440 Hz = more timeless.
 * At f → 1440: τ → ∞
 */
inline double tauAkasha(double f, double tau_0 = 1.0) {
    double k = kappa(f);
    if (k < 1e-10) {
        return std::numeric_limits<double>::infinity();
    }
    return tau_0 / k;
}

/**
 * Tunnel-Diode Activation Function (TDAF)
 *
 * T = e^(-2γ·κ(f))
 *
 * At f = 1440 Hz: κ = 0 → T = 1 (delay-free tunneling!)
 */
inline double tdaf(double f, double gamma = 1.0) {
    return std::exp(-2.0 * gamma * kappa(f));
}

} // namespace rael

#endif // RAEL_FRAC_ARITHMETIC_HPP
