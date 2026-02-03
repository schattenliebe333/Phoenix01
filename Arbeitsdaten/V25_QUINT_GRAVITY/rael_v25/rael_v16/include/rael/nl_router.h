#pragma once

#include <string>

namespace rael {

// Lightweight natural-language router.
//
// Purpose:
// - User can say things in free-form German/English.
// - RAEL recognizes a small set of high-value intents (e.g. "top 10 improvements").
// - Router emits structured Improvements so the user can approve/reject.
//
// Safety:
// - Deterministic + offline.
// - MUST NOT perform network/system actions.
// - May ONLY emit Improvements (suggestions).

struct NLRouterResult {
    bool handled = false;
    std::string response;
};

class NaturalLanguageRouter {
public:
    NLRouterResult route_and_emit(const std::string& raw_input) const;
};

} // namespace rael
