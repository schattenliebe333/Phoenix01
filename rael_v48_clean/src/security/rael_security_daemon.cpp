// ═══════════════════════════════════════════════════════════════════════════════
// RAEL SECURITY DAEMON - Attack-to-Defense Conversion Service
// ═══════════════════════════════════════════════════════════════════════════════
//
// Das Herz von Phoenix/RAEL: Ein lebendiges Klärwerk für den Informationsfluss
//
// Konzept:
//   ANGRIFF → LABYRINTH → SPIRALE → DE-LAVAL DÜSE → VERTEIDIGUNG
//   Je mehr Angriffe, desto stärker die Verteidigung
//
// Nutzung:
//   rael_security --start     # Daemon starten
//   rael_security --scan      # Einmaliger Scan
//   rael_security --status    # Status anzeigen
//   rael_security --kill PID  # Prozess beenden
//
// ═══════════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <string>
#include <thread>
#include <chrono>
#include <csignal>
#include <atomic>
#include <cstring>

#include "rael/security_core.hpp"

using namespace rael::security_core;

// ANSI Colors
#ifdef _WIN32
#define COLOR_RED     ""
#define COLOR_GREEN   ""
#define COLOR_YELLOW  ""
#define COLOR_BLUE    ""
#define COLOR_MAGENTA ""
#define COLOR_CYAN    ""
#define COLOR_RESET   ""
#else
#define COLOR_RED     "\033[91m"
#define COLOR_GREEN   "\033[92m"
#define COLOR_YELLOW  "\033[93m"
#define COLOR_BLUE    "\033[94m"
#define COLOR_MAGENTA "\033[95m"
#define COLOR_CYAN    "\033[96m"
#define COLOR_RESET   "\033[0m"
#endif

// Globale Variable für Signal-Handler
std::atomic<bool> g_running(true);

void signal_handler(int sig) {
    (void)sig;
    g_running = false;
}

void print_banner() {
    std::cout << COLOR_CYAN;
    std::cout << R"(
 ██████╗  █████╗ ███████╗██╗         ███████╗███████╗ ██████╗██╗   ██╗██████╗ ██╗████████╗██╗   ██╗
 ██╔══██╗██╔══██╗██╔════╝██║         ██╔════╝██╔════╝██╔════╝██║   ██║██╔══██╗██║╚══██╔══╝╚██╗ ██╔╝
 ██████╔╝███████║█████╗  ██║         ███████╗█████╗  ██║     ██║   ██║██████╔╝██║   ██║    ╚████╔╝
 ██╔══██╗██╔══██║██╔══╝  ██║         ╚════██║██╔══╝  ██║     ██║   ██║██╔══██╗██║   ██║     ╚██╔╝
 ██║  ██║██║  ██║███████╗███████╗    ███████║███████╗╚██████╗╚██████╔╝██║  ██║██║   ██║      ██║
 ╚═╝  ╚═╝╚═╝  ╚═╝╚══════╝╚══════╝    ╚══════╝╚══════╝ ╚═════╝ ╚═════╝ ╚═╝  ╚═╝╚═╝   ╚═╝      ╚═╝
)" << COLOR_RESET << "\n";

    std::cout << COLOR_YELLOW;
    std::cout << "═══════════════════════════════════════════════════════════════════════════════════════════════\n";
    std::cout << "                    ATTACK → DEFENSE CONVERSION SYSTEM\n";
    std::cout << "                    LABYRINTH (Gate53) → SPIRALE (φ) → DE-LAVAL DÜSE\n";
    std::cout << "═══════════════════════════════════════════════════════════════════════════════════════════════\n";
    std::cout << COLOR_RESET << "\n";
}

void print_help() {
    std::cout << "Usage: rael_security [OPTION]\n\n";
    std::cout << "Options:\n";
    std::cout << "  --start        Start security daemon (continuous monitoring)\n";
    std::cout << "  --scan         Perform single security scan\n";
    std::cout << "  --status       Show current security status\n";
    std::cout << "  --kill PID     Terminate suspicious process by PID\n";
    std::cout << "  --protect DIR  Add directory to protection list\n";
    std::cout << "  --help         Show this help message\n";
    std::cout << "\n";
    std::cout << "RST Constants:\n";
    std::cout << "  G0 = 8/9 (WAHRHEIT)    Gate53 = 53 Hz (Sophie-Germain)\n";
    std::cout << "  φ  = 1.618...          e = 2.718... (Nozzle Expansion)\n";
    std::cout << "\n";
    std::cout << "Attack → Defense Conversion:\n";
    std::cout << "  1. LABYRINTH: 7-layer Gate53 absorption (κ damping)\n";
    std::cout << "  2. SPIRALE:   Golden ratio compression (φ³)\n";
    std::cout << "  3. NOZZLE:    De-Laval supersonic expansion\n";
    std::cout << "  4. DEFENSE:   Automated response (terminate, block, quarantine)\n";
}

void print_threat(const Threat& t) {
    const char* color = COLOR_YELLOW;
    if (t.attack_energy > 0.8) color = COLOR_RED;
    else if (t.attack_energy > 0.5) color = COLOR_YELLOW;
    else color = COLOR_CYAN;

    std::cout << color;
    std::cout << "[" << threat_name(t.type) << "] ";
    std::cout << COLOR_RESET;
    std::cout << t.source << " - " << t.details << "\n";
    std::cout << "  Attack Energy:  " << t.attack_energy << "\n";
    std::cout << "  Defense Energy: " << t.defense_energy;
    if (t.defense_energy > rst::G0) {
        std::cout << COLOR_GREEN << " [SUPERSONIC]" << COLOR_RESET;
    }
    std::cout << "\n\n";
}

void run_daemon(SecurityCore& core) {
    print_banner();

    std::cout << COLOR_GREEN << "[DAEMON] Starting security monitoring..." << COLOR_RESET << "\n";
    std::cout << "[DAEMON] Press Ctrl+C to stop\n\n";

    // Callbacks setzen
    core.set_threat_callback([](const Threat& t) {
        std::cout << COLOR_RED << "[THREAT DETECTED]" << COLOR_RESET << "\n";
        print_threat(t);
    });

    core.set_defense_callback([](const std::string& action) {
        std::cout << COLOR_GREEN << "[DEFENSE ACTION] " << action << COLOR_RESET << "\n";
    });

    // Signal Handler
    std::signal(SIGINT, signal_handler);
    std::signal(SIGTERM, signal_handler);

    // Monitoring Loop
    while (g_running) {
        core.scan_all();

        // Status alle 30 Sekunden ausgeben
        static int counter = 0;
        if (++counter >= 6) {  // 6 × 5s = 30s
            counter = 0;
            std::cout << "\n" << COLOR_CYAN << "[STATUS] " << COLOR_RESET;
            std::cout << "Pressure: " << core.get_labyrinth_pressure();
            std::cout << " | Defense: " << core.get_defense_power();
            std::cout << " | Supersonic: " << (core.is_supersonic() ? "YES" : "NO");
            std::cout << "\n";
        }

        std::this_thread::sleep_for(std::chrono::seconds(5));
    }

    std::cout << "\n" << COLOR_YELLOW << "[DAEMON] Shutting down..." << COLOR_RESET << "\n";
}

void run_scan(SecurityCore& core) {
    print_banner();

    std::cout << COLOR_CYAN << "[SCAN] Performing security scan..." << COLOR_RESET << "\n\n";

    int threat_count = 0;

    // Callbacks für diesen Scan
    core.set_threat_callback([&threat_count](const Threat& t) {
        threat_count++;
        print_threat(t);
    });

    core.scan_all();

    std::cout << "═══════════════════════════════════════════════════════════════\n";
    if (threat_count == 0) {
        std::cout << COLOR_GREEN << "[RESULT] No threats detected. System clean." << COLOR_RESET << "\n";
    } else {
        std::cout << COLOR_RED << "[RESULT] " << threat_count << " threat(s) detected!" << COLOR_RESET << "\n";
        std::cout << "Defense Power accumulated: " << core.get_defense_power() << "\n";
    }
    std::cout << "═══════════════════════════════════════════════════════════════\n";
}

void show_status(SecurityCore& core) {
    print_banner();
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
        run_daemon(core);
        return 0;
    }

    if (cmd == "--scan") {
        run_scan(core);
        return 0;
    }

    if (cmd == "--status") {
        show_status(core);
        return 0;
    }

    if (cmd == "--kill" && argc >= 3) {
        uint32_t pid = std::stoul(argv[2]);
        print_banner();
        std::cout << "[KILL] Attempting to terminate PID " << pid << "...\n";

        // Energie aufladen für Kill
        Threat fake;
        fake.type = ThreatType::PROCESS_SUSPICIOUS;
        fake.attack_energy = 0.5;
        core.process_threat(fake);

        if (core.kill_process(pid)) {
            std::cout << COLOR_GREEN << "[SUCCESS] Process " << pid << " terminated." << COLOR_RESET << "\n";
        } else {
            std::cout << COLOR_RED << "[FAILED] Could not terminate process " << pid << "." << COLOR_RESET << "\n";
            std::cout << "Possible reasons: insufficient permissions, process not found, or not enough defense energy.\n";
        }
        return 0;
    }

    if (cmd == "--protect" && argc >= 3) {
        std::string dir = argv[2];
        core.protect_directory(dir);
        std::cout << "[PROTECT] Added " << dir << " to protected directories.\n";
        return 0;
    }

    std::cout << "Unknown command: " << cmd << "\n";
    print_help();
    return 1;
}
