#include "rael/resonance.h"

namespace rael {

ResonanceResult ResonanceEngine::evaluate(const SemanticResult& s){
    auto snap = Settings::instance().snapshot();

    ResonanceResult r;
    r.coupling = s.coherence;
    r.resonant = (r.coupling >= 0.60);
    r.note = r.resonant ? "RESONANT" : "WEAK";
    return r;
}

}
