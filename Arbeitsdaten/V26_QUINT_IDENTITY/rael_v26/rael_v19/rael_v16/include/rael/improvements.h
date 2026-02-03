#pragma once
#include <string>
#include <vector>
#include <cstdint>

namespace rael {

// Global human-in-the-loop improvement record.
// IMPORTANT: Improvements are suggestions only; applying them is always a human decision.
struct Improvement {
    uint64_t id = 0;
    std::string ts;        // ISO8601
    std::string src;       // REFLECT | USER_TASK | LIVE_DEFENSE | SELF_OPT | LEARNING | OTHER
    int importance = 5;    // 1..10
    int risk = 3;          // 1..10
    double confidence = 0; // 0..1
    std::string title;
    std::string problem;
    std::string rationale;
    std::string testplan;
    std::string code;      // optional (snippet)
    std::string status;    // PENDING | APPROVED | REJECTED | DEFERRED
};

class ImprovementBus final {
public:
    // Adds an improvement suggestion, persists it, and emits a GUI-friendly event line.
    // Returns the assigned id.
    static uint64_t emit(Improvement imp);

    // Returns the last n improvements (most recent first).
    static std::vector<Improvement> last(size_t n);

    // Persistence path used by the core (jsonl).
    static std::string storage_path();
};

} // namespace rael
