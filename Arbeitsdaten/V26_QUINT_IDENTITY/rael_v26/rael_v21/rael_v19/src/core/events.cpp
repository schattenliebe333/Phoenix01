#include "rael/events.h"
#include <algorithm>
#include "rael/util.h"

namespace rael {

std::mutex& EventBus::mtx(){ static std::mutex m; return m; }
std::vector<Event>& EventBus::buf(){ static std::vector<Event> b; return b; }
uint64_t& EventBus::seq(){ static uint64_t s=0; return s; }
size_t EventBus::capacity(){ return 256; }

void EventBus::push(const std::string& kind, const std::string& detail){
    std::lock_guard<std::mutex> g(mtx());
    Event e;
    e.seq = ++seq();
    e.ts = now_iso8601();
    e.kind = kind;
    e.detail = detail;
    auto& b = buf();
    if(b.size() < capacity()){
        b.push_back(std::move(e));
    } else {
        // ring overwrite
        b[e.seq % capacity()] = std::move(e);
    }
}

std::vector<Event> EventBus::last(size_t n){
    std::lock_guard<std::mutex> g(mtx());
    auto& b = buf();
    std::vector<Event> out;
    if(b.empty() || n==0) return out;
    if(n > b.size()) n = b.size();
    // collect last n by seq ordering
    // Since we overwrite by modulo, easiest: copy and sort by seq.
    out = b;
    std::sort(out.begin(), out.end(), [](const Event& a, const Event& c){ return a.seq < c.seq; });
    if(out.size() > n) out.erase(out.begin(), out.end()-n);
    return out;
}

} // namespace rael
