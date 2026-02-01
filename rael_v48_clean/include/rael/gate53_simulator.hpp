#pragma once
#include <array>
#include <vector>
#include <random>
#include <atomic>
#include <mutex>
#include <chrono>
#include <cmath>
#include "rael/rst_constants.hpp"

namespace rael {

class VortexDuese {
public:
    std::atomic<double> external_pressure{0.0};
    std::atomic<double> defense_energy{0.0};
    std::atomic<double> cpu_saved{0.0};
    
    double calculate_vortex_boost(double f, double H) const {
        double kappa = rst::kappa(f);
        double sog = (1.0 - kappa) / (H + rst::G5);
        double pressure_harvest = external_pressure.load() * rst::G1;
        return sog * (1.0 + pressure_harvest);
    }
    
    void absorb_pressure(double pressure) {
        external_pressure.store(external_pressure.load() + pressure);
        defense_energy.store(defense_energy.load() + pressure * rst::ETA_DUESE);
        cpu_saved.store(cpu_saved.load() + pressure * rst::CPU_SPAR_FAKTOR);
    }
    
    void reset() {
        external_pressure.store(0.0);
        defense_energy.store(0.0);
        cpu_saved.store(0.0);
    }
};

enum class SimVisitorType : uint8_t {
    LOGIC_AI = 0, PATTERN_SEEKER = 1, RANDOM = 2, HUMAN = 3, ARCHITECT = 4
};

struct SimResult {
    uint64_t visitor_id{0};
    SimVisitorType type{SimVisitorType::LOGIC_AI};
    double signature{0.0};
    int rings_traversed{0};
    double energy_spent{0.0};
    bool reached_core{false};
    bool trapped{false};
    bool used_tunnel{false};
};

struct SimStats {
    uint64_t total_visitors{0};
    uint64_t logic_ais{0}, pattern_seekers{0}, randoms{0}, humans{0}, architects{0};
    uint64_t logic_ais_trapped{0}, pattern_seekers_trapped{0}, randoms_trapped{0};
    uint64_t humans_passed{0}, architects_passed{0};
    double total_pressure{0.0}, total_defense{0.0}, total_cpu_saved{0.0};
    
    double trap_rate() const {
        uint64_t t = logic_ais_trapped + pattern_seekers_trapped + randoms_trapped;
        uint64_t a = logic_ais + pattern_seekers + randoms;
        return a > 0 ? (double)t / a * 100.0 : 0.0;
    }
    double pass_rate_architects() const {
        return architects > 0 ? (double)architects_passed / architects * 100.0 : 0.0;
    }
    double cpu_savings_percent() const {
        return total_pressure > 0.001 ? total_cpu_saved / total_pressure * 100.0 : 0.0;
    }
};

class Gate53Simulator {
public:
    explicit Gate53Simulator(uint64_t seed = 0) : rng_(seed == 0 ? std::random_device{}() : seed) {}
    
    SimResult simulate_visitor(SimVisitorType type, double signature = 0.0) {
        std::lock_guard<std::mutex> lock(mutex_);
        SimResult r;
        r.visitor_id = ++visitor_counter_;
        r.type = type;
        r.signature = signature;
        
        stats_.total_visitors++;
        switch(type) {
            case SimVisitorType::LOGIC_AI: stats_.logic_ais++; break;
            case SimVisitorType::PATTERN_SEEKER: stats_.pattern_seekers++; break;
            case SimVisitorType::RANDOM: stats_.randoms++; break;
            case SimVisitorType::HUMAN: stats_.humans++; break;
            case SimVisitorType::ARCHITECT: stats_.architects++; break;
        }
        
        // Architekt mit 88er-Signatur → Tunnel
        if (type == SimVisitorType::ARCHITECT && rst::is_master_signature(signature)) {
            r.used_tunnel = true;
            r.reached_core = true;
            r.rings_traversed = rst::LABYRINTH_RINGS;
            stats_.architects_passed++;
            return r;
        }
        
        // Labyrinth für diesen Besucher
        uint64_t seed = r.visitor_id ^ std::chrono::steady_clock::now().time_since_epoch().count();
        std::mt19937_64 local_rng(seed);
        std::uniform_int_distribution<int> bridge_dist(0, rst::BRIDGES_PER_RING - 1);
        std::uniform_int_distribution<int> invert_dist(0, 9);
        
        int current_ring = 0;
        while (current_ring < rst::LABYRINTH_RINGS) {
            int intent_pos = bridge_dist(local_rng);
            bool inverted = invert_dist(local_rng) < 3;
            
            int chosen = choose_bridge(type, intent_pos);
            r.energy_spent += (chosen == intent_pos) ? 0.5 : 1.0;
            
            if (chosen == intent_pos) {
                if (inverted) { r.trapped = true; break; }
                current_ring += 2;
            } else {
                current_ring += 1;
                if (current_ring > 0 && current_ring % 2 == 1 && current_ring < rst::LABYRINTH_RINGS) {
                    r.trapped = true;
                    duese_.absorb_pressure(r.energy_spent);
                    break;
                }
            }
        }
        
        r.rings_traversed = current_ring;
        if (current_ring >= rst::LABYRINTH_RINGS && !r.trapped) r.reached_core = true;
        
        if (r.trapped) {
            switch(type) {
                case SimVisitorType::LOGIC_AI: stats_.logic_ais_trapped++; break;
                case SimVisitorType::PATTERN_SEEKER: stats_.pattern_seekers_trapped++; break;
                case SimVisitorType::RANDOM: stats_.randoms_trapped++; break;
                default: break;
            }
            stats_.total_pressure += r.energy_spent;
        }
        
        stats_.total_defense = duese_.defense_energy.load();
        stats_.total_cpu_saved = duese_.cpu_saved.load();
        return r;
    }
    
    SimStats simulate_population(int n) {
        reset_stats();
        for (int i = 0; i < n; ++i) {
            SimVisitorType type;
            double sig = 0.0;
            int pct = i % 10;
            if (pct == 0) { type = SimVisitorType::ARCHITECT; sig = rst::SIGNATURE_88; }
            else if (pct < 4) { type = SimVisitorType::PATTERN_SEEKER; sig = 12.12121212; }
            else { type = SimVisitorType::LOGIC_AI; sig = (double)i; }
            simulate_visitor(type, sig);
        }
        return get_stats();
    }
    
    static double theoretical_breakthrough_probability(SimVisitorType type) {
        switch(type) {
            case SimVisitorType::LOGIC_AI: return rst::P_DURCHBRUCH_LOGIC;
            case SimVisitorType::PATTERN_SEEKER: return 0.003;
            case SimVisitorType::HUMAN: return 0.001;
            case SimVisitorType::ARCHITECT: return 1.0;
            default: return rst::P_RING_LOGIC;
        }
    }
    
    SimStats get_stats() const { std::lock_guard<std::mutex> lock(mutex_); return stats_; }
    const VortexDuese& duese() const { return duese_; }
    void reset_stats() { std::lock_guard<std::mutex> lock(mutex_); stats_ = SimStats{}; duese_.reset(); visitor_counter_ = 0; }

private:
    int choose_bridge(SimVisitorType type, int intent_pos) {
        std::uniform_int_distribution<int> dist(0, rst::BRIDGES_PER_RING - 1);
        switch(type) {
            case SimVisitorType::LOGIC_AI: {
                int c = dist(rng_);
                return (c == intent_pos) ? (c + 1) % rst::BRIDGES_PER_RING : c;
            }
            case SimVisitorType::PATTERN_SEEKER:
                return (std::uniform_int_distribution<int>(0,99)(rng_) < 20) ? intent_pos : dist(rng_);
            case SimVisitorType::HUMAN:
                return (std::uniform_int_distribution<int>(0,99)(rng_) < 50) ? intent_pos : dist(rng_);
            case SimVisitorType::ARCHITECT:
                return intent_pos;
            default:
                return dist(rng_);
        }
    }
    
    mutable std::mutex mutex_;
    std::mt19937_64 rng_;
    uint64_t visitor_counter_{0};
    SimStats stats_;
    VortexDuese duese_;
};

} // namespace rael
