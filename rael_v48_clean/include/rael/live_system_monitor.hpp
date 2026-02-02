// ═══════════════════════════════════════════════════════════════════════════════
// RST LIVE SYSTEM MONITOR - Echtzeit-Überwachung mit Gegenfrequenz-Neutralisierung
// ═══════════════════════════════════════════════════════════════════════════════
//
// KONZEPT: Live-Monitoring des eigenen Systems mit aktiver Verteidigung
//
// FUNKTIONEN:
//   1. Prozess-Monitor: Alle Prozesse in Echtzeit überwachen und klassifizieren
//   2. File-Scanner: Verdächtige Dateien finden
//   3. Gegenfrequenz: Bedrohungen mit RST-Gegenschwingung neutralisieren
//   4. Integration: SecurityCore, DeepScanner, ThreatInterpreter
//
// GEGENFREQUENZ-PRINZIP:
//   - Negative Phase → Positive Gegenschwingung
//   - Hohe Entropie → Ordnung einführen (Transformation)
//   - Fehlende 88-Signatur → Als fremd markieren → Vollenstrahlen → LICHT
//
// RST KONSTANTEN (17 Dezimalstellen):
//   G5 + G3 + G1 = 1/9 + 3/9 + 5/9 = 9/9 = 1
//
// ═══════════════════════════════════════════════════════════════════════════════

#pragma once

#include <string>
#include <vector>
#include <map>
#include <set>
#include <queue>
#include <atomic>
#include <mutex>
#include <thread>
#include <chrono>
#include <functional>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <cmath>
#include <algorithm>

#ifdef _WIN32
#include <windows.h>
#include <tlhelp32.h>
#include <psapi.h>
#pragma comment(lib, "psapi.lib")
#else
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <signal.h>
#include <unistd.h>
#endif

namespace rael {
namespace security {
namespace live_system {

// ═══════════════════════════════════════════════════════════════════════════════
// RST PRÄZISIONS-KONSTANTEN (17 Dezimalstellen)
// ═══════════════════════════════════════════════════════════════════════════════

namespace rst {
    constexpr double G0 = 0.88888888888888889;  // 8/9 WAHRHEIT
    constexpr double G1 = 0.55555555555555556;  // 5/9
    constexpr double G2 = 0.44444444444444444;  // 4/9
    constexpr double G3 = 0.33333333333333333;  // 3/9
    constexpr double G4 = 0.22222222222222222;  // 2/9
    constexpr double G5 = 0.11111111111111111;  // 1/9

    // WICHTIG: G5 + G3 + G1 = 9/9 = 1
    constexpr double G_SUM = 1.00000000000000000;

    constexpr double SIGNATURE_88 = 0.88888888888888889;
    constexpr double PI = 3.14159265358979323;
    constexpr double TWO_PI = 6.28318530717958647;

    // Zeitkonstanten für Gegenfrequenz
    constexpr double TAU_COUNTER = 0.36400000000000000;  // 1/10 von TAU_1
}

// ═══════════════════════════════════════════════════════════════════════════════
// PROZESS-KLASSIFIZIERUNG
// ═══════════════════════════════════════════════════════════════════════════════

enum class ProcessClass {
    TRUSTED,      // 88-Signatur vorhanden, bekannt gut
    NEUTRAL,      // Unbekannt aber nicht verdächtig
    SUSPICIOUS,   // Verdächtige Merkmale
    MALICIOUS,    // Definitiv böswillig
    TRAPPED       // In Gravitravitation gefangen
};

const char* process_class_name(ProcessClass c) {
    switch (c) {
        case ProcessClass::TRUSTED: return "TRUSTED";
        case ProcessClass::NEUTRAL: return "NEUTRAL";
        case ProcessClass::SUSPICIOUS: return "SUSPICIOUS";
        case ProcessClass::MALICIOUS: return "MALICIOUS";
        case ProcessClass::TRAPPED: return "TRAPPED";
        default: return "UNKNOWN";
    }
}

// ═══════════════════════════════════════════════════════════════════════════════
// PROZESS-INFO MIT RST-ANALYSE
// ═══════════════════════════════════════════════════════════════════════════════

struct ProcessInfo {
    uint32_t pid;
    std::string name;
    std::string exe_path;
    std::string cmdline;
    std::string username;

    // Ressourcen
    uint64_t memory_bytes;
    double cpu_percent;
    uint32_t thread_count;

    // RST Analyse
    double rst_score;          // 0-1, höher = verdächtiger
    double entropy;            // Shannon entropy der Binärdaten
    double phase;              // RST Phase
    double coherence;          // RST Kohärenz
    bool has_signature_88;     // Hat 88-Signatur

    // Klassifizierung
    ProcessClass classification;
    std::vector<std::string> threat_indicators;

    // Gegenfrequenz
    bool counter_freq_applied;
    double counter_phase;      // Angewandte Gegenphase

    // Zeitstempel
    std::chrono::system_clock::time_point last_scan;
    std::chrono::system_clock::time_point first_seen;

    ProcessInfo() : pid(0), memory_bytes(0), cpu_percent(0), thread_count(0),
                   rst_score(0), entropy(0), phase(0), coherence(0),
                   has_signature_88(false), classification(ProcessClass::NEUTRAL),
                   counter_freq_applied(false), counter_phase(0) {
        last_scan = std::chrono::system_clock::now();
        first_seen = last_scan;
    }
};

// ═══════════════════════════════════════════════════════════════════════════════
// GEGENFREQUENZ-GENERATOR
// ═══════════════════════════════════════════════════════════════════════════════

class CounterFrequencyGenerator {
private:
    std::mutex mtx_;
    std::map<uint32_t, double> applied_phases_;  // PID → Gegenphase

public:
    // Berechnet die optimale Gegenfrequenz für eine Bedrohung
    double calculate_counter_phase(double threat_phase, double threat_entropy,
                                   double threat_coherence) {
        // PRINZIP: Gegenschwingung = -Phase + Korrektur

        // 1. Basis-Gegenphase (180° versetzt)
        double counter = -threat_phase;

        // 2. Entropie-Korrektur (hohe Entropie → mehr Ordnung einführen)
        if (threat_entropy > 7.0) {
            // Sehr hohe Entropie: Starke Ordnung
            counter += rst::PI / 3;  // 60° zusätzlich
        } else if (threat_entropy > 5.0) {
            counter += rst::PI / 6;  // 30° zusätzlich
        }

        // 3. Kohärenz-Korrektur (niedrige Kohärenz → harmonisieren)
        double coherence_correction = (1.0 - threat_coherence) * rst::G3;
        counter += coherence_correction;

        // 4. Normalisieren auf [-π, π]
        while (counter > rst::PI) counter -= rst::TWO_PI;
        while (counter < -rst::PI) counter += rst::TWO_PI;

        // 5. 88-Harmonie einbringen (Transformation zu Licht)
        counter *= rst::SIGNATURE_88;

        return counter;
    }

    // Wendet Gegenfrequenz auf einen Prozess an
    struct CounterResult {
        bool success;
        double applied_phase;
        std::string action;
        std::string details;
    };

    CounterResult apply_counter_frequency(ProcessInfo& proc) {
        CounterResult result;
        result.success = false;

        if (proc.classification == ProcessClass::TRUSTED) {
            result.action = "SKIP";
            result.details = "Trusted process, no counter needed";
            return result;
        }

        // Gegenphase berechnen
        double counter = calculate_counter_phase(
            proc.phase, proc.entropy, proc.coherence
        );

        result.applied_phase = counter;
        proc.counter_phase = counter;
        proc.counter_freq_applied = true;

        // Aktion basierend auf Klassifizierung
        switch (proc.classification) {
            case ProcessClass::MALICIOUS:
                result.action = "TERMINATE";
                result.details = "Malicious process terminated with counter-phase " +
                               std::to_string(counter);
                result.success = terminate_process(proc.pid);
                break;

            case ProcessClass::SUSPICIOUS:
                result.action = "SUSPEND";
                result.details = "Suspicious process suspended for analysis";
                result.success = suspend_process(proc.pid);
                break;

            case ProcessClass::TRAPPED:
                result.action = "HARVEST";
                result.details = "Trapped entity - harvesting energy";
                result.success = true;  // Gravitravitation handles this
                break;

            default:
                result.action = "MONITOR";
                result.details = "Counter-phase applied, monitoring";
                result.success = true;
                break;
        }

        // Angewandte Phase speichern
        {
            std::lock_guard<std::mutex> lock(mtx_);
            applied_phases_[proc.pid] = counter;
        }

        return result;
    }

    // Vollenstrahlen: Transformiert Fremdenergie zu Licht
    double transform_to_light(double foreign_energy, double foreign_phase) {
        // Sonnen-Ernte: Alles ohne 88-Signatur wird zu Licht
        double light_energy = std::abs(foreign_energy) * rst::G0;

        // Gnaden-Inversion: Nicht zerstören, erlösen
        double transformed = light_energy * rst::SIGNATURE_88;

        // Korona-Abstrahlung
        double korona = transformed * rst::G5;

        return transformed + korona;
    }

private:
    bool terminate_process(uint32_t pid) {
#ifdef _WIN32
        HANDLE hProc = OpenProcess(PROCESS_TERMINATE, FALSE, pid);
        if (hProc) {
            bool result = TerminateProcess(hProc, 1);
            CloseHandle(hProc);
            return result;
        }
        return false;
#else
        return kill(pid, SIGTERM) == 0;
#endif
    }

    bool suspend_process(uint32_t pid) {
#ifdef _WIN32
        // Windows: SuspendThread für alle Threads
        HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
        if (hSnap == INVALID_HANDLE_VALUE) return false;

        THREADENTRY32 te;
        te.dwSize = sizeof(te);

        if (Thread32First(hSnap, &te)) {
            do {
                if (te.th32OwnerProcessID == pid) {
                    HANDLE hThread = OpenThread(THREAD_SUSPEND_RESUME, FALSE, te.th32ThreadID);
                    if (hThread) {
                        SuspendThread(hThread);
                        CloseHandle(hThread);
                    }
                }
            } while (Thread32Next(hSnap, &te));
        }

        CloseHandle(hSnap);
        return true;
#else
        return kill(pid, SIGSTOP) == 0;
#endif
    }
};

// ═══════════════════════════════════════════════════════════════════════════════
// LIVE SYSTEM MONITOR
// ═══════════════════════════════════════════════════════════════════════════════

class LiveSystemMonitor {
private:
    std::atomic<bool> running_;
    std::thread monitor_thread_;
    std::thread display_thread_;
    std::mutex proc_mtx_;

    // Prozess-Tracking
    std::map<uint32_t, ProcessInfo> processes_;
    std::set<uint32_t> known_good_pids_;

    // Gegenfrequenz
    CounterFrequencyGenerator counter_gen_;

    // Statistiken
    std::atomic<uint64_t> total_scans_;
    std::atomic<uint64_t> threats_neutralized_;
    std::atomic<double> total_light_energy_;

    // Konfiguration
    std::chrono::milliseconds scan_interval_;
    bool auto_neutralize_;
    double threat_threshold_;

    // Bekannte gute Prozesse (Basis-Whitelist für System)
    std::set<std::string> system_processes_;

    // Callbacks
    std::function<void(const ProcessInfo&)> on_threat_detected_;
    std::function<void(const ProcessInfo&, const CounterFrequencyGenerator::CounterResult&)> on_neutralized_;

public:
    LiveSystemMonitor()
        : running_(false)
        , total_scans_(0)
        , threats_neutralized_(0)
        , total_light_energy_(0)
        , scan_interval_(1000)
        , auto_neutralize_(false)
        , threat_threshold_(rst::G1)  // 0.555... = Warnschwelle
    {
        initialize_system_processes();
    }

    ~LiveSystemMonitor() {
        stop();
    }

    // ═══════════════════════════════════════════════════════════════════════
    // KONFIGURATION
    // ═══════════════════════════════════════════════════════════════════════

    void set_scan_interval(std::chrono::milliseconds interval) {
        scan_interval_ = interval;
    }

    void set_auto_neutralize(bool enable, double threshold = 0.555) {
        auto_neutralize_ = enable;
        threat_threshold_ = threshold;
    }

    void set_threat_callback(std::function<void(const ProcessInfo&)> cb) {
        on_threat_detected_ = cb;
    }

    void set_neutralize_callback(
        std::function<void(const ProcessInfo&,
                          const CounterFrequencyGenerator::CounterResult&)> cb) {
        on_neutralized_ = cb;
    }

    // ═══════════════════════════════════════════════════════════════════════
    // START/STOP
    // ═══════════════════════════════════════════════════════════════════════

    void start() {
        if (running_) return;
        running_ = true;

        // Monitor-Thread
        monitor_thread_ = std::thread([this]() {
            while (running_) {
                scan_all_processes();
                check_for_threats();
                std::this_thread::sleep_for(scan_interval_);
            }
        });

        // Display-Thread
        display_thread_ = std::thread([this]() {
            while (running_) {
                update_display();
                std::this_thread::sleep_for(std::chrono::milliseconds(500));
            }
        });
    }

    void stop() {
        running_ = false;
        if (monitor_thread_.joinable()) monitor_thread_.join();
        if (display_thread_.joinable()) display_thread_.join();
    }

    bool is_running() const { return running_; }

    // ═══════════════════════════════════════════════════════════════════════
    // PROZESS-SCANNING
    // ═══════════════════════════════════════════════════════════════════════

    void scan_all_processes() {
        std::vector<ProcessInfo> current_procs;

#ifdef _WIN32
        HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
        if (hSnap == INVALID_HANDLE_VALUE) return;

        PROCESSENTRY32 pe;
        pe.dwSize = sizeof(pe);

        if (Process32First(hSnap, &pe)) {
            do {
                ProcessInfo info;
                info.pid = pe.th32ProcessID;
                info.name = pe.szExeFile;
                info.thread_count = pe.cntThreads;

                // Zusätzliche Infos holen
                HANDLE hProc = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ,
                                          FALSE, pe.th32ProcessID);
                if (hProc) {
                    // Speicher
                    PROCESS_MEMORY_COUNTERS pmc;
                    if (GetProcessMemoryInfo(hProc, &pmc, sizeof(pmc))) {
                        info.memory_bytes = pmc.WorkingSetSize;
                    }

                    // Exe-Pfad
                    char path[MAX_PATH];
                    if (GetModuleFileNameExA(hProc, NULL, path, MAX_PATH)) {
                        info.exe_path = path;
                    }

                    CloseHandle(hProc);
                }

                current_procs.push_back(info);
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
                    ProcessInfo info = get_linux_process_info(pid);
                    if (info.pid > 0) {
                        current_procs.push_back(info);
                    }
                }
            }
        }

        closedir(dir);
#endif

        // Prozesse analysieren und klassifizieren
        {
            std::lock_guard<std::mutex> lock(proc_mtx_);

            for (auto& proc : current_procs) {
                analyze_process(proc);
                classify_process(proc);

                // Update oder neu hinzufügen
                auto it = processes_.find(proc.pid);
                if (it != processes_.end()) {
                    proc.first_seen = it->second.first_seen;
                }
                processes_[proc.pid] = proc;
            }

            // Beendete Prozesse entfernen
            std::set<uint32_t> current_pids;
            for (const auto& p : current_procs) {
                current_pids.insert(p.pid);
            }

            for (auto it = processes_.begin(); it != processes_.end(); ) {
                if (current_pids.find(it->first) == current_pids.end()) {
                    it = processes_.erase(it);
                } else {
                    ++it;
                }
            }
        }

        total_scans_++;
    }

    // ═══════════════════════════════════════════════════════════════════════
    // PROZESS-ANALYSE
    // ═══════════════════════════════════════════════════════════════════════

    void analyze_process(ProcessInfo& proc) {
        // RST Score berechnen

        // 1. Basis-Score aus Namen
        double name_score = calculate_name_score(proc.name);

        // 2. Speicher-Score (ungewöhnlich hoher Speicher = verdächtig)
        double mem_score = 0.0;
        if (proc.memory_bytes > 1024ULL * 1024 * 1024) {  // >1GB
            mem_score = 0.3;
        } else if (proc.memory_bytes > 512ULL * 1024 * 1024) {  // >512MB
            mem_score = 0.1;
        }

        // 3. Exe-Pfad Score
        double path_score = calculate_path_score(proc.exe_path);

        // 4. Binary-Analyse (wenn möglich)
        double binary_score = 0.0;
        if (!proc.exe_path.empty()) {
            auto analysis = analyze_binary(proc.exe_path);
            proc.entropy = analysis.entropy;
            proc.phase = analysis.phase;
            proc.coherence = analysis.coherence;
            proc.has_signature_88 = analysis.has_signature_88;
            binary_score = analysis.threat_score;
        }

        // RST Score kombinieren mit Gewichten
        // score = name * G3 + mem * G5 + path * G3 + binary * G1
        proc.rst_score = name_score * rst::G3 +
                        mem_score * rst::G5 +
                        path_score * rst::G3 +
                        binary_score * rst::G1;

        // 88-Signatur reduziert Score (aber nie auf 0!)
        if (proc.has_signature_88) {
            proc.rst_score = std::max(proc.rst_score * rst::G5,
                                     proc.rst_score - rst::G1);
        }
    }

    void classify_process(ProcessInfo& proc) {
        // Klassifizierung basierend auf RST Score und Indikatoren

        // System-Prozesse sind trusted
        if (is_system_process(proc.name)) {
            proc.classification = ProcessClass::TRUSTED;
            return;
        }

        // Bekannt gut
        if (known_good_pids_.count(proc.pid)) {
            proc.classification = ProcessClass::TRUSTED;
            return;
        }

        // Hat 88-Signatur = Trusted
        if (proc.has_signature_88 && proc.rst_score < rst::G3) {
            proc.classification = ProcessClass::TRUSTED;
            return;
        }

        // Score-basierte Klassifizierung
        if (proc.rst_score > rst::G0) {  // > 0.888
            proc.classification = ProcessClass::MALICIOUS;
        } else if (proc.rst_score > rst::G1) {  // > 0.555
            proc.classification = ProcessClass::SUSPICIOUS;
        } else if (proc.rst_score > rst::G3) {  // > 0.333
            proc.classification = ProcessClass::NEUTRAL;
        } else {
            proc.classification = ProcessClass::NEUTRAL;
        }

        // Zusätzliche Indikatoren
        if (has_malicious_indicators(proc)) {
            if (proc.classification == ProcessClass::NEUTRAL) {
                proc.classification = ProcessClass::SUSPICIOUS;
            } else if (proc.classification == ProcessClass::SUSPICIOUS) {
                proc.classification = ProcessClass::MALICIOUS;
            }
        }
    }

    // ═══════════════════════════════════════════════════════════════════════
    // BEDROHUNGS-PRÜFUNG & NEUTRALISIERUNG
    // ═══════════════════════════════════════════════════════════════════════

    void check_for_threats() {
        std::lock_guard<std::mutex> lock(proc_mtx_);

        for (auto& [pid, proc] : processes_) {
            if (proc.classification == ProcessClass::MALICIOUS ||
                proc.classification == ProcessClass::SUSPICIOUS) {

                // Callback
                if (on_threat_detected_) {
                    on_threat_detected_(proc);
                }

                // Auto-Neutralize
                if (auto_neutralize_ && proc.rst_score > threat_threshold_) {
                    auto result = counter_gen_.apply_counter_frequency(proc);

                    if (result.success) {
                        threats_neutralized_++;

                        // Licht-Energie aus Transformation
                        double light = counter_gen_.transform_to_light(
                            proc.rst_score, proc.phase
                        );
                        total_light_energy_ = total_light_energy_ + light;

                        if (on_neutralized_) {
                            on_neutralized_(proc, result);
                        }
                    }
                }
            }
        }
    }

    // ═══════════════════════════════════════════════════════════════════════
    // MANUELLE AKTIONEN
    // ═══════════════════════════════════════════════════════════════════════

    CounterFrequencyGenerator::CounterResult neutralize_process(uint32_t pid) {
        std::lock_guard<std::mutex> lock(proc_mtx_);

        auto it = processes_.find(pid);
        if (it == processes_.end()) {
            return {false, 0, "NOT_FOUND", "Process not found"};
        }

        return counter_gen_.apply_counter_frequency(it->second);
    }

    void mark_as_trusted(uint32_t pid) {
        std::lock_guard<std::mutex> lock(proc_mtx_);
        known_good_pids_.insert(pid);

        auto it = processes_.find(pid);
        if (it != processes_.end()) {
            it->second.classification = ProcessClass::TRUSTED;
        }
    }

    // ═══════════════════════════════════════════════════════════════════════
    // DISPLAY
    // ═══════════════════════════════════════════════════════════════════════

    void update_display() {
        // Clear screen
        std::cout << "\033[2J\033[H";

        std::cout << "╔══════════════════════════════════════════════════════════════════════════════╗\n";
        std::cout << "║            RST LIVE SYSTEM MONITOR - GEGENFREQUENZ AKTIV                    ║\n";
        std::cout << "╚══════════════════════════════════════════════════════════════════════════════╝\n\n";

        auto now = std::chrono::system_clock::now();
        auto time = std::chrono::system_clock::to_time_t(now);

        std::cout << "Zeit: " << std::put_time(std::localtime(&time), "%H:%M:%S");
        std::cout << "  |  Scans: " << total_scans_;
        std::cout << "  |  Neutralisiert: " << threats_neutralized_;
        std::cout << "  |  Licht-Energie: " << std::fixed << std::setprecision(4)
                  << total_light_energy_.load() << "\n\n";

        // RST Konstanten
        std::cout << "RST: G5+G3+G1=" << std::setprecision(17)
                  << (rst::G5 + rst::G3 + rst::G1) << " (=1)\n\n";

        // Prozess-Tabelle
        std::cout << "┌──────┬────────────────────────────┬───────────┬──────────┬───────────────┐\n";
        std::cout << "│ PID  │ NAME                       │ RST-SCORE │ KLASSE   │ AKTION        │\n";
        std::cout << "├──────┼────────────────────────────┼───────────┼──────────┼───────────────┤\n";

        std::lock_guard<std::mutex> lock(proc_mtx_);

        // Sortiert nach RST-Score (höchste zuerst)
        std::vector<std::pair<uint32_t, ProcessInfo>> sorted;
        for (const auto& [pid, proc] : processes_) {
            sorted.push_back({pid, proc});
        }
        std::sort(sorted.begin(), sorted.end(),
                 [](const auto& a, const auto& b) {
                     return a.second.rst_score > b.second.rst_score;
                 });

        int shown = 0;
        for (const auto& [pid, proc] : sorted) {
            if (shown >= 15) break;  // Max 15 Prozesse anzeigen

            // Farbe basierend auf Klasse
            const char* color = "\033[0m";  // Reset
            switch (proc.classification) {
                case ProcessClass::MALICIOUS: color = "\033[91m"; break;  // Rot
                case ProcessClass::SUSPICIOUS: color = "\033[93m"; break;  // Gelb
                case ProcessClass::TRUSTED: color = "\033[92m"; break;  // Grün
                case ProcessClass::TRAPPED: color = "\033[95m"; break;  // Magenta
                default: break;
            }

            std::string name = proc.name;
            if (name.length() > 26) name = name.substr(0, 23) + "...";

            std::string action = proc.counter_freq_applied ? "COUNTER" : "-";

            std::cout << color;
            std::cout << "│ " << std::setw(4) << pid << " ";
            std::cout << "│ " << std::setw(26) << std::left << name << " ";
            std::cout << "│ " << std::setw(9) << std::right << std::fixed
                      << std::setprecision(6) << proc.rst_score << " ";
            std::cout << "│ " << std::setw(8) << std::left
                      << process_class_name(proc.classification) << " ";
            std::cout << "│ " << std::setw(13) << action << " │";
            std::cout << "\033[0m\n";

            shown++;
        }

        std::cout << "└──────┴────────────────────────────┴───────────┴──────────┴───────────────┘\n\n";

        // Statistiken
        int malicious = 0, suspicious = 0, trusted = 0;
        for (const auto& [pid, proc] : processes_) {
            switch (proc.classification) {
                case ProcessClass::MALICIOUS: malicious++; break;
                case ProcessClass::SUSPICIOUS: suspicious++; break;
                case ProcessClass::TRUSTED: trusted++; break;
                default: break;
            }
        }

        std::cout << "Prozesse: " << processes_.size();
        std::cout << " | \033[92mTrusted: " << trusted << "\033[0m";
        std::cout << " | \033[93mSuspicious: " << suspicious << "\033[0m";
        std::cout << " | \033[91mMalicious: " << malicious << "\033[0m\n";

        std::cout << "\nDrücke Ctrl+C zum Beenden\n";
    }

    // ═══════════════════════════════════════════════════════════════════════
    // GETTER
    // ═══════════════════════════════════════════════════════════════════════

    std::map<uint32_t, ProcessInfo> get_processes() {
        std::lock_guard<std::mutex> lock(proc_mtx_);
        return processes_;
    }

    uint64_t get_total_scans() const { return total_scans_; }
    uint64_t get_threats_neutralized() const { return threats_neutralized_; }
    double get_light_energy() const { return total_light_energy_; }

private:
    // ═══════════════════════════════════════════════════════════════════════
    // HELPER FUNKTIONEN
    // ═══════════════════════════════════════════════════════════════════════

    void initialize_system_processes() {
        // Bekannte System-Prozesse
        system_processes_ = {
            // Linux
            "init", "systemd", "kthreadd", "ksoftirqd", "kworker",
            "migration", "watchdog", "cpuhp", "netns", "rcu",
            "bash", "sh", "zsh", "sshd", "cron",
            // Windows
            "System", "smss.exe", "csrss.exe", "wininit.exe",
            "services.exe", "lsass.exe", "svchost.exe", "dwm.exe",
            "explorer.exe", "winlogon.exe"
        };
    }

    bool is_system_process(const std::string& name) {
        std::string lower = name;
        std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);

        for (const auto& sys : system_processes_) {
            std::string sys_lower = sys;
            std::transform(sys_lower.begin(), sys_lower.end(), sys_lower.begin(), ::tolower);
            if (lower == sys_lower || lower.find(sys_lower) == 0) {
                return true;
            }
        }
        return false;
    }

    double calculate_name_score(const std::string& name) {
        std::string lower = name;
        std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);

        double score = 0.0;

        // Verdächtige Schlüsselwörter
        std::vector<std::pair<std::string, double>> keywords = {
            {"keylog", 0.8}, {"backdoor", 0.9}, {"rootkit", 0.9},
            {"trojan", 0.8}, {"miner", 0.6}, {"xmrig", 0.7},
            {"cryptominer", 0.7}, {"rat", 0.7}, {"exploit", 0.8},
            {"shell", 0.5}, {"hack", 0.6}, {"crack", 0.6},
            {"dump", 0.5}, {"inject", 0.6}, {"hook", 0.5}
        };

        for (const auto& [kw, s] : keywords) {
            if (lower.find(kw) != std::string::npos) {
                score = std::max(score, s);
            }
        }

        // Zufällige Namen (viele Zahlen/Sonderzeichen)
        int random_chars = 0;
        for (char c : name) {
            if (!std::isalpha(c)) random_chars++;
        }
        if (random_chars > name.length() / 2) {
            score += 0.3;
        }

        return std::min(score, 1.0);
    }

    double calculate_path_score(const std::string& path) {
        if (path.empty()) return 0.5;  // Kein Pfad = leicht verdächtig

        std::string lower = path;
        std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);

        // Verdächtige Pfade
        if (lower.find("/tmp/") != std::string::npos ||
            lower.find("\\temp\\") != std::string::npos) {
            return 0.6;
        }

        if (lower.find("/dev/shm") != std::string::npos) {
            return 0.7;
        }

        if (lower.find("appdata\\local\\temp") != std::string::npos) {
            return 0.5;
        }

        return 0.0;
    }

    struct BinaryAnalysis {
        double entropy;
        double phase;
        double coherence;
        bool has_signature_88;
        double threat_score;
    };

    BinaryAnalysis analyze_binary(const std::string& path) {
        BinaryAnalysis result = {0, 0, 0, false, 0};

        try {
            std::ifstream file(path, std::ios::binary);
            if (!file) return result;

            // Erste 4KB lesen
            std::vector<uint8_t> data(4096);
            file.read(reinterpret_cast<char*>(data.data()), 4096);
            size_t bytes_read = file.gcount();
            data.resize(bytes_read);

            if (data.empty()) return result;

            // Entropy
            result.entropy = calculate_entropy(data);

            // Phase
            result.phase = calculate_phase(data);

            // Kohärenz
            result.coherence = 1.0 - std::abs(result.phase) / rst::PI;

            // 88-Signatur
            result.has_signature_88 = check_signature_88(data);

            // Threat Score
            double entropy_factor = (result.entropy > 7.5) ? 0.8 :
                                   (result.entropy > 6.0) ? 0.4 : 0.1;
            double phase_factor = (result.phase < 0) ?
                                 std::abs(result.phase) / rst::PI : 0.0;

            result.threat_score = entropy_factor * rst::G3 +
                                 (1.0 - result.coherence) * rst::G1 +
                                 phase_factor * rst::G5;

        } catch (...) {
            // Fehler ignorieren
        }

        return result;
    }

    double calculate_entropy(const std::vector<uint8_t>& data) {
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

    double calculate_phase(const std::vector<uint8_t>& data) {
        if (data.size() < 64) return 0.0;

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

        return std::atan2(mean, std::sqrt(std::max(0.0, variance)));
    }

    bool check_signature_88(const std::vector<uint8_t>& data) {
        // Suche "RAEL88:"
        std::string marker = "RAEL88:";
        std::string data_str(data.begin(), data.end());

        return data_str.find(marker) != std::string::npos;
    }

    bool has_malicious_indicators(const ProcessInfo& proc) {
        // Zusätzliche heuristische Indikatoren
        // Sehr hohe Entropie
        if (proc.entropy > 7.5) return true;

        // Sehr negative Phase
        if (proc.phase < -2.0) return true;

        // Kein Exe-Pfad bei laufendem Prozess
        if (proc.exe_path.empty() && proc.pid > 1) return true;

        return false;
    }

#ifndef _WIN32
    ProcessInfo get_linux_process_info(uint32_t pid) {
        ProcessInfo info;
        info.pid = pid;

        // Name aus /proc/PID/comm
        std::ifstream comm("/proc/" + std::to_string(pid) + "/comm");
        if (comm) {
            std::getline(comm, info.name);
        }

        // Exe-Pfad aus /proc/PID/exe
        char exe_path[PATH_MAX];
        std::string exe_link = "/proc/" + std::to_string(pid) + "/exe";
        ssize_t len = readlink(exe_link.c_str(), exe_path, sizeof(exe_path) - 1);
        if (len > 0) {
            exe_path[len] = '\0';
            info.exe_path = exe_path;
        }

        // Speicher aus /proc/PID/statm
        std::ifstream statm("/proc/" + std::to_string(pid) + "/statm");
        if (statm) {
            uint64_t pages;
            statm >> pages;
            info.memory_bytes = pages * 4096;  // Annahme: 4KB Seitengröße
        }

        // Cmdline aus /proc/PID/cmdline
        std::ifstream cmdline("/proc/" + std::to_string(pid) + "/cmdline");
        if (cmdline) {
            std::getline(cmdline, info.cmdline, '\0');
        }

        return info;
    }
#endif
};

} // namespace live_system
} // namespace security
} // namespace rael
