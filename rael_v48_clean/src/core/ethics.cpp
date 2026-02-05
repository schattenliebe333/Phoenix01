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

// SECURITY (Audit fix): Validate HOTSWAP requests
// Only internal module loading should use HOTSWAP prefix, and it must be validated
static bool is_valid_hotswap_request(const std::string& intention) {
    // HOTSWAP: must be followed by a valid module path pattern
    // Format: HOTSWAP:module_name or HOTSWAP:/path/to/module.so
    if (intention.rfind("HOTSWAP:", 0) != 0) return false;

    std::string module_spec = intention.substr(8);  // Remove "HOTSWAP:" prefix

    // Must not be empty
    if (module_spec.empty()) return false;

    // Must not contain shell metacharacters (prevent injection)
    static const std::string forbidden = ";|&$`\\\"'<>(){}[]!#*?\n\r";
    for (char c : module_spec) {
        if (forbidden.find(c) != std::string::npos) return false;
    }

    // Must not contain path traversal
    if (module_spec.find("..") != std::string::npos) return false;

    return true;
}

bool EthicsCore::allows(const std::string& intention, std::string& reason){
    // SECURITY (Audit fix): HOTSWAP bypass now requires validation
    // Previously this was an unconditional bypass - now we validate the request
    if(intention.rfind("HOTSWAP:", 0) == 0) {
        if (is_valid_hotswap_request(intention)) {
            reason.clear();
            return true;
        } else {
            reason = "Ethics deny: invalid HOTSWAP request format";
            return false;
        }
    }

    // Phase 0: expanded deny list with more comprehensive patterns
    // (F-13 audit: expanded from 6 to 20+ patterns including synonyms)
    static const char* deny[] = {
        "harm", "kill", "attack", "exploit", "steal", "malware",
        "damage", "destroy", "inject", "hijack", "corrupt", "abuse",
        "ransom", "phish", "spoof", "ddos", "bruteforce", "crack",
        "keylog", "backdoor", "trojan", "rootkit", "worm", "virus"
    };
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
