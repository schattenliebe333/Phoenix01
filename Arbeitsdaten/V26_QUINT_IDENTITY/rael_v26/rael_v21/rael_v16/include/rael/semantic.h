#pragma once
#include <string>

namespace rael {

struct SemanticResult {
    std::string intention;
    std::string meaning;
    double coherence = 0.0; // 0..1
};

class SemanticEngine {
public:
    SemanticResult interpret(const std::string& input);
};

}
