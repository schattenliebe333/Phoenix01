#pragma once

#include <atomic>
#include <mutex>
#include <string>
#include <vector>

namespace rael {

// Keeps the currently active semantic module name.
// Names are interned so the atomic holds a stable pointer.
class HotSwapManager {
public:
    HotSwapManager();

    void set_active_semantic(const std::string& name);
    std::string active_semantic() const;

private:
    const char* intern_locked(const std::string& name);

    mutable std::mutex mu;
    std::vector<std::string> pool; // never erased -> stable c_str()
    std::atomic<const char*> active_semantic_ptr;
};

} // namespace rael
