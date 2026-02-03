#include "rael/metrics.h"
#include <chrono>

namespace rael {

Metrics gMetrics;

static std::atomic<uint64_t> last_sec{0};
static std::atomic<uint64_t> sec_count{0};

static uint64_t now_sec(){
    using namespace std::chrono;
    return (uint64_t)duration_cast<seconds>(system_clock::now().time_since_epoch()).count();
}

void metrics_tick_op(){
    gMetrics.ops_total.fetch_add(1, std::memory_order_relaxed);
    uint64_t s = now_sec();
    uint64_t prev = last_sec.load(std::memory_order_relaxed);
    if(prev != s){
        // rollover
        last_sec.store(s, std::memory_order_relaxed);
        gMetrics.ops_sec.store(sec_count.exchange(0), std::memory_order_relaxed);
    }
    sec_count.fetch_add(1, std::memory_order_relaxed);
}

void metrics_mark_semantic(){ gMetrics.semantic_calls.fetch_add(1, std::memory_order_relaxed); }
void metrics_mark_resonance(){ gMetrics.resonance_calls.fetch_add(1, std::memory_order_relaxed); }
void metrics_mark_ethics_block(){ gMetrics.ethics_blocks.fetch_add(1, std::memory_order_relaxed); }
void metrics_mark_module_load(){ gMetrics.module_loads.fetch_add(1, std::memory_order_relaxed); }
void metrics_mark_module_activation(){ gMetrics.module_activations.fetch_add(1, std::memory_order_relaxed); }
void metrics_mark_module_unload(){ gMetrics.module_unloads.fetch_add(1, std::memory_order_relaxed); }
void metrics_mark_hotswap(){ gMetrics.hotswaps.fetch_add(1, std::memory_order_relaxed); }

} // namespace rael
