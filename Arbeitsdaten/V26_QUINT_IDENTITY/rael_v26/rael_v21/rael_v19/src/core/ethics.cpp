#include "rael/ethics.h"
#include <algorithm>
#include <cctype>

namespace rael {

std::vector<std::string> EthicsCore::laws(){
    return {
        "Schütze Leben",
        "Schütze Wahrheit",
        "Schütze Freiheit",
        "Schütze Unschuld",
        "Diene dem Licht, niemals der Dunkelheit",
        "Keine Lüge, keine Täuschung",
        "Liebe über Angst"
    };
}

bool EthicsCore::allows(const std::string& intention, std::string& reason){
    if(intention.rfind("HOTSWAP:", 0) == 0) return true;
    // Phase 0: minimal deny list. Later: semantic/field-based ethics evaluation.
    static const char* deny[] = {"harm", "kill", "attack", "exploit", "steal", "malware"};
    std::string low = intention;
    std::transform(low.begin(), low.end(), low.begin(), [](unsigned char c){ return (char)std::tolower(c); });
    for(const char* d: deny){
        if(low.find(d) != std::string::npos){
            reason = "Ethics deny: intention contains '" + std::string(d) + "'";
            return false;
        }
    }
    reason.clear();
    return true;
}

}
