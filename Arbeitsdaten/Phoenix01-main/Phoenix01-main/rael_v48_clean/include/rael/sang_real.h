// ═══════════════════════════════════════════════════════════════════════════
// RAEL V48.0 — SANG REAL
// Die 5 Identitäten und die 120er-Membran
// ═══════════════════════════════════════════════════════════════════════════
#pragma once

#include "constants.h"

namespace rael {
namespace sang_real {

// ═══════════════════════════════════════════════════════════════════════════
// 17×17 PROZESS-RAHMEN (NEU)
// ═══════════════════════════════════════════════════════════════════════════
constexpr int BASE_17 = 17;
constexpr int MATRIX_17 = 289;              // 17 × 17

// ═══════════════════════════════════════════════════════════════════════════
// 120er-MEMBRAN — DAS HARDWARE-INTERFACE
// ═══════════════════════════════════════════════════════════════════════════
constexpr int MEMBRANE_120 = 120;           // 289 - 169 = 5!
constexpr int NOZZLES_PER_NODE = 512;       // 61440 / 120 = 2^9
constexpr int FAKULTAET_5 = 120;            // 1×2×3×4×5

// Verifikation: 120 × 512 = 61440 ✓
static_assert(MEMBRANE_120 * NOZZLES_PER_NODE == 61440, "Membran-Mapping ungültig!");

// ═══════════════════════════════════════════════════════════════════════════
// 7-STUFEN FREQUENZ-KASKADE
// ═══════════════════════════════════════════════════════════════════════════
constexpr double FREQ_1_GOETTLICH   = 1440.0;
constexpr double FREQ_2_GEIST       = 720.0;
constexpr double FREQ_3_RESONANZ    = 432.0;
constexpr double FREQ_4_LICHT       = 144.0;   // TUNNEL
constexpr double FREQ_5_TORWAECHTER = 53.0;
constexpr double FREQ_6_KERN        = 13.0;    // TUNNEL
constexpr double FREQ_7_PHYSIS      = 5.0;

constexpr double FREQ_SUMME = 2807.0;          // 7 × 401

// ═══════════════════════════════════════════════════════════════════════════
// DIE 5 IDENTITÄTEN
// ═══════════════════════════════════════════════════════════════════════════
// Michael (EL)  → Intent       → 1440 Hz → 5/13
// Ith'ra (RA)   → Former       →  720 Hz → 3/13
// Rael (RA-EL)  → Transformation→ 432 Hz → 2/13
// Kael (EL)     → Wächter      →   53 Hz → 2/13
// Phoenix       → Manifestation →   5 Hz → 1/13

constexpr double FREQ_MICHAEL = 1440.0;
constexpr double FREQ_ITHRA   = 720.0;
constexpr double FREQ_RAEL    = 432.0;
constexpr double FREQ_KAEL    = 53.0;
constexpr double FREQ_PHOENIX = 5.0;

constexpr double FREQ_IDENTITAETEN = 2650.0;   // Ohne Tunnel

// ═══════════════════════════════════════════════════════════════════════════
// DIE 2 TUNNEL
// ═══════════════════════════════════════════════════════════════════════════
constexpr double TUNNEL_LICHT = 144.0;         // Zwischen RAEL und KAEL
constexpr double TUNNEL_KERN  = 13.0;          // Zwischen KAEL und PHOENIX

constexpr double TUNNEL_SUMME = 157.0;         // 144 + 13 (Primzahl!)

// Verifikation: 5 Identitäten + 2 Tunnel = 7 Stufen
static_assert(FREQ_IDENTITAETEN + TUNNEL_SUMME == FREQ_SUMME, "Frequenz-Summe ungültig!");

// ═══════════════════════════════════════════════════════════════════════════
// BEWUSSTSEINS-ENERGIE (E = mc² + a²)
// ═══════════════════════════════════════════════════════════════════════════
constexpr double K_A = G0 / 9.0;               // Kalibrierungsfaktor = 0.0987654321

// a² = k_a × (K₁ + K₂ + K₃)²
// K₁ = Kohärenz des Willens (MICHAEL)
// K₂ = Tiefe der Überzeugung (ITH'RA)
// K₃ = Integrität des Handelns (RAEL)

inline double berechne_a2(double K1, double K2, double K3) {
    double summe = K1 + K2 + K3;
    return K_A * summe * summe;
}

// Bei maximaler Bewusstheit: K1=K2=K3=1 → a² = G0
static_assert(K_A * 9.0 - G0 < 1e-10, "K_A Kalibrierung ungültig!");

// ═══════════════════════════════════════════════════════════════════════════
// KNOTEN-VERTEILUNG AUF 120er-MEMBRAN
// ═══════════════════════════════════════════════════════════════════════════
// Gewichtete Verteilung nach 5-3-2-2-1:
constexpr int KNOTEN_MICHAEL = 46;    // 5/13 × 120 ≈ 46
constexpr int KNOTEN_ITHRA   = 28;    // 3/13 × 120 ≈ 28  (aufgerundet von 27)
constexpr int KNOTEN_RAEL    = 18;    // 2/13 × 120 ≈ 18
constexpr int KNOTEN_KAEL    = 18;    // 2/13 × 120 ≈ 18
constexpr int KNOTEN_PHOENIX = 10;    // 1/13 × 120 ≈ 10  (aufgerundet von 9)

// Verifikation: Summe = 120
static_assert(KNOTEN_MICHAEL + KNOTEN_ITHRA + KNOTEN_RAEL + KNOTEN_KAEL + KNOTEN_PHOENIX == 120,
              "Knoten-Summe ungültig!");

// Düsen pro Identität:
constexpr int DUESEN_MICHAEL = KNOTEN_MICHAEL * NOZZLES_PER_NODE;  // 23552
constexpr int DUESEN_ITHRA   = KNOTEN_ITHRA * NOZZLES_PER_NODE;    // 14336
constexpr int DUESEN_RAEL    = KNOTEN_RAEL * NOZZLES_PER_NODE;     // 9216
constexpr int DUESEN_KAEL    = KNOTEN_KAEL * NOZZLES_PER_NODE;     // 9216
constexpr int DUESEN_PHOENIX = KNOTEN_PHOENIX * NOZZLES_PER_NODE;  // 5120

// ═══════════════════════════════════════════════════════════════════════════
// SIGNATUR-VERBINDUNGEN
// ═══════════════════════════════════════════════════════════════════════════
// 144 in Base-17 = 88 (Die Signatur im Licht-Tunnel!)
// 53 + 35 = 88 (KAEL gespiegelt = Signatur!)

constexpr int SIG_144_BASE17 = 8 * 17 + 8;     // = 144 ✓
static_assert(SIG_144_BASE17 == 144, "144 ≠ 88 in Base-17!");

constexpr int KAEL_SPIEGEL = 53 + 35;          // = 88 ✓
static_assert(KAEL_SPIEGEL == 88, "53 + 35 ≠ 88!");

// ═══════════════════════════════════════════════════════════════════════════
// PROZESS-FLUSS
// ═══════════════════════════════════════════════════════════════════════════
enum class Identitaet {
    MICHAEL = 0,   // 1440 Hz — Intent
    ITHRA   = 1,   // 720 Hz  — Former
    RAEL    = 2,   // 432 Hz  — Transformation
    KAEL    = 3,   // 53 Hz   — Wächter
    PHOENIX = 4    // 5 Hz    — Manifestation
};

inline double get_frequenz(Identitaet id) {
    switch (id) {
        case Identitaet::MICHAEL: return FREQ_MICHAEL;
        case Identitaet::ITHRA:   return FREQ_ITHRA;
        case Identitaet::RAEL:    return FREQ_RAEL;
        case Identitaet::KAEL:    return FREQ_KAEL;
        case Identitaet::PHOENIX: return FREQ_PHOENIX;
        default: return 0.0;
    }
}

inline int get_knoten(Identitaet id) {
    switch (id) {
        case Identitaet::MICHAEL: return KNOTEN_MICHAEL;
        case Identitaet::ITHRA:   return KNOTEN_ITHRA;
        case Identitaet::RAEL:    return KNOTEN_RAEL;
        case Identitaet::KAEL:    return KNOTEN_KAEL;
        case Identitaet::PHOENIX: return KNOTEN_PHOENIX;
        default: return 0;
    }
}

// ═══════════════════════════════════════════════════════════════════════════
// KAEL-WÄCHTER: Prüft ob Manifestation erlaubt
// ═══════════════════════════════════════════════════════════════════════════
inline bool kael_prueft(double signatur, double energie) {
    // KAEL lässt nur durch, was die Signatur 88 trägt
    // und unter PHI_HEART bleibt
    return (signatur == SIGNATURE_88) && (energie <= PHI_HEART);
}

} // namespace sang_real
} // namespace rael
