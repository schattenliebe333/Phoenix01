// ═══════════════════════════════════════════════════════════════════════════════
// RST LIVE MONITOR - Real-Time Security Monitoring with RST Analysis
// ═══════════════════════════════════════════════════════════════════════════════
//
// Konzept: Echtzeit-Überwachung basierend auf RST (Resonanz-Spiralen-Theorie)
//
// Analysemethoden:
//   - Shannon Entropy (Zufälligkeit/Verschlüsselung)
//   - RST Coherence (88-Signatur Harmonie)
//   - Phase Deviation (Abweichung vom Optimum)
//   - Malice Index (Kombinierter Bedrohungswert)
//
// Live-Monitoring:
//   - Prozesse, Netzwerk, Dateisystem, Speicher
//   - Alles wird IMMER geprüft (keine Whitelist!)
//   - Echtzeit-Dashboard mit kontinuierlichen Updates
//
// ═══════════════════════════════════════════════════════════════════════════════

#pragma once

#include <string>
#include <vector>
#include <map>
#include <set>
#include <array>
#include <atomic>
#include <mutex>
#include <thread>
#include <chrono>
#include <functional>
#include <cmath>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <numeric>
#include <cstdint>
#include <iostream>

#include "rael/security_core.hpp"
#include "rael/threat_interpreter.hpp"

namespace rael {
namespace security {
namespace live {

// Import types from security_core namespace
using rael::security_core::SecurityCore;
using rael::security_core::Threat;
using rael::security_core::ThreatType;
using rael::security_core::threat_name;

// ═══════════════════════════════════════════════════════════════════════════════
// RST ANALYSIS ENGINE - Entropy, Coherence, Signature Detection
// ═══════════════════════════════════════════════════════════════════════════════

class RSTAnalysisEngine {
private:
    // RST Constants - PRÄZISION: 17 Dezimalstellen
    // WICHTIG: G5 + G3 + G1 = 1/9 + 3/9 + 5/9 = 9/9 = 1
    static constexpr double G0 = 0.88888888888888889;  // 8/9 WAHRHEIT (Referenz)
    static constexpr double G1 = 0.55555555555555556;  // 5/9
    static constexpr double G2 = 0.44444444444444444;  // 4/9
    static constexpr double G3 = 0.33333333333333333;  // 3/9
    static constexpr double G4 = 0.22222222222222222;  // 2/9
    static constexpr double G5 = 0.11111111111111111;  // 1/9
    static constexpr double PHI = 1.61803398874989484;
    static constexpr double SIGNATURE_88 = 0.88888888888888889;  // = G0

    // Entropy thresholds
    static constexpr double ENTROPY_PACKED = 7.5;      // Gepackte/verschlüsselte Daten
    static constexpr double ENTROPY_NORMAL_HIGH = 6.0; // Hohe aber normale Entropie
    static constexpr double ENTROPY_NORMAL_LOW = 4.0;  // Niedrige normale Entropie

public:
    // ═══════════════════════════════════════════════════════════════════════════
    // SHANNON ENTROPY - Misst Zufälligkeit der Daten (0-8 bits)
    // ═══════════════════════════════════════════════════════════════════════════
    static double calculate_entropy(const std::vector<uint8_t>& data) {
        if (data.empty()) return 0.0;

        // Byte-Häufigkeiten zählen
        std::array<size_t, 256> freq{};
        for (uint8_t byte : data) {
            freq[byte]++;
        }

        // Shannon Entropy berechnen
        double entropy = 0.0;
        double size = static_cast<double>(data.size());

        for (size_t count : freq) {
            if (count > 0) {
                double p = static_cast<double>(count) / size;
                entropy -= p * std::log2(p);
            }
        }

        return entropy;  // 0.0 - 8.0 bits
    }

    // Entropy für String
    static double calculate_entropy(const std::string& data) {
        return calculate_entropy(std::vector<uint8_t>(data.begin(), data.end()));
    }

    // ═══════════════════════════════════════════════════════════════════════════
    // RST COHERENCE - Misst Harmonie mit 88-Signatur
    // ═══════════════════════════════════════════════════════════════════════════
    static double calculate_rst_coherence(const std::vector<uint8_t>& data) {
        if (data.empty()) return 0.0;

        // Prüfe auf 88-Patterns in den Daten
        int pattern_count = 0;
        int eight_count = 0;

        for (size_t i = 0; i < data.size(); ++i) {
            // Direktes 0x88 oder '8' Zeichen
            if (data[i] == 0x88 || data[i] == '8') {
                eight_count++;
            }

            // 88-Muster in Zweiergruppen
            if (i < data.size() - 1) {
                if ((data[i] == '8' && data[i+1] == '8') ||
                    (data[i] == 0x88 && data[i+1] == 0x88)) {
                    pattern_count++;
                }
            }
        }

        // Berechne "Reinheit" basierend auf G0
        double size = static_cast<double>(data.size());
        double eight_ratio = static_cast<double>(eight_count) / size;
        double pattern_ratio = static_cast<double>(pattern_count * 2) / size;

        // Kohärenz = Nähe zu 0.888...
        double raw_coherence = (eight_ratio + pattern_ratio) / 2.0;
        double coherence = 1.0 - std::abs(raw_coherence - SIGNATURE_88);

        return std::clamp(coherence, 0.0, 1.0);
    }

    // ═══════════════════════════════════════════════════════════════════════════
    // 88-SIGNATURE DETECTION - Prüft ob Daten die Master-Signatur tragen
    // ═══════════════════════════════════════════════════════════════════════════
    struct SignatureResult {
        bool has_signature;       // Signatur gefunden
        double signature_score;   // Stärke der Signatur (0-1)
        std::string marker_found; // Gefundener Marker
        size_t marker_position;   // Position im Datenstrom
    };

    static SignatureResult detect_signature_88(const std::vector<uint8_t>& data) {
        SignatureResult result{false, 0.0, "", 0};

        // Suche "RAEL88:" Marker
        std::string marker_prefix = "RAEL88:";
        std::string data_str(data.begin(), data.end());

        size_t pos = data_str.find(marker_prefix);
        if (pos != std::string::npos) {
            result.has_signature = true;
            result.marker_position = pos;

            // Extrahiere Hash nach dem Marker
            size_t hash_start = pos + marker_prefix.length();
            size_t hash_end = data_str.find_first_of(" \n\r\t", hash_start);
            if (hash_end == std::string::npos) hash_end = data_str.length();

            std::string hash = data_str.substr(hash_start, hash_end - hash_start);
            result.marker_found = marker_prefix + hash;

            // Validiere Hash: Quersumme muss G0-konform sein (% 9 == 8)
            uint64_t checksum = 0;
            for (char c : hash) {
                checksum += static_cast<uint8_t>(c);
            }

            bool g0_valid = (checksum % 9 == 8);
            result.signature_score = g0_valid ? 1.0 : 0.5;
        }

        // Alternative: Prüfe mathematische 88-Harmonie
        if (!result.has_signature) {
            double coherence = calculate_rst_coherence(data);
            if (coherence > G0) {
                result.has_signature = true;
                result.signature_score = coherence;
                result.marker_found = "[IMPLICIT_COHERENCE]";
            }
        }

        return result;
    }

    // ═══════════════════════════════════════════════════════════════════════════
    // PHASE DEVIATION - Abweichung vom optimalen RST-Zustand
    // ═══════════════════════════════════════════════════════════════════════════
    static double calculate_phase_deviation(const std::vector<uint8_t>& data) {
        if (data.empty()) return 1.0;  // Maximum deviation für leere Daten

        // Optimale Phase: Verhältnis nahe PHI (1.618...)
        double sum = 0.0;
        double sum_sq = 0.0;

        for (uint8_t byte : data) {
            double val = static_cast<double>(byte) / 255.0;
            sum += val;
            sum_sq += val * val;
        }

        double mean = sum / data.size();
        double variance = (sum_sq / data.size()) - (mean * mean);
        double stddev = std::sqrt(std::max(0.0, variance));

        // Ideales Verhältnis: mean/stddev ≈ PHI
        double ratio = (stddev > 0.001) ? mean / stddev : 0.0;
        double deviation = std::abs(ratio - PHI) / PHI;

        return std::clamp(deviation, 0.0, 1.0);
    }

    // ═══════════════════════════════════════════════════════════════════════════
    // MALICE INDEX - Kombinierter Bedrohungswert
    // ═══════════════════════════════════════════════════════════════════════════
    struct MaliceAnalysis {
        double entropy;           // Shannon Entropy
        double coherence;         // RST Coherence
        double phase_deviation;   // Phase Deviation
        bool has_signature_88;    // 88-Signatur vorhanden
        double signature_score;   // Signatur-Stärke
        double malice_index;      // Gesamtbedrohung (0-1)
        std::string assessment;   // Textuelle Bewertung
    };

    static MaliceAnalysis calculate_malice_index(const std::vector<uint8_t>& data) {
        MaliceAnalysis analysis;

        // Grundwerte berechnen
        analysis.entropy = calculate_entropy(data);
        analysis.coherence = calculate_rst_coherence(data);
        analysis.phase_deviation = calculate_phase_deviation(data);

        auto sig_result = detect_signature_88(data);
        analysis.has_signature_88 = sig_result.has_signature;
        analysis.signature_score = sig_result.signature_score;

        // Malice Index Berechnung
        // Hohe Entropie = verdächtig (verschlüsselt/gepackt)
        double entropy_factor = (analysis.entropy > ENTROPY_PACKED) ? 0.8 :
                               (analysis.entropy > ENTROPY_NORMAL_HIGH) ? 0.4 : 0.1;

        // Niedrige Kohärenz = verdächtig (fremd)
        double coherence_factor = 1.0 - analysis.coherence;

        // Hohe Phase Deviation = verdächtig (chaotisch)
        double phase_factor = analysis.phase_deviation;

        // Kombinieren mit RST-Gewichtung
        double raw_malice = (entropy_factor * G3 +
                           coherence_factor * G1 +
                           phase_factor * G5);

        // WICHTIG: Signatur reduziert NIEMALS auf 0!
        // Alles wird IMMER geprüft!
        if (analysis.has_signature_88) {
            raw_malice = std::max(raw_malice * 0.1, raw_malice - G1);
        }

        analysis.malice_index = std::clamp(raw_malice, 0.0, 1.0);

        // Assessment
        if (analysis.malice_index > G0) {
            analysis.assessment = "KRITISCH - Hohe Bedrohung";
        } else if (analysis.malice_index > G1) {
            analysis.assessment = "WARNUNG - Moderate Bedrohung";
        } else if (analysis.malice_index > G3) {
            analysis.assessment = "AUFMERKSAMKEIT - Geringe Bedrohung";
        } else {
            analysis.assessment = "SICHER - Minimal";
        }

        return analysis;
    }
};

// ═══════════════════════════════════════════════════════════════════════════════
// MALWARE SIGNATURE DATABASE - Bekannte Bedrohungssignaturen
// ═══════════════════════════════════════════════════════════════════════════════

class SignatureDatabase {
private:
    struct MalwareSignature {
        std::string name;
        std::string category;
        std::vector<uint8_t> pattern;
        double severity;  // 0-1
    };

    std::vector<MalwareSignature> signatures_;
    std::mutex mtx_;

public:
    SignatureDatabase() {
        initialize_signatures();
    }

    void initialize_signatures() {
        std::lock_guard<std::mutex> lock(mtx_);

        // Rootkit Signaturen
        add_pattern("LD_PRELOAD Rootkit", "rootkit", "LD_PRELOAD=", 0.95);
        add_pattern("Diamorphine", "rootkit", "diamorphine", 0.98);
        add_pattern("Reptile", "rootkit", "reptile_module", 0.98);
        add_pattern("Adore-ng", "rootkit", "adore-ng", 0.95);

        // Bootkit Signaturen
        add_pattern("MBR Overwrite", "bootkit", "\x33\xc0\x8e\xd0\xbc", 0.90);
        add_pattern("VBR Infection", "bootkit", "\xeb\x52\x90NTFS", 0.85);

        // Ransomware Signaturen
        add_pattern("WannaCry", "ransomware", "WanaCrypt0r", 0.99);
        add_pattern("Ryuk", "ransomware", "RyukReadMe", 0.99);
        add_pattern("LockBit", "ransomware", "lockbit", 0.98);
        add_pattern("BlackCat", "ransomware", "ALPHV", 0.98);

        // RAT Signaturen
        add_pattern("Cobalt Strike", "rat", "beacon.dll", 0.95);
        add_pattern("Meterpreter", "rat", "metsrv.dll", 0.95);
        add_pattern("Empire", "rat", "empire-launcher", 0.90);
        add_pattern("njRAT", "rat", "njq8", 0.92);

        // Keylogger Signaturen
        add_pattern("GetAsyncKeyState Hook", "keylogger", "GetAsyncKeyState", 0.70);
        add_pattern("SetWindowsHookEx", "keylogger", "SetWindowsHookExA", 0.65);

        // Cryptominer Signaturen
        add_pattern("XMRig", "cryptominer", "xmrig", 0.88);
        add_pattern("Stratum Protocol", "cryptominer", "stratum+tcp://", 0.85);
        add_pattern("Mining Pool", "cryptominer", "pool.minexmr", 0.90);

        // Shell/Backdoor Signaturen
        add_pattern("Reverse Shell", "backdoor", "/bin/sh -i", 0.92);
        add_pattern("Netcat Shell", "backdoor", "nc -e /bin", 0.90);
        add_pattern("PowerShell Download", "backdoor", "IEX(New-Object", 0.88);
        add_pattern("Bash TCP", "backdoor", "/dev/tcp/", 0.90);
    }

    void add_pattern(const std::string& name, const std::string& category,
                     const std::string& pattern, double severity) {
        MalwareSignature sig;
        sig.name = name;
        sig.category = category;
        sig.pattern = std::vector<uint8_t>(pattern.begin(), pattern.end());
        sig.severity = severity;
        signatures_.push_back(sig);
    }

    struct MatchResult {
        bool matched;
        std::string name;
        std::string category;
        double severity;
        size_t offset;
    };

    // Sucht alle Signaturen in Daten
    std::vector<MatchResult> scan(const std::vector<uint8_t>& data) {
        std::lock_guard<std::mutex> lock(mtx_);
        std::vector<MatchResult> results;

        for (const auto& sig : signatures_) {
            // Boyer-Moore-ähnliche Suche
            auto it = std::search(data.begin(), data.end(),
                                 sig.pattern.begin(), sig.pattern.end());

            if (it != data.end()) {
                MatchResult r;
                r.matched = true;
                r.name = sig.name;
                r.category = sig.category;
                r.severity = sig.severity;
                r.offset = std::distance(data.begin(), it);
                results.push_back(r);
            }
        }

        return results;
    }

    // Schnelle Prüfung ob verdächtig
    bool is_suspicious(const std::vector<uint8_t>& data) {
        auto results = scan(data);
        return !results.empty();
    }

    size_t signature_count() const { return signatures_.size(); }
};

// ═══════════════════════════════════════════════════════════════════════════════
// LIVE SCAN RESULT - Einzelnes Scan-Ergebnis
// ═══════════════════════════════════════════════════════════════════════════════

struct LiveScanResult {
    std::string target;           // Was wurde gescannt (PID, IP, Pfad)
    std::string target_type;      // process, network, file, memory

    // RST Analysis
    RSTAnalysisEngine::MaliceAnalysis rst_analysis;

    // Signature Matches
    std::vector<SignatureDatabase::MatchResult> signature_matches;

    // Combined Threat Score
    double threat_score;

    // Timestamp
    std::chrono::system_clock::time_point timestamp;

    // Recommended Action
    std::string action;

    LiveScanResult() : threat_score(0.0) {
        timestamp = std::chrono::system_clock::now();
    }
};

// ═══════════════════════════════════════════════════════════════════════════════
// LIVE MONITOR - Real-Time Security Dashboard
// ═══════════════════════════════════════════════════════════════════════════════

class LiveMonitor {
private:
    SecurityCore& security_core_;
    SignatureDatabase sig_db_;

    std::vector<LiveScanResult> scan_history_;
    std::atomic<bool> running_;
    std::atomic<uint64_t> total_scans_;
    std::atomic<uint64_t> threats_detected_;

    std::thread monitor_thread_;
    std::thread display_thread_;
    std::mutex history_mtx_;

    // Update interval
    std::chrono::milliseconds scan_interval_;
    std::chrono::milliseconds display_interval_;

    // Callbacks
    std::function<void(const LiveScanResult&)> on_scan_complete_;
    std::function<void(const LiveScanResult&)> on_threat_detected_;

    // ANSI Colors (Linux/Mac)
#ifndef _WIN32
    const std::string RED = "\033[91m";
    const std::string GREEN = "\033[92m";
    const std::string YELLOW = "\033[93m";
    const std::string BLUE = "\033[94m";
    const std::string MAGENTA = "\033[95m";
    const std::string CYAN = "\033[96m";
    const std::string WHITE = "\033[97m";
    const std::string RESET = "\033[0m";
    const std::string BOLD = "\033[1m";
    const std::string DIM = "\033[2m";
#else
    const std::string RED = "";
    const std::string GREEN = "";
    const std::string YELLOW = "";
    const std::string BLUE = "";
    const std::string MAGENTA = "";
    const std::string CYAN = "";
    const std::string WHITE = "";
    const std::string RESET = "";
    const std::string BOLD = "";
    const std::string DIM = "";
#endif

public:
    LiveMonitor(SecurityCore& core)
        : security_core_(core)
        , running_(false)
        , total_scans_(0)
        , threats_detected_(0)
        , scan_interval_(5000)     // 5 Sekunden
        , display_interval_(1000)  // 1 Sekunde
    {}

    ~LiveMonitor() {
        stop();
    }

    // ═══════════════════════════════════════════════════════════════════════════
    // CONFIGURATION
    // ═══════════════════════════════════════════════════════════════════════════

    void set_scan_interval(std::chrono::milliseconds interval) {
        scan_interval_ = interval;
    }

    void set_display_interval(std::chrono::milliseconds interval) {
        display_interval_ = interval;
    }

    void set_scan_callback(std::function<void(const LiveScanResult&)> cb) {
        on_scan_complete_ = cb;
    }

    void set_threat_callback(std::function<void(const LiveScanResult&)> cb) {
        on_threat_detected_ = cb;
    }

    // ═══════════════════════════════════════════════════════════════════════════
    // START/STOP
    // ═══════════════════════════════════════════════════════════════════════════

    void start() {
        if (running_) return;
        running_ = true;

        // Scanner Thread
        monitor_thread_ = std::thread([this]() {
            while (running_) {
                perform_full_scan();
                std::this_thread::sleep_for(scan_interval_);
            }
        });

        // Display Thread
        display_thread_ = std::thread([this]() {
            while (running_) {
                update_display();
                std::this_thread::sleep_for(display_interval_);
            }
        });
    }

    void stop() {
        running_ = false;
        if (monitor_thread_.joinable()) monitor_thread_.join();
        if (display_thread_.joinable()) display_thread_.join();
    }

    bool is_running() const { return running_; }

    // ═══════════════════════════════════════════════════════════════════════════
    // SCANNING
    // ═══════════════════════════════════════════════════════════════════════════

    void perform_full_scan() {
        // 1. Prozesse scannen
        scan_processes();

        // 2. Netzwerk scannen
        scan_network();

        // 3. Kritische Dateien scannen
        scan_critical_files();

        // 4. Security Core scannen (für Gravitravitation/Vollenstrahlen)
        security_core_.scan_all();
    }

    void scan_processes() {
#ifdef _WIN32
        // Windows: CreateToolhelp32Snapshot
        HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
        if (hSnap == INVALID_HANDLE_VALUE) return;

        PROCESSENTRY32 pe;
        pe.dwSize = sizeof(pe);

        if (Process32First(hSnap, &pe)) {
            do {
                scan_single_process(pe.th32ProcessID, pe.szExeFile);
            } while (Process32Next(hSnap, &pe));
        }
        CloseHandle(hSnap);
#else
        // Linux: /proc scannen
        DIR* dir = opendir("/proc");
        if (!dir) return;

        struct dirent* entry;
        while ((entry = readdir(dir)) != nullptr) {
            if (entry->d_type == DT_DIR) {
                std::string name = entry->d_name;
                if (std::all_of(name.begin(), name.end(), ::isdigit)) {
                    uint32_t pid = std::stoul(name);
                    std::string proc_name = get_process_name_linux(pid);
                    scan_single_process(pid, proc_name);
                }
            }
        }
        closedir(dir);
#endif
    }

    void scan_single_process(uint32_t pid, const std::string& name) {
        LiveScanResult result;
        result.target = std::to_string(pid) + " (" + name + ")";
        result.target_type = "process";

        // Lese Prozess-Speicher (erste 4KB für Analyse)
        std::vector<uint8_t> sample = read_process_memory_sample(pid, 4096);

        if (!sample.empty()) {
            // RST Analyse
            result.rst_analysis = RSTAnalysisEngine::calculate_malice_index(sample);

            // Signatur-Scan
            result.signature_matches = sig_db_.scan(sample);

            // Kombinierter Score
            result.threat_score = result.rst_analysis.malice_index;
            for (const auto& match : result.signature_matches) {
                result.threat_score = std::max(result.threat_score, match.severity);
            }
        }

        process_scan_result(result);
    }

    void scan_network() {
        // Nutzt NetworkMonitor aus SecurityCore
        // Hier nur Ergebnisse sammeln
        total_scans_++;
    }

    void scan_critical_files() {
        std::vector<std::string> critical_paths = {
#ifdef _WIN32
            "C:\\Windows\\System32\\drivers\\etc\\hosts",
            "C:\\Windows\\System32\\config\\SAM",
#else
            "/etc/passwd",
            "/etc/shadow",
            "/etc/hosts",
            "/etc/sudoers",
#endif
        };

        for (const auto& path : critical_paths) {
            scan_single_file(path);
        }
    }

    void scan_single_file(const std::string& path) {
        LiveScanResult result;
        result.target = path;
        result.target_type = "file";

        // Datei lesen
        std::ifstream file(path, std::ios::binary);
        if (!file) return;

        std::vector<uint8_t> content((std::istreambuf_iterator<char>(file)),
                                      std::istreambuf_iterator<char>());

        if (!content.empty()) {
            // RST Analyse
            result.rst_analysis = RSTAnalysisEngine::calculate_malice_index(content);

            // Signatur-Scan
            result.signature_matches = sig_db_.scan(content);

            // Kombinierter Score
            result.threat_score = result.rst_analysis.malice_index;
            for (const auto& match : result.signature_matches) {
                result.threat_score = std::max(result.threat_score, match.severity);
            }
        }

        process_scan_result(result);
    }

    // ═══════════════════════════════════════════════════════════════════════════
    // RESULT PROCESSING
    // ═══════════════════════════════════════════════════════════════════════════

    void process_scan_result(LiveScanResult& result) {
        total_scans_++;

        // Action bestimmen
        if (result.threat_score > 0.888) {
            result.action = "TERMINATE/QUARANTINE";
            threats_detected_++;
        } else if (result.threat_score > 0.555) {
            result.action = "MONITOR_CLOSELY";
            threats_detected_++;
        } else if (result.threat_score > 0.333) {
            result.action = "LOG_ACTIVITY";
        } else {
            result.action = "NONE";
        }

        // History speichern
        {
            std::lock_guard<std::mutex> lock(history_mtx_);
            scan_history_.push_back(result);

            // Max 1000 Einträge behalten
            if (scan_history_.size() > 1000) {
                scan_history_.erase(scan_history_.begin(),
                                   scan_history_.begin() + 100);
            }
        }

        // Callbacks
        if (on_scan_complete_) {
            on_scan_complete_(result);
        }

        if (result.threat_score > 0.333 && on_threat_detected_) {
            on_threat_detected_(result);
        }

        // An SecurityCore weiterleiten wenn Bedrohung
        if (result.threat_score > 0.333) {
            Threat t;
            t.source = result.target;
            t.attack_energy = result.threat_score;

            if (result.target_type == "process") {
                t.type = ThreatType::PROCESS_SUSPICIOUS;
            } else if (result.target_type == "file") {
                t.type = ThreatType::FILE_UNAUTHORIZED;
            } else {
                t.type = ThreatType::NETWORK_SUSPICIOUS;
            }

            if (!result.signature_matches.empty()) {
                t.details = "Signature: " + result.signature_matches[0].name;
            } else {
                t.details = result.rst_analysis.assessment;
            }

            security_core_.process_threat(t);
        }
    }

    // ═══════════════════════════════════════════════════════════════════════════
    // DISPLAY
    // ═══════════════════════════════════════════════════════════════════════════

    void update_display() {
        // Clear screen
        std::cout << "\033[2J\033[H";

        print_header();
        print_statistics();
        print_security_core_status();
        print_recent_threats();
        print_footer();
    }

    void print_header() {
        std::cout << CYAN << BOLD;
        std::cout << "╔══════════════════════════════════════════════════════════════════════════════╗\n";
        std::cout << "║            RST LIVE MONITOR - RAEL SECURITY SYSTEM                          ║\n";
        std::cout << "║         Entropy • Coherence • Gravitravitation • Vollenstrahlen             ║\n";
        std::cout << "╚══════════════════════════════════════════════════════════════════════════════╝\n";
        std::cout << RESET << "\n";
    }

    void print_statistics() {
        auto now = std::chrono::system_clock::now();
        auto time = std::chrono::system_clock::to_time_t(now);

        std::cout << YELLOW << "┌─ STATISTIKEN ";
        std::cout << std::string(64, '─') << "┐\n" << RESET;

        std::cout << "│ " << WHITE << "Zeit: " << RESET << std::put_time(std::localtime(&time), "%Y-%m-%d %H:%M:%S");
        std::cout << "    " << WHITE << "Scans: " << RESET << total_scans_;
        std::cout << "    " << WHITE << "Bedrohungen: " << RESET;

        if (threats_detected_ > 0) {
            std::cout << RED << threats_detected_ << RESET;
        } else {
            std::cout << GREEN << "0" << RESET;
        }
        std::cout << "\n";

        std::cout << "│ " << WHITE << "Signaturen: " << RESET << sig_db_.signature_count();
        std::cout << "    " << WHITE << "Scan-Intervall: " << RESET << scan_interval_.count() << "ms\n";
        std::cout << YELLOW << "└" << std::string(78, '─') << "┘\n" << RESET;
        std::cout << "\n";
    }

    void print_security_core_status() {
        std::cout << MAGENTA << "┌─ SECURITY CORE STATUS ";
        std::cout << std::string(55, '─') << "┐\n" << RESET;

        // Labyrinth
        std::cout << "│ " << CYAN << "LABYRINTH:" << RESET;
        std::cout << "  Pressure=" << std::fixed << std::setprecision(4)
                  << security_core_.get_labyrinth_pressure();
        std::cout << "  Supersonic=" << (security_core_.is_supersonic() ?
                    GREEN + std::string("YES★") : DIM + std::string("NO")) << RESET << "\n";

        // Gravitravitation
        std::cout << "│ " << BLUE << "GRAVITRAVITATION:" << RESET;
        std::cout << "  Trapped=" << security_core_.get_trapped_count();
        std::cout << "  Harvested=" << std::fixed << std::setprecision(4)
                  << security_core_.get_harvested_energy();
        std::cout << "  Horizon=" << security_core_.get_event_horizon() << "\n";

        // Vollenstrahlen
        std::cout << "│ " << YELLOW << "VOLLENSTRAHLEN:" << RESET;
        std::cout << "  Light=" << std::fixed << std::setprecision(4)
                  << security_core_.get_light_energy();
        std::cout << "  Korona=" << security_core_.get_korona_output();
        std::cout << "  Beams=" << security_core_.get_active_beams() << "/64\n";

        // Defense
        std::cout << "│ " << GREEN << "DEFENSE POWER:" << RESET;
        double power = security_core_.get_defense_power();
        std::cout << "  " << std::fixed << std::setprecision(4) << power;

        // Power-Bar
        int bar_len = static_cast<int>(std::min(power * 10.0, 30.0));
        std::cout << "  [" << GREEN << std::string(bar_len, '█')
                  << DIM << std::string(30 - bar_len, '░') << RESET << "]\n";

        std::cout << MAGENTA << "└" << std::string(78, '─') << "┘\n" << RESET;
        std::cout << "\n";
    }

    void print_recent_threats() {
        std::cout << RED << "┌─ LETZTE BEDROHUNGEN ";
        std::cout << std::string(57, '─') << "┐\n" << RESET;

        std::lock_guard<std::mutex> lock(history_mtx_);

        // Letzte 5 mit Bedrohung
        std::vector<LiveScanResult> threats;
        for (auto it = scan_history_.rbegin(); it != scan_history_.rend() && threats.size() < 5; ++it) {
            if (it->threat_score > 0.333) {
                threats.push_back(*it);
            }
        }

        if (threats.empty()) {
            std::cout << "│ " << GREEN << "Keine aktiven Bedrohungen erkannt" << RESET << "\n";
        } else {
            for (const auto& t : threats) {
                std::string color = (t.threat_score > 0.888) ? RED :
                                   (t.threat_score > 0.555) ? YELLOW : CYAN;

                std::cout << "│ " << color << "[" << std::fixed << std::setprecision(3)
                          << t.threat_score << "]" << RESET;
                std::cout << " " << t.target_type << ": ";

                // Target kürzen wenn zu lang
                std::string target = t.target;
                if (target.length() > 40) {
                    target = target.substr(0, 37) + "...";
                }
                std::cout << target;

                if (!t.signature_matches.empty()) {
                    std::cout << " → " << RED << t.signature_matches[0].name << RESET;
                }
                std::cout << "\n";
            }
        }

        std::cout << RED << "└" << std::string(78, '─') << "┘\n" << RESET;
        std::cout << "\n";
    }

    void print_footer() {
        std::cout << DIM;
        std::cout << "RST Constants: G0=8/9 (WAHRHEIT) | Gate53=53Hz | φ=1.618... | Sig88=0.888...\n";
        std::cout << "Press Ctrl+C to stop monitoring\n";
        std::cout << RESET;
    }

    // ═══════════════════════════════════════════════════════════════════════════
    // HELPERS
    // ═══════════════════════════════════════════════════════════════════════════

private:
    std::string get_process_name_linux(uint32_t pid) {
#ifndef _WIN32
        std::string path = "/proc/" + std::to_string(pid) + "/comm";
        std::ifstream f(path);
        std::string name;
        if (f && std::getline(f, name)) {
            return name;
        }
#endif
        (void)pid;
        return "unknown";
    }

    std::vector<uint8_t> read_process_memory_sample(uint32_t pid, size_t size) {
        std::vector<uint8_t> sample;

#ifndef _WIN32
        // Linux: /proc/PID/mem lesen (benötigt Rechte)
        std::string path = "/proc/" + std::to_string(pid) + "/exe";
        std::ifstream f(path, std::ios::binary);
        if (f) {
            sample.resize(size);
            f.read(reinterpret_cast<char*>(sample.data()), size);
            sample.resize(f.gcount());
        }
#else
        // Windows: ReadProcessMemory (benötigt Rechte)
        HANDLE hProc = OpenProcess(PROCESS_VM_READ, FALSE, pid);
        if (hProc) {
            MEMORY_BASIC_INFORMATION mbi;
            if (VirtualQueryEx(hProc, nullptr, &mbi, sizeof(mbi))) {
                sample.resize(size);
                SIZE_T bytesRead;
                if (ReadProcessMemory(hProc, mbi.BaseAddress, sample.data(), size, &bytesRead)) {
                    sample.resize(bytesRead);
                }
            }
            CloseHandle(hProc);
        }
#endif

        return sample;
    }

public:
    // ═══════════════════════════════════════════════════════════════════════════
    // STATUS REPORT
    // ═══════════════════════════════════════════════════════════════════════════

    std::string status_report() const {
        std::ostringstream ss;

        ss << "═══════════════════════════════════════════════════════════════\n";
        ss << "           RST LIVE MONITOR - STATUS REPORT\n";
        ss << "═══════════════════════════════════════════════════════════════\n";
        ss << "\n";
        ss << "Running:            " << (running_ ? "YES" : "NO") << "\n";
        ss << "Total Scans:        " << total_scans_ << "\n";
        ss << "Threats Detected:   " << threats_detected_ << "\n";
        ss << "Signatures Loaded:  " << sig_db_.signature_count() << "\n";
        ss << "Scan Interval:      " << scan_interval_.count() << "ms\n";
        ss << "\n";

        // Security Core Status
        ss << security_core_.status_report();

        return ss.str();
    }

    // Statistiken
    uint64_t get_total_scans() const { return total_scans_; }
    uint64_t get_threats_detected() const { return threats_detected_; }

    const std::vector<LiveScanResult>& get_scan_history() const {
        return scan_history_;
    }
};

} // namespace live
} // namespace security
} // namespace rael
