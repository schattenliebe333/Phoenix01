// ═══════════════════════════════════════════════════════════════════════════════
// RAEL V49.0 — MACHINE BINDING (Windows 11)
// Hardware-Fingerprint basierte Lizenzierung
// Bindet die Software beim ersten Start an die Hardware
// ═══════════════════════════════════════════════════════════════════════════════
#pragma once

#ifdef _WIN32

#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <cstdint>

// Windows Headers
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <intrin.h>
#include <iphlpapi.h>
#include <shlobj.h>

#pragma comment(lib, "iphlpapi.lib")

namespace rael {
namespace machine {

// ═══════════════════════════════════════════════════════════════════════════════
// SHA256 für Fingerprint-Hash (minimale Implementierung)
// ═══════════════════════════════════════════════════════════════════════════════

class SHA256 {
public:
    static std::string hash(const std::string& input) {
        uint32_t h[8] = {
            0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a,
            0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19
        };

        static const uint32_t k[64] = {
            0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5,
            0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
            0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3,
            0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
            0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc,
            0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
            0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7,
            0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
            0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13,
            0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
            0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3,
            0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
            0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5,
            0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
            0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208,
            0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2
        };

        std::string padded = input;
        size_t original_len = input.length();
        size_t bit_len = original_len * 8;

        padded += static_cast<char>(0x80);
        while ((padded.length() % 64) != 56) {
            padded += static_cast<char>(0x00);
        }

        for (int i = 7; i >= 0; --i) {
            padded += static_cast<char>((bit_len >> (i * 8)) & 0xff);
        }

        for (size_t chunk = 0; chunk < padded.length(); chunk += 64) {
            uint32_t w[64];
            for (int i = 0; i < 16; ++i) {
                w[i] = (static_cast<uint8_t>(padded[chunk + i * 4]) << 24) |
                       (static_cast<uint8_t>(padded[chunk + i * 4 + 1]) << 16) |
                       (static_cast<uint8_t>(padded[chunk + i * 4 + 2]) << 8) |
                       static_cast<uint8_t>(padded[chunk + i * 4 + 3]);
            }

            for (int i = 16; i < 64; ++i) {
                uint32_t s0 = rotr(w[i-15], 7) ^ rotr(w[i-15], 18) ^ (w[i-15] >> 3);
                uint32_t s1 = rotr(w[i-2], 17) ^ rotr(w[i-2], 19) ^ (w[i-2] >> 10);
                w[i] = w[i-16] + s0 + w[i-7] + s1;
            }

            uint32_t a = h[0], b = h[1], c = h[2], d = h[3];
            uint32_t e = h[4], f = h[5], g = h[6], hh = h[7];

            for (int i = 0; i < 64; ++i) {
                uint32_t S1 = rotr(e, 6) ^ rotr(e, 11) ^ rotr(e, 25);
                uint32_t ch = (e & f) ^ (~e & g);
                uint32_t temp1 = hh + S1 + ch + k[i] + w[i];
                uint32_t S0 = rotr(a, 2) ^ rotr(a, 13) ^ rotr(a, 22);
                uint32_t maj = (a & b) ^ (a & c) ^ (b & c);
                uint32_t temp2 = S0 + maj;

                hh = g; g = f; f = e; e = d + temp1;
                d = c; c = b; b = a; a = temp1 + temp2;
            }

            h[0] += a; h[1] += b; h[2] += c; h[3] += d;
            h[4] += e; h[5] += f; h[6] += g; h[7] += hh;
        }

        std::ostringstream result;
        for (int i = 0; i < 8; ++i) {
            result << std::hex << std::setfill('0') << std::setw(8) << h[i];
        }
        return result.str();
    }

private:
    static uint32_t rotr(uint32_t x, int n) {
        return (x >> n) | (x << (32 - n));
    }
};

// ═══════════════════════════════════════════════════════════════════════════════
// HARDWARE FINGERPRINT SAMMLUNG
// ═══════════════════════════════════════════════════════════════════════════════

inline std::string get_cpu_id() {
    int cpuInfo[4] = {0};
    __cpuid(cpuInfo, 0);

    std::ostringstream ss;
    ss << std::hex << cpuInfo[0] << cpuInfo[1] << cpuInfo[2] << cpuInfo[3];

    __cpuid(cpuInfo, 1);
    ss << cpuInfo[0] << cpuInfo[3];

    return ss.str();
}

inline std::string get_volume_serial() {
    DWORD serial = 0;
    char volumeName[MAX_PATH + 1] = {0};
    char fsName[MAX_PATH + 1] = {0};

    if (GetVolumeInformationA("C:\\", volumeName, MAX_PATH, &serial,
                               nullptr, nullptr, fsName, MAX_PATH)) {
        std::ostringstream ss;
        ss << std::hex << serial;
        return ss.str();
    }
    return "unknown_volume";
}

inline std::string get_mac_address() {
    IP_ADAPTER_INFO adapterInfo[16];
    DWORD bufLen = sizeof(adapterInfo);

    if (GetAdaptersInfo(adapterInfo, &bufLen) == ERROR_SUCCESS) {
        std::ostringstream ss;
        for (int i = 0; i < 6; ++i) {
            ss << std::hex << std::setfill('0') << std::setw(2)
               << static_cast<int>(adapterInfo[0].Address[i]);
        }
        return ss.str();
    }
    return "unknown_mac";
}

inline std::string get_computer_name() {
    char buffer[MAX_COMPUTERNAME_LENGTH + 1];
    DWORD size = sizeof(buffer);
    if (GetComputerNameA(buffer, &size)) {
        return std::string(buffer);
    }
    return "unknown_computer";
}

inline std::string get_username() {
    char buffer[256];
    DWORD size = sizeof(buffer);
    if (GetUserNameA(buffer, &size)) {
        return std::string(buffer);
    }
    return "unknown_user";
}

inline std::string get_windows_product_id() {
    HKEY hKey;
    char buffer[256] = {0};
    DWORD bufferSize = sizeof(buffer);

    if (RegOpenKeyExA(HKEY_LOCAL_MACHINE,
                       "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion",
                       0, KEY_READ, &hKey) == ERROR_SUCCESS) {
        RegQueryValueExA(hKey, "ProductId", nullptr, nullptr,
                         reinterpret_cast<LPBYTE>(buffer), &bufferSize);
        RegCloseKey(hKey);
        return std::string(buffer);
    }
    return "unknown_product";
}

// ═══════════════════════════════════════════════════════════════════════════════
// MACHINE FINGERPRINT
// ═══════════════════════════════════════════════════════════════════════════════

struct MachineFingerprint {
    std::string cpu_id;
    std::string volume_serial;
    std::string mac_address;
    std::string computer_name;
    std::string username;
    std::string product_id;
    std::string combined_hash;

    void collect() {
        cpu_id = get_cpu_id();
        volume_serial = get_volume_serial();
        mac_address = get_mac_address();
        computer_name = get_computer_name();
        username = get_username();
        product_id = get_windows_product_id();

        // Kombinierter Hash aus allen Werten
        std::string combined = cpu_id + "|" + volume_serial + "|" +
                               mac_address + "|" + computer_name + "|" +
                               product_id + "|RAEL_V49_SANG_REAL_88";
        combined_hash = SHA256::hash(combined);
    }

    std::string to_string() const {
        std::ostringstream ss;
        ss << "CPU: " << cpu_id << "\n";
        ss << "Volume: " << volume_serial << "\n";
        ss << "MAC: " << mac_address << "\n";
        ss << "Computer: " << computer_name << "\n";
        ss << "User: " << username << "\n";
        ss << "ProductID: " << product_id << "\n";
        ss << "Hash: " << combined_hash << "\n";
        return ss.str();
    }
};

// ═══════════════════════════════════════════════════════════════════════════════
// LICENSE MANAGER
// ═══════════════════════════════════════════════════════════════════════════════

class LicenseManager {
public:
    static std::string get_license_path() {
        char path[MAX_PATH];
        if (SUCCEEDED(SHGetFolderPathA(nullptr, CSIDL_LOCAL_APPDATA, nullptr, 0, path))) {
            std::string licensePath = std::string(path) + "\\RAEL";
            CreateDirectoryA(licensePath.c_str(), nullptr);
            return licensePath + "\\rael_license.dat";
        }
        return "rael_license.dat";
    }

    static bool license_exists() {
        std::string path = get_license_path();
        std::ifstream file(path);
        return file.good();
    }

    static bool save_license(const MachineFingerprint& fp) {
        std::string path = get_license_path();
        std::ofstream file(path, std::ios::binary);
        if (!file) return false;

        // Verschlüsselter Header
        file << "RAEL_V49_LICENSE_1.0\n";
        file << "SANG_REAL_88\n";
        file << "BINDING_HASH:" << fp.combined_hash << "\n";
        file << "CREATED:" << time(nullptr) << "\n";

        // XOR-verschlüsselte Daten (einfache Obfuskation)
        std::string data = fp.cpu_id + "|" + fp.volume_serial + "|" +
                          fp.mac_address + "|" + fp.computer_name;
        std::string key = "RAEL_PHOENIX_88_SANG_REAL";
        std::string encrypted;
        for (size_t i = 0; i < data.length(); ++i) {
            encrypted += static_cast<char>(data[i] ^ key[i % key.length()]);
        }

        file << "DATA:" << SHA256::hash(encrypted) << "\n";
        file << "SIGNATURE:" << SHA256::hash(fp.combined_hash + "RAEL_MASTER") << "\n";

        return true;
    }

    static bool verify_license() {
        std::string path = get_license_path();
        std::ifstream file(path);
        if (!file) return false;

        std::string line;
        std::string stored_hash;

        while (std::getline(file, line)) {
            if (line.find("BINDING_HASH:") == 0) {
                stored_hash = line.substr(13);
                break;
            }
        }

        if (stored_hash.empty()) return false;

        // Aktuellen Fingerprint sammeln und vergleichen
        MachineFingerprint current;
        current.collect();

        return current.combined_hash == stored_hash;
    }

    static std::string get_stored_hash() {
        std::string path = get_license_path();
        std::ifstream file(path);
        if (!file) return "";

        std::string line;
        while (std::getline(file, line)) {
            if (line.find("BINDING_HASH:") == 0) {
                return line.substr(13);
            }
        }
        return "";
    }
};

// ═══════════════════════════════════════════════════════════════════════════════
// BINDING STATUS
// ═══════════════════════════════════════════════════════════════════════════════

enum class BindingStatus {
    NOT_BOUND,          // Erste Ausführung, noch nicht gebunden
    BOUND_VALID,        // Gebunden und Maschine stimmt überein
    BOUND_INVALID,      // Gebunden aber Maschine stimmt nicht überein
    LICENSE_CORRUPTED   // Lizenzdatei beschädigt
};

inline BindingStatus check_binding() {
    if (!LicenseManager::license_exists()) {
        return BindingStatus::NOT_BOUND;
    }

    if (LicenseManager::verify_license()) {
        return BindingStatus::BOUND_VALID;
    }

    std::string stored = LicenseManager::get_stored_hash();
    if (stored.empty()) {
        return BindingStatus::LICENSE_CORRUPTED;
    }

    return BindingStatus::BOUND_INVALID;
}

inline bool perform_first_binding() {
    MachineFingerprint fp;
    fp.collect();
    return LicenseManager::save_license(fp);
}

inline std::string get_binding_info() {
    MachineFingerprint fp;
    fp.collect();

    std::ostringstream ss;
    ss << "═══════════════════════════════════════════════════\n";
    ss << "  RAEL V49 - MACHINE BINDING INFO\n";
    ss << "═══════════════════════════════════════════════════\n";
    ss << "Computer: " << fp.computer_name << "\n";
    ss << "User: " << fp.username << "\n";
    ss << "Fingerprint: " << fp.combined_hash.substr(0, 16) << "...\n";
    ss << "License: " << LicenseManager::get_license_path() << "\n";
    ss << "═══════════════════════════════════════════════════\n";
    return ss.str();
}

} // namespace machine
} // namespace rael

#endif // _WIN32
