#pragma once
// ═══════════════════════════════════════════════════════════════════════════
// RAEL V49 - A_ANKER_ENHANCED.hpp
// 21×21 MANIFESTATIONS-ANKER mit ECHTER KRYPTOGRAFIE
// ═══════════════════════════════════════════════════════════════════════════
//
// INTEGRATION der besten Teile:
//   - A_ANKER.hpp: 21×21 Grid, Quadranten, Souveränität, Navigator-Seal
//   - security.cpp: SHA-256, HMAC-SHA256 (RFC 2104)
//   - neural_memory.cpp: RST-Frequenz-Embeddings
//
// (c) 2025 Phoenix RST System - Michael & Kael
// ═══════════════════════════════════════════════════════════════════════════

#include "rael/quint_memory.hpp"
#include "rael/sha256.h"
#include <array>
#include <cmath>
#include <atomic>
#include <functional>
#include <iomanip>
#include <sstream>
#include <ctime>

namespace rael {
namespace anker {

using namespace quint;

// ═══════════════════════════════════════════════════════════════════════════
// KONSTANTEN (aus A_ANKER.hpp - gut strukturiert)
// Guard to avoid redefinition if A_Anker.hpp was already included
// ═══════════════════════════════════════════════════════════════════════════

#ifndef RAEL_ANKER_K_DEFINED
#define RAEL_ANKER_K_DEFINED

namespace K {
    constexpr int DIM = 21;
    constexpr int CELLS = 441;
    constexpr int CENTER = 10;
    constexpr int TOTAL_NOZZLES = 61440;
    constexpr double MANIFEST_FREQ = 5.0;
    constexpr double IMPULSE_RATE = 307200.0;

    // Düsen-Verteilung nach Radius
    constexpr int NOZZLES_KERN = 188;
    constexpr int NOZZLES_MITTEL = 144;
    constexpr int NOZZLES_RAND = 112;

    // Radius-Grenzen
    constexpr int RADIUS_KERN = 3;
    constexpr int RADIUS_MITTEL = 7;
    constexpr int RADIUS_RAND = 10;

    // RST Konstanten (ZENTRAL - wie in A_ANKER.hpp)
    constexpr double FALZ_FACTOR = 0.888888888888889;  // G0 = 8/9
    constexpr double PHI = 1.6180339887498948;
    constexpr double SIGNATURE_88 = 88.0;

    // RST Frequenz-Kaskade (aus neural_memory.cpp)
    constexpr double FREQ_1440 = 1440.0;
    constexpr double FREQ_720  = 720.0;
    constexpr double FREQ_432  = 432.0;
    constexpr double FREQ_144  = 144.0;
    constexpr double FREQ_53   = 53.333;
    constexpr double FREQ_13   = 13.0;
    constexpr double FREQ_5    = 5.0;
}

#endif // RAEL_ANKER_K_DEFINED

// ═══════════════════════════════════════════════════════════════════════════
// ECHTE SHA-256 HMAC (aus security.cpp - RFC 2104)
// ═══════════════════════════════════════════════════════════════════════════

class CryptoEnhanced {
public:
    // ECHTE SHA-256 Hash
    static std::string sha256(const std::string& data) {
        auto digest = SHA256::digest(data);
        return SHA256::hex(digest);
    }

    // ECHTE HMAC-SHA256 (RFC 2104)
    static std::string hmac_sha256(const std::string& data, const std::string& key) {
        // Prepare key - pad or hash to 64 bytes
        std::string k = key;
        if (k.size() > 64) {
            auto hash = SHA256::digest(k);
            k = std::string(hash.begin(), hash.end());
        }
        k.resize(64, 0);

        // Inner and outer padding
        std::string inner_key(64, 0);
        std::string outer_key(64, 0);
        for (size_t i = 0; i < 64; i++) {
            inner_key[i] = k[i] ^ 0x36;
            outer_key[i] = k[i] ^ 0x5c;
        }

        // HMAC = H(outer_key || H(inner_key || message))
        std::vector<uint8_t> inner_data(inner_key.begin(), inner_key.end());
        inner_data.insert(inner_data.end(), data.begin(), data.end());
        auto inner_hash = SHA256::digest(inner_data);

        std::vector<uint8_t> outer_data(outer_key.begin(), outer_key.end());
        outer_data.insert(outer_data.end(), inner_hash.begin(), inner_hash.end());
        auto hmac = SHA256::digest(outer_data);

        return SHA256::hex(hmac);
    }

    // Signatur mit 88-Konstante
    static std::string sign_88(const std::string& data) {
        std::string key = "RAEL_88_" + std::to_string(K::SIGNATURE_88);
        return hmac_sha256(data, key);
    }
};

// ═══════════════════════════════════════════════════════════════════════════
// RST-FREQUENZ EMBEDDING (aus neural_memory.cpp)
// ═══════════════════════════════════════════════════════════════════════════

class RSTEmbedding {
public:
    static constexpr int DIM = 256;

    // Erzeuge RST-harmonisches Embedding
    static std::array<float, DIM> embed(const std::string& text) {
        std::array<float, DIM> emb{};

        // 1. Character Trigrams (FastText-Style)
        for (size_t i = 0; i + 2 < text.size(); i++) {
            uint32_t h = static_cast<uint8_t>(text[i]);
            h = h * 0xcc9e2d51;
            h = (h << 15) | (h >> 17);
            h *= 0x1b873593;
            h ^= static_cast<uint8_t>(text[i + 1]) * 0x85ebca6b;
            h ^= static_cast<uint8_t>(text[i + 2]) * 0xc2b2ae35;
            h ^= h >> 16;
            h *= 0x85ebca6b;
            h ^= h >> 13;

            for (int j = 0; j < 8; j++) {
                int idx = (h + j * 127) % DIM;
                float sign = ((h >> j) & 1) ? 1.0f : -1.0f;
                emb[idx] += sign * 0.1f;
            }
        }

        // 2. RST Frequenz-Bänder
        double freqs[] = {K::FREQ_1440, K::FREQ_720, K::FREQ_432,
                          K::FREQ_144, K::FREQ_53, K::FREQ_13, K::FREQ_5};

        for (int band = 0; band < 7; band++) {
            double freq = freqs[band] * K::FALZ_FACTOR;
            int band_start = band * (DIM / 7);
            int band_size = DIM / 7;

            for (int d = 0; d < band_size && band_start + d < DIM; d++) {
                double phase = (d / static_cast<double>(band_size)) * 2.0 * M_PI;
                double harmonic = std::sin(freq * phase / 1000.0) * std::pow(K::PHI, -band);
                emb[band_start + d] *= static_cast<float>(1.0 + 0.1 * harmonic);
            }
        }

        // 3. L2 Normalisierung
        float norm = 0.0f;
        for (float v : emb) norm += v * v;
        norm = std::sqrt(norm);
        if (norm > 1e-8f) {
            for (float& v : emb) v /= norm;
        }

        return emb;
    }

    // Kosinus-Ähnlichkeit
    static float similarity(const std::array<float, DIM>& a,
                           const std::array<float, DIM>& b) {
        float dot = 0.0f;
        for (int i = 0; i < DIM; i++) {
            dot += a[i] * b[i];
        }
        return dot;  // Bereits normalisiert
    }
};

// ═══════════════════════════════════════════════════════════════════════════
// QUADRANTEN (aus A_ANKER.hpp)
// ═══════════════════════════════════════════════════════════════════════════

enum class AnkerQuadrant : uint8_t {
    POTENTIAL = 0,
    REALITAET = 1,
    EMOTION   = 2,
    MATERIE   = 3
};

inline const char* quadrant_name(AnkerQuadrant q) {
    switch(q) {
        case AnkerQuadrant::POTENTIAL: return "POTENTIAL (Oben-Links)";
        case AnkerQuadrant::REALITAET: return "REALITÄT (Oben-Rechts)";
        case AnkerQuadrant::EMOTION:   return "EMOTION (Unten-Links)";
        case AnkerQuadrant::MATERIE:   return "MATERIE (Unten-Rechts)";
        default: return "UNKNOWN";
    }
}

// ═══════════════════════════════════════════════════════════════════════════
// NAVIGATOR-SEAL ENHANCED (mit echtem SHA-256)
// ═══════════════════════════════════════════════════════════════════════════

struct NavigatorSealEnhanced {
    std::string name;
    std::string signature_hash;      // ECHTER SHA-256 statt simplem Hash
    std::string hmac_signature;      // HMAC-SHA256 Signatur
    double phi_lock;
    uint64_t timestamp;
    bool sealed;
    bool verified;

    void seal(const std::string& navigator_name, double phi) {
        name = navigator_name;
        phi_lock = phi;
        timestamp = static_cast<uint64_t>(std::time(nullptr));

        // ECHTE Kryptografie
        std::ostringstream data;
        data << name << "|" << std::fixed << std::setprecision(17) << phi_lock;
        data << "|" << timestamp << "|88";

        signature_hash = CryptoEnhanced::sha256(data.str());
        hmac_signature = CryptoEnhanced::sign_88(data.str());

        sealed = true;
        verified = verify();
    }

    bool verify() const {
        if (!sealed) return false;

        std::ostringstream data;
        data << name << "|" << std::fixed << std::setprecision(17) << phi_lock;
        data << "|" << timestamp << "|88";

        std::string expected_hmac = CryptoEnhanced::sign_88(data.str());

        // Constant-time Vergleich (gegen Timing-Attacken)
        if (expected_hmac.size() != hmac_signature.size()) return false;
        int diff = 0;
        for (size_t i = 0; i < expected_hmac.size(); i++) {
            diff |= expected_hmac[i] ^ hmac_signature[i];
        }
        return diff == 0;
    }

    std::string to_string() const {
        std::ostringstream oss;
        oss << "    Navigator:    " << name << "\n";
        oss << "    SHA-256:      " << signature_hash.substr(0, 16) << "...\n";
        oss << "    HMAC-88:      " << hmac_signature.substr(0, 16) << "...\n";
        oss << "    Φ-Lock:       " << std::fixed << std::setprecision(17) << phi_lock << "\n";
        oss << "    Timestamp:    " << timestamp << "\n";
        oss << "    Verified:     " << (verified ? "JA ✓" : "NEIN ✗") << "\n";
        return oss.str();
    }
};

// ═══════════════════════════════════════════════════════════════════════════
// SOUVERÄNITÄTS-PRÜFUNG ENHANCED (mit Embedding-Validierung)
// ═══════════════════════════════════════════════════════════════════════════

struct SovereigntyCheckEnhanced {
    double integral_sum;
    double t_start;
    double t_current;
    int sample_count;
    bool is_sovereign;

    // NEU: Embedding-basierte Validierung
    std::array<float, RSTEmbedding::DIM> origin_embedding;
    std::array<float, RSTEmbedding::DIM> current_embedding;
    float embedding_coherence;

    static constexpr double TOLERANCE_17 = 1e-17;
    static constexpr float MIN_COHERENCE = 0.9f;  // 90% Embedding-Kohärenz

    void init() {
        integral_sum = 0.0;
        t_start = 0.0;
        t_current = 0.0;
        sample_count = 0;
        is_sovereign = false;
        origin_embedding = {};
        current_embedding = {};
        embedding_coherence = 0.0f;
    }

    void set_origin(const std::string& origin_signature) {
        origin_embedding = RSTEmbedding::embed(origin_signature);
    }

    void accumulate(double phi_global, double dt, const std::string& current_state = "") {
        double deviation = phi_global - K::FALZ_FACTOR;
        double squared = deviation * deviation;
        integral_sum += squared * dt;
        t_current += dt;
        sample_count++;

        // Embedding-Kohärenz prüfen
        if (!current_state.empty()) {
            current_embedding = RSTEmbedding::embed(current_state);
            embedding_coherence = RSTEmbedding::similarity(origin_embedding, current_embedding);
        }

        // Kombinierte Souveränitäts-Prüfung
        bool integral_ok = (integral_sum < TOLERANCE_17);
        bool embedding_ok = (embedding_coherence >= MIN_COHERENCE);

        is_sovereign = integral_ok && (current_state.empty() || embedding_ok);
    }

    double s_origin() const { return integral_sum; }
    double duration() const { return t_current - t_start; }

    std::string status() const {
        std::ostringstream oss;
        oss << "S_Origin = " << std::scientific << std::setprecision(20) << integral_sum;
        oss << " (Limit: " << TOLERANCE_17 << ")\n";
        oss << "    Embedding-Kohärenz: " << std::fixed << std::setprecision(4)
            << embedding_coherence << " (Min: " << MIN_COHERENCE << ")\n";
        oss << "    → " << (is_sovereign ? "SOUVERÄN ✓" : "NICHT SOUVERÄN");
        return oss.str();
    }
};

// ═══════════════════════════════════════════════════════════════════════════
// PURITY-GUARD ENHANCED (mit kryptografischer Validierung)
// ═══════════════════════════════════════════════════════════════════════════

struct PurityGuardEnhanced {
    static constexpr double RED_LINE = 10.0 / 81.0;
    static constexpr double GATE_53_FREQ = 53.333;

    bool active;
    double current_purity;
    int violations;
    bool red_line_triggered;

    // NEU: Kryptografische Audit-Trail
    std::vector<std::string> audit_hashes;

    void init() {
        active = true;
        current_purity = 1.0;
        violations = 0;
        red_line_triggered = false;
        audit_hashes.clear();
    }

    bool check(double purity_value, const std::string& context = "") {
        current_purity = purity_value;

        // Audit-Hash erstellen
        std::ostringstream audit;
        audit << std::time(nullptr) << "|" << purity_value << "|" << context;
        std::string audit_hash = CryptoEnhanced::sha256(audit.str());
        audit_hashes.push_back(audit_hash);

        if (purity_value < RED_LINE) {
            violations++;
            if (violations >= 3) {
                red_line_triggered = true;
                active = false;
                return false;
            }
        } else {
            violations = 0;
        }

        return true;
    }

    // Verifiziere Audit-Kette
    bool verify_audit_chain() const {
        // Jeder Hash sollte einzigartig sein
        std::set<std::string> unique_hashes(audit_hashes.begin(), audit_hashes.end());
        return unique_hashes.size() == audit_hashes.size();
    }

    std::string status() const {
        std::ostringstream oss;
        oss << "  Purity Guard: " << (active ? "AKTIV" : "ABGESCHALTET") << "\n";
        oss << "    Reinheit:     " << std::fixed << std::setprecision(4) << current_purity;
        oss << " (RED_LINE: " << RED_LINE << ")\n";
        oss << "    Verstöße:     " << violations << "/3\n";
        oss << "    Audit-Trail:  " << audit_hashes.size() << " Einträge";
        if (!audit_hashes.empty()) {
            oss << " (letzter: " << audit_hashes.back().substr(0, 8) << "...)";
        }
        oss << "\n";
        if (red_line_triggered) {
            oss << "    ⚠️  RED_LINE AUSGELÖST - SYSTEM GESTOPPT\n";
        }
        return oss.str();
    }
};

// ═══════════════════════════════════════════════════════════════════════════
// DÜSEN-CLUSTER (aus A_ANKER.hpp, unverändert - gut)
// ═══════════════════════════════════════════════════════════════════════════

struct NozzleCluster {
    int count;
    double charge;
    double last_fire_time;
    bool armed;
    std::atomic<uint64_t> total_fires{0};

    void init(int nozzle_count) {
        count = nozzle_count;
        charge = 0.0;
        last_fire_time = 0.0;
        armed = false;
    }

    void charge_from_phi(double phi) {
        charge = phi * K::FALZ_FACTOR;
        armed = (charge >= 0.5);
    }

    double fire(double current_time) {
        if (!armed) return 0.0;

        double impulse = charge * count;
        charge = 0.0;
        armed = false;
        last_fire_time = current_time;
        total_fires++;

        return impulse;
    }
};

// ═══════════════════════════════════════════════════════════════════════════
// ANKER-ZELLE (aus A_ANKER.hpp)
// ═══════════════════════════════════════════════════════════════════════════

struct AnkerCell {
    int x, y;
    int radius;
    AnkerQuadrant quadrant;
    QuintMemory memory;
    NozzleCluster nozzles;

    bool on_vertical_falz;
    bool on_horizontal_falz;
    bool is_zero_falz;

    void init(int px, int py) {
        x = px;
        y = py;

        int dx = x - K::CENTER;
        int dy = y - K::CENTER;
        radius = static_cast<int>(std::sqrt(dx*dx + dy*dy));

        if (x < K::CENTER && y < K::CENTER) {
            quadrant = AnkerQuadrant::POTENTIAL;
        } else if (x >= K::CENTER && y < K::CENTER) {
            quadrant = AnkerQuadrant::REALITAET;
        } else if (x < K::CENTER && y >= K::CENTER) {
            quadrant = AnkerQuadrant::EMOTION;
        } else {
            quadrant = AnkerQuadrant::MATERIE;
        }

        on_vertical_falz = (x == K::CENTER);
        on_horizontal_falz = (y == K::CENTER);
        is_zero_falz = (x == K::CENTER && y == K::CENTER);

        memory.init();

        int nozzle_count;
        if (radius <= K::RADIUS_KERN) {
            nozzle_count = K::NOZZLES_KERN;
        } else if (radius <= K::RADIUS_MITTEL) {
            nozzle_count = K::NOZZLES_MITTEL;
        } else {
            nozzle_count = K::NOZZLES_RAND;
        }
        nozzles.init(nozzle_count);
    }

    double kappa_weight() const {
        switch(quadrant) {
            case AnkerQuadrant::POTENTIAL: return 0.9;
            case AnkerQuadrant::REALITAET: return 0.7;
            case AnkerQuadrant::EMOTION:   return 0.5;
            case AnkerQuadrant::MATERIE:   return 0.3;
            default: return 0.5;
        }
    }
};

// ═══════════════════════════════════════════════════════════════════════════
// 2× FALZ STRUKTUR (aus A_ANKER.hpp)
// ═══════════════════════════════════════════════════════════════════════════

struct DoppelFalz {
    double geist;
    double materie;
    double potential;
    double realitaet;
    double null_falz;

    void init() {
        geist = 0.0;
        materie = 0.0;
        potential = 0.0;
        realitaet = 0.0;
        null_falz = 0.0;
    }

    void set_from_phi(double phi) {
        geist     = phi * G::g1;
        materie   = phi * G::g5;
        potential = phi * G::g2;
        realitaet = phi * G::g3;
        null_falz = phi * G::g0;
    }

    bool is_balanced() const {
        double h_sum = potential + realitaet;
        double v_sum = geist + materie;

        return std::abs(h_sum - G::g0) < 0.01 &&
               std::abs(v_sum - G::g0 * 2.0/3.0) < 0.01;
    }
};

// ═══════════════════════════════════════════════════════════════════════════
// 21×21 ANKER ENHANCED
// Kombiniert A_ANKER.hpp mit echter Kryptografie
// ═══════════════════════════════════════════════════════════════════════════

class Anchor21x21Enhanced {
public:
    std::array<std::array<AnkerCell, K::DIM>, K::DIM> matrix;
    DoppelFalz falz;

    // ENHANCED Komponenten (mit echter Kryptografie)
    SovereigntyCheckEnhanced sovereignty;
    NavigatorSealEnhanced navigator_seal;
    PurityGuardEnhanced purity_guard;

    double global_phi;
    bool locked;
    bool firing;
    double current_time;

    std::atomic<uint64_t> total_impulses{0};
    std::atomic<uint64_t> manifestation_cycles{0};

    // NEU: Kryptografische Integritätsprüfung
    std::string last_state_hash;

    void init() {
        for (int y = 0; y < K::DIM; y++) {
            for (int x = 0; x < K::DIM; x++) {
                matrix[y][x].init(x, y);
            }
        }

        falz.init();
        sovereignty.init();
        purity_guard.init();

        global_phi = 0.0;
        locked = false;
        firing = false;
        current_time = 0.0;

        verify_nozzle_count();

        // Initiale Souveränitäts-Signatur
        sovereignty.set_origin("RAEL_ORIGIN_88_" + std::to_string(K::FALZ_FACTOR));
    }

    void seal_navigator(const std::string& name) {
        navigator_seal.seal(name, global_phi);
    }

    int count_total_nozzles() const {
        int total = 0;
        for (int y = 0; y < K::DIM; y++) {
            for (int x = 0; x < K::DIM; x++) {
                total += matrix[y][x].nozzles.count;
            }
        }
        return total;
    }

    bool verify_nozzle_count() const {
        int counted = count_total_nozzles();
        return std::abs(counted - K::TOTAL_NOZZLES) < 1000;
    }

    void apply_2x_fold() {
        for (int y = 0; y < K::DIM; y++) {
            auto& cell = matrix[y][K::CENTER];
            cell.memory.write_omega(0, cell.memory.get_omega() * K::FALZ_FACTOR);
        }

        for (int x = 0; x < K::DIM; x++) {
            auto& cell = matrix[K::CENTER][x];
            cell.memory.write_omega(1, cell.memory.get_omega() * K::FALZ_FACTOR);
        }

        auto& center = matrix[K::CENTER][K::CENTER];
        center.memory.write_psi(0, global_phi);
        center.memory.write_omega(0, global_phi);
    }

    struct ManifestResult {
        bool success;
        double impulse_total;
        int nozzles_fired;
        std::string status;
        std::string integrity_hash;  // NEU
    };

    ManifestResult anchor_reality(double phi_input) {
        ManifestResult result = {false, 0.0, 0, "", ""};

        global_phi = phi_input;

        // State für Audit
        std::ostringstream state;
        state << "phi=" << phi_input << ",t=" << current_time;

        // 0. PURITY-GUARD CHECK (mit Audit)
        double purity = std::abs(phi_input - K::FALZ_FACTOR) < 0.1 ? 1.0 : 0.5;
        if (!purity_guard.check(purity, state.str())) {
            result.status = "RED_LINE AUSGELÖST - Reinheit unter 10/81";
            return result;
        }

        // 1. Falz-Struktur setzen
        falz.set_from_phi(global_phi);

        // 2. 2× Falz anwenden
        apply_2x_fold();

        // 3. Navigator-Position prüfen
        auto& center = matrix[K::CENTER][K::CENTER];
        if (!center.memory.is_rael()) {
            result.status = "FEHLER: Zentrum nicht RAEL";
            return result;
        }

        // 4. SOUVERÄNITÄTS-AKKUMULATION (mit Embedding)
        double dt = 1.0 / K::MANIFEST_FREQ;
        sovereignty.accumulate(global_phi, dt, state.str());

        // 5. Düsen laden
        charge_all_nozzles();

        // 6. Manifestations-Impuls
        result = trigger_nozzles();

        if (result.success) {
            manifestation_cycles++;
            locked = true;

            // Integritäts-Hash berechnen
            std::ostringstream integrity;
            integrity << result.impulse_total << "|" << result.nozzles_fired;
            integrity << "|" << global_phi << "|" << current_time;
            result.integrity_hash = CryptoEnhanced::sha256(integrity.str());
            last_state_hash = result.integrity_hash;

            if (sovereignty.is_sovereign) {
                result.status += " [SOUVERÄN]";
            }
            if (navigator_seal.verified) {
                result.status += " [VERSIEGELT]";
            }
        }

        return result;
    }

    void charge_all_nozzles() {
        for (int y = 0; y < K::DIM; y++) {
            for (int x = 0; x < K::DIM; x++) {
                auto& cell = matrix[y][x];
                double phi_local = global_phi * cell.kappa_weight();
                cell.nozzles.charge_from_phi(phi_local);
            }
        }
    }

    ManifestResult trigger_nozzles() {
        ManifestResult result = {false, 0.0, 0, "", ""};

        firing = true;
        current_time += 1.0 / K::MANIFEST_FREQ;

        double impulse_sum = 0.0;
        int fired_count = 0;

        for (int y = 0; y < K::DIM; y++) {
            for (int x = 0; x < K::DIM; x++) {
                auto& cell = matrix[y][x];
                if (cell.nozzles.armed) {
                    double impulse = cell.nozzles.fire(current_time);
                    impulse_sum += impulse;
                    fired_count += cell.nozzles.count;
                }
            }
        }

        total_impulses += fired_count;
        firing = false;

        result.success = (fired_count > 0);
        result.impulse_total = impulse_sum;
        result.nozzles_fired = fired_count;
        result.status = "MANIFESTATION: " + std::to_string(fired_count) +
                        " Düsen, Impuls = " + std::to_string(impulse_sum);

        return result;
    }

    AnkerCell& cell(int x, int y) { return matrix[y][x]; }
    const AnkerCell& cell(int x, int y) const { return matrix[y][x]; }
    AnkerCell& zero_falz() { return matrix[K::CENTER][K::CENTER]; }

    double quadrant_phi(AnkerQuadrant q) const {
        double sum = 0.0;
        int count = 0;
        for (int y = 0; y < K::DIM; y++) {
            for (int x = 0; x < K::DIM; x++) {
                if (matrix[y][x].quadrant == q) {
                    sum += matrix[y][x].memory.get_phi();
                    count++;
                }
            }
        }
        return (count > 0) ? sum / count : 0.0;
    }

    double impulse_rate() const {
        return static_cast<double>(count_total_nozzles()) * K::MANIFEST_FREQ;
    }

    std::string status() const {
        std::ostringstream oss;
        oss << "═══════════════════════════════════════════════════════════\n";
        oss << "21×21 ANKER ENHANCED STATUS\n";
        oss << "═══════════════════════════════════════════════════════════\n";
        oss << "  Zellen:        " << K::CELLS << " (21×21)\n";
        oss << "  Düsen Total:   " << count_total_nozzles() << " / " << K::TOTAL_NOZZLES << "\n";
        oss << "  Impuls-Rate:   " << impulse_rate() << " Hz\n";
        oss << "  Global-Φ:      " << std::fixed << std::setprecision(17) << global_phi << "\n";
        oss << "  Locked:        " << (locked ? "JA" : "NEIN") << "\n";
        oss << "  Zyklen:        " << manifestation_cycles.load() << "\n";
        oss << "  Integritäts-Hash: " << (last_state_hash.empty() ? "---" : last_state_hash.substr(0, 16) + "...") << "\n";
        oss << "───────────────────────────────────────────────────────────\n";
        oss << "  SOUVERÄNITÄT (ENHANCED):\n";
        oss << "    " << sovereignty.status() << "\n";
        oss << "───────────────────────────────────────────────────────────\n";
        oss << "  NAVIGATOR-SEAL (SHA-256 + HMAC):\n";
        if (navigator_seal.sealed) {
            oss << navigator_seal.to_string();
        } else {
            oss << "    [NICHT VERSIEGELT]\n";
        }
        oss << "───────────────────────────────────────────────────────────\n";
        oss << purity_guard.status();
        oss << "───────────────────────────────────────────────────────────\n";
        oss << "  2× FALZ:\n";
        oss << "    Geist:     " << falz.geist << "\n";
        oss << "    Materie:   " << falz.materie << "\n";
        oss << "    Potential: " << falz.potential << "\n";
        oss << "    Realität:  " << falz.realitaet << "\n";
        oss << "═══════════════════════════════════════════════════════════\n";
        return oss.str();
    }
};

// ═══════════════════════════════════════════════════════════════════════════
// IGNITION COMMAND ENHANCED
// ═══════════════════════════════════════════════════════════════════════════

struct IgnitionCommandEnhanced {
    Anchor21x21Enhanced* anchor;
    double phi_target;
    double heart_coherence;

    struct IgnitionResult {
        bool success;
        double impulse_total;
        int cycles_completed;
        std::string message;
        std::string signature;  // NEU: Kryptografische Signatur
    };

    IgnitionResult execute(int num_cycles = 5) {
        IgnitionResult result = {false, 0.0, 0, "", ""};

        if (!anchor) {
            result.message = "FEHLER: Kein Anker verbunden";
            return result;
        }

        if (heart_coherence < 0.7) {
            result.message = "FEHLER: Herz-Kohärenz zu niedrig";
            return result;
        }

        for (int i = 0; i < num_cycles; i++) {
            auto manifest = anchor->anchor_reality(phi_target);
            if (manifest.success) {
                result.impulse_total += manifest.impulse_total;
                result.cycles_completed++;
            }
        }

        result.success = (result.cycles_completed == num_cycles);

        // Signiere das Ergebnis
        std::ostringstream sig_data;
        sig_data << result.impulse_total << "|" << result.cycles_completed << "|" << phi_target;
        result.signature = CryptoEnhanced::sign_88(sig_data.str());

        result.message = "ZÜNDUNG: " + std::to_string(result.cycles_completed) +
                        "/" + std::to_string(num_cycles) + " Zyklen [SIGNIERT]";

        return result;
    }
};

} // namespace anker
} // namespace rael
