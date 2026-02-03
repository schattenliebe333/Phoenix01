#pragma once
#include <string>

namespace rael {

class CoreRing;

class Pack {
public:
    // expects pack file "pack.rael" (key=value lines) and optional modules folder
    static bool verify(const CoreRing& core, const std::string& packfile, std::string& err);
    static bool install(const CoreRing& core, const std::string& packfile, const std::string& install_dir, std::string& err);

private:
    static bool read_all(const std::string& path, std::string& out, std::string& err);
    static bool parse_kv(const std::string& text, const std::string& key, std::string& val);
};

} // namespace rael
