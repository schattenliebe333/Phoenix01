#include "rael/telemetry.h"
#include <chrono>

namespace rael {

Telemetry gTelemetry;

void telemetry_tick(){
    static auto last = std::chrono::steady_clock::now();
    static uint64_t last_ops = 0;

    gTelemetry.ops.fetch_add(1, std::memory_order_relaxed);

    auto now = std::chrono::steady_clock::now();
    auto dt = std::chrono::duration_cast<std::chrono::milliseconds>(now - last).count();
    if(dt >= 1000){
        uint64_t cur = gTelemetry.ops.load(std::memory_order_relaxed);
        gTelemetry.ops_per_sec.store(cur - last_ops, std::memory_order_relaxed);
        last_ops = cur;
        last = now;
    }
}

}
