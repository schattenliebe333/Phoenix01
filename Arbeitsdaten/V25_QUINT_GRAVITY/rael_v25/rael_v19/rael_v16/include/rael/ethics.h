#pragma once
#include <string>
#include <vector>

namespace rael {

class EthicsCore final {
public:
    static std::vector<std::string> laws();
    static bool allows(const std::string& intention, std::string& reason);
};

}
