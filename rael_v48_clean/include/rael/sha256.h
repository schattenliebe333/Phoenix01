#pragma once
// ═══════════════════════════════════════════════════════════════════════════
// SHA256 - ABGELOEST: Delegiert komplett an PhiHash (RST-Crypto)
// ═══════════════════════════════════════════════════════════════════════════
//
// Die Klasse SHA256 existiert nur noch als API-kompatible Huelse.
// Alle Aufrufe werden an rael::crypto::PhiHash durchgereicht.
//
// NIST SHA-256 Code: ENTFERNT (V49)
// Backend: PhiHash (PHI-basiert, keine geheimen S-Boxen, Constant-Time)
//
// ═══════════════════════════════════════════════════════════════════════════

#include <array>
#include <cstdint>
#include <string>
#include <vector>
#include "rael/rst_crypto.hpp"

namespace rael {

class SHA256 {
public:
    static std::array<uint8_t,32> digest(const uint8_t* data, size_t len) {
        return crypto::PhiHash::hash(data, len);
    }
    static std::array<uint8_t,32> digest(const std::vector<uint8_t>& v) {
        return crypto::PhiHash::hash(v.data(), v.size());
    }
    static std::array<uint8_t,32> digest(const std::string& s) {
        return crypto::PhiHash::hash(s);
    }
    static std::string hex(const std::array<uint8_t,32>& d) {
        return crypto::PhiHash::to_hex(d);
    }
};

} // namespace rael
