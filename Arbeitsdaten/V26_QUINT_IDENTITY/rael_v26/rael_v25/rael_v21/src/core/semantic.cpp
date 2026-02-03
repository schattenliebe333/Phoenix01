#include "rael/semantic.h"
#include "rael/util.h"

namespace rael {

SemanticResult SemanticEngine::interpret(const std::string& input){
    SemanticResult r;
    r.intention = trim(input);
    if(r.intention.empty()){
        r.meaning = "EMPTY";
        r.coherence = 0.0;
        return r;
    }
    // Phase 0: minimal semantic parse. Next phase will implement Quint-values.
    r.meaning = "TEXT:" + r.intention;
    r.coherence = (r.intention.size() >= 4) ? 0.72 : 0.41;
    return r;
}

}
