#include "rael/star8.h"
#include "rael/raelcore.h"
#include "rael/events.h"
#include <chrono>

namespace rael {

Star8::Star8(RaelCore& core) : core_(core) {}
Star8::~Star8(){ stop(); }

void Star8::start(){
    bool expected=false;
    if(!running_.compare_exchange_strong(expected, true)) return;
    threads_.clear();
    threads_.reserve(8);
    for(size_t i=0;i<8;i++){
        threads_.emplace_back(&Star8::worker, this, i);
    }
    EventBus::push("STAR8_START", "nodes=8");
}

void Star8::stop(){
    bool expected=true;
    if(!running_.compare_exchange_strong(expected, false)) return;
    for(auto& t: threads_) if(t.joinable()) t.join();
    threads_.clear();
    EventBus::push("STAR8_STOP", "ok");
}

bool Star8::submit(Lane lane, const std::string& payload){
    Task t;
    t.lane = lane;
    t.payload = payload;
    return sched_.enqueue(std::move(t));
}

void Star8::worker(size_t node_id){
    while(running_.load()){
        Task t;
        if(sched_.try_dequeue(t)){
            sched_.mark_taken(node_id, t.lane);
            if(t.slow){
                std::this_thread::sleep_for(std::chrono::milliseconds(2));
            }
            auto out = core_.process_payload(t.payload);
            EventBus::push("ANSWER", out);
            sched_.mark_done(node_id);
        } else {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    }
}

} // namespace rael
