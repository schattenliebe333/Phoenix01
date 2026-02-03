#pragma once
#include <array>
#include <cstdint>
#include <string>
#include <vector>

namespace rael {

class SHA256 {
public:
    static std::array<uint8_t,32> digest(const uint8_t* data, size_t len);
    static std::array<uint8_t,32> digest(const std::vector<uint8_t>& v);
    static std::array<uint8_t,32> digest(const std::string& s);
    static std::string hex(const std::array<uint8_t,32>& d);

private:
    static inline uint32_t rotr(uint32_t x, uint32_t n){ return (x>>n) | (x<<(32-n)); }
    static inline uint32_t ch(uint32_t x,uint32_t y,uint32_t z){ return (x & y) ^ (~x & z); }
    static inline uint32_t maj(uint32_t x,uint32_t y,uint32_t z){ return (x & y) ^ (x & z) ^ (y & z); }
    static inline uint32_t bsig0(uint32_t x){ return rotr(x,2) ^ rotr(x,13) ^ rotr(x,22); }
    static inline uint32_t bsig1(uint32_t x){ return rotr(x,6) ^ rotr(x,11) ^ rotr(x,25); }
    static inline uint32_t ssig0(uint32_t x){ return rotr(x,7) ^ rotr(x,18) ^ (x>>3); }
    static inline uint32_t ssig1(uint32_t x){ return rotr(x,17) ^ rotr(x,19) ^ (x>>10); }
};

} // namespace rael
