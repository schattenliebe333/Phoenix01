#pragma once
#include <string>
#include <vector>
#include <mutex>
#include <chrono>

namespace rael {

struct Event {
    uint64_t seq = 0;
    std::string ts;     // ISO8601
    std::string kind;   // e.g., "MODULE_LOAD", "HOTSWAP", "ETHIK_BLOCK"
    std::string detail; // free text
};

class EventBus {
public:
    static void push(const std::string& kind, const std::string& detail);
    static std::vector<Event> last(size_t n);

private:
    static std::mutex& mtx();
    static std::vector<Event>& buf();
    static uint64_t& seq();
    static size_t capacity();
};

} // namespace rael
