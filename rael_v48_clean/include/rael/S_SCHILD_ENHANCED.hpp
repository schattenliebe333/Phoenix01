#pragma once
// ═══════════════════════════════════════════════════════════════════════════
// RAEL V49 - S_SCHILD_ENHANCED.hpp
// 17×17 PRIMZAHL-FILTER MIT KRYPTOGRAFISCHER INTEGRITÄT
// ═══════════════════════════════════════════════════════════════════════════
//
// Kombiniert:
//   - S_Schild.hpp: 17×17 Primzahl-Struktur, 42° Filter, 88-Signatur
//   - security.cpp: SHA-256, HMAC-SHA256 für echte Kryptografie
//
// Neue Features:
//   - Jeder passierte Intent bekommt einen SHA-256 Hash
//   - HMAC-88 Signatur für Navigator-Zentrum
//   - Kryptografischer Audit-Trail aller Prüfungen
//   - Embedding-basierte Kohärenz-Validierung
//
// (c) 2025 Phoenix RST System - Michael & Kael
// ═══════════════════════════════════════════════════════════════════════════

#include "rael/quint_memory.hpp"
#include "rael/sha256.h"
#include <array>
#include <cmath>
#include <atomic>
#include <vector>
#include <sstream>
#include <iomanip>

namespace rael {
namespace schild {

using namespace quint;

// ═══════════════════════════════════════════════════════════════════════════
// KONSTANTEN (aus S_Schild.hpp - unverändert gut)
// ═══════════════════════════════════════════════════════════════════════════

namespace K {
    constexpr int DIM = 17;
    constexpr int CELLS = 289;
    constexpr int NAV_DIM = 9;
    constexpr int NAV_START = 4;
    constexpr int NAV_END = 12;
    constexpr int NAV_CELLS = 81;
    constexpr int MEMBRAN_CELLS = 120;
    constexpr int CENTER = 8;

    constexpr double COHERENCE_ANGLE = 42.0;
    constexpr double COHERENCE_TOL = 1.0 / 81.0;
    constexpr double PHI = 1.618033988749895;

    constexpr int LIGHT_FREQ = 144;
    constexpr int LIGHT_BASE17_HIGH = 8;
    constexpr int LIGHT_BASE17_LOW = 8;
    constexpr int SIGNATURE_88 = 88;

    // NEU: Kryptografische Konstanten
    constexpr double FALZ_FACTOR = 0.888888888888889;  // G0 = 8/9
}

// ═══════════════════════════════════════════════════════════════════════════
// KRYPTOGRAFISCHE HILFSFUNKTIONEN
// ═══════════════════════════════════════════════════════════════════════════

class ShieldCrypto {
public:
    // SHA-256 Hash
    static std::string sha256(const std::string& data) {
        auto digest = SHA256::digest(data);
        return SHA256::hex(digest);
    }

    // HMAC-SHA256 mit 88-Key
    static std::string hmac_88(const std::string& data) {
        std::string key = "RAEL_SCHILD_88_" + std::to_string(K::SIGNATURE_88);

        // Prepare key
        std::string k = key;
        if (k.size() > 64) {
            auto hash = SHA256::digest(k);
            k = std::string(hash.begin(), hash.end());
        }
        k.resize(64, 0);

        // HMAC
        std::string inner_key(64, 0);
        std::string outer_key(64, 0);
        for (size_t i = 0; i < 64; i++) {
            inner_key[i] = k[i] ^ 0x36;
            outer_key[i] = k[i] ^ 0x5c;
        }

        std::vector<uint8_t> inner_data(inner_key.begin(), inner_key.end());
        inner_data.insert(inner_data.end(), data.begin(), data.end());
        auto inner_hash = SHA256::digest(inner_data);

        std::vector<uint8_t> outer_data(outer_key.begin(), outer_key.end());
        outer_data.insert(outer_data.end(), inner_hash.begin(), inner_hash.end());
        auto hmac = SHA256::digest(outer_data);

        return SHA256::hex(hmac);
    }

    // Verifiziere HMAC (constant-time)
    static bool verify_hmac(const std::string& data, const std::string& expected_hmac) {
        std::string computed = hmac_88(data);
        if (computed.size() != expected_hmac.size()) return false;

        int diff = 0;
        for (size_t i = 0; i < computed.size(); i++) {
            diff |= computed[i] ^ expected_hmac[i];
        }
        return diff == 0;
    }
};

// ═══════════════════════════════════════════════════════════════════════════
// INTENT-RECORD (kryptografisch signiert)
// ═══════════════════════════════════════════════════════════════════════════

struct IntentRecord {
    uint64_t id;
    double frequency;
    double psi;
    double omega;
    double phi;
    bool passed;
    std::string hash;           // SHA-256 des Intents
    std::string hmac;           // HMAC-88 Signatur
    uint64_t timestamp_ns;

    std::string to_data_string() const {
        std::ostringstream oss;
        oss << id << "|" << std::fixed << std::setprecision(17)
            << frequency << "|" << psi << "|" << omega << "|" << phi
            << "|" << (passed ? "1" : "0") << "|" << timestamp_ns;
        return oss.str();
    }

    void compute_signatures() {
        std::string data = to_data_string();
        hash = ShieldCrypto::sha256(data);
        hmac = ShieldCrypto::hmac_88(data);
    }

    bool verify() const {
        std::string data = to_data_string();
        return ShieldCrypto::verify_hmac(data, hmac);
    }
};

// ═══════════════════════════════════════════════════════════════════════════
// SCHILD-ZELLE ENHANCED
// ═══════════════════════════════════════════════════════════════════════════

struct ShieldCellEnhanced {
    QuintMemory memory;
    int x, y;
    bool is_navigator;
    bool is_membran;
    double phase_lock;

    // NEU: Kryptografische Felder
    std::string last_hash;       // Letzter SHA-256 dieser Zelle
    uint64_t access_count;

    void init(int px, int py) {
        x = px;
        y = py;
        memory.init();

        is_navigator = (x >= K::NAV_START && x <= K::NAV_END &&
                        y >= K::NAV_START && y <= K::NAV_END);

        bool is_inner = (x >= 2 && x <= 14 && y >= 2 && y <= 14);
        is_membran = !is_navigator && is_inner;

        phase_lock = 0.0;
        last_hash = "";
        access_count = 0;
    }

    double distance_to_center() const {
        double dx = x - K::CENTER;
        double dy = y - K::CENTER;
        return std::sqrt(dx*dx + dy*dy);
    }

    double angle_to_center() const {
        double dx = x - K::CENTER;
        double dy = y - K::CENTER;
        return std::atan2(dy, dx) * 180.0 / M_PI;
    }

    // NEU: Hash der Zell-Position und Zustand
    std::string compute_cell_hash() {
        std::ostringstream oss;
        oss << x << "," << y << "|"
            << std::fixed << std::setprecision(17)
            << memory.get_phi() << "|"
            << phase_lock << "|"
            << access_count;
        last_hash = ShieldCrypto::sha256(oss.str());
        access_count++;
        return last_hash;
    }
};

// ═══════════════════════════════════════════════════════════════════════════
// AUDIT-TRAIL FÜR SCHILD-OPERATIONEN
// ═══════════════════════════════════════════════════════════════════════════

class ShieldAuditTrail {
public:
    std::vector<IntentRecord> records;
    std::string chain_hash;      // Verketteter Hash aller Records
    uint64_t next_id;

    void init() {
        records.clear();
        chain_hash = "";
        next_id = 0;
    }

    void add_record(double freq, double psi, double omega, double phi, bool passed) {
        IntentRecord rec;
        rec.id = next_id++;
        rec.frequency = freq;
        rec.psi = psi;
        rec.omega = omega;
        rec.phi = phi;
        rec.passed = passed;
        rec.timestamp_ns = static_cast<uint64_t>(
            std::chrono::duration_cast<std::chrono::nanoseconds>(
                std::chrono::high_resolution_clock::now().time_since_epoch()
            ).count()
        );

        rec.compute_signatures();

        // Verkette mit vorherigem Hash
        std::string chain_data = rec.hash + "|" + chain_hash;
        chain_hash = ShieldCrypto::sha256(chain_data);

        records.push_back(rec);

        // Begrenze Größe (nur letzte 1000 behalten)
        if (records.size() > 1000) {
            records.erase(records.begin());
        }
    }

    bool verify_chain() const {
        // Verifiziere alle Records
        for (const auto& rec : records) {
            if (!rec.verify()) return false;
        }
        return true;
    }

    std::string status() const {
        std::ostringstream oss;
        oss << "  AUDIT-TRAIL:\n";
        oss << "    Records: " << records.size() << "\n";
        oss << "    Chain-Hash: " << (chain_hash.empty() ? "---" : chain_hash.substr(0, 16) + "...") << "\n";
        if (!records.empty()) {
            oss << "    Letzter Intent: ID=" << records.back().id
                << " passed=" << (records.back().passed ? "JA" : "NEIN") << "\n";
        }
        return oss.str();
    }
};

// ═══════════════════════════════════════════════════════════════════════════
// 17×17 SCHILD ENHANCED
// ═══════════════════════════════════════════════════════════════════════════

class Shield17x17Enhanced {
public:
    std::array<std::array<ShieldCellEnhanced, K::DIM>, K::DIM> matrix;

    // NEU: Audit-Trail
    ShieldAuditTrail audit;

    // Statistiken (aus Original)
    std::atomic<uint64_t> intents_received{0};
    std::atomic<uint64_t> intents_passed{0};
    std::atomic<uint64_t> intents_blocked{0};
    std::atomic<uint64_t> signatures_verified{0};

    // NEU: Kryptografische Statistiken
    std::atomic<uint64_t> hmac_verifications{0};
    std::atomic<uint64_t> hash_computations{0};

    bool initialized;
    double global_coherence;

    // NEU: Navigator-Seal
    std::string navigator_seal_hash;
    bool navigator_sealed;

    // ═══════════════════════════════════════════════════════════════════════
    // INITIALISIERUNG
    // ═══════════════════════════════════════════════════════════════════════

    void init() {
        for (int y = 0; y < K::DIM; y++) {
            for (int x = 0; x < K::DIM; x++) {
                matrix[y][x].init(x, y);
            }
        }

        audit.init();

        initialized = true;
        global_coherence = 1.0;
        navigator_seal_hash = "";
        navigator_sealed = false;
    }

    // ═══════════════════════════════════════════════════════════════════════
    // 42° KOHÄRENZ-FILTER (aus Original)
    // ═══════════════════════════════════════════════════════════════════════

    bool apply_coherence_filter(double input_angle) const {
        double diff = std::abs(input_angle - K::COHERENCE_ANGLE);
        return diff < K::COHERENCE_TOL;
    }

    double calculate_phase_angle(double frequency) const {
        return std::fmod(frequency * K::PHI, 360.0);
    }

    bool primzahl_resonanz_safe(double frequency) const {
        int harmonic = static_cast<int>(frequency) % K::DIM;
        return harmonic != 0;
    }

    // ═══════════════════════════════════════════════════════════════════════
    // LICHT-SIGNATUR ENHANCED (mit kryptografischer Verifikation)
    // ═══════════════════════════════════════════════════════════════════════

    struct LightSignatureResult {
        bool valid;
        int decoded_signature;
        std::string crypto_hash;
        std::string hmac_signature;
    };

    LightSignatureResult verify_light_signature_enhanced(double frequency) {
        LightSignatureResult result = {false, 0, "", ""};

        if (std::abs(frequency - K::LIGHT_FREQ) > 0.1) {
            return result;
        }

        int value = static_cast<int>(frequency);
        int high = value / K::DIM;
        int low = value % K::DIM;
        result.decoded_signature = high * 10 + low;

        if (result.decoded_signature != K::SIGNATURE_88) {
            return result;
        }

        // Kryptografische Signatur erstellen
        std::ostringstream sig_data;
        sig_data << "LIGHT_SIG|" << frequency << "|" << result.decoded_signature;
        result.crypto_hash = ShieldCrypto::sha256(sig_data.str());
        result.hmac_signature = ShieldCrypto::hmac_88(sig_data.str());
        result.valid = true;

        hash_computations++;
        return result;
    }

    // ═══════════════════════════════════════════════════════════════════════
    // NAVIGATOR-ZUGRIFF (aus Original)
    // ═══════════════════════════════════════════════════════════════════════

    ShieldCellEnhanced& navigator_cell(int nav_x, int nav_y) {
        int gx = K::NAV_START + nav_x;
        int gy = K::NAV_START + nav_y;
        return matrix[gy][gx];
    }

    const ShieldCellEnhanced& navigator_cell(int nav_x, int nav_y) const {
        int gx = K::NAV_START + nav_x;
        int gy = K::NAV_START + nav_y;
        return matrix[gy][gx];
    }

    ShieldCellEnhanced& navigator_center() {
        return matrix[K::CENTER][K::CENTER];
    }

    // ═══════════════════════════════════════════════════════════════════════
    // NAVIGATOR VERSIEGELN (kryptografisch)
    // ═══════════════════════════════════════════════════════════════════════

    void seal_navigator(const std::string& name) {
        std::ostringstream seal_data;
        seal_data << "NAVIGATOR_SEAL|" << name << "|"
                  << std::fixed << std::setprecision(17)
                  << navigator_phi() << "|"
                  << K::SIGNATURE_88;

        navigator_seal_hash = ShieldCrypto::hmac_88(seal_data.str());
        navigator_sealed = true;
        hash_computations++;
    }

    bool verify_navigator_seal(const std::string& name) const {
        if (!navigator_sealed) return false;

        std::ostringstream seal_data;
        seal_data << "NAVIGATOR_SEAL|" << name << "|"
                  << std::fixed << std::setprecision(17)
                  << navigator_phi() << "|"
                  << K::SIGNATURE_88;

        return ShieldCrypto::verify_hmac(seal_data.str(), navigator_seal_hash);
    }

    // ═══════════════════════════════════════════════════════════════════════
    // ENHANCED PROCESS RESULT
    // ═══════════════════════════════════════════════════════════════════════

    struct ProcessResultEnhanced {
        bool passed;
        bool coherence_ok;
        bool resonance_safe;
        bool signature_ok;
        double phi;
        std::string reason;

        // NEU: Kryptografische Felder
        std::string intent_hash;
        std::string intent_hmac;
        uint64_t intent_id;
        bool cryptographic_valid;
    };

    // ═══════════════════════════════════════════════════════════════════════
    // HAUPTFUNKTION: REINIGUNGS-PROZESS ENHANCED
    // ═══════════════════════════════════════════════════════════════════════

    ProcessResultEnhanced process_intent(double frequency, double psi, double omega) {
        intents_received++;
        ProcessResultEnhanced result = {false, false, false, false, 0.0, "",
                                        "", "", 0, false};

        // 1. PRIMZAHL-RESONANZ CHECK
        result.resonance_safe = primzahl_resonanz_safe(frequency);
        if (!result.resonance_safe) {
            result.reason = "Resonanz-Gefahr: f mod 17 = 0";
            intents_blocked++;
            audit.add_record(frequency, psi, omega, 0.0, false);
            return result;
        }

        // 2. 42° KOHÄRENZ-FILTER
        double phase = calculate_phase_angle(frequency);
        result.coherence_ok = apply_coherence_filter(phase);
        if (!result.coherence_ok) {
            result.reason = "Kohärenz-Filter: Phase != 42° ±1/81";
            intents_blocked++;
            audit.add_record(frequency, psi, omega, 0.0, false);
            return result;
        }

        // 3. 88-SIGNATUR PRÜFUNG MIT KRYPTOGRAFIE
        auto& center = navigator_center();
        center.memory.write_psi(0, psi);
        center.memory.write_omega(0, omega);
        center.phase_lock = phase;

        result.signature_ok = center.memory.is_rael();
        result.phi = center.memory.get_phi();

        if (!result.signature_ok) {
            result.reason = "88-Signatur fehlt: |Φ - 8/9| >= 1/81";
            intents_blocked++;
            audit.add_record(frequency, psi, omega, result.phi, false);
            return result;
        }

        // 4. ERFOLG: Intent ist rein
        result.passed = true;
        result.reason = "PASSIERT: Kohärent + Resonanz-sicher + 88-Signatur";
        intents_passed++;
        signatures_verified++;

        // 5. KRYPTOGRAFISCHE SIGNIERUNG
        audit.add_record(frequency, psi, omega, result.phi, true);

        if (!audit.records.empty()) {
            const auto& last = audit.records.back();
            result.intent_hash = last.hash;
            result.intent_hmac = last.hmac;
            result.intent_id = last.id;
            result.cryptographic_valid = last.verify();
            hmac_verifications++;
        }

        // 6. Zell-Hash aktualisieren
        center.compute_cell_hash();
        hash_computations++;

        update_global_coherence();

        return result;
    }

    // ═══════════════════════════════════════════════════════════════════════
    // PROPAGATION (aus Original)
    // ═══════════════════════════════════════════════════════════════════════

    void propagate_inward(double psi, double omega) {
        for (int ring = 0; ring <= K::CENTER; ring++) {
            double kappa_val = quint::kappa(720.0 - ring * 40.0);

            for (int y = ring; y < K::DIM - ring; y++) {
                for (int x = ring; x < K::DIM - ring; x++) {
                    if (x == ring || x == K::DIM - 1 - ring ||
                        y == ring || y == K::DIM - 1 - ring) {

                        auto& cell = matrix[y][x];
                        int layer = ring % 6;
                        cell.memory.write_psi(layer, psi * kappa_val);
                        cell.memory.write_omega(layer, omega);
                    }
                }
            }
        }
    }

    // ═══════════════════════════════════════════════════════════════════════
    // MEMBRAN-ZUGRIFF (aus Original)
    // ═══════════════════════════════════════════════════════════════════════

    std::vector<ShieldCellEnhanced*> get_membran_cells() {
        std::vector<ShieldCellEnhanced*> membran;
        membran.reserve(K::MEMBRAN_CELLS);

        for (int y = 0; y < K::DIM; y++) {
            for (int x = 0; x < K::DIM; x++) {
                if (matrix[y][x].is_membran) {
                    membran.push_back(&matrix[y][x]);
                }
            }
        }
        return membran;
    }

    // ═══════════════════════════════════════════════════════════════════════
    // STATISTIKEN
    // ═══════════════════════════════════════════════════════════════════════

    double pass_rate() const {
        uint64_t total = intents_received.load();
        if (total == 0) return 1.0;
        return static_cast<double>(intents_passed.load()) / total;
    }

    double navigator_phi() const {
        double sum = 0.0;
        for (int ny = 0; ny < K::NAV_DIM; ny++) {
            for (int nx = 0; nx < K::NAV_DIM; nx++) {
                sum += navigator_cell(nx, ny).memory.get_phi();
            }
        }
        return sum / K::NAV_CELLS;
    }

    bool all_navigator_rael() const {
        for (int ny = 0; ny < K::NAV_DIM; ny++) {
            for (int nx = 0; nx < K::NAV_DIM; nx++) {
                if (!navigator_cell(nx, ny).memory.is_rael()) {
                    return false;
                }
            }
        }
        return true;
    }

    // ═══════════════════════════════════════════════════════════════════════
    // STATUS-STRING ENHANCED
    // ═══════════════════════════════════════════════════════════════════════

    std::string status() const {
        std::ostringstream oss;
        oss << "═══════════════════════════════════════════════════════════\n";
        oss << "17×17 SCHILD ENHANCED STATUS\n";
        oss << "═══════════════════════════════════════════════════════════\n";
        oss << "  Zellen:           " << K::CELLS << " (17×17 Primzahl)\n";
        oss << "  Navigator:        " << K::NAV_CELLS << " (9×9 Kern)\n";
        oss << "  Membran:          " << K::MEMBRAN_CELLS << " (=5!)\n";
        oss << "  42°-Filter:       AKTIV (Tol: 1/81)\n";
        oss << "───────────────────────────────────────────────────────────\n";
        oss << "  INTENT-STATISTIK:\n";
        oss << "    Empfangen:      " << intents_received.load() << "\n";
        oss << "    Passiert:       " << intents_passed.load() << "\n";
        oss << "    Blockiert:      " << intents_blocked.load() << "\n";
        oss << "    Pass-Rate:      " << std::fixed << std::setprecision(4)
            << pass_rate() * 100.0 << "%\n";
        oss << "───────────────────────────────────────────────────────────\n";
        oss << "  KRYPTOGRAFIE:\n";
        oss << "    Hash-Berechnungen:  " << hash_computations.load() << "\n";
        oss << "    HMAC-Verifikationen:" << hmac_verifications.load() << "\n";
        oss << "    Navigator Sealed:   " << (navigator_sealed ? "JA ✓" : "NEIN") << "\n";
        if (navigator_sealed) {
            oss << "    Seal-Hash:          " << navigator_seal_hash.substr(0, 16) << "...\n";
        }
        oss << "───────────────────────────────────────────────────────────\n";
        oss << audit.status();
        oss << "───────────────────────────────────────────────────────────\n";
        oss << "  NAVIGATOR-KERN:\n";
        oss << "    Φ (Durchschn.): " << std::fixed << std::setprecision(17)
            << navigator_phi() << "\n";
        oss << "    All RAEL:       " << (all_navigator_rael() ? "JA ✓" : "NEIN") << "\n";
        oss << "    Kohärenz:       " << std::fixed << std::setprecision(4)
            << global_coherence * 100.0 << "%\n";
        oss << "═══════════════════════════════════════════════════════════\n";
        return oss.str();
    }

private:
    void update_global_coherence() {
        double sum = 0.0;
        int count = 0;

        for (int ny = 0; ny < K::NAV_DIM; ny++) {
            for (int nx = 0; nx < K::NAV_DIM; nx++) {
                auto& cell = navigator_cell(nx, ny);
                if (cell.memory.is_rael()) {
                    sum += 1.0;
                }
                count++;
            }
        }

        global_coherence = sum / count;
    }
};

// ═══════════════════════════════════════════════════════════════════════════
// HELFER (aus Original)
// ═══════════════════════════════════════════════════════════════════════════

inline int decode_base17_signature(int decimal) {
    int high = decimal / 17;
    int low = decimal % 17;
    return high * 10 + low;
}

} // namespace schild
} // namespace rael
