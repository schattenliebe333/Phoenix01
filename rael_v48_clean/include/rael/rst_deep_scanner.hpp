// ═══════════════════════════════════════════════════════════════════════════════
// RST OMEGA DEEP SCANNER - Allumfassender System-Scanner
// ═══════════════════════════════════════════════════════════════════════════════
//
// Scannt ALLES: Dateisysteme, Schattenpartitionen, Bootsektoren, RAM, Netzwerk
//
// RST KONSTANTEN (17 Dezimalstellen):
//   G1 = 0.55555555555555556 (5/9) - Bewusstsein → Realität
//   G2 = 0.33333333333333333 (3/9) - Realität → Bewusstsein
//   G3 = 0.11111111111111111 (1/9) - Entropie/Drift
//   SIGNATURE_88 = G1 + G3 = 0.66... NEIN! G1 + G2 = 0.888...
//
// WICHTIG: G5 + G3 + G1 = 1/9 + 3/9 + 5/9 = 9/9 = 1
//
// ═══════════════════════════════════════════════════════════════════════════════

#pragma once

#include <string>
#include <vector>
#include <map>
#include <set>
#include <array>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <cmath>
#include <cstdint>
#include <algorithm>
#include <functional>
#include <chrono>
#include <mutex>
#include <thread>
#include <atomic>
#include <filesystem>
#include <queue>

#ifdef _WIN32
#include <windows.h>
#include <tlhelp32.h>
#include <psapi.h>
#else
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>
#include <fcntl.h>
#endif

namespace rael {
namespace security {
namespace deep {

// ═══════════════════════════════════════════════════════════════════════════════
// RST PRÄZISIONS-KONSTANTEN (17 Dezimalstellen)
// ═══════════════════════════════════════════════════════════════════════════════

namespace rst {
    // Die Neuntel-Basis
    constexpr double G0 = 0.88888888888888889;  // 8/9 WAHRHEIT (Referenz)
    constexpr double G1 = 0.55555555555555556;  // 5/9 Bewusstsein → Realität
    constexpr double G2 = 0.44444444444444444;  // 4/9
    constexpr double G3 = 0.33333333333333333;  // 3/9 Realität → Bewusstsein
    constexpr double G4 = 0.22222222222222222;  // 2/9
    constexpr double G5 = 0.11111111111111111;  // 1/9 Entropie/Drift

    // WICHTIG: G5 + G3 + G1 = 1/9 + 3/9 + 5/9 = 9/9 = 1
    constexpr double G_SUM_ODD = 1.00000000000000000;

    // 88-Signatur = G0 (nicht G1+G2!)
    constexpr double SIGNATURE_88 = 0.88888888888888889;

    // Ratio G1/G3 = (5/9)/(3/9) = 5/3 = 1.666...
    constexpr double RATIO_G1_G3 = 1.66666666666666666;

    // Zeitkonstanten
    constexpr double TAU_1 = 3.64000000000000000;  // Bewusstseins-Zeitskala
    constexpr double TAU_2 = 4.33000000000000000;  // Realitäts-Zeitskala
    constexpr double TAU_3 = 14.8100000000000000;  // Strategische Zeitskala

    // Kritische Schwellen
    constexpr double COHERENCE_THRESHOLD = 0.77777777777777779;   // 7/9 - Verstärkung
    constexpr double MALICE_THRESHOLD = 1.73205080756887729;      // tan(π/3) = √3
    constexpr double PHASE_ATTACK_THRESHOLD = 2.51327412287183449; // 144° in rad
    constexpr double PERCOLATION_THRESHOLD = 0.33333333333333333;  // 1/3
    constexpr double QEC_THRESHOLD = 0.12500000000000000;          // 1/8

    // Entropy-Schwellen
    constexpr double ENTROPY_HIGH = 7.50000000000000000;
    constexpr double ENTROPY_SUSPICIOUS = 6.50000000000000000;

    // Pi mit Präzision
    constexpr double PI = 3.14159265358979323;
    constexpr double TWO_PI = 6.28318530717958647;
}

// ═══════════════════════════════════════════════════════════════════════════════
// THREAT SIGNATURE - Malware-Signatur mit RST-Erweiterung
// ═══════════════════════════════════════════════════════════════════════════════

struct ThreatSignature {
    std::string name;
    std::vector<uint8_t> pattern;
    double threat_level;      // 0-10
    std::string category;
    double rst_phase;         // RST Phase-Signatur
    bool is_polymorphic;

    ThreatSignature() : threat_level(0), rst_phase(0), is_polymorphic(false) {}

    ThreatSignature(const std::string& n, const std::string& p,
                   double level, const std::string& cat)
        : name(n), pattern(p.begin(), p.end()), threat_level(level),
          category(cat), rst_phase(0), is_polymorphic(false) {}

    ThreatSignature(const std::string& n, const std::vector<uint8_t>& p,
                   double level, const std::string& cat)
        : name(n), pattern(p), threat_level(level),
          category(cat), rst_phase(0), is_polymorphic(false) {}
};

// ═══════════════════════════════════════════════════════════════════════════════
// SCAN RESULT - Einzelnes Scan-Ergebnis
// ═══════════════════════════════════════════════════════════════════════════════

struct DeepScanResult {
    std::string path;
    std::string threat_name;
    double threat_level;
    std::string category;

    // RST Analyse
    double entropy;
    double coherence;
    double phase;
    double malice_index;
    bool has_signature_88;

    // Details
    std::string details;
    std::string action_taken;
    std::chrono::system_clock::time_point timestamp;

    DeepScanResult() : threat_level(0), entropy(0), coherence(0),
                       phase(0), malice_index(0), has_signature_88(false) {
        timestamp = std::chrono::system_clock::now();
    }
};

// ═══════════════════════════════════════════════════════════════════════════════
// THREAT DATABASE - Signatur-Datenbank mit RST-Erweiterung
// ═══════════════════════════════════════════════════════════════════════════════

class ThreatDatabase {
private:
    std::vector<ThreatSignature> signatures_;
    std::mutex mtx_;

public:
    ThreatDatabase() {
        load_signatures();
    }

    void load_signatures() {
        std::lock_guard<std::mutex> lock(mtx_);

        // ═══════════════════════════════════════════════════════════════════
        // RANSOMWARE SIGNATUREN
        // ═══════════════════════════════════════════════════════════════════
        add_sig("Generic.Ransomware", "ENCRYPTED", 9.5, "ransomware");
        add_sig("Locky.Ransomware", ".locky", 9.8, "ransomware");
        add_sig("WannaCry.Ransomware", ".wannacry", 10.0, "ransomware");
        add_sig("WannaCry.Variant", "WNCRY", 10.0, "ransomware");
        add_sig("CryptoLocker", ".cryptolocker", 9.7, "ransomware");
        add_sig("Petya.Ransomware", "PETYA", 9.9, "ransomware");
        add_sig("GoldenEye.Ransomware", "GOLDENEYE", 9.9, "ransomware");
        add_sig("LockBit.Ransomware", "lockbit", 9.8, "ransomware");
        add_sig("REvil.Ransomware", "sodinokibi", 9.9, "ransomware");
        add_sig("Ryuk.Ransomware", "RyukReadMe", 9.9, "ransomware");

        // ═══════════════════════════════════════════════════════════════════
        // TROJANER SIGNATUREN
        // ═══════════════════════════════════════════════════════════════════
        add_sig("Trojan.Injector", "CreateRemoteThread", 8.5, "trojan");
        add_sig("Trojan.Injector.Memory", "VirtualAllocEx", 8.3, "trojan");
        add_sig("Trojan.Injector.Process", "WriteProcessMemory", 8.4, "trojan");
        add_sig("Trojan.ProcessHollowing", "NtUnmapViewOfSection", 9.0, "trojan");
        add_sig("Trojan.Generic", "trojan", 7.0, "trojan");

        // ═══════════════════════════════════════════════════════════════════
        // BACKDOOR SIGNATUREN
        // ═══════════════════════════════════════════════════════════════════
        add_sig("Backdoor.CmdExec", "cmd.exe /c", 7.5, "backdoor");
        add_sig("Backdoor.ShellExec", "/bin/sh -c", 7.5, "backdoor");
        add_sig("Backdoor.PowerShell.Encoded", "powershell -e", 8.5, "backdoor");
        add_sig("Backdoor.Netcat", "nc -e", 8.0, "backdoor");
        add_sig("Backdoor.ReverseShell", "/dev/tcp/", 8.5, "backdoor");
        add_sig("Backdoor.BashTCP", "bash -i >&", 8.5, "backdoor");

        // ═══════════════════════════════════════════════════════════════════
        // ROOTKIT SIGNATUREN
        // ═══════════════════════════════════════════════════════════════════
        add_sig("Rootkit.Generic", "_root_", 9.0, "rootkit");
        add_sig("Rootkit.ProcessHider", "hide_process", 9.2, "rootkit");
        add_sig("Rootkit.SyscallHook", "hook_syscall", 9.5, "rootkit");
        add_sig("Rootkit.LDPreload", "LD_PRELOAD=", 9.3, "rootkit");
        add_sig("Rootkit.Diamorphine", "diamorphine", 9.8, "rootkit");
        add_sig("Rootkit.Reptile", "reptile_module", 9.8, "rootkit");

        // ═══════════════════════════════════════════════════════════════════
        // KEYLOGGER/SPYWARE SIGNATUREN
        // ═══════════════════════════════════════════════════════════════════
        add_sig("Keylogger.Generic", "GetAsyncKeyState", 7.0, "spyware");
        add_sig("Keylogger.Hook", "SetWindowsHookEx", 7.2, "spyware");
        add_sig("Keylogger.Obvious", "keylog", 7.5, "spyware");
        add_sig("Spyware.ScreenCapture", "BitBlt", 6.5, "spyware");

        // ═══════════════════════════════════════════════════════════════════
        // MINER SIGNATUREN
        // ═══════════════════════════════════════════════════════════════════
        add_sig("Miner.Stratum", "stratum+tcp://", 6.0, "miner");
        add_sig("Miner.XMRig", "xmrig", 6.5, "miner");
        add_sig("Miner.CryptoNight", "cryptonight", 6.3, "miner");
        add_sig("Miner.Pool", "pool.minexmr", 6.5, "miner");

        // ═══════════════════════════════════════════════════════════════════
        // C2 BEACON SIGNATUREN
        // ═══════════════════════════════════════════════════════════════════
        add_sig("C2.Beacon", "beacon", 8.0, "c2");
        add_sig("C2.CobaltStrike", "cobaltstrike", 9.5, "c2");
        add_sig("C2.Meterpreter", "meterpreter", 9.3, "c2");
        add_sig("C2.Empire", "empire-launcher", 9.0, "c2");

        // ═══════════════════════════════════════════════════════════════════
        // WEBSHELL SIGNATUREN
        // ═══════════════════════════════════════════════════════════════════
        add_sig("Webshell.PHPEval", "eval($_", 8.5, "webshell");
        add_sig("Webshell.PHPSystem", "system($_", 8.5, "webshell");
        add_sig("Webshell.ASPX", "<%@ Page", 8.0, "webshell");
        add_sig("Webshell.JSP", "<%@ page import", 8.0, "webshell");

        // ═══════════════════════════════════════════════════════════════════
        // EXPLOIT SIGNATUREN
        // ═══════════════════════════════════════════════════════════════════
        add_sig("Exploit.NOPSled", std::vector<uint8_t>{0x90, 0x90, 0x90, 0x90}, 8.0, "exploit");
        add_sig("Exploit.BreakpointSled", std::vector<uint8_t>{0xCC, 0xCC, 0xCC, 0xCC}, 7.5, "exploit");
        add_sig("Exploit.Shellcode", "shellcode", 8.5, "exploit");

        // ═══════════════════════════════════════════════════════════════════
        // PERSISTENCE SIGNATUREN
        // ═══════════════════════════════════════════════════════════════════
        add_sig("Persistence.Registry", "CurrentVersion\\Run", 6.5, "persistence");
        add_sig("Persistence.Cron", "/etc/cron", 6.0, "persistence");
        add_sig("Persistence.Systemd", "systemctl enable", 5.5, "persistence");

        // ═══════════════════════════════════════════════════════════════════
        // EXFILTRATION SIGNATUREN
        // ═══════════════════════════════════════════════════════════════════
        add_sig("Exfiltration.Generic", "exfil", 7.5, "exfil");
        add_sig("Exfiltration.Pastebin", "pastebin.com", 6.0, "exfil");
        add_sig("Exfiltration.Base64", "base64 -d", 5.0, "exfil");

        // ═══════════════════════════════════════════════════════════════════
        // BOOTKIT SIGNATUREN
        // ═══════════════════════════════════════════════════════════════════
        add_sig("Bootkit.Generic", "bootkit", 9.5, "bootkit");
        add_sig("Bootkit.MBRWiped", std::vector<uint8_t>(440, 0x00), 9.0, "bootkit");
    }

    void add_sig(const std::string& name, const std::string& pattern,
                 double level, const std::string& category) {
        ThreatSignature sig(name, pattern, level, category);
        signatures_.push_back(sig);
    }

    void add_sig(const std::string& name, const std::vector<uint8_t>& pattern,
                 double level, const std::string& category) {
        ThreatSignature sig(name, pattern, level, category);
        signatures_.push_back(sig);
    }

    // Sucht Signaturen in Daten
    std::vector<ThreatSignature> scan(const std::vector<uint8_t>& data) const {
        std::vector<ThreatSignature> found;

        for (const auto& sig : signatures_) {
            if (sig.pattern.size() > data.size()) continue;

            // Suche Pattern
            auto it = std::search(data.begin(), data.end(),
                                 sig.pattern.begin(), sig.pattern.end());
            if (it != data.end()) {
                found.push_back(sig);
            }
        }

        return found;
    }

    // Shannon Entropy berechnen
    static double calculate_entropy(const std::vector<uint8_t>& data) {
        if (data.empty()) return 0.0;

        std::array<size_t, 256> freq{};
        for (uint8_t byte : data) {
            freq[byte]++;
        }

        double entropy = 0.0;
        double size = static_cast<double>(data.size());

        for (size_t count : freq) {
            if (count > 0) {
                double p = static_cast<double>(count) / size;
                entropy -= p * std::log2(p);
            }
        }

        return entropy;
    }

    // RST Phasen-Analyse
    static double calculate_rst_phase(const std::vector<uint8_t>& data) {
        if (data.size() < 64) return 0.0;

        // Einfache Phasen-Schätzung basierend auf Byte-Verteilung
        double sum = 0.0;
        double sum_sq = 0.0;

        for (size_t i = 0; i < std::min(data.size(), size_t(1024)); ++i) {
            double val = (static_cast<double>(data[i]) - 128.0) / 128.0;
            sum += val;
            sum_sq += val * val;
        }

        double n = std::min(data.size(), size_t(1024));
        double mean = sum / n;
        double variance = (sum_sq / n) - (mean * mean);

        // Phase basierend auf Asymmetrie
        double phase = std::atan2(mean, std::sqrt(std::max(0.0, variance)));

        return phase;
    }

    // 88-Signatur prüfen
    static bool check_signature_88(const std::vector<uint8_t>& data) {
        if (data.size() < 64) return false;

        // Suche "RAEL88:" Marker
        std::string marker = "RAEL88:";
        std::string data_str(data.begin(), data.end());

        if (data_str.find(marker) != std::string::npos) {
            return true;
        }

        // Kohärenz-basierte Prüfung
        double entropy = calculate_entropy(data);
        double phase = calculate_rst_phase(data);

        // Harmonisches Muster = 88-konform
        bool harmonic = (std::abs(phase) < rst::PI / 3) && (entropy < 6.0);

        return harmonic;
    }

    size_t signature_count() const { return signatures_.size(); }
    const std::vector<ThreatSignature>& get_signatures() const { return signatures_; }
};

// ═══════════════════════════════════════════════════════════════════════════════
// BOOT SECTOR ANALYSIS
// ═══════════════════════════════════════════════════════════════════════════════

struct BootSectorAnalysis {
    std::string device;
    bool valid_mbr;
    std::string boot_signature;
    double entropy;
    std::vector<std::string> threats;

    // MBR Details
    std::string boot_code_hash;
    std::vector<std::map<std::string, std::string>> partitions;

    // GPT Details
    bool is_gpt;
    std::string disk_guid;
};

class BootSectorScanner {
public:
    BootSectorAnalysis scan_device(const std::string& device) {
        BootSectorAnalysis result;
        result.device = device;
        result.valid_mbr = false;
        result.is_gpt = false;

#ifndef _WIN32
        // Linux: Direkter Zugriff auf Block-Devices
        int fd = open(device.c_str(), O_RDONLY);
        if (fd < 0) {
            result.threats.push_back("Access.Denied");
            return result;
        }

        // MBR lesen (512 bytes)
        std::vector<uint8_t> mbr(512);
        if (read(fd, mbr.data(), 512) == 512) {
            // Boot Signatur prüfen
            uint16_t sig = (mbr[511] << 8) | mbr[510];
            result.boot_signature = "0x" + to_hex(sig);
            result.valid_mbr = (sig == 0xAA55);

            // Entropy
            result.entropy = ThreatDatabase::calculate_entropy(mbr);

            // Boot Code Hash (erste 440 bytes)
            result.boot_code_hash = sha256_first16(mbr.data(), 440);

            // Bekannte MBR-Malware prüfen
            check_mbr_threats(mbr, result);

            // Partition Table parsen (Offset 446)
            parse_partition_table(mbr, result);

            // GPT prüfen (LBA 1)
            std::vector<uint8_t> gpt(512);
            if (read(fd, gpt.data(), 512) == 512) {
                if (std::string(gpt.begin(), gpt.begin() + 8) == "EFI PART") {
                    result.is_gpt = true;
                    // GUID extrahieren (Offset 56-72)
                    result.disk_guid = bytes_to_guid(gpt.data() + 56);
                }
            }
        }

        close(fd);
#else
        // Windows: CreateFile mit GENERIC_READ
        HANDLE hDevice = CreateFileA(
            device.c_str(),
            GENERIC_READ,
            FILE_SHARE_READ | FILE_SHARE_WRITE,
            NULL,
            OPEN_EXISTING,
            0,
            NULL
        );

        if (hDevice != INVALID_HANDLE_VALUE) {
            std::vector<uint8_t> mbr(512);
            DWORD bytesRead;

            if (ReadFile(hDevice, mbr.data(), 512, &bytesRead, NULL) && bytesRead == 512) {
                uint16_t sig = (mbr[511] << 8) | mbr[510];
                result.boot_signature = "0x" + to_hex(sig);
                result.valid_mbr = (sig == 0xAA55);
                result.entropy = ThreatDatabase::calculate_entropy(mbr);
                result.boot_code_hash = sha256_first16(mbr.data(), 440);
                check_mbr_threats(mbr, result);
                parse_partition_table(mbr, result);
            }

            CloseHandle(hDevice);
        } else {
            result.threats.push_back("Access.Denied");
        }
#endif

        return result;
    }

private:
    void check_mbr_threats(const std::vector<uint8_t>& mbr, BootSectorAnalysis& result) {
        // Bekannte MBR-Malware Signaturen
        struct MBRThreat {
            std::vector<uint8_t> pattern;
            std::string name;
        };

        std::vector<MBRThreat> threats = {
            {{0x00, 0x00, 0x00, 0x00}, "Wiped.MBR"},  // Viele Nullen
            {{'P', 'E', 'T', 'Y', 'A'}, "Petya.Ransomware"},
            {{'G', 'O', 'L', 'D', 'E', 'N', 'E', 'Y', 'E'}, "GoldenEye.Ransomware"},
        };

        for (const auto& threat : threats) {
            if (std::search(mbr.begin(), mbr.end(),
                           threat.pattern.begin(), threat.pattern.end()) != mbr.end()) {
                result.threats.push_back(threat.name);
            }
        }

        // Entropie-basierte Erkennung
        if (result.entropy > 7.0) {
            result.threats.push_back("Suspicious.HighEntropy");
        }

        // Viele Nullen = möglicherweise gewischt
        size_t null_count = std::count(mbr.begin(), mbr.begin() + 440, 0);
        if (null_count > 400) {
            result.threats.push_back("Suspicious.MostlyNull");
        }
    }

    void parse_partition_table(const std::vector<uint8_t>& mbr, BootSectorAnalysis& result) {
        for (int i = 0; i < 4; ++i) {
            size_t offset = 446 + i * 16;
            uint8_t type = mbr[offset + 4];

            if (type != 0) {
                std::map<std::string, std::string> part;
                part["number"] = std::to_string(i + 1);
                part["bootable"] = (mbr[offset] == 0x80) ? "yes" : "no";
                part["type"] = "0x" + to_hex(type);

                // Start/Size (Little Endian)
                uint32_t start = mbr[offset + 8] | (mbr[offset + 9] << 8) |
                                (mbr[offset + 10] << 16) | (mbr[offset + 11] << 24);
                uint32_t size = mbr[offset + 12] | (mbr[offset + 13] << 8) |
                               (mbr[offset + 14] << 16) | (mbr[offset + 15] << 24);

                part["start_sector"] = std::to_string(start);
                part["total_sectors"] = std::to_string(size);

                result.partitions.push_back(part);
            }
        }
    }

    static std::string to_hex(uint16_t val) {
        std::ostringstream ss;
        ss << std::hex << std::setw(4) << std::setfill('0') << val;
        return ss.str();
    }

    static std::string to_hex(uint8_t val) {
        std::ostringstream ss;
        ss << std::hex << std::setw(2) << std::setfill('0') << (int)val;
        return ss.str();
    }

    static std::string sha256_first16(const uint8_t* data, size_t len) {
        // Einfacher Hash (nicht kryptographisch sicher, nur für Fingerprinting)
        uint64_t hash = 0;
        for (size_t i = 0; i < len; ++i) {
            hash = hash * 31 + data[i];
        }
        std::ostringstream ss;
        ss << std::hex << std::setw(16) << std::setfill('0') << hash;
        return ss.str();
    }

    static std::string bytes_to_guid(const uint8_t* data) {
        std::ostringstream ss;
        ss << std::hex << std::setfill('0');

        // GUID Format: XXXXXXXX-XXXX-XXXX-XXXX-XXXXXXXXXXXX
        for (int i = 3; i >= 0; --i) ss << std::setw(2) << (int)data[i];
        ss << "-";
        for (int i = 5; i >= 4; --i) ss << std::setw(2) << (int)data[i];
        ss << "-";
        for (int i = 7; i >= 6; --i) ss << std::setw(2) << (int)data[i];
        ss << "-";
        for (int i = 8; i < 10; ++i) ss << std::setw(2) << (int)data[i];
        ss << "-";
        for (int i = 10; i < 16; ++i) ss << std::setw(2) << (int)data[i];

        return ss.str();
    }
};

// ═══════════════════════════════════════════════════════════════════════════════
// SHADOW PARTITION SCANNER
// ═══════════════════════════════════════════════════════════════════════════════

struct HiddenPartition {
    std::string source;
    std::string type;
    std::string info;
};

class ShadowPartitionScanner {
public:
    struct ScanResult {
        std::vector<std::map<std::string, std::string>> partitions;
        std::vector<HiddenPartition> hidden_partitions;
        std::vector<BootSectorAnalysis> boot_sectors;
    };

    ScanResult scan_all() {
        ScanResult result;

        // 1. Partitionen auflisten
        result.partitions = list_partitions();

        // 2. Versteckte Partitionen finden
        result.hidden_partitions = find_hidden_partitions();

        // 3. Boot-Sektoren scannen
        BootSectorScanner boot_scanner;
        std::vector<std::string> devices = {
#ifdef _WIN32
            "\\\\.\\PhysicalDrive0",
            "\\\\.\\PhysicalDrive1"
#else
            "/dev/sda",
            "/dev/nvme0n1",
            "/dev/vda",
            "/dev/hda"
#endif
        };

        for (const auto& dev : devices) {
            if (device_exists(dev)) {
                result.boot_sectors.push_back(boot_scanner.scan_device(dev));
            }
        }

        return result;
    }

private:
    std::vector<std::map<std::string, std::string>> list_partitions() {
        std::vector<std::map<std::string, std::string>> partitions;

#ifndef _WIN32
        // Linux: /proc/partitions parsen
        std::ifstream f("/proc/partitions");
        std::string line;

        // Header überspringen
        std::getline(f, line);
        std::getline(f, line);

        while (std::getline(f, line)) {
            std::istringstream iss(line);
            int major, minor;
            uint64_t blocks;
            std::string name;

            if (iss >> major >> minor >> blocks >> name) {
                std::map<std::string, std::string> part;
                part["major"] = std::to_string(major);
                part["minor"] = std::to_string(minor);
                part["blocks"] = std::to_string(blocks);
                part["name"] = name;
                part["path"] = "/dev/" + name;
                partitions.push_back(part);
            }
        }
#else
        // Windows: GetLogicalDriveStrings
        char drives[256];
        DWORD len = GetLogicalDriveStringsA(sizeof(drives), drives);

        for (DWORD i = 0; i < len; ) {
            std::string drive = &drives[i];
            if (!drive.empty()) {
                std::map<std::string, std::string> part;
                part["name"] = drive;
                part["path"] = drive;

                UINT type = GetDriveTypeA(drive.c_str());
                switch (type) {
                    case DRIVE_FIXED: part["type"] = "fixed"; break;
                    case DRIVE_REMOVABLE: part["type"] = "removable"; break;
                    case DRIVE_REMOTE: part["type"] = "network"; break;
                    case DRIVE_CDROM: part["type"] = "cdrom"; break;
                    default: part["type"] = "unknown"; break;
                }

                partitions.push_back(part);
            }
            i += drive.length() + 1;
        }
#endif

        return partitions;
    }

    std::vector<HiddenPartition> find_hidden_partitions() {
        std::vector<HiddenPartition> hidden;

#ifndef _WIN32
        // LVM prüfen
        FILE* pipe = popen("lvs --noheadings -o lv_name,vg_name 2>/dev/null", "r");
        if (pipe) {
            char buffer[256];
            while (fgets(buffer, sizeof(buffer), pipe)) {
                std::string line(buffer);
                if (!line.empty() && line.find("No") == std::string::npos) {
                    hidden.push_back({"lvm", "LVM", line});
                }
            }
            pclose(pipe);
        }

        // dm-crypt/LUKS prüfen
        pipe = popen("dmsetup ls 2>/dev/null", "r");
        if (pipe) {
            char buffer[256];
            while (fgets(buffer, sizeof(buffer), pipe)) {
                std::string line(buffer);
                if (!line.empty() && line.find("No devices") == std::string::npos) {
                    hidden.push_back({"dm-crypt", "encrypted", line});
                }
            }
            pclose(pipe);
        }
#endif

        return hidden;
    }

    bool device_exists(const std::string& path) {
#ifdef _WIN32
        HANDLE h = CreateFileA(path.c_str(), 0, FILE_SHARE_READ | FILE_SHARE_WRITE,
                              NULL, OPEN_EXISTING, 0, NULL);
        if (h != INVALID_HANDLE_VALUE) {
            CloseHandle(h);
            return true;
        }
        return false;
#else
        return access(path.c_str(), F_OK) == 0;
#endif
    }
};

// ═══════════════════════════════════════════════════════════════════════════════
// RST OMEGA DEEP SCANNER - Hauptklasse
// ═══════════════════════════════════════════════════════════════════════════════

class RSTOmegaDeepScanner {
private:
    ThreatDatabase threat_db_;
    ShadowPartitionScanner shadow_scanner_;
    BootSectorScanner boot_scanner_;

    std::vector<DeepScanResult> all_threats_;
    std::atomic<uint64_t> files_scanned_;
    std::atomic<uint64_t> threats_found_;
    std::mutex mtx_;

    // Quarantine
    std::string quarantine_dir_;

public:
    RSTOmegaDeepScanner()
        : files_scanned_(0), threats_found_(0) {

#ifdef _WIN32
        quarantine_dir_ = "C:\\RAEL\\quarantine\\";
#else
        quarantine_dir_ = "/var/rael/quarantine/";
#endif
        std::filesystem::create_directories(quarantine_dir_);
    }

    // ═══════════════════════════════════════════════════════════════════════
    // VOLLSTÄNDIGER SYSTEM-SCAN
    // ═══════════════════════════════════════════════════════════════════════

    struct FullScanReport {
        std::string scan_id;
        std::chrono::system_clock::time_point start_time;
        std::chrono::system_clock::time_point end_time;
        double duration_seconds;

        uint64_t files_scanned;
        uint64_t threats_found;
        uint64_t critical_threats;

        std::vector<DeepScanResult> filesystem_threats;
        ShadowPartitionScanner::ScanResult shadow_results;
        std::vector<std::string> quarantined_files;

        std::string system_status;  // CLEAN, INFECTED, CRITICAL
    };

    FullScanReport full_system_scan(
        const std::vector<std::string>& paths = {},
        bool scan_shadows = true,
        bool auto_quarantine = false,
        double threat_threshold = 7.0
    ) {
        FullScanReport report;
        report.start_time = std::chrono::system_clock::now();
        report.scan_id = generate_scan_id();

        // 1. Dateisystem scannen
        std::vector<std::string> scan_paths = paths;
        if (scan_paths.empty()) {
#ifdef _WIN32
            scan_paths = {"C:\\"};
#else
            scan_paths = {"/"};
#endif
        }

        for (const auto& path : scan_paths) {
            scan_directory(path, report, auto_quarantine, threat_threshold);
        }

        // 2. Schattenpartitionen scannen
        if (scan_shadows) {
            report.shadow_results = shadow_scanner_.scan_all();

            // Boot-Sektor-Bedrohungen hinzufügen
            for (const auto& boot : report.shadow_results.boot_sectors) {
                for (const auto& threat : boot.threats) {
                    DeepScanResult result;
                    result.path = boot.device;
                    result.threat_name = threat;
                    result.threat_level = 9.5;
                    result.category = "bootkit";
                    report.filesystem_threats.push_back(result);
                }
            }
        }

        // 3. Zusammenfassung
        report.end_time = std::chrono::system_clock::now();
        report.duration_seconds = std::chrono::duration<double>(
            report.end_time - report.start_time).count();

        report.files_scanned = files_scanned_;
        report.threats_found = report.filesystem_threats.size();
        report.critical_threats = std::count_if(
            report.filesystem_threats.begin(),
            report.filesystem_threats.end(),
            [](const DeepScanResult& r) { return r.threat_level >= 8.0; }
        );

        // Status
        if (report.threats_found == 0) {
            report.system_status = "CLEAN";
        } else if (report.critical_threats > 0) {
            report.system_status = "CRITICAL";
        } else {
            report.system_status = "INFECTED";
        }

        return report;
    }

    // ═══════════════════════════════════════════════════════════════════════
    // EINZELNE DATEI SCANNEN
    // ═══════════════════════════════════════════════════════════════════════

    DeepScanResult scan_file(const std::string& filepath) {
        DeepScanResult result;
        result.path = filepath;

        try {
            // Datei lesen
            std::ifstream file(filepath, std::ios::binary);
            if (!file) {
                result.threat_name = "Access.Denied";
                result.threat_level = 1.0;
                result.category = "access";
                return result;
            }

            std::vector<uint8_t> data(
                (std::istreambuf_iterator<char>(file)),
                std::istreambuf_iterator<char>()
            );

            if (data.empty()) {
                return result;  // Leere Datei
            }

            // RST Analyse
            result.entropy = ThreatDatabase::calculate_entropy(data);
            result.phase = ThreatDatabase::calculate_rst_phase(data);
            result.has_signature_88 = ThreatDatabase::check_signature_88(data);

            // Kohärenz (vereinfacht)
            result.coherence = 1.0 - std::abs(result.phase) / rst::PI;

            // Malice Index (mit präzisen RST-Gewichten)
            double entropy_factor = (result.entropy > rst::ENTROPY_HIGH) ? 0.8 :
                                   (result.entropy > rst::ENTROPY_SUSPICIOUS) ? 0.4 : 0.1;
            double coherence_factor = 1.0 - result.coherence;
            double phase_factor = (result.phase < 0) ?
                                 std::abs(result.phase) / rst::PI : 0.0;

            // malice = entropy * G3 + coherence * G1 + phase * G5
            result.malice_index = entropy_factor * rst::G3 +
                                 coherence_factor * rst::G1 +
                                 phase_factor * rst::G5;

            // WICHTIG: 88-Signatur reduziert NIEMALS auf 0!
            if (result.has_signature_88) {
                result.malice_index = std::max(result.malice_index * rst::G5,
                                              result.malice_index - rst::G1);
            }

            // Signatur-Scan
            auto matches = threat_db_.scan(data);
            if (!matches.empty()) {
                // Höchste Bedrohung nehmen
                auto max_threat = std::max_element(matches.begin(), matches.end(),
                    [](const ThreatSignature& a, const ThreatSignature& b) {
                        return a.threat_level < b.threat_level;
                    });

                result.threat_name = max_threat->name;
                result.threat_level = max_threat->threat_level;
                result.category = max_threat->category;
            }

            // Heuristische Erkennung
            if (result.malice_index > rst::G1 && result.threat_level < 5.0) {
                result.threat_name = "RST.Suspicious";
                result.threat_level = result.malice_index * 10.0;
                result.category = "heuristic";
            }

            files_scanned_++;
            if (result.threat_level > 0) {
                threats_found_++;
            }

        } catch (const std::exception& e) {
            result.details = std::string("Error: ") + e.what();
        }

        return result;
    }

    // ═══════════════════════════════════════════════════════════════════════
    // QUARANTINE
    // ═══════════════════════════════════════════════════════════════════════

    bool quarantine_file(const std::string& filepath, DeepScanResult& result) {
        try {
            std::filesystem::path src(filepath);
            std::string q_name = generate_timestamp() + "_" +
                                src.filename().string() + ".quarantine";
            std::filesystem::path dst = quarantine_dir_ + q_name;

            std::filesystem::rename(src, dst);

            result.action_taken = "Quarantined: " + dst.string();
            return true;
        } catch (...) {
            result.action_taken = "Quarantine failed";
            return false;
        }
    }

    // ═══════════════════════════════════════════════════════════════════════
    // STATUS REPORT
    // ═══════════════════════════════════════════════════════════════════════

    std::string generate_report(const FullScanReport& report) {
        std::ostringstream ss;

        ss << "═══════════════════════════════════════════════════════════════════\n";
        ss << "            RST OMEGA DEEP SCANNER - BERICHT\n";
        ss << "═══════════════════════════════════════════════════════════════════\n\n";

        ss << "SCAN-ID:    " << report.scan_id << "\n";
        ss << "DAUER:      " << std::fixed << std::setprecision(2)
           << report.duration_seconds << " Sekunden\n";
        ss << "STATUS:     " << report.system_status << "\n\n";

        ss << "───────────────────────────────────────────────────────────────────\n";
        ss << "RST KONSTANTEN (17 Dezimalstellen)\n";
        ss << "───────────────────────────────────────────────────────────────────\n";
        ss << std::fixed << std::setprecision(17);
        ss << "G0 (8/9): " << rst::G0 << " (WAHRHEIT)\n";
        ss << "G1 (5/9): " << rst::G1 << "\n";
        ss << "G3 (3/9): " << rst::G3 << "\n";
        ss << "G5 (1/9): " << rst::G5 << "\n";
        ss << "Summe:    " << (rst::G5 + rst::G3 + rst::G1) << " (= 9/9 = 1)\n\n";

        ss << "───────────────────────────────────────────────────────────────────\n";
        ss << "STATISTIK\n";
        ss << "───────────────────────────────────────────────────────────────────\n";
        ss << "Dateien gescannt:     " << report.files_scanned << "\n";
        ss << "Bedrohungen gefunden: " << report.threats_found << "\n";
        ss << "Kritische Bedrohungen:" << report.critical_threats << "\n\n";

        if (!report.filesystem_threats.empty()) {
            ss << "───────────────────────────────────────────────────────────────────\n";
            ss << "BEDROHUNGEN\n";
            ss << "───────────────────────────────────────────────────────────────────\n";

            for (const auto& t : report.filesystem_threats) {
                ss << "[" << std::fixed << std::setprecision(1) << t.threat_level << "] ";
                ss << t.threat_name << "\n";
                ss << "    Pfad: " << t.path << "\n";
                ss << "    Kategorie: " << t.category << "\n";
                ss << "    Malice-Index: " << std::setprecision(8) << t.malice_index << "\n";
                ss << "    88-Signatur: " << (t.has_signature_88 ? "JA" : "NEIN") << "\n\n";
            }
        }

        ss << "═══════════════════════════════════════════════════════════════════\n";

        return ss.str();
    }

private:
    void scan_directory(const std::string& path, FullScanReport& report,
                       bool auto_quarantine, double threshold) {
        try {
            for (const auto& entry : std::filesystem::recursive_directory_iterator(
                    path, std::filesystem::directory_options::skip_permission_denied)) {

                if (entry.is_regular_file()) {
                    auto result = scan_file(entry.path().string());

                    if (result.threat_level > 0) {
                        report.filesystem_threats.push_back(result);

                        if (auto_quarantine && result.threat_level >= threshold) {
                            if (quarantine_file(entry.path().string(), result)) {
                                report.quarantined_files.push_back(entry.path().string());
                            }
                        }
                    }
                }
            }
        } catch (...) {
            // Zugriff verweigert oder andere Fehler - ignorieren
        }
    }

    std::string generate_scan_id() {
        auto now = std::chrono::system_clock::now();
        auto duration = now.time_since_epoch();
        auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();

        std::ostringstream ss;
        ss << std::hex << millis;
        return ss.str();
    }

    std::string generate_timestamp() {
        auto now = std::chrono::system_clock::now();
        auto time = std::chrono::system_clock::to_time_t(now);
        std::ostringstream ss;
        ss << std::put_time(std::localtime(&time), "%Y%m%d_%H%M%S");
        return ss.str();
    }
};

} // namespace deep
} // namespace security
} // namespace rael
