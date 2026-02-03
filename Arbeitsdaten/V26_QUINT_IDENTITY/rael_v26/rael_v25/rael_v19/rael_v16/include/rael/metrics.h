#pragma once
#include <atomic>
#include <cstdint>
#include <string>

namespace rael {

struct Metrics {
    std::atomic<uint64_t> ops_total{0};
    std::atomic<uint64_t> ops_sec{0};

    std::atomic<uint64_t> semantic_calls{0};
    std::atomic<uint64_t> resonance_calls{0};
    std::atomic<uint64_t> ethics_blocks{0};

    std::atomic<uint64_t> module_loads{0};
    std::atomic<uint64_t> module_activations{0};
    std::atomic<uint64_t> module_unloads{0};
    std::atomic<uint64_t> hotswaps{0};
};

extern Metrics gMetrics;

void metrics_tick_op();          // call once per user-level operation
void metrics_mark_semantic();
void metrics_mark_resonance();
void metrics_mark_ethics_block();
void metrics_mark_module_load();
void metrics_mark_module_activation();
void metrics_mark_module_unload();
void metrics_mark_hotswap();

} // namespace rael
