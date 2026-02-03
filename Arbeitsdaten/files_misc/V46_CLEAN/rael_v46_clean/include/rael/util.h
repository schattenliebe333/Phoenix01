#pragma once
#include <string>
#include <vector>
#include <cstdint>

namespace rael {

std::string trim(const std::string& s);
std::vector<std::string> split_ws(const std::string& s);
std::string now_iso8601();

// Minimal SHA-256 helper (Phase 0). Later we can swap to RST-Hash without deps.
std::string sha256_file_hex(const std::string& path);
std::string sha256_bytes_hex(const uint8_t* data, size_t n);

}
