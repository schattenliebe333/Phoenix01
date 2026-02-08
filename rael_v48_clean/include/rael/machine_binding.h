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

// V49: NIST SHA-256 ENTFERNT - PhiHash Wrapper
#include "rael/rst_crypto.hpp"

class SHA256 {
public:
    static std::string hash(const std::string& input) {
        return crypto::PhiHash::to_hex(crypto::PhiHash::hash(input));
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
