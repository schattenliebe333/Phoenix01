#include "rael/util.h"
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

/* Minimal SHA-256 implementation (compact, no deps).
   Used only to produce stable content IDs for attachments in Phase 0.
*/
namespace {
    inline uint32_t rotr(uint32_t x, uint32_t n){ return (x>>n) | (x<<(32-n)); }
    inline uint32_t ch(uint32_t x,uint32_t y,uint32_t z){ return (x&y) ^ (~x&z); }
    inline uint32_t maj(uint32_t x,uint32_t y,uint32_t z){ return (x&y) ^ (x&z) ^ (y&z); }
    inline uint32_t bsig0(uint32_t x){ return rotr(x,2)^rotr(x,13)^rotr(x,22); }
    inline uint32_t bsig1(uint32_t x){ return rotr(x,6)^rotr(x,11)^rotr(x,25); }
    inline uint32_t ssig0(uint32_t x){ return rotr(x,7)^rotr(x,18)^(x>>3); }
    inline uint32_t ssig1(uint32_t x){ return rotr(x,17)^rotr(x,19)^(x>>10); }

    const uint32_t K[64] = {
        0x428a2f98,0x71374491,0xb5c0fbcf,0xe9b5dba5,0x3956c25b,0x59f111f1,0x923f82a4,0xab1c5ed5,
        0xd807aa98,0x12835b01,0x243185be,0x550c7dc3,0x72be5d74,0x80deb1fe,0x9bdc06a7,0xc19bf174,
        0xe49b69c1,0xefbe4786,0x0fc19dc6,0x240ca1cc,0x2de92c6f,0x4a7484aa,0x5cb0a9dc,0x76f988da,
        0x983e5152,0xa831c66d,0xb00327c8,0xbf597fc7,0xc6e00bf3,0xd5a79147,0x06ca6351,0x14292967,
        0x27b70a85,0x2e1b2138,0x4d2c6dfc,0x53380d13,0x650a7354,0x766a0abb,0x81c2c92e,0x92722c85,
        0xa2bfe8a1,0xa81a664b,0xc24b8b70,0xc76c51a3,0xd192e819,0xd6990624,0xf40e3585,0x106aa070,
        0x19a4c116,0x1e376c08,0x2748774c,0x34b0bcb5,0x391c0cb3,0x4ed8aa4a,0x5b9cca4f,0x682e6ff3,
        0x748f82ee,0x78a5636f,0x84c87814,0x8cc70208,0x90befffa,0xa4506ceb,0xbef9a3f7,0xc67178f2
    };

    void sha256(const uint8_t* msg, size_t len, uint8_t out[32]){
        uint64_t bitlen = (uint64_t)len * 8ull;
        size_t new_len = len + 1;
        while((new_len % 64) != 56) new_len++;
        std::vector<uint8_t> data(new_len + 8, 0);
        for(size_t i=0;i<len;i++) data[i]=msg[i];
        data[len] = 0x80;
        for(int i=0;i<8;i++){
            data[new_len + i] = (uint8_t)((bitlen >> (56 - 8*i)) & 0xff);
        }

        uint32_t h[8] = {
            0x6a09e667,0xbb67ae85,0x3c6ef372,0xa54ff53a,
            0x510e527f,0x9b05688c,0x1f83d9ab,0x5be0cd19
        };

        uint32_t w[64];
        for(size_t chunk=0; chunk < data.size(); chunk += 64){
            for(int i=0;i<16;i++){
                w[i] = (data[chunk+4*i]<<24) | (data[chunk+4*i+1]<<16) | (data[chunk+4*i+2]<<8) | (data[chunk+4*i+3]);
            }
            for(int i=16;i<64;i++){
                w[i] = ssig1(w[i-2]) + w[i-7] + ssig0(w[i-15]) + w[i-16];
            }
            uint32_t a=h[0],b=h[1],c=h[2],d=h[3],e=h[4],f=h[5],g=h[6],hh=h[7];
            for(int i=0;i<64;i++){
                uint32_t t1 = hh + bsig1(e) + ch(e,f,g) + K[i] + w[i];
                uint32_t t2 = bsig0(a) + maj(a,b,c);
                hh = g;
                g = f;
                f = e;
                e = d + t1;
                d = c;
                c = b;
                b = a;
                a = t1 + t2;
            }
            h[0]+=a;h[1]+=b;h[2]+=c;h[3]+=d;h[4]+=e;h[5]+=f;h[6]+=g;h[7]+=hh;
        }

        for(int i=0;i<8;i++){
            out[4*i]   = (uint8_t)((h[i]>>24)&0xff);
            out[4*i+1] = (uint8_t)((h[i]>>16)&0xff);
            out[4*i+2] = (uint8_t)((h[i]>>8)&0xff);
            out[4*i+3] = (uint8_t)(h[i]&0xff);
        }
    }

    std::string hex32(const uint8_t* b, size_t n){
        static const char* hexd="0123456789abcdef";
        std::string s; s.reserve(n*2);
        for(size_t i=0;i<n;i++){
            s.push_back(hexd[(b[i]>>4)&0xF]);
            s.push_back(hexd[b[i]&0xF]);
        }
        return s;
    }
}

std::string sha256_bytes_hex(const uint8_t* data, size_t n){
    uint8_t out[32]; sha256(data,n,out);
    return hex32(out,32);
}

std::string sha256_file_hex(const std::string& path){
    std::ifstream f(path, std::ios::binary);
    if(!f) return "";
    std::vector<uint8_t> buf((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
    if(buf.empty()) return sha256_bytes_hex(nullptr, 0);
    return sha256_bytes_hex(buf.data(), buf.size());
}

} // namespace rael
