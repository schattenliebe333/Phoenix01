#pragma once

#include <string>

namespace rael {

struct SemanticResult;
struct ResonanceResult;

enum class ActionOutcome {
    OK,
    WEAK_RESONANCE,
    ETHIK_BLOCK
};

// Lightweight, non-blocking reflection core.
// Purpose:
//  - Extract "what happened" + "why" (intent) + "what could have happened" (paths)
//  - Emit only Suggestions (Improvements), never actions.
//
// Design constraints:
//  - Must be fast and rate-limited.
//  - Uses global norms (QUELLE/TOR/κ) where helpful for scoring.
class ReflectionEngine final {
public:
    // Analyze the latest outcome and (optionally) emit a REFLECT improvement.
    // Returns true if an improvement was emitted.
    static bool reflect_and_emit(const SemanticResult& s,
                                 const ResonanceResult& r,
                                 ActionOutcome outcome,
                                 const std::string& raw_input);
};

} // namespace rael
