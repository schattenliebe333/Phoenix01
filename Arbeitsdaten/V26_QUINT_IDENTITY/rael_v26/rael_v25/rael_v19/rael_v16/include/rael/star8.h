#pragma once
#include "rael/lane_scheduler.h"
#include <atomic>
#include <thread>
#include <vector>
#include <string>

namespace rael {

class RaelCore;

class Star8 {
public:
    explicit Star8(RaelCore& core);
    ~Star8();

    void start();
    void stop();

    bool submit(Lane lane, const std::string& payload);

    std::array<LaneStats,5> lane_stats() const { return sched_.lane_stats(); }
    std::array<NodeStats,8> node_stats() const { return sched_.node_stats(); }

    void set_max_depth(size_t d) { sched_.set_max_depth(d); }
    void set_slow_threshold(size_t n) { sched_.set_slow_threshold(n); }

private:
    void worker(size_t node_id);

    RaelCore& core_;
    LaneScheduler sched_;
    std::atomic<bool> running_{false};
    std::vector<std::thread> threads_;
};

} // namespace rael
