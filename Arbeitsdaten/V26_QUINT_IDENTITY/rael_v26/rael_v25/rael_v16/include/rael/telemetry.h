#pragma once
#include <atomic>
#include <cstdint>

namespace rael {

struct Telemetry {
    std::atomic<uint64_t> ops{0};
    std::atomic<uint64_t> ops_per_sec{0};
};

extern Telemetry gTelemetry;
void telemetry_tick(); // call once per loop iteration

}
