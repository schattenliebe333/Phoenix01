// ═══════════════════════════════════════════════════════════════════════════════
// RAEL LIVE MONITOR - Real-Time Security Monitoring Daemon
// ═══════════════════════════════════════════════════════════════════════════════
//
// Echtzeit-Überwachung mit RST-Analyse
//
// Nutzung:
//   rael_monitor --start     # Live-Monitoring starten
//   rael_monitor --scan      # Einmaliger Scan mit RST-Analyse
//   rael_monitor --analyze FILE  # Einzelne Datei analysieren
//   rael_monitor --status    # Status anzeigen
//
// Features:
//   - Shannon Entropy Analyse
//   - RST Coherence (88-Signatur)
//   - Phase Deviation Detection
//   - Malice Index Berechnung
//   - Signatur-Datenbank Matching
//   - Gravitravitation (Angreifer-Falle)
//   - Vollenstrahlen (Sonnen-Transformation)
//
// WICHTIG: Alles wird IMMER geprüft! Keine Whitelist!
//
// ═══════════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <string>
#include <thread>
#include <chrono>
#include <csignal>
#include <atomic>
#include <cstring>
#include <fstream>
#include <vector>
#include <iomanip>

#include "rael/security_core.hpp"
#include "rael/rst_live_monitor.hpp"

using namespace rael::security_core;
using namespace rael::security::live;

// ANSI Colors
#ifdef _WIN32
#define COLOR_RED     ""
#define COLOR_GREEN   ""
#define COLOR_YELLOW  ""
#define COLOR_BLUE    ""
#define COLOR_MAGENTA ""
#define COLOR_CYAN    ""
#define COLOR_WHITE   ""
#define COLOR_RESET   ""
#define COLOR_BOLD    ""
#define COLOR_DIM     ""
#else
#define COLOR_RED     "\033[91m"
#define COLOR_GREEN   "\033[92m"
#define COLOR_YELLOW  "\033[93m"
#define COLOR_BLUE    "\033[94m"
#define COLOR_MAGENTA "\033[95m"
#define COLOR_CYAN    "\033[96m"
#define COLOR_WHITE   "\033[97m"
#define COLOR_RESET   "\033[0m"
#define COLOR_BOLD    "\033[1m"
#define COLOR_DIM     "\033[2m"
#endif

// Globale Variable für Signal-Handler
std::atomic<bool> g_running(true);

void signal_handler(int sig) {
    (void)sig;
    g_running = false;
}

void print_banner() {
    std::cout << COLOR_CYAN << COLOR_BOLD;
    std::cout << R"(
 ██████╗  ███████╗████████╗    ██╗     ██╗██╗   ██╗███████╗    ███╗   ███╗ ██████╗ ███╗   ██╗██╗████████╗ ██████╗ ██████╗
 ██╔══██╗ ██╔════╝╚══██╔══╝    ██║     ██║██║   ██║██╔════╝    ████╗ ████║██╔═══██╗████╗  ██║██║╚══██╔══╝██╔═══██╗██╔══██╗
 ██████╔╝ ███████╗   ██║       ██║     ██║██║   ██║█████╗      ██╔████╔██║██║   ██║██╔██╗ ██║██║   ██║   ██║   ██║██████╔╝
 ██╔══██╗ ╚════██║   ██║       ██║     ██║╚██╗ ██╔╝██╔══╝      ██║╚██╔╝██║██║   ██║██║╚██╗██║██║   ██║   ██║   ██║██╔══██╗
 ██║  ██║ ███████║   ██║       ███████╗██║ ╚████╔╝ ███████╗    ██║ ╚═╝ ██║╚██████╔╝██║ ╚████║██║   ██║   ╚██████╔╝██║  ██║
 ╚═╝  ╚═╝ ╚══════╝   ╚═╝       ╚══════╝╚═╝  ╚═══╝  ╚══════╝    ╚═╝     ╚═╝ ╚═════╝ ╚═╝  ╚═══╝╚═╝   ╚═╝    ╚═════╝ ╚═╝  ╚═╝
)" << COLOR_RESET << "\n";

    std::cout << COLOR_YELLOW;
    std::cout << "═══════════════════════════════════════════════════════════════════════════════════════════════════════════════\n";
    std::cout << "                    REAL-TIME SECURITY MONITORING WITH RST ANALYSIS\n";
    std::cout << "              Entropy • Coherence • Phase Deviation • GRAVITRAVITATION • VOLLENSTRAHLEN\n";
    std::cout << "═══════════════════════════════════════════════════════════════════════════════════════════════════════════════\n";
    std::cout << COLOR_RESET << "\n";
}

void print_help() {
    std::cout << "Usage: rael_monitor [OPTION]\n\n";
    std::cout << "Options:\n";
    std::cout << "  --start             Start live monitoring daemon\n";
    std::cout << "  --scan              Perform single full scan\n";
    std::cout << "  --analyze FILE      Analyze single file with RST methods\n";
    std::cout << "  --status            Show current status\n";
    std::cout << "  --interval MS       Set scan interval (default: 5000ms)\n";
    std::cout << "  --help              Show this help message\n";
    std::cout << "\n";
    std::cout << "RST Analysis Methods:\n";
    std::cout << "  Shannon Entropy     Measures randomness (0-8 bits)\n";
    std::cout << "  RST Coherence       Harmony with 88-signature (0-1)\n";
    std::cout << "  Phase Deviation     Deviation from optimal RST state (0-1)\n";
    std::cout << "  Malice Index        Combined threat score (0-1)\n";
    std::cout << "\n";
    std::cout << "RST Constants:\n";
    std::cout << "  G0 = 8/9 (WAHRHEIT)    Gate53 = 53 Hz (Sophie-Germain)\n";
    std::cout << "  φ  = 1.618...          Sig88 = 0.888... (Michael-Signatur)\n";
    std::cout << "\n";
    std::cout << "WICHTIG: Alles wird IMMER geprüft! Keine Whitelist!\n";
}

void print_rst_constants() {
    std::cout << COLOR_CYAN << "┌─ RST KONSTANTEN ";
    std::cout << std::string(61, '─') << "┐\n" << COLOR_RESET;

    std::cout << "│ " << COLOR_WHITE << "G0" << COLOR_RESET << " = 8/9 = " << COLOR_GREEN << "0.888..." << COLOR_RESET << " (WAHRHEIT)\n";
    std::cout << "│ " << COLOR_WHITE << "G1" << COLOR_RESET << " = 5/9 = " << COLOR_YELLOW << "0.555..." << COLOR_RESET << " (GRENZWERT)\n";
    std::cout << "│ " << COLOR_WHITE << "G3" << COLOR_RESET << " = 3/9 = " << COLOR_CYAN << "0.333..." << COLOR_RESET << " (AUFMERKSAMKEIT)\n";
    std::cout << "│ " << COLOR_WHITE << "G5" << COLOR_RESET << " = 1/9 = " << COLOR_DIM << "0.111..." << COLOR_RESET << " (MINIMAL)\n";
    std::cout << "│\n";
    std::cout << "│ " << COLOR_WHITE << "φ (PHI)" << COLOR_RESET << " = 1.6180339... (Goldener Schnitt)\n";
    std::cout << "│ " << COLOR_WHITE << "Gate53" << COLOR_RESET << " = 53 Hz (Sophie-Germain Primzahl)\n";
    std::cout << "│ " << COLOR_WHITE << "Sig88" << COLOR_RESET << " = 0.888... (Michael-Signatur)\n";

    std::cout << COLOR_CYAN << "└" << std::string(78, '─') << "┘\n" << COLOR_RESET;
}

void analyze_file(const std::string& path) {
    print_banner();

    std::cout << COLOR_CYAN << "[ANALYZE] " << COLOR_RESET << "Analysiere: " << path << "\n\n";

    // Datei lesen
    std::ifstream file(path, std::ios::binary);
    if (!file) {
        std::cout << COLOR_RED << "[ERROR] Datei konnte nicht geöffnet werden: " << path << COLOR_RESET << "\n";
        return;
    }

    std::vector<uint8_t> content((std::istreambuf_iterator<char>(file)),
                                  std::istreambuf_iterator<char>());

    std::cout << COLOR_WHITE << "Dateigröße: " << COLOR_RESET << content.size() << " bytes\n\n";

    // RST Analyse
    auto analysis = RSTAnalysisEngine::calculate_malice_index(content);

    // Ergebnisse anzeigen
    std::cout << COLOR_YELLOW << "┌─ RST ANALYSE ERGEBNISSE ";
    std::cout << std::string(53, '─') << "┐\n" << COLOR_RESET;

    // Entropy
    std::cout << "│ " << COLOR_WHITE << "Shannon Entropy:    " << COLOR_RESET;
    std::cout << std::fixed << std::setprecision(4) << analysis.entropy << " bits";
    if (analysis.entropy > 7.5) {
        std::cout << COLOR_RED << " [GEPACKT/VERSCHLÜSSELT]" << COLOR_RESET;
    } else if (analysis.entropy > 6.0) {
        std::cout << COLOR_YELLOW << " [HOHE ZUFÄLLIGKEIT]" << COLOR_RESET;
    } else {
        std::cout << COLOR_GREEN << " [NORMAL]" << COLOR_RESET;
    }
    std::cout << "\n";

    // Coherence
    std::cout << "│ " << COLOR_WHITE << "RST Coherence:      " << COLOR_RESET;
    std::cout << std::fixed << std::setprecision(4) << analysis.coherence;
    if (analysis.coherence > 0.888) {
        std::cout << COLOR_GREEN << " [HOHE HARMONIE]" << COLOR_RESET;
    } else if (analysis.coherence > 0.555) {
        std::cout << COLOR_YELLOW << " [MODERATE HARMONIE]" << COLOR_RESET;
    } else {
        std::cout << COLOR_RED << " [NIEDRIGE HARMONIE]" << COLOR_RESET;
    }
    std::cout << "\n";

    // Phase Deviation
    std::cout << "│ " << COLOR_WHITE << "Phase Deviation:    " << COLOR_RESET;
    std::cout << std::fixed << std::setprecision(4) << analysis.phase_deviation;
    if (analysis.phase_deviation > 0.666) {
        std::cout << COLOR_RED << " [CHAOTISCH]" << COLOR_RESET;
    } else if (analysis.phase_deviation > 0.333) {
        std::cout << COLOR_YELLOW << " [INSTABIL]" << COLOR_RESET;
    } else {
        std::cout << COLOR_GREEN << " [STABIL]" << COLOR_RESET;
    }
    std::cout << "\n";

    // 88-Signatur
    std::cout << "│ " << COLOR_WHITE << "88-Signatur:        " << COLOR_RESET;
    if (analysis.has_signature_88) {
        std::cout << COLOR_GREEN << "GEFUNDEN" << COLOR_RESET;
        std::cout << " (Score: " << std::fixed << std::setprecision(4) << analysis.signature_score << ")";
    } else {
        std::cout << COLOR_RED << "NICHT GEFUNDEN" << COLOR_RESET;
    }
    std::cout << "\n";

    std::cout << "│\n";

    // Malice Index
    std::cout << "│ " << COLOR_BOLD << "MALICE INDEX:       " << COLOR_RESET;

    const char* malice_color = COLOR_GREEN;
    if (analysis.malice_index > 0.888) malice_color = COLOR_RED;
    else if (analysis.malice_index > 0.555) malice_color = COLOR_YELLOW;
    else if (analysis.malice_index > 0.333) malice_color = COLOR_CYAN;

    std::cout << malice_color << std::fixed << std::setprecision(4) << analysis.malice_index << COLOR_RESET;

    // Progress-Bar
    int bar_len = static_cast<int>(analysis.malice_index * 30.0);
    std::cout << "  [" << malice_color << std::string(bar_len, '█')
              << COLOR_DIM << std::string(30 - bar_len, '░') << COLOR_RESET << "]\n";

    // Assessment
    std::cout << "│ " << COLOR_WHITE << "Bewertung:          " << COLOR_RESET;
    std::cout << malice_color << analysis.assessment << COLOR_RESET << "\n";

    std::cout << COLOR_YELLOW << "└" << std::string(78, '─') << "┘\n" << COLOR_RESET;

    // Signatur-Scan
    std::cout << "\n";
    SignatureDatabase sig_db;
    auto matches = sig_db.scan(content);

    if (!matches.empty()) {
        std::cout << COLOR_RED << "┌─ SIGNATUR MATCHES ";
        std::cout << std::string(59, '─') << "┐\n" << COLOR_RESET;

        for (const auto& match : matches) {
            std::cout << "│ " << COLOR_RED << "[" << match.category << "]" << COLOR_RESET;
            std::cout << " " << match.name;
            std::cout << " @ offset " << match.offset;
            std::cout << " (severity: " << std::fixed << std::setprecision(2) << match.severity << ")\n";
        }

        std::cout << COLOR_RED << "└" << std::string(78, '─') << "┘\n" << COLOR_RESET;
    } else {
        std::cout << COLOR_GREEN << "[OK] Keine bekannten Malware-Signaturen gefunden\n" << COLOR_RESET;
    }

    // RST Konstanten
    std::cout << "\n";
    print_rst_constants();
}

void run_single_scan(SecurityCore& core) {
    print_banner();

    std::cout << COLOR_CYAN << "[SCAN] Führe einmaligen Scan durch..." << COLOR_RESET << "\n\n";

    // Callbacks setzen
    int threat_count = 0;

    core.set_threat_callback([&threat_count](const Threat& t) {
        threat_count++;

        const char* color = COLOR_YELLOW;
        if (t.attack_energy > 0.8) color = COLOR_RED;
        else if (t.attack_energy > 0.5) color = COLOR_YELLOW;
        else color = COLOR_CYAN;

        std::cout << color << "[" << threat_name(t.type) << "]" << COLOR_RESET;
        std::cout << " " << t.source << " - " << t.details << "\n";
        std::cout << "  Attack: " << t.attack_energy;
        std::cout << " → Defense: " << t.defense_energy;

        if (t.defense_energy > 0.888) {
            std::cout << COLOR_GREEN << " [SUPERSONIC]" << COLOR_RESET;
        }
        std::cout << "\n\n";
    });

    core.set_trap_callback([](uint32_t entity_id, double harvested) {
        std::cout << COLOR_MAGENTA << "[GRAVITRAVITATION]" << COLOR_RESET;
        std::cout << " Entity " << entity_id << " gefangen! Ernte: " << harvested << "\n";
    });

    core.set_eruption_callback([](double energy) {
        std::cout << COLOR_YELLOW << "[VOLLENSTRAHLEN]" << COLOR_RESET;
        std::cout << " Sonnen-Eruption! Energie: " << energy << "\n";
    });

    // Scan durchführen
    core.scan_all();

    // Ergebnis
    std::cout << "═══════════════════════════════════════════════════════════════\n";
    if (threat_count == 0) {
        std::cout << COLOR_GREEN << "[RESULT] Keine Bedrohungen erkannt. System sicher.\n" << COLOR_RESET;
    } else {
        std::cout << COLOR_RED << "[RESULT] " << threat_count << " Bedrohung(en) erkannt!\n" << COLOR_RESET;
    }

    // Status
    std::cout << "\n";
    std::cout << "Defense Power:     " << core.get_defense_power() << "\n";
    std::cout << "Trapped Entities:  " << core.get_trapped_count() << "\n";
    std::cout << "Harvested Energy:  " << core.get_harvested_energy() << "\n";
    std::cout << "Light Energy:      " << core.get_light_energy() << "\n";
    std::cout << "Active Beams:      " << core.get_active_beams() << "/64\n";
    std::cout << "═══════════════════════════════════════════════════════════════\n";
}

void run_live_monitor(SecurityCore& core, int interval_ms) {
    print_banner();

    std::cout << COLOR_GREEN << "[DAEMON] Starte Live-Monitoring..." << COLOR_RESET << "\n";
    std::cout << "[DAEMON] Scan-Intervall: " << interval_ms << "ms\n";
    std::cout << "[DAEMON] Drücke Ctrl+C zum Beenden\n\n";

    // Signal Handler
    std::signal(SIGINT, signal_handler);
    std::signal(SIGTERM, signal_handler);

    // Live Monitor erstellen
    LiveMonitor monitor(core);
    monitor.set_scan_interval(std::chrono::milliseconds(interval_ms));
    monitor.set_display_interval(std::chrono::milliseconds(1000));

    // Threat Callback
    monitor.set_threat_callback([](const LiveScanResult& result) {
        if (result.threat_score > 0.555) {
            std::cout << "\a";  // Beep bei hoher Bedrohung
        }
    });

    // Starten
    monitor.start();

    // Warten auf Signal
    while (g_running) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    // Stoppen
    monitor.stop();

    std::cout << "\n" << COLOR_YELLOW << "[DAEMON] Beende..." << COLOR_RESET << "\n";
    std::cout << "\nFinale Statistiken:\n";
    std::cout << "  Total Scans:       " << monitor.get_total_scans() << "\n";
    std::cout << "  Threats Detected:  " << monitor.get_threats_detected() << "\n";
    std::cout << "  Defense Power:     " << core.get_defense_power() << "\n";
    std::cout << "  Trapped Entities:  " << core.get_trapped_count() << "\n";
}

void show_status(SecurityCore& core) {
    print_banner();

    // RST Konstanten
    print_rst_constants();
    std::cout << "\n";

    // Security Core Status
    std::cout << core.status_report();
}

int main(int argc, char* argv[]) {
    SecurityCore core;

    if (argc < 2) {
        print_help();
        return 0;
    }

    std::string cmd = argv[1];

    if (cmd == "--help" || cmd == "-h") {
        print_help();
        return 0;
    }

    if (cmd == "--start") {
        int interval = 5000;  // Default 5 Sekunden

        // Check für --interval
        for (int i = 2; i < argc - 1; ++i) {
            if (std::string(argv[i]) == "--interval") {
                interval = std::stoi(argv[i + 1]);
            }
        }

        run_live_monitor(core, interval);
        return 0;
    }

    if (cmd == "--scan") {
        run_single_scan(core);
        return 0;
    }

    if (cmd == "--analyze" && argc >= 3) {
        analyze_file(argv[2]);
        return 0;
    }

    if (cmd == "--status") {
        show_status(core);
        return 0;
    }

    std::cout << "Unbekannter Befehl: " << cmd << "\n";
    print_help();
    return 1;
}
