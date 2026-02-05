// ═══════════════════════════════════════════════════════════════════════════════
// RAEL SECURITY DASHBOARD - Unified GUI für alle Scanner
// ═══════════════════════════════════════════════════════════════════════════════
//
// INTEGRIERTE KOMPONENTEN:
//   - Live System Monitor (Prozesse)
//   - Deep Scanner (Dateien, Boot, Shadow Partitions)
//   - Network Filter (Verbindungen, Pakete)
//   - Security Core (Gravitravitation, Vollenstrahlen)
//   - Threat Interpreter (RST Analyse)
//
// ANSICHTEN:
//   [0] KI System    - LLM, Neural Memory, Knowledge Graph, Semantic Engine
//   [1] Dashboard    - Übersicht aller Systeme
//   [2] Prozesse     - Live Prozess-Monitor mit Klassifizierung
//   [3] Dateien      - File Scanner mit RST Analyse
//   [4] Netzwerk     - Verbindungen und Paket-Inspektion
//   [5] Bedrohungen  - Alert-Log mit Details
//   [6] RST Status   - Gravitravitation, Vollenstrahlen, Defense Power
//   [7] Einstellungen
//   [8] SI Module     - Semantische Intelligenz / Programmierbarkeit
//   [9] Improve       - Reflektion & Verbesserungsvorschläge
//
// ═══════════════════════════════════════════════════════════════════════════════

#pragma once

#include <string>
#include <vector>
#include <map>
#include <atomic>
#include <mutex>
#include <thread>
#include <chrono>
#include <functional>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <deque>

#include "rael/security_core.hpp"
#include "rael/live_system_monitor.hpp"
#include "rael/rst_deep_scanner.hpp"
#include "rael/threat_interpreter.hpp"
#include "rael/module_manager.h"
#include "rael/improvements.h"
#include "rael/reflection_engine.h"
#include "rael/shadow_sim.h"
#include "rael/metrics.h"
#include "rael/telemetry.h"
#include "rael/ethics.h"
#include "rael/resonance.h"
#include "rael/ichbin.h"
// KI Headers
#include "rael/rst_semantic_engine.hpp"
// Note: llm_runtime.h, neural_memory.h, knowledge_graph.h haben Windows-Makro-Konflikte
// (NEAR, FAR etc.) - werden für Windows ohne direkte Header-Einbindung unterstützt

#ifdef _WIN32
#include <windows.h>
#include <conio.h>
#else
#include <termios.h>
#include <unistd.h>
#include <sys/ioctl.h>
#endif

namespace rael {
namespace security {
namespace dashboard {

// Import types from security_core namespace
using rael::security_core::SecurityCore;
using rael::security_core::Threat;
using rael::security_core::ThreatType;
using rael::security_core::threat_name;

// ═══════════════════════════════════════════════════════════════════════════════
// ANSI FARBEN UND STYLES
// ═══════════════════════════════════════════════════════════════════════════════

namespace color {
#ifdef _WIN32
    const std::string RESET = "";
    const std::string BOLD = "";
    const std::string DIM = "";
    const std::string RED = "";
    const std::string GREEN = "";
    const std::string YELLOW = "";
    const std::string BLUE = "";
    const std::string MAGENTA = "";
    const std::string CYAN = "";
    const std::string WHITE = "";
    const std::string BG_RED = "";
    const std::string BG_GREEN = "";
    const std::string BG_BLUE = "";
#else
    const std::string RESET = "\033[0m";
    const std::string BOLD = "\033[1m";
    const std::string DIM = "\033[2m";
    const std::string RED = "\033[91m";
    const std::string GREEN = "\033[92m";
    const std::string YELLOW = "\033[93m";
    const std::string BLUE = "\033[94m";
    const std::string MAGENTA = "\033[95m";
    const std::string CYAN = "\033[96m";
    const std::string WHITE = "\033[97m";
    const std::string BG_RED = "\033[41m";
    const std::string BG_GREEN = "\033[42m";
    const std::string BG_BLUE = "\033[44m";
#endif
}

// ═══════════════════════════════════════════════════════════════════════════════
// ALERT STRUCT
// ═══════════════════════════════════════════════════════════════════════════════

struct Alert {
    enum Level { INFO, WARNING, CRITICAL };

    Level level;
    std::string source;      // "PROCESS", "FILE", "NETWORK", "BOOT"
    std::string message;
    std::string details;
    double threat_score;
    std::chrono::system_clock::time_point timestamp;
    bool acknowledged;

    Alert() : level(INFO), threat_score(0), acknowledged(false) {
        timestamp = std::chrono::system_clock::now();
    }
};

// ═══════════════════════════════════════════════════════════════════════════════
// VIEW ENUM
// ═══════════════════════════════════════════════════════════════════════════════

enum class View {
    KI_SYSTEM = 0,
    DASHBOARD = 1,
    PROCESSES = 2,
    FILES = 3,
    NETWORK = 4,
    ALERTS = 5,
    RST_STATUS = 6,
    SETTINGS = 7,
    SI_MODULES = 8,
    IMPROVE = 9
};

// ═══════════════════════════════════════════════════════════════════════════════
// RST KONSTANTEN
// ═══════════════════════════════════════════════════════════════════════════════

namespace rst {
    constexpr double G0 = 0.88888888888888889;
    constexpr double G1 = 0.55555555555555556;
    constexpr double G3 = 0.33333333333333333;
    constexpr double G5 = 0.11111111111111111;
}

// ═══════════════════════════════════════════════════════════════════════════════
// SECURITY DASHBOARD - Hauptklasse
// ═══════════════════════════════════════════════════════════════════════════════

class SecurityDashboard {
private:
    // Komponenten
    SecurityCore security_core_;
    live_system::LiveSystemMonitor process_monitor_;
    deep::RSTOmegaDeepScanner deep_scanner_;
    interpret::ThreatInterpreter interpreter_;
    ModuleManager module_manager_;  // SI Programmierbarkeit

    // State
    std::atomic<bool> running_;
    View current_view_;
    std::mutex mtx_;

    // Threads
    std::thread render_thread_;
    std::thread input_thread_;
    std::thread scan_thread_;

    // Alerts
    std::deque<Alert> alerts_;
    static constexpr size_t MAX_ALERTS = 100;

    // Statistiken
    std::atomic<uint64_t> total_scans_;
    std::atomic<uint64_t> threats_blocked_;
    std::atomic<uint64_t> files_quarantined_;
    std::atomic<uint64_t> files_scanned_count_;
    std::atomic<uint64_t> threats_found_count_;

    // Settings
    bool auto_neutralize_;
    bool scan_on_start_;
    int refresh_rate_ms_;
    double threat_threshold_;

    // Terminal Size
    int term_width_;
    int term_height_;

public:
    SecurityDashboard()
        : running_(false)
        , current_view_(View::DASHBOARD)
        , total_scans_(0)
        , threats_blocked_(0)
        , files_quarantined_(0)
        , files_scanned_count_(0)
        , threats_found_count_(0)
        , auto_neutralize_(true)
        , scan_on_start_(true)
        , refresh_rate_ms_(500)
        , threat_threshold_(rst::G1)
        , term_width_(120)
        , term_height_(40)
    {
        initialize_callbacks();
    }

    ~SecurityDashboard() {
        stop();
    }

    // ═══════════════════════════════════════════════════════════════════════
    // START/STOP
    // ═══════════════════════════════════════════════════════════════════════

    void start() {
        if (running_) return;
        running_ = true;

        // Terminal vorbereiten
        setup_terminal();
        get_terminal_size();

        // Security Core starten
        security_core_.start();

        // Process Monitor starten
        process_monitor_.set_auto_neutralize(auto_neutralize_, threat_threshold_);
        process_monitor_.start();

        // Threads starten
        render_thread_ = std::thread([this]() { render_loop(); });
        input_thread_ = std::thread([this]() { input_loop(); });

        // Initial Scan
        if (scan_on_start_) {
            scan_thread_ = std::thread([this]() {
                add_alert(Alert::INFO, "SYSTEM", "Initialer System-Scan gestartet...", "");
                // Quick scan
                std::vector<std::string> paths = {"/home", "/tmp"};
#ifdef _WIN32
                paths = {"C:\\Users", "C:\\Windows\\Temp"};
#endif
                auto report = deep_scanner_.full_system_scan(paths, false, false, 7.0);

                for (const auto& threat : report.filesystem_threats) {
                    add_alert(
                        threat.threat_level > rst::G0 ? Alert::CRITICAL : Alert::WARNING,
                        "FILE",
                        threat.threat_name,
                        threat.path
                    );
                }
            });
        }
    }

    void stop() {
        running_ = false;

        security_core_.stop();
        process_monitor_.stop();

        if (render_thread_.joinable()) render_thread_.join();
        if (input_thread_.joinable()) input_thread_.join();
        if (scan_thread_.joinable()) scan_thread_.join();

        restore_terminal();
    }

    // ═══════════════════════════════════════════════════════════════════════
    // RENDER LOOP
    // ═══════════════════════════════════════════════════════════════════════

    void render_loop() {
        while (running_) {
            clear_screen();

            render_header();

            switch (current_view_) {
                case View::KI_SYSTEM:
                    render_ki_system();
                    break;
                case View::DASHBOARD:
                    render_dashboard();
                    break;
                case View::PROCESSES:
                    render_processes();
                    break;
                case View::FILES:
                    render_files();
                    break;
                case View::NETWORK:
                    render_network();
                    break;
                case View::ALERTS:
                    render_alerts();
                    break;
                case View::RST_STATUS:
                    render_rst_status();
                    break;
                case View::SETTINGS:
                    render_settings();
                    break;
                case View::SI_MODULES:
                    render_si_modules();
                    break;
                case View::IMPROVE:
                    render_improve();
                    break;
            }

            render_footer();

            std::this_thread::sleep_for(std::chrono::milliseconds(refresh_rate_ms_));
        }
    }

    // ═══════════════════════════════════════════════════════════════════════
    // HEADER
    // ═══════════════════════════════════════════════════════════════════════

    void render_header() {
        auto now = std::chrono::system_clock::now();
        auto time = std::chrono::system_clock::to_time_t(now);

        // Logo Box
        std::cout << color::CYAN << color::BOLD;
        std::cout << "╔══════════════════════════════════════════════════════════════════════════════════════════════════════════════════╗\n";
        std::cout << "║     ██████╗  █████╗ ███████╗██╗         ███████╗███████╗ ██████╗██╗   ██╗██████╗ ██╗████████╗██╗   ██╗          ║\n";
        std::cout << "║     ██╔══██╗██╔══██╗██╔════╝██║         ██╔════╝██╔════╝██╔════╝██║   ██║██╔══██╗██║╚══██╔══╝╚██╗ ██╔╝          ║\n";
        std::cout << "║     ██████╔╝███████║█████╗  ██║         ███████╗█████╗  ██║     ██║   ██║██████╔╝██║   ██║    ╚████╔╝           ║\n";
        std::cout << "║     ██╔══██╗██╔══██║██╔══╝  ██║         ╚════██║██╔══╝  ██║     ██║   ██║██╔══██╗██║   ██║     ╚██╔╝            ║\n";
        std::cout << "║     ██║  ██║██║  ██║███████╗███████╗    ███████║███████╗╚██████╗╚██████╔╝██║  ██║██║   ██║      ██║             ║\n";
        std::cout << "║     ╚═╝  ╚═╝╚═╝  ╚═╝╚══════╝╚══════╝    ╚══════╝╚══════╝ ╚═════╝ ╚═════╝ ╚═╝  ╚═╝╚═╝   ╚═╝      ╚═╝             ║\n";
        std::cout << "╠══════════════════════════════════════════════════════════════════════════════════════════════════════════════════╣\n";
        std::cout << color::RESET;

        // Status Bar
        std::cout << "║ " << color::WHITE;
        std::cout << std::put_time(std::localtime(&time), "%Y-%m-%d %H:%M:%S");
        std::cout << color::RESET << " │ ";

        // Quick Stats
        int critical_alerts = count_critical_alerts();
        if (critical_alerts > 0) {
            std::cout << color::BG_RED << color::WHITE << " ⚠ " << critical_alerts << " KRITISCH " << color::RESET << " ";
        } else {
            std::cout << color::BG_GREEN << color::WHITE << " ✓ SICHER " << color::RESET << " ";
        }

        std::cout << "│ Scans: " << total_scans_;
        std::cout << " │ Blockiert: " << threats_blocked_;
        std::cout << " │ Defense: " << std::fixed << std::setprecision(2)
                  << security_core_.get_defense_power();

        // Padding
        std::cout << std::string(30, ' ') << "║\n";

        // Navigation
        std::cout << "╠══════════════════════════════════════════════════════════════════════════════════════════════════════════════════╣\n";
        std::cout << "║ ";

        render_nav_item("0", "KI", current_view_ == View::KI_SYSTEM);
        render_nav_item("1", "Dashboard", current_view_ == View::DASHBOARD);
        render_nav_item("2", "Prozesse", current_view_ == View::PROCESSES);
        render_nav_item("3", "Dateien", current_view_ == View::FILES);
        render_nav_item("4", "Netzwerk", current_view_ == View::NETWORK);
        render_nav_item("5", "Alerts", current_view_ == View::ALERTS);
        render_nav_item("6", "RST", current_view_ == View::RST_STATUS);
        render_nav_item("7", "Settings", current_view_ == View::SETTINGS);
        render_nav_item("8", "SI", current_view_ == View::SI_MODULES);
        render_nav_item("9", "Improve", current_view_ == View::IMPROVE);

        std::cout << "║\n";
        std::cout << "╠══════════════════════════════════════════════════════════════════════════════════════════════════════════════════╣\n";
    }

    void render_nav_item(const std::string& key, const std::string& label, bool active) {
        if (active) {
            std::cout << color::BG_BLUE << color::WHITE << color::BOLD;
        } else {
            std::cout << color::DIM;
        }
        std::cout << " [" << key << "] " << label << " ";
        std::cout << color::RESET << " ";
    }

    // ═══════════════════════════════════════════════════════════════════════
    // DASHBOARD VIEW
    // ═══════════════════════════════════════════════════════════════════════

    // ═══════════════════════════════════════════════════════════════════════
    // KI SYSTEM VIEW
    // ═══════════════════════════════════════════════════════════════════════

    void render_ki_system() {
        std::cout << "║                                " << color::BOLD << "RAEL KI SYSTEM - KÜNSTLICHE INTELLIGENZ" << color::RESET << "                                         ║\n";
        std::cout << "╠══════════════════════════════════════════════════════════════════════════════════════════════════════════════════════╣\n";

        // LLM Runtime Status
        std::cout << "║ " << color::CYAN << color::BOLD << "LLM RUNTIME" << color::RESET << " (Lokale Inferenz ohne Cloud)                                                                   ║\n";
        std::cout << "╠══════════════════════════════════════════════════════════════════════════════════════════════════════════════════════╣\n";
        std::cout << "║   Status:             " << color::GREEN << "BEREIT" << color::RESET << "                                                                              ║\n";
        std::cout << "║   Quantisierung:      Q4_0 (4-bit, schnell)                                                                          ║\n";
        std::cout << "║   Kontext:            4096 Token                                                                                     ║\n";
        std::cout << "║   Threads:            " << std::thread::hardware_concurrency() << "                                                                                            ║\n";

        // Neural Memory
        std::cout << "╠══════════════════════════════════════════════════════════════════════════════════════════════════════════════════════╣\n";
        std::cout << "║ " << color::MAGENTA << color::BOLD << "NEURAL MEMORY" << color::RESET << " (Langzeitgedächtnis mit Vergessenskurve)                                                       ║\n";
        std::cout << "╠══════════════════════════════════════════════════════════════════════════════════════════════════════════════════════╣\n";
        std::cout << "║   EPISODIC:   Ereignisse/Erfahrungen    │  SEMANTIC:    Fakten/Konzepte                                              ║\n";
        std::cout << "║   PROCEDURAL: Wie man Dinge tut         │  EMOTIONAL:   Gefühls-Assoziationen                                        ║\n";
        std::cout << "║   WORKING:    Kurzzeit/Aktiv            │  Embeddings:  Similarity Search                                            ║\n";

        // Knowledge Graph
        std::cout << "╠══════════════════════════════════════════════════════════════════════════════════════════════════════════════════════╣\n";
        std::cout << "║ " << color::YELLOW << color::BOLD << "KNOWLEDGE GRAPH" << color::RESET << " (Graph-basiertes Wissen)                                                                      ║\n";
        std::cout << "╠══════════════════════════════════════════════════════════════════════════════════════════════════════════════════════╣\n";
        std::cout << "║   Node-Typen:   ENTITY, CONCEPT, EVENT, PROPERTY, LITERAL, RULE, QUERY, CONTEXT                                      ║\n";
        std::cout << "║   Edge-Typen:   IS_A, PART_OF, RELATED_TO, SIMILAR_TO, CAUSES, ENABLES, PREVENTS                                     ║\n";
        std::cout << "║   Reasoning:    Pfad-Traversal, Pattern-Matching, Inferenz-Regeln                                                    ║\n";

        // RST Semantic Engine
        std::cout << "╠══════════════════════════════════════════════════════════════════════════════════════════════════════════════════════╣\n";
        std::cout << "║ " << color::CYAN << color::BOLD << "RST SEMANTIC ENGINE" << color::RESET << " (Resonanz-basierte Verarbeitung)                                                           ║\n";
        std::cout << "╠══════════════════════════════════════════════════════════════════════════════════════════════════════════════════════╣\n";
        std::cout << "║   Ethics Filter:      53 Hz (Sophie-Germain Primzahl)                                                                ║\n";
        std::cout << "║   Semantic Field:     Resonanz-basiert mit RST-Konstanten                                                            ║\n";
        std::cout << "║   Intent Processing:  INTENTION{...} Parsing                                                                         ║\n";

        // RST Frequenzen
        std::cout << "╠══════════════════════════════════════════════════════════════════════════════════════════════════════════════════════╣\n";
        std::cout << "║ " << color::GREEN << color::BOLD << "RST FREQUENZEN" << color::RESET << "                                                                                                ║\n";
        std::cout << "╠══════════════════════════════════════════════════════════════════════════════════════════════════════════════════════╣\n";
        std::cout << "║   F_QUELLE  = 1440 Hz (Quell-Frequenz)    │  F_KAMMER = 432 Hz (Resonanz-Kammer)                                     ║\n";
        std::cout << "║   F_FILTER  =   53 Hz (Ethics Filter)     │  F_MATERIE=   5 Hz (Materiefeld)                                         ║\n";
        std::cout << "║   PHI       = 1.618... (Goldener Schnitt) │  PI       = 3.141... (Kreiszahl)                                         ║\n";

        // Konstanten
        std::cout << "╠══════════════════════════════════════════════════════════════════════════════════════════════════════════════════════╣\n";
        std::cout << "║ " << color::DIM << "G0=8/9 (WAHRHEIT) │ G1=5/9 │ G2=4/9 │ G3=3/9 │ G4=2/9 │ G5=1/9 │ G5+G3+G1=9/9=1"
                  << color::RESET << std::string(20, ' ') << "║\n";

        // Aktionen
        std::cout << "╠══════════════════════════════════════════════════════════════════════════════════════════════════════════════════════╣\n";
        std::cout << "║ " << color::DIM << "[L] LLM laden │ [M] Memory Query │ [K] Knowledge Query │ [S] Semantic Process │ [E] Ethics Check"
                  << color::RESET << "  ║\n";
    }

    // ═══════════════════════════════════════════════════════════════════════
    // DASHBOARD VIEW
    // ═══════════════════════════════════════════════════════════════════════

    void render_dashboard() {
        std::cout << "║                                    " << color::BOLD << "SYSTEM ÜBERSICHT" << color::RESET << "                                                          ║\n";
        std::cout << "╠═══════════════════════════════════════╦═══════════════════════════════════════╦══════════════════════════════════════╣\n";

        // Drei Spalten: Prozesse | Netzwerk | RST Status

        // Row 1: Headers
        std::cout << "║ " << color::CYAN << "PROZESSE" << color::RESET << "                              ";
        std::cout << "║ " << color::CYAN << "NETZWERK" << color::RESET << "                              ";
        std::cout << "║ " << color::CYAN << "RST VERTEIDIGUNG" << color::RESET << "                    ║\n";
        std::cout << "╠═══════════════════════════════════════╬═══════════════════════════════════════╬══════════════════════════════════════╣\n";

        // Prozess-Stats
        auto procs = process_monitor_.get_processes();
        int malicious = 0, suspicious = 0, trusted = 0;
        for (const auto& [pid, proc] : procs) {
            switch (proc.classification) {
                case live_system::ProcessClass::MALICIOUS: malicious++; break;
                case live_system::ProcessClass::SUSPICIOUS: suspicious++; break;
                case live_system::ProcessClass::TRUSTED: trusted++; break;
                default: break;
            }
        }

        std::cout << "║ Total:      " << std::setw(5) << procs.size() << "                      ";
        std::cout << "║ Verbindungen: " << std::setw(5) << "N/A" << "                   ";
        std::cout << "║ Defense Power: " << std::setw(8) << std::fixed << std::setprecision(4)
                  << security_core_.get_defense_power() << "          ║\n";

        std::cout << "║ " << color::GREEN << "Trusted:    " << std::setw(5) << trusted << color::RESET << "                      ";
        std::cout << "║ Eingehend:    " << std::setw(5) << "N/A" << "                   ";
        std::cout << "║ Trapped:       " << std::setw(5) << security_core_.get_trapped_count() << "            ║\n";

        std::cout << "║ " << color::YELLOW << "Suspicious: " << std::setw(5) << suspicious << color::RESET << "                      ";
        std::cout << "║ Ausgehend:    " << std::setw(5) << "N/A" << "                   ";
        std::cout << "║ Harvested:     " << std::setw(8) << std::setprecision(4)
                  << security_core_.get_harvested_energy() << "          ║\n";

        std::cout << "║ " << color::RED << "Malicious:  " << std::setw(5) << malicious << color::RESET << "                      ";
        std::cout << "║ Blocked:      " << std::setw(5) << threats_blocked_.load() << "                   ";
        std::cout << "║ Light Energy:  " << std::setw(8) << std::setprecision(4)
                  << security_core_.get_light_energy() << "          ║\n";

        std::cout << "╠═══════════════════════════════════════╩═══════════════════════════════════════╩══════════════════════════════════════╣\n";

        // Recent Alerts
        std::cout << "║ " << color::BOLD << "LETZTE ALERTS" << color::RESET << "                                                                                                   ║\n";
        std::cout << "╠══════════════════════════════════════════════════════════════════════════════════════════════════════════════════════╣\n";

        std::lock_guard<std::mutex> lock(mtx_);
        int shown = 0;
        for (auto it = alerts_.rbegin(); it != alerts_.rend() && shown < 5; ++it, ++shown) {
            render_alert_line(*it);
        }

        if (alerts_.empty()) {
            std::cout << "║ " << color::GREEN << "Keine Alerts - System läuft normal" << color::RESET
                      << std::string(75, ' ') << "║\n";
        }

        // RAEL Core Status
        std::cout << "╠═══════════════════════════════════════╦═══════════════════════════════════════╦══════════════════════════════════════╣\n";
        std::cout << "║ " << color::CYAN << "METRICS" << color::RESET << "                               ";
        std::cout << "║ " << color::CYAN << "REFLEKTION" << color::RESET << "                            ";
        std::cout << "║ " << color::CYAN << "ETHICS" << color::RESET << "                              ║\n";
        std::cout << "╠═══════════════════════════════════════╬═══════════════════════════════════════╬══════════════════════════════════════╣\n";

        // Metriken
        std::cout << "║ Ops/sec:      " << std::setw(10) << gMetrics.ops_sec.load() << "              ";

        // Improvements
        auto pending_imps = ImprovementBus::by_status(ImprovementStatus::PENDING);
        std::cout << "║ Pending:      " << std::setw(10) << pending_imps.size() << "              ";

        // Ethics
        std::cout << "║ Blocks:        " << std::setw(6) << gMetrics.ethics_blocks.load() << "            ║\n";

        std::cout << "║ Semantic:     " << std::setw(10) << gMetrics.semantic_calls.load() << "              ";
        std::cout << "║ Shadow-Sims:  " << std::setw(10) << gShadowSim.get_history().size() << "              ";
        std::cout << "║ Resonance:     " << std::setw(6) << gMetrics.resonance_calls.load() << "            ║\n";

        std::cout << "║ Hotswaps:     " << std::setw(10) << gMetrics.hotswaps.load() << "              ";
        std::cout << "║ Rollbacks:    " << std::setw(10) << gRollback.list_points().size() << "              ";
        std::cout << "║ " << color::GREEN << "ICHBIN: " << IchBinCore::signature() << color::RESET << "       ║\n";

        std::cout << "╠═══════════════════════════════════════╩═══════════════════════════════════════╩══════════════════════════════════════╣\n";

        // RST Konstanten
        std::cout << "║ " << color::DIM << "RST: G0=8/9 (WAHRHEIT) │ G1=5/9 │ G3=3/9 │ G5=1/9 │ G5+G3+G1=9/9=1 │ Sig88=0.888..."
                  << color::RESET << std::string(19, ' ') << "║\n";
    }

    // ═══════════════════════════════════════════════════════════════════════
    // PROCESSES VIEW
    // ═══════════════════════════════════════════════════════════════════════

    void render_processes() {
        std::cout << "║                                     " << color::BOLD << "PROZESS MONITOR" << color::RESET << "                                                           ║\n";
        std::cout << "╠══════════════════════════════════════════════════════════════════════════════════════════════════════════════════════╣\n";

        // Header
        std::cout << "║ " << color::BOLD;
        std::cout << std::setw(6) << "PID" << " │ ";
        std::cout << std::setw(25) << std::left << "NAME" << " │ ";
        std::cout << std::setw(10) << std::right << "RST-SCORE" << " │ ";
        std::cout << std::setw(10) << "ENTROPY" << " │ ";
        std::cout << std::setw(10) << "KLASSE" << " │ ";
        std::cout << std::setw(12) << "MEM (MB)" << " │ ";
        std::cout << std::setw(10) << "STATUS";
        std::cout << color::RESET << "  ║\n";
        std::cout << "╠══════════════════════════════════════════════════════════════════════════════════════════════════════════════════════╣\n";

        auto procs = process_monitor_.get_processes();

        // Sortieren nach RST-Score
        std::vector<std::pair<uint32_t, live_system::ProcessInfo>> sorted;
        for (const auto& [pid, proc] : procs) {
            sorted.push_back({pid, proc});
        }
        std::sort(sorted.begin(), sorted.end(),
                 [](const auto& a, const auto& b) {
                     return a.second.rst_score > b.second.rst_score;
                 });

        int shown = 0;
        for (const auto& [pid, proc] : sorted) {
            if (shown >= 20) break;

            // Farbe basierend auf Klasse
            std::string line_color = color::RESET;
            switch (proc.classification) {
                case live_system::ProcessClass::MALICIOUS: line_color = color::RED; break;
                case live_system::ProcessClass::SUSPICIOUS: line_color = color::YELLOW; break;
                case live_system::ProcessClass::TRUSTED: line_color = color::GREEN; break;
                case live_system::ProcessClass::TRAPPED: line_color = color::MAGENTA; break;
                default: break;
            }

            std::string name = proc.name;
            if (name.length() > 23) name = name.substr(0, 20) + "...";

            std::string status = proc.counter_freq_applied ? "COUNTER" :
                                proc.has_signature_88 ? "88-SIG" : "-";

            std::cout << "║ " << line_color;
            std::cout << std::setw(6) << pid << " │ ";
            std::cout << std::setw(25) << std::left << name << " │ ";
            std::cout << std::setw(10) << std::right << std::fixed << std::setprecision(6) << proc.rst_score << " │ ";
            std::cout << std::setw(10) << std::setprecision(4) << proc.entropy << " │ ";
            std::cout << std::setw(10) << live_system::process_class_name(proc.classification) << " │ ";
            std::cout << std::setw(12) << std::setprecision(1) << (proc.memory_bytes / 1024.0 / 1024.0) << " │ ";
            std::cout << std::setw(10) << status;
            std::cout << color::RESET << "  ║\n";

            shown++;
        }

        // Padding
        for (int i = shown; i < 20; ++i) {
            std::cout << "║" << std::string(116, ' ') << "║\n";
        }

        std::cout << "╠══════════════════════════════════════════════════════════════════════════════════════════════════════════════════════╣\n";
        std::cout << "║ " << color::DIM << "[N] Neutralisieren │ [T] Als Trusted markieren │ [K] Kill Prozess │ [R] Refresh"
                  << color::RESET << std::string(34, ' ') << "║\n";
    }

    // ═══════════════════════════════════════════════════════════════════════
    // FILES VIEW
    // ═══════════════════════════════════════════════════════════════════════

    void render_files() {
        std::cout << "║                                      " << color::BOLD << "DATEI SCANNER" << color::RESET << "                                                            ║\n";
        std::cout << "╠══════════════════════════════════════════════════════════════════════════════════════════════════════════════════════╣\n";

        std::cout << "║ " << color::CYAN << "SCAN-STATUS" << color::RESET << "                                                                                                   ║\n";
        std::cout << "║   Gescannte Dateien: " << std::setw(10) << files_scanned_count_ << "                                                                     ║\n";
        std::cout << "║   Gefundene Threats: " << std::setw(10) << threats_found_count_ << "                                                                     ║\n";
        std::cout << "╠══════════════════════════════════════════════════════════════════════════════════════════════════════════════════════╣\n";

        std::cout << "║ " << color::CYAN << "AKTIONEN" << color::RESET << "                                                                                                      ║\n";
        std::cout << "║   [S] Schnell-Scan (Home/Temp)                                                                                       ║\n";
        std::cout << "║   [F] Voll-Scan (gesamtes System)                                                                                    ║\n";
        std::cout << "║   [D] Deep-Scan (inkl. Boot/Shadow)                                                                                  ║\n";
        std::cout << "║   [P] Pfad scannen...                                                                                                ║\n";
        std::cout << "╠══════════════════════════════════════════════════════════════════════════════════════════════════════════════════════╣\n";

        std::cout << "║ " << color::CYAN << "QUARANTÄNE" << color::RESET << "                                                                                                    ║\n";
        std::cout << "║   Dateien in Quarantäne: " << std::setw(5) << files_quarantined_.load() << "                                                                          ║\n";
        std::cout << "║   [Q] Quarantäne anzeigen │ [R] Wiederherstellen │ [X] Endgültig löschen                                             ║\n";
        std::cout << "╠══════════════════════════════════════════════════════════════════════════════════════════════════════════════════════╣\n";

        // Letzte Threats
        std::cout << "║ " << color::CYAN << "LETZTE FUNDE" << color::RESET << "                                                                                                  ║\n";

        // Hier würden die letzten gefundenen Dateien angezeigt
        std::cout << "║   (Starte einen Scan um Ergebnisse zu sehen)                                                                         ║\n";

        // Padding
        for (int i = 0; i < 8; ++i) {
            std::cout << "║" << std::string(116, ' ') << "║\n";
        }
    }

    // ═══════════════════════════════════════════════════════════════════════
    // NETWORK VIEW
    // ═══════════════════════════════════════════════════════════════════════

    void render_network() {
        std::cout << "║                                     " << color::BOLD << "NETZWERK MONITOR" << color::RESET << "                                                          ║\n";
        std::cout << "╠══════════════════════════════════════════════════════════════════════════════════════════════════════════════════════╣\n";

        std::cout << "║ " << color::CYAN << "VERBINDUNGEN" << color::RESET << "                                                                                                  ║\n";
        std::cout << "╠══════════════════════════════════════════════════════════════════════════════════════════════════════════════════════╣\n";

        // Header
        std::cout << "║ " << color::BOLD;
        std::cout << std::setw(20) << std::left << "LOKAL" << " │ ";
        std::cout << std::setw(25) << "REMOTE" << " │ ";
        std::cout << std::setw(12) << "STATUS" << " │ ";
        std::cout << std::setw(8) << "PID" << " │ ";
        std::cout << std::setw(15) << "PROZESS" << " │ ";
        std::cout << std::setw(10) << "RST";
        std::cout << color::RESET << "  ║\n";
        std::cout << "╠══════════════════════════════════════════════════════════════════════════════════════════════════════════════════════╣\n";

        // Hier würden Netzwerkverbindungen angezeigt
        std::cout << "║   (Netzwerk-Monitor Integration ausstehend)                                                                          ║\n";

        // Padding
        for (int i = 0; i < 15; ++i) {
            std::cout << "║" << std::string(116, ' ') << "║\n";
        }

        std::cout << "╠══════════════════════════════════════════════════════════════════════════════════════════════════════════════════════╣\n";
        std::cout << "║ " << color::DIM << "[B] Verbindung blockieren │ [W] Zur Whitelist │ [I] IP-Info │ [P] Paket-Capture"
                  << color::RESET << std::string(25, ' ') << "║\n";
    }

    // ═══════════════════════════════════════════════════════════════════════
    // ALERTS VIEW
    // ═══════════════════════════════════════════════════════════════════════

    void render_alerts() {
        std::cout << "║                                      " << color::BOLD << "ALERT ZENTRALE" << color::RESET << "                                                            ║\n";
        std::cout << "╠══════════════════════════════════════════════════════════════════════════════════════════════════════════════════════╣\n";

        // Stats
        int critical = 0, warning = 0, info = 0;
        {
            std::lock_guard<std::mutex> lock(mtx_);
            for (const auto& a : alerts_) {
                switch (a.level) {
                    case Alert::CRITICAL: critical++; break;
                    case Alert::WARNING: warning++; break;
                    case Alert::INFO: info++; break;
                }
            }
        }

        std::cout << "║ " << color::RED << "Kritisch: " << critical << color::RESET;
        std::cout << " │ " << color::YELLOW << "Warnung: " << warning << color::RESET;
        std::cout << " │ " << color::BLUE << "Info: " << info << color::RESET;
        std::cout << std::string(70, ' ') << "║\n";
        std::cout << "╠══════════════════════════════════════════════════════════════════════════════════════════════════════════════════════╣\n";

        // Alert Liste
        std::lock_guard<std::mutex> lock(mtx_);
        int shown = 0;
        for (auto it = alerts_.rbegin(); it != alerts_.rend() && shown < 18; ++it, ++shown) {
            render_alert_full(*it);
        }

        if (alerts_.empty()) {
            std::cout << "║ " << color::GREEN << "Keine Alerts vorhanden" << color::RESET
                      << std::string(92, ' ') << "║\n";
        }

        // Padding
        for (int i = shown; i < 18; ++i) {
            std::cout << "║" << std::string(116, ' ') << "║\n";
        }

        std::cout << "╠══════════════════════════════════════════════════════════════════════════════════════════════════════════════════════╣\n";
        std::cout << "║ " << color::DIM << "[A] Alle bestätigen │ [C] Alle löschen │ [E] Exportieren"
                  << color::RESET << std::string(51, ' ') << "║\n";
    }

    // ═══════════════════════════════════════════════════════════════════════
    // RST STATUS VIEW
    // ═══════════════════════════════════════════════════════════════════════

    void render_rst_status() {
        std::cout << "║                                  " << color::BOLD << "RST VERTEIDIGUNGSSYSTEM" << color::RESET << "                                                       ║\n";
        std::cout << "╠═══════════════════════════════════════════════════════════════════════════════════════════════════════════════════════╣\n";

        // Gravitravitation
        std::cout << "║ " << color::MAGENTA << color::BOLD << "GRAVITRAVITATION" << color::RESET << " (Schwarzes Loch - Angreifer-Falle)                                                        ║\n";
        std::cout << "╠═══════════════════════════════════════════════════════════════════════════════════════════════════════════════════════╣\n";
        std::cout << "║   Gefangene Entitäten:  " << std::setw(8) << security_core_.get_trapped_count() << "  (werden als Batterien genutzt)                                       ║\n";
        std::cout << "║   Geerntete Energie:    " << std::setw(15) << std::fixed << std::setprecision(8)
                  << security_core_.get_harvested_energy() << "                                                            ║\n";
        std::cout << "║   Ereignishorizont:     " << std::setw(15) << std::setprecision(12)
                  << security_core_.get_event_horizon() << "                                                            ║\n";

        // Vollenstrahlen
        std::cout << "╠═══════════════════════════════════════════════════════════════════════════════════════════════════════════════════════╣\n";
        std::cout << "║ " << color::YELLOW << color::BOLD << "VOLLENSTRAHLEN" << color::RESET << " (61.440 Sonnen-Strahlen - Transformation zu Licht)                                            ║\n";
        std::cout << "╠═══════════════════════════════════════════════════════════════════════════════════════════════════════════════════════╣\n";
        std::cout << "║   Licht-Energie:        " << std::setw(15) << std::setprecision(8)
                  << security_core_.get_light_energy() << "                                                            ║\n";
        std::cout << "║   Korona-Output:        " << std::setw(15) << security_core_.get_korona_output() << "                                                            ║\n";
        std::cout << "║   Aktive Strahlen:      " << std::setw(5) << security_core_.get_active_beams() << " / 64" << std::string(70, ' ') << "║\n";

        // Defense Engine
        std::cout << "╠═══════════════════════════════════════════════════════════════════════════════════════════════════════════════════════╣\n";
        std::cout << "║ " << color::GREEN << color::BOLD << "DEFENSE ENGINE" << color::RESET << "                                                                                               ║\n";
        std::cout << "╠═══════════════════════════════════════════════════════════════════════════════════════════════════════════════════════╣\n";

        double power = security_core_.get_defense_power();
        std::cout << "║   Defense Power:        " << std::setw(15) << std::setprecision(8) << power;

        // Power Bar
        int bar_len = static_cast<int>(std::min(power * 30.0, 50.0));
        std::cout << "  [" << color::GREEN << std::string(bar_len, '#')
                  << color::DIM << std::string(50 - bar_len, '-') << color::RESET << "]  ║\n";

        std::cout << "║   Labyrinth Pressure:   " << std::setw(15) << security_core_.get_labyrinth_pressure() << "                                                            ║\n";
        std::cout << "║   Supersonic Mode:      " << std::setw(8) << (security_core_.is_supersonic() ? "JA ★" : "NEIN") << "                                                                   ║\n";

        // RST Konstanten
        std::cout << "╠═══════════════════════════════════════════════════════════════════════════════════════════════════════════════════════╣\n";
        std::cout << "║ " << color::CYAN << color::BOLD << "RST KONSTANTEN" << color::RESET << " (17 Dezimalstellen Präzision)                                                                  ║\n";
        std::cout << "╠═══════════════════════════════════════════════════════════════════════════════════════════════════════════════════════╣\n";
        std::cout << std::fixed << std::setprecision(17);
        std::cout << "║   G0 (8/9) WAHRHEIT:    " << rst::G0 << "                                            ║\n";
        std::cout << "║   G1 (5/9):             " << rst::G1 << "                                            ║\n";
        std::cout << "║   G3 (3/9):             " << rst::G3 << "                                            ║\n";
        std::cout << "║   G5 (1/9):             " << rst::G5 << "                                            ║\n";
        std::cout << "║   " << color::BOLD << "G5+G3+G1 = 9/9 = 1:   " << color::RESET
                  << (rst::G5 + rst::G3 + rst::G1) << "                                            ║\n";
    }

    // ═══════════════════════════════════════════════════════════════════════
    // SETTINGS VIEW
    // ═══════════════════════════════════════════════════════════════════════

    void render_settings() {
        std::cout << "║                                      " << color::BOLD << "EINSTELLUNGEN" << color::RESET << "                                                             ║\n";
        std::cout << "╠══════════════════════════════════════════════════════════════════════════════════════════════════════════════════════╣\n";

        std::cout << "║ " << color::CYAN << "AUTO-VERTEIDIGUNG" << color::RESET << "                                                                                              ║\n";
        std::cout << "║   [1] Auto-Neutralize:      " << (auto_neutralize_ ? color::GREEN + "AN" : color::RED + "AUS") << color::RESET << "                                                                               ║\n";
        std::cout << "║   [2] Threat Threshold:     " << std::fixed << std::setprecision(6) << threat_threshold_ << " (G1 = 0.555...)                                               ║\n";
        std::cout << "╠══════════════════════════════════════════════════════════════════════════════════════════════════════════════════════╣\n";

        std::cout << "║ " << color::CYAN << "SCANNING" << color::RESET << "                                                                                                       ║\n";
        std::cout << "║   [3] Scan beim Start:      " << (scan_on_start_ ? color::GREEN + "AN" : color::RED + "AUS") << color::RESET << "                                                                               ║\n";
        std::cout << "║   [4] Refresh Rate:         " << refresh_rate_ms_ << " ms                                                                         ║\n";
        std::cout << "╠══════════════════════════════════════════════════════════════════════════════════════════════════════════════════════╣\n";

        std::cout << "║ " << color::CYAN << "GEGENFREQUENZ" << color::RESET << "                                                                                                  ║\n";
        std::cout << "║   Prinzip: Negative Phase → Positive Gegenschwingung                                                                 ║\n";
        std::cout << "║            Hohe Entropie → Ordnung einführen (Transformation)                                                        ║\n";
        std::cout << "║            Fehlende 88-Signatur → VOLLENSTRAHLEN → LICHT                                                             ║\n";
        std::cout << "╠══════════════════════════════════════════════════════════════════════════════════════════════════════════════════════╣\n";

        std::cout << "║ " << color::CYAN << "ÜBER" << color::RESET << "                                                                                                           ║\n";
        std::cout << "║   RAEL Security Dashboard v1.0                                                                                       ║\n";
        std::cout << "║   RST Konstanten: G5 + G3 + G1 = 1/9 + 3/9 + 5/9 = 9/9 = 1                                                           ║\n";
        std::cout << "║   WICHTIG: Alles wird IMMER geprüft - keine Whitelist!                                                               ║\n";

        // Padding
        for (int i = 0; i < 8; ++i) {
            std::cout << "║" << std::string(116, ' ') << "║\n";
        }
    }

    // ═══════════════════════════════════════════════════════════════════════
    // SI MODULES VIEW (Semantische Intelligenz / Programmierbarkeit)
    // ═══════════════════════════════════════════════════════════════════════

    void render_si_modules() {
        std::cout << "║                           " << color::BOLD << "SI MODULE - SEMANTISCHE INTELLIGENZ" << color::RESET << "                                               ║\n";
        std::cout << "╠══════════════════════════════════════════════════════════════════════════════════════════════════════════════════════╣\n";

        // Aktives semantisches Modul
        std::string active_semantic = module_manager_.active_semantic_name();
        std::cout << "║ " << color::CYAN << "AKTIVES SEMANTIK-MODUL" << color::RESET << ": ";
        if (active_semantic.empty()) {
            std::cout << color::DIM << "(keines)" << color::RESET << std::string(77, ' ') << "║\n";
        } else {
            std::cout << color::GREEN << active_semantic << color::RESET << std::string(85 - active_semantic.length(), ' ') << "║\n";
        }
        std::cout << "╠══════════════════════════════════════════════════════════════════════════════════════════════════════════════════════╣\n";

        // Module Liste Header
        std::cout << "║ " << color::CYAN << "GELADENE MODULE" << color::RESET << "                                                                                                  ║\n";
        std::cout << "╠══════════════════════════════════════════════════════════════════════════════════════════════════════════════════════╣\n";

        // Header
        std::cout << "║ " << color::BOLD;
        std::cout << std::setw(20) << std::left << "NAME" << " │ ";
        std::cout << std::setw(10) << "VERSION" << " │ ";
        std::cout << std::setw(12) << "TYP" << " │ ";
        std::cout << std::setw(10) << "STATUS" << " │ ";
        std::cout << std::setw(40) << "PFAD";
        std::cout << color::RESET << "   ║\n";
        std::cout << "╠══════════════════════════════════════════════════════════════════════════════════════════════════════════════════════╣\n";

        // Module auflisten
        auto module_names = module_manager_.list_names();
        int shown = 0;

        for (const auto& name : module_names) {
            if (shown >= 12) break;

            const LoadedModule* mod = module_manager_.get(name);
            if (!mod || !mod->api) continue;

            const RaelModuleInfo& info = mod->api->info;

            // Typ als String
            std::string type_str;
            switch (info.kind) {
                case RAEL_MOD_SEMANTIC: type_str = "SEMANTIC"; break;
                case RAEL_MOD_MATH:     type_str = "MATH"; break;
                case RAEL_MOD_POLICY:   type_str = "POLICY"; break;
                default:                type_str = "UNKNOWN"; break;
            }

            // Status Farbe
            std::string status_color = mod->active ? color::GREEN : color::DIM;
            std::string status_str = mod->active ? "AKTIV" : "INAKTIV";

            // Pfad kürzen
            std::string path = mod->path;
            if (path.length() > 38) {
                path = "..." + path.substr(path.length() - 35);
            }

            std::cout << "║ ";
            std::cout << std::setw(20) << std::left << name.substr(0, 20) << " │ ";
            std::cout << std::setw(10) << (info.version ? info.version : "-") << " │ ";
            std::cout << std::setw(12) << type_str << " │ ";
            std::cout << status_color << std::setw(10) << status_str << color::RESET << " │ ";
            std::cout << std::setw(40) << path;
            std::cout << "   ║\n";

            shown++;
        }

        if (module_names.empty()) {
            std::cout << "║ " << color::DIM << "Keine Module geladen. Lade Module mit [L]oad."
                      << color::RESET << std::string(66, ' ') << "║\n";
        }

        // Padding
        for (int i = shown; i < 12; ++i) {
            std::cout << "║" << std::string(116, ' ') << "║\n";
        }

        // Funktionen
        std::cout << "╠══════════════════════════════════════════════════════════════════════════════════════════════════════════════════════╣\n";
        std::cout << "║ " << color::CYAN << "SI FUNKTIONEN" << color::RESET << "                                                                                                     ║\n";
        std::cout << "╠══════════════════════════════════════════════════════════════════════════════════════════════════════════════════════╣\n";
        std::cout << "║   • Semantische Text-Transformation (INTENTION{...})                                                                 ║\n";
        std::cout << "║   • Mathematische Formeln (RST-Konstanten, Quint-Werte)                                                              ║\n";
        std::cout << "║   • Hot-Swap für Live-Modul-Austausch                                                                                ║\n";
        std::cout << "║   • Ethics-Layer für sichere Ausführung                                                                              ║\n";
        std::cout << "╠══════════════════════════════════════════════════════════════════════════════════════════════════════════════════════╣\n";

        // Extra Formeln
        auto formulas = module_manager_.get_extra_formulas();
        std::cout << "║ " << color::CYAN << "VERFÜGBARE FORMELN" << color::RESET << " (aus geladenen Modulen): " << formulas.size() << std::string(53, ' ') << "║\n";

        // Aktionen
        std::cout << "╠══════════════════════════════════════════════════════════════════════════════════════════════════════════════════════╣\n";
        std::cout << "║ " << color::DIM << "[L] Modul laden │ [A] Aktivieren │ [D] Deaktivieren │ [U] Entladen │ [H] Hot-Swap │ [T] Text transformieren"
                  << color::RESET << " ║\n";
    }

    // ═══════════════════════════════════════════════════════════════════════
    // IMPROVE VIEW (Reflektion & Verbesserungsvorschläge)
    // ═══════════════════════════════════════════════════════════════════════

    void render_improve() {
        std::cout << "║                        " << color::BOLD << "REFLEKTION & VERBESSERUNGSVORSCHLÄGE" << color::RESET << "                                              ║\n";
        std::cout << "╠══════════════════════════════════════════════════════════════════════════════════════════════════════════════════════╣\n";

        // Statistik Header
        auto pending = ImprovementBus::by_status(ImprovementStatus::PENDING);
        auto approved = ImprovementBus::by_status(ImprovementStatus::APPROVED);
        auto applied = ImprovementBus::by_status(ImprovementStatus::APPLIED);

        std::cout << "║ " << color::YELLOW << "PENDING: " << pending.size() << color::RESET;
        std::cout << " │ " << color::GREEN << "APPROVED: " << approved.size() << color::RESET;
        std::cout << " │ " << color::CYAN << "APPLIED: " << applied.size() << color::RESET;
        std::cout << std::string(65, ' ') << "║\n";
        std::cout << "╠══════════════════════════════════════════════════════════════════════════════════════════════════════════════════════╣\n";

        // Quellen-Übersicht
        std::cout << "║ " << color::CYAN << "QUELLEN" << color::RESET << "                                                                                                        ║\n";

        auto reflect = ImprovementBus::by_source(ImprovementSource::REFLECT, 10);
        auto defense = ImprovementBus::by_source(ImprovementSource::LIVE_DEFENSE, 10);
        auto self_opt = ImprovementBus::by_source(ImprovementSource::SELF_OPT, 10);
        auto aar = ImprovementBus::by_source(ImprovementSource::AAR, 10);

        std::cout << "║   REFLECT (Selbstreflexion):     " << std::setw(5) << reflect.size();
        std::cout << "  │  LIVE_DEFENSE (Verteidigung): " << std::setw(5) << defense.size() << std::string(29, ' ') << "║\n";
        std::cout << "║   SELF_OPT (Selbstoptimierung):  " << std::setw(5) << self_opt.size();
        std::cout << "  │  AAR (After-Action-Review):   " << std::setw(5) << aar.size() << std::string(29, ' ') << "║\n";

        std::cout << "╠══════════════════════════════════════════════════════════════════════════════════════════════════════════════════════╣\n";

        // Vorschläge Liste Header
        std::cout << "║ " << color::CYAN << "AKTUELLE VORSCHLÄGE" << color::RESET << " (neueste zuerst)                                                                          ║\n";
        std::cout << "╠══════════════════════════════════════════════════════════════════════════════════════════════════════════════════════╣\n";

        // Header
        std::cout << "║ " << color::BOLD;
        std::cout << std::setw(4) << "ID" << " │ ";
        std::cout << std::setw(12) << std::left << "QUELLE" << " │ ";
        std::cout << std::setw(3) << "IMP" << " │ ";
        std::cout << std::setw(3) << "RSK" << " │ ";
        std::cout << std::setw(10) << "STATUS" << " │ ";
        std::cout << std::setw(55) << "TITEL";
        std::cout << color::RESET << std::right << "  ║\n";
        std::cout << "╠══════════════════════════════════════════════════════════════════════════════════════════════════════════════════════╣\n";

        // Letzte Verbesserungen anzeigen
        auto improvements = ImprovementBus::last(10);
        int shown = 0;

        for (const auto& imp : improvements) {
            if (shown >= 10) break;

            // Farbe basierend auf Status
            std::string status_color = color::RESET;
            switch (imp.typed_status) {
                case ImprovementStatus::PENDING:  status_color = color::YELLOW; break;
                case ImprovementStatus::APPROVED: status_color = color::GREEN; break;
                case ImprovementStatus::REJECTED: status_color = color::RED; break;
                case ImprovementStatus::DEFERRED: status_color = color::DIM; break;
                case ImprovementStatus::APPLIED:  status_color = color::CYAN; break;
                default: break;
            }

            // Importance Farbe
            std::string imp_color = imp.importance >= 8 ? color::RED :
                                   imp.importance >= 5 ? color::YELLOW : color::GREEN;

            std::string title = imp.title;
            if (title.length() > 53) title = title.substr(0, 50) + "...";

            std::cout << "║ ";
            std::cout << std::setw(4) << imp.id << " │ ";
            std::cout << std::setw(12) << std::left << source_to_string(imp.source) << " │ ";
            std::cout << imp_color << std::setw(3) << imp.importance << color::RESET << " │ ";
            std::cout << std::setw(3) << imp.risk << " │ ";
            std::cout << status_color << std::setw(10) << status_to_string(imp.typed_status) << color::RESET << " │ ";
            std::cout << std::setw(55) << std::left << title << std::right;
            std::cout << "  ║\n";

            shown++;
        }

        if (improvements.empty()) {
            std::cout << "║ " << color::DIM << "Keine Verbesserungsvorschläge vorhanden. RAEL reflektiert automatisch."
                      << color::RESET << std::string(36, ' ') << "║\n";
        }

        // Padding
        for (int i = shown; i < 10; ++i) {
            std::cout << "║" << std::string(116, ' ') << "║\n";
        }

        // Shadow Simulation & Rollback Status
        std::cout << "╠═══════════════════════════════════════════════════════════════╦══════════════════════════════════════════════════════╣\n";
        std::cout << "║ " << color::MAGENTA << "SHADOW SIMULATOR" << color::RESET << "                                             ";
        std::cout << "║ " << color::CYAN << "ROLLBACK MANAGER" << color::RESET << "                                   ║\n";
        std::cout << "╠═══════════════════════════════════════════════════════════════╬══════════════════════════════════════════════════════╣\n";

        auto shadow_history = gShadowSim.get_history();
        auto rollback_points = gRollback.list_points();

        std::cout << "║ Simulationen:     " << std::setw(5) << shadow_history.size() << "                                    ";
        std::cout << "║ Rollback-Punkte:  " << std::setw(5) << rollback_points.size() << "                         ║\n";

        // Letzte Shadow-Simulation
        if (!shadow_history.empty()) {
            const auto& last_sim = shadow_history.back();
            std::cout << "║ Letzte: " << color::DIM << std::setw(40) << last_sim.description.substr(0, 40) << color::RESET << "      ";
        } else {
            std::cout << "║ Letzte: " << color::DIM << "(keine)" << color::RESET << std::string(40, ' ') << "      ";
        }

        // Letzter Rollback-Punkt
        if (!rollback_points.empty()) {
            const auto& last_rb = rollback_points.back();
            std::cout << "║ Letzte: " << color::DIM << std::setw(30) << last_rb.description.substr(0, 30) << color::RESET << "         ║\n";
        } else {
            std::cout << "║ Letzte: " << color::DIM << "(keine)" << color::RESET << std::string(30, ' ') << "         ║\n";
        }

        std::cout << "╠═══════════════════════════════════════════════════════════════╩══════════════════════════════════════════════════════╣\n";

        // Reflektion Prinzip
        std::cout << "║ " << color::CYAN << "REFLEKTION" << color::RESET << ": Selbstanalyse → Vorschlag → " << color::GREEN << "MENSCH entscheidet" << color::RESET << " → Shadow-Test → Apply/Rollback       ║\n";

        // Aktionen
        std::cout << "╠══════════════════════════════════════════════════════════════════════════════════════════════════════════════════════╣\n";
        std::cout << "║ " << color::DIM << "[A] Approve │ [X] Reject │ [D] Defer │ [P] Apply │ [B] Rollback │ [S] Shadow-Sim │ [R] Run AAR"
                  << color::RESET << "    ║\n";
    }

    // ═══════════════════════════════════════════════════════════════════════
    // FOOTER
    // ═══════════════════════════════════════════════════════════════════════

    void render_footer() {
        std::cout << "╠══════════════════════════════════════════════════════════════════════════════════════════════════════════════════════╣\n";
        std::cout << "║ " << color::DIM << "[0-9] Navigation │ [Q] Beenden │ [R] Refresh │ [H] Hilfe"
                  << color::RESET << std::string(51, ' ') << "║\n";
        std::cout << "╚══════════════════════════════════════════════════════════════════════════════════════════════════════════════════════╝\n";
    }

    // ═══════════════════════════════════════════════════════════════════════
    // INPUT HANDLING
    // ═══════════════════════════════════════════════════════════════════════

    void input_loop() {
        while (running_) {
            char c = get_char();

            switch (c) {
                // Navigation
                case '0': current_view_ = View::KI_SYSTEM; break;
                case '1': current_view_ = View::DASHBOARD; break;
                case '2': current_view_ = View::PROCESSES; break;
                case '3': current_view_ = View::FILES; break;
                case '4': current_view_ = View::NETWORK; break;
                case '5': current_view_ = View::ALERTS; break;
                case '6': current_view_ = View::RST_STATUS; break;
                case '7': current_view_ = View::SETTINGS; break;
                case '8': current_view_ = View::SI_MODULES; break;
                case '9': current_view_ = View::IMPROVE; break;

                // Quit
                case 'q':
                case 'Q':
                    running_ = false;
                    break;

                // Refresh
                case 'r':
                case 'R':
                    // Force refresh
                    break;

                // View-spezifische Aktionen
                case 's':
                case 'S':
                    if (current_view_ == View::FILES) {
                        // Quick Scan
                        start_quick_scan();
                    }
                    break;

                case 'f':
                case 'F':
                    if (current_view_ == View::FILES) {
                        // Full Scan
                        start_full_scan();
                    }
                    break;

                // SI Module Aktionen
                case 'l':
                case 'L':
                    if (current_view_ == View::SI_MODULES) {
                        load_si_module_interactive();
                    }
                    break;

                case 'a':
                case 'A':
                    if (current_view_ == View::SI_MODULES) {
                        activate_si_module_interactive();
                    } else if (current_view_ == View::IMPROVE) {
                        approve_improvement_interactive();
                    }
                    break;

                case 'd':
                case 'D':
                    if (current_view_ == View::SI_MODULES) {
                        deactivate_si_module_interactive();
                    } else if (current_view_ == View::IMPROVE) {
                        defer_improvement_interactive();
                    }
                    break;

                case 'x':
                case 'X':
                    if (current_view_ == View::IMPROVE) {
                        reject_improvement_interactive();
                    }
                    break;

                case 'p':
                case 'P':
                    if (current_view_ == View::IMPROVE) {
                        apply_improvement_interactive();
                    }
                    break;

                case 'b':
                case 'B':
                    if (current_view_ == View::IMPROVE) {
                        rollback_interactive();
                    }
                    break;

                case 'u':
                case 'U':
                    if (current_view_ == View::SI_MODULES) {
                        unload_si_module_interactive();
                    }
                    break;

                case 'h':
                case 'H':
                    if (current_view_ == View::SI_MODULES) {
                        hotswap_si_module_interactive();
                    }
                    break;

                case 't':
                case 'T':
                    if (current_view_ == View::SI_MODULES) {
                        transform_text_interactive();
                    }
                    break;
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
    }

    // ═══════════════════════════════════════════════════════════════════════
    // HELPER FUNCTIONS
    // ═══════════════════════════════════════════════════════════════════════

private:
    void initialize_callbacks() {
        // Process Monitor Callbacks
        process_monitor_.set_threat_callback([this](const live_system::ProcessInfo& proc) {
            add_alert(
                proc.classification == live_system::ProcessClass::MALICIOUS ? Alert::CRITICAL : Alert::WARNING,
                "PROCESS",
                proc.name + " (" + std::to_string(proc.pid) + ")",
                "RST-Score: " + std::to_string(proc.rst_score)
            );
        });

        process_monitor_.set_neutralize_callback(
            [this](const live_system::ProcessInfo& proc,
                   const live_system::CounterFrequencyGenerator::CounterResult& result) {
                threats_blocked_++;
                add_alert(
                    Alert::INFO,
                    "COUNTER",
                    result.action + ": " + proc.name,
                    result.details
                );
            }
        );

        // Security Core Callbacks
        security_core_.set_threat_callback([this](const Threat& t) {
            total_scans_++;
        });

        security_core_.set_trap_callback([this](uint32_t entity_id, double harvested) {
            add_alert(
                Alert::INFO,
                "GRAV",
                "Entity " + std::to_string(entity_id) + " gefangen",
                "Harvested: " + std::to_string(harvested)
            );
        });

        security_core_.set_eruption_callback([this](double energy) {
            add_alert(
                Alert::INFO,
                "SONNE",
                "Sonnen-Eruption!",
                "Energie: " + std::to_string(energy)
            );
        });
    }

    void add_alert(Alert::Level level, const std::string& source,
                   const std::string& message, const std::string& details) {
        std::lock_guard<std::mutex> lock(mtx_);

        Alert a;
        a.level = level;
        a.source = source;
        a.message = message;
        a.details = details;

        alerts_.push_back(a);

        if (alerts_.size() > MAX_ALERTS) {
            alerts_.pop_front();
        }
    }

    int count_critical_alerts() {
        std::lock_guard<std::mutex> lock(mtx_);
        return std::count_if(alerts_.begin(), alerts_.end(),
                            [](const Alert& a) { return a.level == Alert::CRITICAL && !a.acknowledged; });
    }

    void render_alert_line(const Alert& a) {
        auto time = std::chrono::system_clock::to_time_t(a.timestamp);

        std::string level_color;
        std::string level_str;
        switch (a.level) {
            case Alert::CRITICAL: level_color = color::RED; level_str = "KRITISCH"; break;
            case Alert::WARNING: level_color = color::YELLOW; level_str = "WARNUNG"; break;
            case Alert::INFO: level_color = color::BLUE; level_str = "INFO"; break;
        }

        std::cout << "║ " << level_color << std::setw(8) << level_str << color::RESET;
        std::cout << " │ " << std::put_time(std::localtime(&time), "%H:%M:%S");
        std::cout << " │ " << std::setw(8) << a.source;
        std::cout << " │ " << std::setw(60) << std::left << a.message.substr(0, 60) << std::right;
        std::cout << "  ║\n";
    }

    void render_alert_full(const Alert& a) {
        render_alert_line(a);
        if (!a.details.empty()) {
            std::cout << "║          │          │          │   " << color::DIM
                      << std::setw(60) << std::left << a.details.substr(0, 60) << std::right
                      << color::RESET << "  ║\n";
        }
    }

    void start_quick_scan() {
        add_alert(Alert::INFO, "SCAN", "Schnell-Scan gestartet...", "");
        std::thread([this]() {
            std::vector<std::string> paths = {"/home", "/tmp"};
#ifdef _WIN32
            paths = {"C:\\Users", "C:\\Windows\\Temp"};
#endif
            deep_scanner_.full_system_scan(paths, false, false, 7.0);
        }).detach();
    }

    void start_full_scan() {
        add_alert(Alert::INFO, "SCAN", "Voll-Scan gestartet...", "Dies kann einige Zeit dauern");
        std::thread([this]() {
            deep_scanner_.full_system_scan({}, true, true, 7.0);
        }).detach();
    }

    // ═══════════════════════════════════════════════════════════════════════
    // SI MODULE FUNKTIONEN
    // ═══════════════════════════════════════════════════════════════════════

    void load_si_module_interactive() {
        // Standard-Pfade für Module
        std::vector<std::string> module_paths = {
            "bin/modules/libsem_quint.so",
            "bin/modules/libmath_pack.so"
        };
#ifdef _WIN32
        module_paths = {
            "bin\\modules\\sem_quint.dll",
            "bin\\modules\\math_pack.dll"
        };
#endif
        // Versuche alle Standard-Module zu laden
        for (const auto& path : module_paths) {
            std::string err;
            if (module_manager_.load(path, err)) {
                add_alert(Alert::INFO, "SI", "Modul geladen: " + path, "");
            }
        }
    }

    void activate_si_module_interactive() {
        auto names = module_manager_.list_names();
        for (const auto& name : names) {
            const LoadedModule* mod = module_manager_.get(name);
            if (mod && !mod->active) {
                std::string err;
                if (module_manager_.activate(name, err)) {
                    add_alert(Alert::INFO, "SI", "Modul aktiviert: " + name, "");
                } else {
                    add_alert(Alert::WARNING, "SI", "Aktivierung fehlgeschlagen: " + name, err);
                }
            }
        }
    }

    void deactivate_si_module_interactive() {
        auto names = module_manager_.list_names();
        for (const auto& name : names) {
            const LoadedModule* mod = module_manager_.get(name);
            if (mod && mod->active) {
                std::string err;
                if (module_manager_.deactivate(name, err)) {
                    add_alert(Alert::INFO, "SI", "Modul deaktiviert: " + name, "");
                }
            }
        }
    }

    void unload_si_module_interactive() {
        auto names = module_manager_.list_names();
        for (const auto& name : names) {
            std::string err;
            if (module_manager_.unload(name, err)) {
                add_alert(Alert::INFO, "SI", "Modul entladen: " + name, "");
            }
        }
    }

    void hotswap_si_module_interactive() {
        auto names = module_manager_.list_names();
        for (const auto& name : names) {
            const LoadedModule* mod = module_manager_.get(name);
            if (mod && mod->api && mod->api->info.kind == RAEL_MOD_SEMANTIC) {
                std::string err;
                if (module_manager_.hotswap_semantic(name, err)) {
                    add_alert(Alert::INFO, "SI", "Hot-Swap zu: " + name, "Semantisches Modul gewechselt");
                } else {
                    add_alert(Alert::WARNING, "SI", "Hot-Swap fehlgeschlagen", err);
                }
                break;  // Nur eines swappen
            }
        }
    }

    void transform_text_interactive() {
        // Demo-Transformation
        std::string test_input = "RAEL Security System aktiviert";
        std::string result = module_manager_.process_text_chain(test_input);

        add_alert(Alert::INFO, "SI", "Text-Transformation", "Input: " + test_input);
        add_alert(Alert::INFO, "SI", "Ergebnis", result);
    }

    // ═══════════════════════════════════════════════════════════════════════
    // IMPROVE VIEW FUNKTIONEN (Reflektion & Verbesserungen)
    // ═══════════════════════════════════════════════════════════════════════

    void approve_improvement_interactive() {
        auto pending = ImprovementBus::by_status(ImprovementStatus::PENDING);
        if (!pending.empty()) {
            auto& imp = pending.front();
            if (ImprovementBus::update_status(imp.id, ImprovementStatus::APPROVED)) {
                add_alert(Alert::INFO, "IMPROVE", "Genehmigt: " + imp.title, "ID: " + std::to_string(imp.id));
            }
        } else {
            add_alert(Alert::INFO, "IMPROVE", "Keine PENDING Vorschläge", "");
        }
    }

    void reject_improvement_interactive() {
        auto pending = ImprovementBus::by_status(ImprovementStatus::PENDING);
        if (!pending.empty()) {
            auto& imp = pending.front();
            if (ImprovementBus::update_status(imp.id, ImprovementStatus::REJECTED)) {
                add_alert(Alert::INFO, "IMPROVE", "Abgelehnt: " + imp.title, "ID: " + std::to_string(imp.id));
            }
        }
    }

    void defer_improvement_interactive() {
        auto pending = ImprovementBus::by_status(ImprovementStatus::PENDING);
        if (!pending.empty()) {
            auto& imp = pending.front();
            if (ImprovementBus::update_status(imp.id, ImprovementStatus::DEFERRED)) {
                add_alert(Alert::INFO, "IMPROVE", "Zurückgestellt: " + imp.title, "ID: " + std::to_string(imp.id));
            }
        }
    }

    void apply_improvement_interactive() {
        auto approved = ImprovementBus::by_status(ImprovementStatus::APPROVED);
        if (!approved.empty()) {
            auto& imp = approved.front();

            // Shadow-Simulation vor Anwendung
            if (imp.shadow_tested) {
                // Erstelle Rollback-Punkt
                uint64_t rb_id = gRollback.create_point("Vor Anwendung: " + imp.title);

                if (ImprovementBus::update_status(imp.id, ImprovementStatus::APPLIED)) {
                    add_alert(Alert::INFO, "IMPROVE", "Angewendet: " + imp.title,
                             "Rollback-Punkt: " + std::to_string(rb_id));
                }
            } else {
                add_alert(Alert::WARNING, "IMPROVE", "Shadow-Test erforderlich!",
                         "Nutze [S] für Shadow-Simulation");
            }
        } else {
            add_alert(Alert::INFO, "IMPROVE", "Keine APPROVED Vorschläge zum Anwenden", "");
        }
    }

    void rollback_interactive() {
        std::string err;
        if (gRollback.rollback_last(err)) {
            add_alert(Alert::INFO, "ROLLBACK", "Erfolgreich zurückgerollt", "");
        } else {
            add_alert(Alert::WARNING, "ROLLBACK", "Rollback fehlgeschlagen", err);
        }
    }

    void run_aar_interactive() {
        add_alert(Alert::INFO, "AAR", "After-Action-Review gestartet...", "");

        // AAR Engine analysiert Metriken
        gAAR.analyze();

        add_alert(Alert::INFO, "AAR", "Analyse abgeschlossen", "Neue Vorschläge generiert");
    }

    // ═══════════════════════════════════════════════════════════════════════
    // ÖFFENTLICHE API
    // ═══════════════════════════════════════════════════════════════════════

    // Öffentliche API für SI-Programmierung
public:
    // Text durch alle aktiven semantischen Module verarbeiten
    std::string si_process_text(const std::string& input) {
        return module_manager_.process_text_chain(input);
    }

    // Modul laden (returns success)
    bool si_load_module(const std::string& path) {
        std::string err;
        bool ok = module_manager_.load(path, err);
        if (!ok) {
            add_alert(Alert::WARNING, "SI", "Laden fehlgeschlagen: " + path, err);
        }
        return ok;
    }

    // Modul aktivieren
    bool si_activate_module(const std::string& name) {
        std::string err;
        return module_manager_.activate(name, err);
    }

    // Alle mathematischen Formeln aus Modulen holen
    std::vector<std::string> si_get_formulas() {
        return module_manager_.get_extra_formulas();
    }

    // Aktives semantisches Modul
    std::string si_get_active_semantic() {
        return module_manager_.active_semantic_name();
    }

    // ModuleManager direkt zugänglich für erweiterte Programmierung
    ModuleManager& si_module_manager() {
        return module_manager_;
    }

    // ═══════════════════════════════════════════════════════════════════════
    // REFLEKTION & IMPROVE API
    // ═══════════════════════════════════════════════════════════════════════

    // Verbesserungsvorschlag erstellen
    uint64_t improve_emit(ImprovementSource source, const std::string& title,
                         const std::string& problem, int importance = 5, int risk = 3) {
        return ImprovementBus::emit(source, title, problem, importance, risk);
    }

    // Letzte Verbesserungen holen
    std::vector<Improvement> improve_last(size_t n = 10) {
        return ImprovementBus::last(n);
    }

    // Status eines Vorschlags ändern
    bool improve_update_status(uint64_t id, ImprovementStatus status) {
        return ImprovementBus::update_status(id, status);
    }

    // Shadow-Simulation für einen Vorschlag
    ShadowResult improve_simulate(const std::string& description, ShadowAction action) {
        ShadowState shadow = gShadowSim.create_shadow(description);
        return gShadowSim.simulate(shadow, action);
    }

    // Rollback-Punkt erstellen
    uint64_t improve_create_rollback(const std::string& description) {
        return gRollback.create_point(description);
    }

    // Zum letzten Rollback-Punkt zurückkehren
    bool improve_rollback_last() {
        std::string err;
        return gRollback.rollback_last(err);
    }

    // After-Action-Review ausführen
    void improve_run_aar() {
        gAAR.analyze();
    }

    // Ethics-Check für eine Intention
    bool ethics_allows(const std::string& intention) {
        std::string reason;
        return EthicsCore::allows(intention, reason);
    }

    // Resonanz evaluieren
    ResonanceResult evaluate_resonance(const SemanticResult& s) {
        ResonanceEngine engine;
        return engine.evaluate(s);
    }

    // ═══════════════════════════════════════════════════════════════════════
    // METRIKEN API
    // ═══════════════════════════════════════════════════════════════════════

    // Aktuelle Metriken
    Metrics& get_metrics() { return gMetrics; }

    // Telemetrie
    Telemetry& get_telemetry() { return gTelemetry; }

private:
    void clear_screen() {
#ifdef _WIN32
        system("cls");
#else
        std::cout << "\033[2J\033[H";
#endif
    }

    void setup_terminal() {
#ifndef _WIN32
        // Raw mode für Eingabe
        struct termios t;
        tcgetattr(STDIN_FILENO, &t);
        t.c_lflag &= ~(ICANON | ECHO);
        tcsetattr(STDIN_FILENO, TCSANOW, &t);
#endif
    }

    void restore_terminal() {
#ifndef _WIN32
        struct termios t;
        tcgetattr(STDIN_FILENO, &t);
        t.c_lflag |= (ICANON | ECHO);
        tcsetattr(STDIN_FILENO, TCSANOW, &t);
#endif
    }

    char get_char() {
#ifdef _WIN32
        if (_kbhit()) {
            return _getch();
        }
        return 0;
#else
        fd_set fds;
        FD_ZERO(&fds);
        FD_SET(STDIN_FILENO, &fds);

        struct timeval tv;
        tv.tv_sec = 0;
        tv.tv_usec = 100000;  // 100ms timeout

        if (select(STDIN_FILENO + 1, &fds, NULL, NULL, &tv) > 0) {
            char c;
            read(STDIN_FILENO, &c, 1);
            return c;
        }
        return 0;
#endif
    }

    void get_terminal_size() {
#ifdef _WIN32
        CONSOLE_SCREEN_BUFFER_INFO csbi;
        GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi);
        term_width_ = csbi.srWindow.Right - csbi.srWindow.Left + 1;
        term_height_ = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;
#else
        struct winsize w;
        ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
        term_width_ = w.ws_col;
        term_height_ = w.ws_row;
#endif
    }
};

} // namespace dashboard
} // namespace security
} // namespace rael
