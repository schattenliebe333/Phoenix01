#pragma once
#include <string>
#include "rael/settings.h"
#include "rael/semantic.h"

namespace rael {

struct ResonanceResult {
    bool resonant = false;
    double coupling = 0.0; // 0..1
    std::string note;
};

class ResonanceEngine {
public:
    ResonanceResult evaluate(const SemanticResult& s);
};

}
