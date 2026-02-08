#include "rael/util.h"
#include "rael/rst_crypto.hpp"  // V49: PhiHash ersetzt inline SHA-256
#include <algorithm>
#include <chrono>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <cctype>

namespace rael {

std::string trim(const std::string& s){
    auto b = s.begin();
    while(b!=s.end() && std::isspace((unsigned char)*b)) ++b;
    auto e = s.end();
    while(e!=b && std::isspace((unsigned char)*(e-1))) --e;
    return std::string(b,e);
}

std::vector<std::string> split_ws(const std::string& s){
    std::istringstream iss(s);
    std::vector<std::string> out;
    std::string tok;
    while(iss >> tok) out.push_back(tok);
    return out;
}

std::string now_iso8601(){
    using namespace std::chrono;
    auto t = system_clock::to_time_t(system_clock::now());
    std::tm tm{};
#if defined(_WIN32)
    gmtime_s(&tm, &t);
#else
    gmtime_r(&t, &tm);
#endif
    std::ostringstream oss;
    oss << std::put_time(&tm, "%Y-%m-%dT%H:%M:%SZ");
    return oss.str();
}

// V49: Inline SHA-256 ENTFERNT - PhiHash ersetzt alles

std::string sha256_bytes_hex(const uint8_t* data, size_t n){
    auto digest = crypto::PhiHash::hash(data, n);
    return crypto::PhiHash::to_hex(digest);
}

std::string sha256_file_hex(const std::string& path){
    std::ifstream f(path, std::ios::binary);
    if(!f) return "";
    std::vector<uint8_t> buf((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
    if(buf.empty()) return sha256_bytes_hex(nullptr, 0);
    return sha256_bytes_hex(buf.data(), buf.size());
}

} // namespace rael
