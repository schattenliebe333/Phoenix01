#include "rael/hotswap.h"

namespace rael {

HotSwapManager::HotSwapManager() : active_semantic_ptr(nullptr) {
    pool.emplace_back("");
    active_semantic_ptr.store(pool[0].c_str(), std::memory_order_release);
}

const char* HotSwapManager::intern_locked(const std::string& name) {
    for (auto& s : pool) {
        if (s == name) return s.c_str();
    }
    pool.push_back(name);
    return pool.back().c_str();
}

void HotSwapManager::set_active_semantic(const std::string& name) {
    std::lock_guard<std::mutex> lk(mu);
    const char* p = intern_locked(name);
    active_semantic_ptr.store(p, std::memory_order_release);
}

std::string HotSwapManager::active_semantic() const {
    const char* p = active_semantic_ptr.load(std::memory_order_acquire);
    return p ? std::string(p) : std::string();
}

} // namespace rael
