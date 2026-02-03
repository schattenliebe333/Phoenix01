#pragma once
// ═══════════════════════════════════════════════════════════════════════════
// RAEL V49 - HIGHWAY QUINT INTEGRATION
// ═══════════════════════════════════════════════════════════════════════════
//
// Integriert das QUINT-Memory-System in den 5-Lane Highway:
//   - Jede Lane hat ihr eigenes QuintMemory
//   - Frequenz-Band-Zuordnung zu Quint-Schichten
//   - Gegen-Frequenz-Vorhang für Parasiten-Reinigung
//
// (c) 2025 Phoenix RST System - Michael & Kael
// ═══════════════════════════════════════════════════════════════════════════

#include "rael/quint_memory.hpp"
#include "rael/lane_scheduler.h"
#include <array>
#include <string>

namespace rael {
namespace quint {

// ═══════════════════════════════════════════════════════════════════════════
// LANE-KONSTANTEN
// ═══════════════════════════════════════════════════════════════════════════

namespace LaneFreq {
    // Zentral-Frequenzen
    constexpr double CENTER[5] = {144.0, 288.0, 432.0, 576.0, 720.0};
    
    // Band-Grenzen (±10%)
    constexpr double LOW[5] = {130.0, 259.0, 389.0, 518.0, 648.0};
    constexpr double HIGH[5] = {158.0, 317.0, 475.0, 634.0, 792.0};
    
    // κ-Werte
    constexpr double KAPPA[5] = {0.9, 0.8, 0.7, 0.6, 0.5};
    
    // Als Brüche
    constexpr Frac KAPPA_FRAC[5] = {
        {9, 10},   // κ(144) = 1296/1440 = 9/10
        {4, 5},    // κ(288) = 1152/1440 = 4/5
        {7, 10},   // κ(432) = 1008/1440 = 7/10
        {3, 5},    // κ(576) = 864/1440 = 3/5
        {1, 2}     // κ(720) = 720/1440 = 1/2
    };
}

// ═══════════════════════════════════════════════════════════════════════════
// PAKET-STRUKTUR
// ═══════════════════════════════════════════════════════════════════════════

struct QuintPacket {
    double frequency;
    double psi;
    double omega;
    double phi;
    int lane_id;
    uint32_t flags;
    
    // Flags
    static constexpr uint32_t FLAG_CLEANED = 0x01;
    static constexpr uint32_t FLAG_RAEL_VERIFIED = 0x02;
    static constexpr uint32_t FLAG_H_OK = 0x04;
    static constexpr uint32_t FLAG_ANOMALY = 0x08;
    
    QuintPacket() : frequency(0), psi(0), omega(0), phi(0), lane_id(-1), flags(0) {}
    
    bool is_cleaned() const { return flags & FLAG_CLEANED; }
    bool is_rael() const { return flags & FLAG_RAEL_VERIFIED; }
    bool is_h_ok() const { return flags & FLAG_H_OK; }
    bool is_anomaly() const { return flags & FLAG_ANOMALY; }
};

// ═══════════════════════════════════════════════════════════════════════════
// QUINT-LANE
// Eine Highway-Lane mit integriertem QuintMemory
// ═══════════════════════════════════════════════════════════════════════════

class QuintLane {
public:
    int lane_id;
    double center_freq;
    double band_low;
    double band_high;
    double kappa_val;
    Frac kappa_frac;
    
    QuintMemory memory;
    
    // Statistiken
    std::atomic<uint64_t> packets_total{0};
    std::atomic<uint64_t> packets_passed{0};
    std::atomic<uint64_t> packets_cleaned{0};
    std::atomic<uint64_t> packets_rejected{0};
    
    void init(int id) {
        lane_id = id;
        center_freq = LaneFreq::CENTER[id];
        band_low = LaneFreq::LOW[id];
        band_high = LaneFreq::HIGH[id];
        kappa_val = LaneFreq::KAPPA[id];
        kappa_frac = LaneFreq::KAPPA_FRAC[id];
        
        memory.init();
    }
    
    // ═══════════════════════════════════════════════════════════════════════
    // BAND-CHECK
    // ═══════════════════════════════════════════════════════════════════════
    
    bool in_band(double freq) const {
        return freq >= band_low && freq <= band_high;
    }
    
    // ═══════════════════════════════════════════════════════════════════════
    // PAKET-VERARBEITUNG
    // ═══════════════════════════════════════════════════════════════════════
    
    bool process(const QuintPacket& in, QuintPacket& out) {
        packets_total++;
        
        // Band-Check
        if (!in_band(in.frequency)) {
            packets_rejected++;
            return false;
        }
        
        out = in;
        out.lane_id = lane_id;
        
        // In QuintMemory schreiben
        int layer = freq_to_layer(in.frequency);
        memory.write_psi(layer, in.psi);
        memory.write_omega(layer, in.omega);
        
        // ═══════════════════════════════════════════════════════════
        // GEGEN-FREQUENZ-VORHANG
        // Korrigiere Parasiten auf Zentral-Frequenz
        // ═══════════════════════════════════════════════════════════
        
        if (std::abs(in.frequency - center_freq) > 0.01) {
            // Parasiten-Frequenz gefunden → Korrektur
            out.frequency = center_freq;
            out.flags |= QuintPacket::FLAG_CLEANED;
            packets_cleaned++;
        }
        
        // ═══════════════════════════════════════════════════════════
        // RAEL-CHECK (88-Signatur)
        // ═══════════════════════════════════════════════════════════
        
        if (memory.is_rael()) {
            out.flags |= QuintPacket::FLAG_RAEL_VERIFIED;
        }
        
        // ═══════════════════════════════════════════════════════════
        // H-SCHWELLE CHECK
        // ═══════════════════════════════════════════════════════════
        
        if (memory.check_H_threshold()) {
            out.flags |= QuintPacket::FLAG_H_OK;
        } else {
            out.flags |= QuintPacket::FLAG_ANOMALY;
        }
        
        // Φ aktualisieren
        out.phi = memory.get_phi();
        
        packets_passed++;
        return true;
    }
    
    // ═══════════════════════════════════════════════════════════════════════
    // STATISTIKEN
    // ═══════════════════════════════════════════════════════════════════════
    
    double pass_rate() const {
        uint64_t total = packets_total.load();
        if (total == 0) return 1.0;
        return static_cast<double>(packets_passed.load()) / total;
    }
    
    double clean_rate() const {
        uint64_t passed = packets_passed.load();
        if (passed == 0) return 0.0;
        return static_cast<double>(packets_cleaned.load()) / passed;
    }
};

// ═══════════════════════════════════════════════════════════════════════════
// QUINT-HIGHWAY
// 5 Lanes mit QuintMemory
// ═══════════════════════════════════════════════════════════════════════════

class QuintHighway {
public:
    std::array<QuintLane, 5> lanes;
    QuintMemory entry_spiral;  // Eingangs-Spirale
    
    void init() {
        for (int i = 0; i < 5; i++) {
            lanes[i].init(i);
        }
        entry_spiral.init();
    }
    
    // ═══════════════════════════════════════════════════════════════════════
    // SPIRALE (Eingangsfilter)
    // f ≥ 53.33 Hz UND H < 4/9
    // ═══════════════════════════════════════════════════════════════════════
    
    bool spirale_check(const QuintPacket& pkt) {
        // Frequenz-Check: f ≥ 160/3 ≈ 53.33 Hz
        if (pkt.frequency < F::GATE53) {
            return false;  // Zu langsam → blockiert
        }
        
        // H-Check: Schreibe temporär in entry_spiral
        int layer = freq_to_layer(pkt.frequency);
        entry_spiral.write_psi(layer, pkt.psi);
        entry_spiral.write_omega(layer, pkt.omega);
        
        // H < 4/9?
        return entry_spiral.check_H_threshold();
    }
    
    // ═══════════════════════════════════════════════════════════════════════
    // ROUTING
    // ═══════════════════════════════════════════════════════════════════════
    
    int find_lane(double freq) const {
        for (int i = 0; i < 5; i++) {
            if (lanes[i].in_band(freq)) {
                return i;
            }
        }
        return -1;  // Keine passende Lane
    }
    
    // ═══════════════════════════════════════════════════════════════════════
    // VOLLSTÄNDIGE VERARBEITUNG
    // ═══════════════════════════════════════════════════════════════════════
    
    struct ProcessResult {
        bool success;
        QuintPacket packet;
        std::string message;
    };
    
    ProcessResult process(const QuintPacket& in) {
        // 1. Spirale-Check
        if (!spirale_check(in)) {
            return {false, in, "Spirale: Frequenz zu niedrig oder H zu hoch"};
        }
        
        // 2. Lane finden
        int lane_id = find_lane(in.frequency);
        if (lane_id < 0) {
            return {false, in, "Keine passende Lane gefunden"};
        }
        
        // 3. Lane-Verarbeitung
        QuintPacket out;
        if (!lanes[lane_id].process(in, out)) {
            return {false, in, "Lane-Verarbeitung fehlgeschlagen"};
        }
        
        // 4. Erfolg
        std::string msg = "OK: Lane " + std::to_string(lane_id);
        if (out.is_cleaned()) msg += " [CLEANED]";
        if (out.is_rael()) msg += " [RAEL]";
        if (out.is_h_ok()) msg += " [H-OK]";
        
        return {true, out, msg};
    }
    
    // ═══════════════════════════════════════════════════════════════════════
    // BATCH-VERARBEITUNG
    // ═══════════════════════════════════════════════════════════════════════
    
    struct BatchResult {
        size_t total;
        size_t passed;
        size_t rejected;
        size_t cleaned;
        size_t rael_verified;
    };
    
    BatchResult process_batch(const QuintPacket* packets, size_t count) {
        BatchResult result = {count, 0, 0, 0, 0};
        
        for (size_t i = 0; i < count; i++) {
            auto res = process(packets[i]);
            if (res.success) {
                result.passed++;
                if (res.packet.is_cleaned()) result.cleaned++;
                if (res.packet.is_rael()) result.rael_verified++;
            } else {
                result.rejected++;
            }
        }
        
        return result;
    }
    
    // ═══════════════════════════════════════════════════════════════════════
    // STATISTIKEN
    // ═══════════════════════════════════════════════════════════════════════
    
    double total_pass_rate() const {
        uint64_t total = 0, passed = 0;
        for (const auto& lane : lanes) {
            total += lane.packets_total.load();
            passed += lane.packets_passed.load();
        }
        if (total == 0) return 1.0;
        return static_cast<double>(passed) / total;
    }
    
    double average_phi() const {
        double sum = 0.0;
        for (const auto& lane : lanes) {
            sum += lane.memory.get_phi();
        }
        return sum / 5.0;
    }
    
    bool all_rael() const {
        for (const auto& lane : lanes) {
            if (!lane.memory.is_rael()) return false;
        }
        return true;
    }
};

// ═══════════════════════════════════════════════════════════════════════════
// ADAPTER: Verbindet QuintHighway mit bestehendem LaneScheduler
// ═══════════════════════════════════════════════════════════════════════════

class HighwayQuintAdapter {
public:
    QuintHighway quint_highway;
    
    void init() {
        quint_highway.init();
    }
    
    // Konvertiere Legacy-Lane zu Quint-Lane
    int legacy_to_quint_lane(Lane legacy_lane) const {
        switch (legacy_lane) {
            case Lane::L1: return 0;  // 144 Hz
            case Lane::L2: return 1;  // 288 Hz
            case Lane::L3: return 2;  // 432 Hz
            case Lane::L4: return 3;  // 576 Hz
            case Lane::L5: return 4;  // 720 Hz
            default: return -1;
        }
    }
    
    // Konvertiere Lane-Frequenz
    double lane_to_frequency(Lane lane) const {
        int idx = legacy_to_quint_lane(lane);
        if (idx < 0 || idx >= 5) return 0.0;
        return LaneFreq::CENTER[idx];
    }
    
    // Verarbeite Task und aktualisiere Quint-State
    bool process_task(Lane lane, double psi_val, double omega_val) {
        QuintPacket pkt;
        pkt.frequency = lane_to_frequency(lane);
        pkt.psi = psi_val;
        pkt.omega = omega_val;
        
        auto result = quint_highway.process(pkt);
        return result.success;
    }
};

} // namespace quint
} // namespace rael
