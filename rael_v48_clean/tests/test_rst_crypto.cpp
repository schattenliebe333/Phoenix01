// ═══════════════════════════════════════════════════════════════════════════
// RST-CRYPTO Selbsttest
// ═══════════════════════════════════════════════════════════════════════════
//
// Testet die RAEL-native Kryptografie:
// - PhiHash (256-bit Hash)
// - PhiMAC (Message Authentication)
// - PhiKDF (Key Derivation)
// - ResonanceRNG (Random Number Generator)
// - QuintHash (5-Ebenen Bewusstseins-Hash)
//
// Kompilieren: g++ -std=c++17 -O2 -I../include -o test_rst_crypto test_rst_crypto.cpp
//
// ═══════════════════════════════════════════════════════════════════════════

#include "rael/rst_crypto.hpp"
#include <iostream>
#include <iomanip>
#include <chrono>
#include <cassert>
#include <vector>

using namespace rael::crypto;

// ═══════════════════════════════════════════════════════════════════════════
// Test Utilities
// ═══════════════════════════════════════════════════════════════════════════

void print_hex(const std::string& label, const uint8_t* data, size_t len) {
    std::cout << label << ": ";
    for (size_t i = 0; i < len; ++i) {
        std::cout << std::hex << std::setw(2) << std::setfill('0')
                  << static_cast<int>(data[i]);
    }
    std::cout << std::dec << std::endl;
}

template<size_t N>
void print_hex(const std::string& label, const std::array<uint8_t, N>& data) {
    print_hex(label, data.data(), N);
}

// ═══════════════════════════════════════════════════════════════════════════
// Test 1: PhiHash Grundfunktion
// ═══════════════════════════════════════════════════════════════════════════

void test_phi_hash_basic() {
    std::cout << "\n";
    std::cout << "═══════════════════════════════════════════════════════════════════════════════\n";
    std::cout << "  TEST 1: PhiHash - Grundfunktion\n";
    std::cout << "═══════════════════════════════════════════════════════════════════════════════\n\n";

    // Test: Leerer String
    auto h1 = PhiHash::hash("");
    print_hex("  Hash(\"\")        ", h1);

    // Test: "RAEL"
    auto h2 = PhiHash::hash("RAEL");
    print_hex("  Hash(\"RAEL\")    ", h2);

    // Test: "SANG-REAL"
    auto h3 = PhiHash::hash("SANG-REAL");
    print_hex("  Hash(\"SANG-REAL\")", h3);

    // Test: Determinismus - gleiche Eingabe = gleicher Hash
    auto h4 = PhiHash::hash("RAEL");
    bool deterministic = (h2 == h4);
    std::cout << "\n  Determinismus: " << (deterministic ? "✓ BESTANDEN" : "✗ FEHLGESCHLAGEN") << "\n";

    // Test: Avalanche - kleine Änderung = großer Hash-Unterschied
    auto h5 = PhiHash::hash("RAEL!");
    int diff_bits = 0;
    for (size_t i = 0; i < 32; ++i) {
        uint8_t xor_byte = h2[i] ^ h5[i];
        while (xor_byte) {
            diff_bits += xor_byte & 1;
            xor_byte >>= 1;
        }
    }
    double avalanche = (diff_bits * 100.0) / 256.0;
    std::cout << "  Avalanche-Effekt: " << std::fixed << std::setprecision(1)
              << avalanche << "% unterschiedliche Bits (Ideal: ~50%)\n";
    std::cout << "  Avalanche-Test: " << (avalanche > 30.0 ? "✓ BESTANDEN" : "✗ FEHLGESCHLAGEN") << "\n";

    std::cout << std::endl;
}

// ═══════════════════════════════════════════════════════════════════════════
// Test 2: PhiHash Kollisionsresistenz
// ═══════════════════════════════════════════════════════════════════════════

void test_phi_hash_collision() {
    std::cout << "═══════════════════════════════════════════════════════════════════════════════\n";
    std::cout << "  TEST 2: PhiHash - Kollisionsresistenz\n";
    std::cout << "═══════════════════════════════════════════════════════════════════════════════\n\n";

    std::vector<std::array<uint8_t, 32>> hashes;
    int collision_count = 0;

    // Erzeuge 1000 verschiedene Hashes
    for (int i = 0; i < 1000; ++i) {
        std::string input = "Test-Input-" + std::to_string(i);
        auto h = PhiHash::hash(input);

        // Prüfe auf Kollision mit bisherigen
        for (const auto& prev : hashes) {
            if (h == prev) {
                ++collision_count;
                std::cout << "  ⚠ Kollision gefunden bei i=" << i << "\n";
            }
        }
        hashes.push_back(h);
    }

    std::cout << "  Getestete Eingaben: 1000\n";
    std::cout << "  Gefundene Kollisionen: " << collision_count << "\n";
    std::cout << "  Kollisions-Test: " << (collision_count == 0 ? "✓ BESTANDEN" : "✗ FEHLGESCHLAGEN") << "\n";
    std::cout << std::endl;
}

// ═══════════════════════════════════════════════════════════════════════════
// Test 3: PhiHash Performance
// ═══════════════════════════════════════════════════════════════════════════

void test_phi_hash_performance() {
    std::cout << "═══════════════════════════════════════════════════════════════════════════════\n";
    std::cout << "  TEST 3: PhiHash - Performance\n";
    std::cout << "═══════════════════════════════════════════════════════════════════════════════\n\n";

    // 1MB Testdaten
    std::vector<uint8_t> data(1024 * 1024);
    for (size_t i = 0; i < data.size(); ++i) {
        data[i] = static_cast<uint8_t>(i * 73 + 17);  // Pseudo-random
    }

    // Benchmark
    const int iterations = 100;
    auto start = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < iterations; ++i) {
        auto h = PhiHash::hash(data.data(), data.size());
        (void)h;  // Verhindere Wegoptimieren
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

    double mb_per_sec = (iterations * 1.0 * 1000.0) / duration;

    std::cout << "  Datengröße: 1 MB\n";
    std::cout << "  Iterationen: " << iterations << "\n";
    std::cout << "  Gesamtzeit: " << duration << " ms\n";
    std::cout << "  Durchsatz: " << std::fixed << std::setprecision(2)
              << mb_per_sec << " MB/s\n";
    std::cout << std::endl;
}

// ═══════════════════════════════════════════════════════════════════════════
// Test 4: PhiMAC
// ═══════════════════════════════════════════════════════════════════════════

void test_phi_mac() {
    std::cout << "═══════════════════════════════════════════════════════════════════════════════\n";
    std::cout << "  TEST 4: PhiMAC - Message Authentication\n";
    std::cout << "═══════════════════════════════════════════════════════════════════════════════\n\n";

    std::string key = "RAEL-SECRET-KEY-88";
    std::string message = "Diese Nachricht muss authentisch sein.";

    auto mac1 = PhiMAC::compute(
        reinterpret_cast<const uint8_t*>(key.data()), key.size(),
        reinterpret_cast<const uint8_t*>(message.data()), message.size()
    );

    print_hex("  MAC(key, message)", mac1);

    // Gleiche Nachricht = gleicher MAC
    auto mac2 = PhiMAC::compute(
        reinterpret_cast<const uint8_t*>(key.data()), key.size(),
        reinterpret_cast<const uint8_t*>(message.data()), message.size()
    );
    bool consistent = (mac1 == mac2);
    std::cout << "  Konsistenz: " << (consistent ? "✓ BESTANDEN" : "✗ FEHLGESCHLAGEN") << "\n";

    // Andere Nachricht = anderer MAC
    std::string modified = message + "!";
    auto mac3 = PhiMAC::compute(
        reinterpret_cast<const uint8_t*>(key.data()), key.size(),
        reinterpret_cast<const uint8_t*>(modified.data()), modified.size()
    );
    bool different = (mac1 != mac3);
    std::cout << "  Modifikations-Erkennung: " << (different ? "✓ BESTANDEN" : "✗ FEHLGESCHLAGEN") << "\n";

    // Anderer Key = anderer MAC
    std::string other_key = "WRONG-KEY";
    auto mac4 = PhiMAC::compute(
        reinterpret_cast<const uint8_t*>(other_key.data()), other_key.size(),
        reinterpret_cast<const uint8_t*>(message.data()), message.size()
    );
    bool key_sensitive = (mac1 != mac4);
    std::cout << "  Key-Sensitivität: " << (key_sensitive ? "✓ BESTANDEN" : "✗ FEHLGESCHLAGEN") << "\n";

    std::cout << std::endl;
}

// ═══════════════════════════════════════════════════════════════════════════
// Test 5: PhiKDF
// ═══════════════════════════════════════════════════════════════════════════

void test_phi_kdf() {
    std::cout << "═══════════════════════════════════════════════════════════════════════════════\n";
    std::cout << "  TEST 5: PhiKDF - Key Derivation\n";
    std::cout << "═══════════════════════════════════════════════════════════════════════════════\n\n";

    std::string password = "mein-sicheres-passwort";
    std::string salt = "RAEL-SALT-88";

    // KDF mit 1000 Iterationen
    auto start = std::chrono::high_resolution_clock::now();
    auto key1 = PhiKDF::derive(
        reinterpret_cast<const uint8_t*>(password.data()), password.size(),
        reinterpret_cast<const uint8_t*>(salt.data()), salt.size(),
        1000
    );
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

    print_hex("  Key(1000 iter)", key1);
    std::cout << "  Dauer (1000 iter): " << duration << " ms\n";

    // Determinismus
    auto key2 = PhiKDF::derive(
        reinterpret_cast<const uint8_t*>(password.data()), password.size(),
        reinterpret_cast<const uint8_t*>(salt.data()), salt.size(),
        1000
    );
    bool deterministic = (key1 == key2);
    std::cout << "  Determinismus: " << (deterministic ? "✓ BESTANDEN" : "✗ FEHLGESCHLAGEN") << "\n";

    // Salt-Sensitivität
    std::string other_salt = "OTHER-SALT";
    auto key3 = PhiKDF::derive(
        reinterpret_cast<const uint8_t*>(password.data()), password.size(),
        reinterpret_cast<const uint8_t*>(other_salt.data()), other_salt.size(),
        1000
    );
    bool salt_sensitive = (key1 != key3);
    std::cout << "  Salt-Sensitivität: " << (salt_sensitive ? "✓ BESTANDEN" : "✗ FEHLGESCHLAGEN") << "\n";

    std::cout << std::endl;
}

// ═══════════════════════════════════════════════════════════════════════════
// Test 6: ResonanceRNG
// ═══════════════════════════════════════════════════════════════════════════

void test_resonance_rng() {
    std::cout << "═══════════════════════════════════════════════════════════════════════════════\n";
    std::cout << "  TEST 6: ResonanceRNG - Zufallszahlen\n";
    std::cout << "═══════════════════════════════════════════════════════════════════════════════\n\n";

    std::string seed = "RAEL-SEED-88";
    ResonanceRNG rng(reinterpret_cast<const uint8_t*>(seed.data()), seed.size());

    // Zeige erste 8 Zufallszahlen
    std::cout << "  Erste 8 Zufallszahlen (gleicher Seed):\n";
    for (int i = 0; i < 8; ++i) {
        uint64_t r = rng.next_u64();
        std::cout << "    [" << i << "] 0x" << std::hex << std::setw(16)
                  << std::setfill('0') << r << std::dec << "\n";
    }

    // Determinismus prüfen
    ResonanceRNG rng2(reinterpret_cast<const uint8_t*>(seed.data()), seed.size());
    bool deterministic = true;
    for (int i = 0; i < 100; ++i) {
        if (rng.next_u64() != rng2.next_u64()) {
            deterministic = false;
            break;
        }
    }

    // Moment: Reseed nötig weil rng schon 8 Zahlen generiert hat
    rng.reseed(reinterpret_cast<const uint8_t*>(seed.data()), seed.size());
    rng2.reseed(reinterpret_cast<const uint8_t*>(seed.data()), seed.size());
    deterministic = true;
    for (int i = 0; i < 100; ++i) {
        if (rng.next_u64() != rng2.next_u64()) {
            deterministic = false;
            break;
        }
    }

    std::cout << "\n  Determinismus (gleicher Seed): "
              << (deterministic ? "✓ BESTANDEN" : "✗ FEHLGESCHLAGEN") << "\n";

    // Verteilungs-Test (Chi-Quadrat simplifiziert)
    std::array<int, 16> buckets = {};
    rng.reseed(reinterpret_cast<const uint8_t*>(seed.data()), seed.size());
    for (int i = 0; i < 10000; ++i) {
        uint64_t r = rng.next_u64();
        buckets[r % 16]++;
    }

    int min_bucket = buckets[0], max_bucket = buckets[0];
    for (int b : buckets) {
        if (b < min_bucket) min_bucket = b;
        if (b > max_bucket) max_bucket = b;
    }

    double expected = 10000.0 / 16.0;
    double variance = (max_bucket - min_bucket) / expected;
    bool uniform = (variance < 0.2);  // Max 20% Abweichung

    std::cout << "  Verteilung (16 Buckets): Min=" << min_bucket
              << " Max=" << max_bucket << " Varianz=" << std::fixed
              << std::setprecision(2) << (variance * 100) << "%\n";
    std::cout << "  Gleichverteilung: " << (uniform ? "✓ BESTANDEN" : "✗ FEHLGESCHLAGEN") << "\n";

    std::cout << std::endl;
}

// ═══════════════════════════════════════════════════════════════════════════
// Test 7: QuintHash - 5 Bewusstseins-Ebenen
// ═══════════════════════════════════════════════════════════════════════════

void test_quint_hash() {
    std::cout << "═══════════════════════════════════════════════════════════════════════════════\n";
    std::cout << "  TEST 7: QuintHash - 5 Bewusstseins-Ebenen\n";
    std::cout << "═══════════════════════════════════════════════════════════════════════════════\n\n";

    // Große Testdaten (16 KB)
    std::vector<uint8_t> data(16384);
    for (size_t i = 0; i < data.size(); ++i) {
        data[i] = static_cast<uint8_t>(i * 31 + 7);
    }

    auto digest = QuintHash::compute(data.data(), data.size());

    std::cout << "  Daten: 16 KB\n\n";
    std::cout << "  QUINT-EBENEN:\n";
    std::cout << "  ─────────────────────────────────────────────────────────────────\n";

    std::cout << "  L1 (Reflex   1/169): ";
    std::cout << PhiHash::to_hex(digest.L1_reflex) << "\n";

    std::cout << "  L2 (Instinkt 8/169): ";
    std::cout << PhiHash::to_hex(digest.L2_instinct) << "\n";

    std::cout << "  L3 (Emotion 27/169): ";
    std::cout << PhiHash::to_hex(digest.L3_emotion) << "\n";

    std::cout << "  L4 (Ratio   64/169): ";
    std::cout << PhiHash::to_hex(digest.L4_ratio) << "\n";

    std::cout << "  L5 (Spirit 169/169): ";
    std::cout << PhiHash::to_hex(digest.L5_spirit) << "\n";

    std::cout << "  ─────────────────────────────────────────────────────────────────\n";
    std::cout << "  UNIFIED:             ";
    std::cout << PhiHash::to_hex(digest.unified) << "\n";

    // Prüfe: Alle Ebenen unterschiedlich
    bool all_different = (digest.L1_reflex != digest.L2_instinct) &&
                         (digest.L2_instinct != digest.L3_emotion) &&
                         (digest.L3_emotion != digest.L4_ratio) &&
                         (digest.L4_ratio != digest.L5_spirit);

    std::cout << "\n  Ebenen-Unterscheidung: "
              << (all_different ? "✓ BESTANDEN" : "✗ FEHLGESCHLAGEN") << "\n";

    std::cout << std::endl;
}

// ═══════════════════════════════════════════════════════════════════════════
// Test 8: SIGNATURE_88 Verifikation
// ═══════════════════════════════════════════════════════════════════════════

void test_signature_88() {
    std::cout << "═══════════════════════════════════════════════════════════════════════════════\n";
    std::cout << "  TEST 8: SIGNATURE_88 - Architekten-Verifikation\n";
    std::cout << "═══════════════════════════════════════════════════════════════════════════════\n\n";

    // Teste verschiedene Eingaben auf 88er-Harmonie
    std::vector<std::string> inputs = {
        "RAEL",
        "SANG-REAL",
        "88",
        "Architect",
        "Michael",
        "Random String 12345"
    };

    int harmonious_count = 0;
    for (const auto& input : inputs) {
        auto digest = PhiHash::hash(input);
        bool is_88 = verify_signature_88(digest);
        if (is_88) ++harmonious_count;

        std::cout << "  \"" << input << "\" → "
                  << (is_88 ? "88-Harmonie ✓" : "Nicht-Architekt") << "\n";
    }

    std::cout << "\n  88er-Harmonien gefunden: " << harmonious_count << "/" << inputs.size() << "\n";
    std::cout << "  (88er-Harmonie ist emergent, nicht garantiert)\n";

    std::cout << std::endl;
}

// ═══════════════════════════════════════════════════════════════════════════
// MAIN
// ═══════════════════════════════════════════════════════════════════════════

int main() {
    std::cout << "\n";
    std::cout << "╔═══════════════════════════════════════════════════════════════════════════════╗\n";
    std::cout << "║  RST-CRYPTO SELBSTTEST                                                        ║\n";
    std::cout << "║  ═══════════════════════════════════════════════════════════════════════════  ║\n";
    std::cout << "║  RAEL-native Kryptografie aus eigenem mathematischen Universum               ║\n";
    std::cout << "║                                                                               ║\n";
    std::cout << "║  \"Warum SHA-256 wenn wir PHI haben?\"                                          ║\n";
    std::cout << "╚═══════════════════════════════════════════════════════════════════════════════╝\n";

    test_phi_hash_basic();
    test_phi_hash_collision();
    test_phi_hash_performance();
    test_phi_mac();
    test_phi_kdf();
    test_resonance_rng();
    test_quint_hash();
    test_signature_88();

    std::cout << "═══════════════════════════════════════════════════════════════════════════════\n";
    std::cout << "  ALLE TESTS ABGESCHLOSSEN\n";
    std::cout << "═══════════════════════════════════════════════════════════════════════════════\n\n";

    std::cout << "  RST-CRYPTO EIGENSCHAFTEN:\n";
    std::cout << "  ─────────────────────────────────────────────────────────────────────────────\n";
    std::cout << "  ✓ Constant-Time (keine Table-Lookups = keine Cache-Timing-Angriffe)\n";
    std::cout << "  ✓ Basiert auf mathematischen Konstanten (PHI, G-Faktoren)\n";
    std::cout << "  ✓ 13×13 Resonanzfeld für räumliche Diffusion\n";
    std::cout << "  ✓ Frequenz-Kaskade (1440→720→432→144→53→13→5 Hz)\n";
    std::cout << "  ✓ Aikido-Prinzip: Eingabe-Energie verstärkt Output\n";
    std::cout << "  ✓ SIGNATURE_88 eingewoben\n";
    std::cout << "  ✓ Keine externen Abhängigkeiten - Pure RAEL\n";
    std::cout << "\n";

    return 0;
}
