#include "rael/sha256.h"
#include <sstream>
#include <iomanip>

namespace rael {

static const uint32_t K[64] = {
  0x428a2f98,0x71374491,0xb5c0fbcf,0xe9b5dba5,0x3956c25b,0x59f111f1,0x923f82a4,0xab1c5ed5,
  0xd807aa98,0x12835b01,0x243185be,0x550c7dc3,0x72be5d74,0x80deb1fe,0x9bdc06a7,0xc19bf174,
  0xe49b69c1,0xefbe4786,0x0fc19dc6,0x240ca1cc,0x2de92c6f,0x4a7484aa,0x5cb0a9dc,0x76f988da,
  0x983e5152,0xa831c66d,0xb00327c8,0xbf597fc7,0xc6e00bf3,0xd5a79147,0x06ca6351,0x14292967,
  0x27b70a85,0x2e1b2138,0x4d2c6dfc,0x53380d13,0x650a7354,0x766a0abb,0x81c2c92e,0x92722c85,
  0xa2bfe8a1,0xa81a664b,0xc24b8b70,0xc76c51a3,0xd192e819,0xd6990624,0xf40e3585,0x106aa070,
  0x19a4c116,0x1e376c08,0x2748774c,0x34b0bcb5,0x391c0cb3,0x4ed8aa4a,0x5b9cca4f,0x682e6ff3,
  0x748f82ee,0x78a5636f,0x84c87814,0x8cc70208,0x90befffa,0xa4506ceb,0xbef9a3f7,0xc67178f2
};

std::array<uint8_t,32> SHA256::digest(const std::vector<uint8_t>& v){
    return digest(v.data(), v.size());
}
std::array<uint8_t,32> SHA256::digest(const std::string& s){
    return digest(reinterpret_cast<const uint8_t*>(s.data()), s.size());
}

std::array<uint8_t,32> SHA256::digest(const uint8_t* data, size_t len){
    uint32_t H[8] = {
        0x6a09e667,0xbb67ae85,0x3c6ef372,0xa54ff53a,
        0x510e527f,0x9b05688c,0x1f83d9ab,0x5be0cd19
    };

    // preprocess
    uint64_t bitlen = (uint64_t)len * 8ULL;
    size_t new_len = len + 1;
    while((new_len % 64) != 56) new_len++;
    std::vector<uint8_t> msg(new_len + 8, 0);
    for(size_t i=0;i<len;i++) msg[i]=data[i];
    msg[len]=0x80;
    for(int i=0;i<8;i++){
        msg[new_len + i] = (uint8_t)((bitlen >> (56 - 8*i)) & 0xFF);
    }

    // process chunks
    for(size_t off=0; off<msg.size(); off += 64){
        uint32_t w[64];
        for(int i=0;i<16;i++){
            w[i] = (uint32_t)msg[off + 4*i] << 24 |
                   (uint32_t)msg[off + 4*i + 1] << 16 |
                   (uint32_t)msg[off + 4*i + 2] << 8 |
                   (uint32_t)msg[off + 4*i + 3];
        }
        for(int i=16;i<64;i++){
            w[i] = ssig1(w[i-2]) + w[i-7] + ssig0(w[i-15]) + w[i-16];
        }

        uint32_t a=H[0],b=H[1],c=H[2],d=H[3],e=H[4],f=H[5],g=H[6],h=H[7];
        for(int i=0;i<64;i++){
            uint32_t T1 = h + bsig1(e) + ch(e,f,g) + K[i] + w[i];
            uint32_t T2 = bsig0(a) + maj(a,b,c);
            h=g; g=f; f=e; e=d + T1;
            d=c; c=b; b=a; a=T1 + T2;
        }
        H[0]+=a; H[1]+=b; H[2]+=c; H[3]+=d;
        H[4]+=e; H[5]+=f; H[6]+=g; H[7]+=h;
    }

    std::array<uint8_t,32> out{};
    for(int i=0;i<8;i++){
        out[4*i]   = (uint8_t)((H[i]>>24)&0xFF);
        out[4*i+1] = (uint8_t)((H[i]>>16)&0xFF);
        out[4*i+2] = (uint8_t)((H[i]>>8)&0xFF);
        out[4*i+3] = (uint8_t)((H[i])&0xFF);
    }
    return out;
}

std::string SHA256::hex(const std::array<uint8_t,32>& d){
    std::ostringstream oss;
    oss << std::hex << std::setfill('0');
    for(auto b: d) oss << std::setw(2) << (int)b;
    return oss.str();
}

} // namespace rael
