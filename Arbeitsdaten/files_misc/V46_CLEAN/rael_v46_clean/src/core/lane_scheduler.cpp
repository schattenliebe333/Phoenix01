#include "rael/lane_scheduler.h"

namespace rael {

void LaneScheduler::set_max_depth(size_t per_lane){
    std::lock_guard<std::mutex> g(mtx_);
    max_depth_ = per_lane;
}
void LaneScheduler::set_slow_threshold(size_t n){
    std::lock_guard<std::mutex> g(mtx_);
    slow_threshold_ = n;
}

bool LaneScheduler::enqueue(Task t){
    std::lock_guard<std::mutex> g(mtx_);
    t.id = ++next_id_;
    t.slow = (t.payload.size() >= slow_threshold_);
    size_t idx = (size_t)t.lane;
    auto& q = q_[idx];
    if(q.size() >= max_depth_){
        ls_[idx].dropped++;
        ls_[idx].depth = q.size();
        return false;
    }
    q.push_back(std::move(t));
    ls_[idx].enqueued++;
    ls_[idx].depth = q.size();
    return true;
}

bool LaneScheduler::pop_lane(Lane l, Task& out){
    size_t idx = (size_t)l;
    auto& q = q_[idx];
    if(q.empty()) return false;
    out = std::move(q.front());
    q.pop_front();
    ls_[idx].dequeued++;
    ls_[idx].depth = q.size();
    return true;
}

bool LaneScheduler::try_dequeue(Task& out){
    std::lock_guard<std::mutex> g(mtx_);
    static const Lane pattern[8] = { Lane::L1, Lane::L2, Lane::L1, Lane::L3, Lane::L2, Lane::L4, Lane::L1, Lane::L5 };
    for(int k=0;k<8;k++){
        Lane l = pattern[(rr_ + k) % 8];
        if(pop_lane(l, out)){
            rr_ = (rr_ + k + 1) % 8;
            return true;
        }
    }
    return false;
}

std::array<LaneStats,5> LaneScheduler::lane_stats() const{
    std::lock_guard<std::mutex> g(mtx_);
    return ls_;
}
std::array<NodeStats,8> LaneScheduler::node_stats() const{
    std::lock_guard<std::mutex> g(mtx_);
    return ns_;
}

void LaneScheduler::mark_taken(size_t node_id, Lane lane){
    if(node_id>=8) return;
    std::lock_guard<std::mutex> g(mtx_);
    ns_[node_id].taken++;
    ns_[node_id].last_lane = lane;
}
void LaneScheduler::mark_done(size_t node_id){
    if(node_id>=8) return;
    std::lock_guard<std::mutex> g(mtx_);
    ns_[node_id].done++;
}

const char* lane_name(Lane l){
    switch(l){
        case Lane::L1: return "L1";
        case Lane::L2: return "L2";
        case Lane::L3: return "L3";
        case Lane::L4: return "L4";
        case Lane::L5: return "L5";
        default: return "L?";
    }
}

} // namespace rael
