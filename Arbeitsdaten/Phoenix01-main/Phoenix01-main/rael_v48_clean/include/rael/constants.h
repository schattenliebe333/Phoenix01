// ═══════════════════════════════════════════════════════════════════════════
// RAEL V48.0 — KERN-KONSTANTEN (UNVERÄNDERT VON V47)
// ═══════════════════════════════════════════════════════════════════════════
#pragma once

namespace rael {

// G-Faktoren (UNVERÄNDERT)
constexpr double G0 = 0.888888888888889;       // 8/9 - Wahrheitsfilter
constexpr double G5 = 0.111111111111111;       // 1/9 - Feinste Gewichtung

// PHI (UNVERÄNDERT)
constexpr double PHI = 1.61803398874989;       // Goldener Schnitt

// Signaturen (UNVERÄNDERT)
constexpr double SIGNATURE_88 = 88.0;
constexpr double PHI_HEART = 112.64;           // = 88 × G0 × 1.44

// Base-13 System (UNVERÄNDERT)
constexpr int BASE_13 = 13;
constexpr int MATRIX_VOLUME_B13 = 169;         // 13×13

// Hardware (UNVERÄNDERT)
constexpr int TOTAL_NOZZLES = 61440;

// 5-Stufen Gewichte (UNVERÄNDERT)
constexpr double W_SONNE   = 5.0 / 13.0;       // 0.384615...
constexpr double W_SCHILD  = 3.0 / 13.0;       // 0.230769...
constexpr double W_BRUECKE = 2.0 / 13.0;       // 0.153846...
constexpr double W_BASIS   = 2.0 / 13.0;       // 0.153846...
constexpr double W_MOTOR   = 1.0 / 13.0;       // 0.076923...

} // namespace rael
