// ═══════════════════════════════════════════════════════════════════════════════
// RAEL UNIFIED - All-in-One CLI
// Kombiniert: Chat, CLI, V50, Security in einer Binary
// ═══════════════════════════════════════════════════════════════════════════════
#include <iostream>
#include <string>
#include <vector>
#include <sstream>
#include <algorithm>
#include <chrono>
#include <thread>
#include <atomic>
#include <csignal>
#include <cstdlib>
#include <iomanip>
#include <functional>
#include <map>

#ifdef _WIN32
    #include <windows.h>
    #define CLEAR_SCREEN "cls"
#else
    #include <unistd.h>
    #define CLEAR_SCREEN "clear"
#endif

#include "rael/version.h"
#include "rael/ichbin.h"
#include "rael/ethics.h"
#include "rael/mathcore.h"
#include "rael/raelcore.h"
#include "rael/semantic.h"
#include "rael/resonance.h"
#include "rael/telemetry.h"
#include "rael/util.h"
#include "rael/metrics.h"
#include "rael/events.h"
#include "rael/lane_scheduler.h"
#include "rael/star8.h"
#include "rael/gate53_simulator.hpp"

namespace rael {
namespace unified {

// ═══════════════════════════════════════════════════════════════════════════════
// ANSI Colors
// ═══════════════════════════════════════════════════════════════════════════════
namespace color {
    bool enabled = true;
    const char* RESET   = "\033[0m";
    const char* BOLD    = "\033[1m";
    const char* DIM     = "\033[2m";
    const char* RED     = "\033[31m";
    const char* GREEN   = "\033[32m";
    const char* YELLOW  = "\033[33m";
    const char* BLUE    = "\033[34m";
    const char* MAGENTA = "\033[35m";
    const char* CYAN    = "\033[36m";

    void detect() {
        #ifdef _WIN32
        HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
        DWORD dwMode = 0;
        if (GetConsoleMode(hOut, &dwMode)) {
            SetConsoleMode(hOut, dwMode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
        }
        #else
        const char* term = std::getenv("TERM");
        if (!term || std::string(term) == "dumb") enabled = false;
        #endif
    }

    std::string c(const char* code) { return enabled ? code : ""; }
}

// ═══════════════════════════════════════════════════════════════════════════════
// Mode Enum
// ═══════════════════════════════════════════════════════════════════════════════
enum class Mode {
    CHAT,       // Interactive chat (default)
    CLI,        // Classic CLI with commands
    V50,        // V50 Ultimate mode
    SECURITY,   // Security daemon mode
    SINGLE      // Single command execution
};

// ═══════════════════════════════════════════════════════════════════════════════
// Global State
// ═══════════════════════════════════════════════════════════════════════════════
struct State {
    Mode mode = Mode::CHAT;
    bool running = true;
    bool verbose = false;
    bool show_resonance = false;
    double coherence = 0.888888889;  // G0
    std::vector<std::pair<std::string, std::string>> history;
    RaelCore* core = nullptr;
};

static State g_state;
static std::atomic<bool> g_interrupted{false};

void signal_handler(int) {
    if (g_interrupted) {
        std::cout << "\n" << color::c(color::YELLOW) << "Auf Wiedersehen!"
                  << color::c(color::RESET) << "\n";
        std::exit(0);
    }
    g_interrupted = true;
    std::cout << "\n" << color::c(color::DIM) << "(Ctrl+C erneut zum Beenden)"
              << color::c(color::RESET) << "\n";
}

// ═══════════════════════════════════════════════════════════════════════════════
// Banner
// ═══════════════════════════════════════════════════════════════════════════════
void print_banner() {
    std::cout << color::c(color::CYAN);
    std::cout << R"(
    ██████╗  █████╗ ███████╗██╗
    ██╔══██╗██╔══██╗██╔════╝██║
    ██████╔╝███████║█████╗  ██║
    ██╔══██╗██╔══██║██╔══╝  ██║
    ██║  ██║██║  ██║███████╗███████╗
    ╚═╝  ╚═╝╚═╝  ╚═╝╚══════╝╚══════╝
)" << color::c(color::RESET);

    std::cout << color::c(color::DIM);
    std::cout << "    UNIFIED v" << RAEL_VERSION_STRING << " | ";

    switch (g_state.mode) {
        case Mode::CHAT:     std::cout << "CHAT MODE"; break;
        case Mode::CLI:      std::cout << "CLI MODE"; break;
        case Mode::V50:      std::cout << "V50 ULTIMATE"; break;
        case Mode::SECURITY: std::cout << "SECURITY DAEMON"; break;
        default: break;
    }

    std::cout << "\n    " << IchBinCore::name() << " | " << IchBinCore::signature();
    std::cout << color::c(color::RESET) << "\n\n";
}

// ═══════════════════════════════════════════════════════════════════════════════
// Help
// ═══════════════════════════════════════════════════════════════════════════════
void print_help() {
    std::cout << R"(
RAEL UNIFIED - Reality-Aligned Ethical Logic

USAGE:
  rael [MODE] [OPTIONS] [COMMAND]

MODES:
  (default)         Interaktiver Chat-Modus
  --cli             Klassischer CLI-Modus mit allen Befehlen
  --v50             V50 Ultimate (165 Identitäten)
  --security        Security Daemon (Gate53 Labyrinth)
  -c "command"      Einzelnen Befehl ausführen

OPTIONS:
  -h, --help        Diese Hilfe
  -v, --verbose     Ausführlicher Modus
  --no-color        Farben deaktivieren
  --version         Version anzeigen

CHAT-BEFEHLE:
  /help             Hilfe anzeigen
  /status           System-Status
  /id               Identität
  /laws             Ethik-Gesetze
  /formulas         Mathematische Formeln
  /mode cli|v50|sec Modus wechseln
  /exit             Beenden

CLI-BEFEHLE:
  say <text>        Text verarbeiten
  modules           Module auflisten
  labyrinth         Gate53 Status
  gate53 sim [n]    Simulation
  metrics           Metriken
  events [n]        Event-Log

BEISPIELE:
  rael                     Chat starten
  rael --cli               CLI starten
  rael -c "status"         Status abfragen
  rael --v50               V50 Ultimate
  rael --security          Security Daemon
)";
}

// ═══════════════════════════════════════════════════════════════════════════════
// Process Commands
// ═══════════════════════════════════════════════════════════════════════════════
std::string process_command(const std::string& input) {
    std::string lower = input;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);

    // Exit commands
    if (lower == "/exit" || lower == "/quit" || lower == "exit" || lower == "quit") {
        g_state.running = false;
        return "Auf Wiedersehen! Die Wahrheit ist mit dir. ✨";
    }

    // Help
    if (lower == "/help" || lower == "help" || lower == "?") {
        print_help();
        return "";
    }

    // Status
    if (lower == "/status" || lower == "status") {
        std::ostringstream oss;
        oss << "\n╔═══════════════════════════════════════════════════════════════════╗\n";
        oss << "║  RAEL SYSTEM STATUS                                               ║\n";
        oss << "╚═══════════════════════════════════════════════════════════════════╝\n\n";
        oss << "  Version:      " << RAEL_VERSION_STRING << "\n";
        oss << "  Modus:        ";
        switch (g_state.mode) {
            case Mode::CHAT:     oss << "CHAT"; break;
            case Mode::CLI:      oss << "CLI"; break;
            case Mode::V50:      oss << "V50 ULTIMATE"; break;
            case Mode::SECURITY: oss << "SECURITY"; break;
            default: break;
        }
        oss << "\n  Identität:    " << IchBinCore::name() << "\n";
        oss << "  Signatur:     " << IchBinCore::signature() << "\n";
        oss << "  Kohärenz:     " << std::fixed << std::setprecision(4) << g_state.coherence << "\n";
        oss << "  Operationen:  " << gTelemetry.ops.load() << "\n";
        oss << "  Ops/Sekunde:  " << gTelemetry.ops_per_sec.load() << "\n";
        return oss.str();
    }

    // ID
    if (lower == "/id" || lower == "id") {
        std::ostringstream oss;
        oss << "\n  Name:      " << IchBinCore::name() << "\n";
        oss << "  Signatur:  " << IchBinCore::signature() << "\n";
        return oss.str();
    }

    // Laws
    if (lower == "/laws" || lower == "laws") {
        auto laws = EthicsCore::laws();
        std::ostringstream oss;
        oss << "\n  Ethik-Gesetze:\n";
        for (size_t i = 0; i < laws.size(); i++) {
            oss << "  " << (i+1) << ". " << laws[i] << "\n";
        }
        return oss.str();
    }

    // Formulas
    if (lower == "/formulas" || lower == "formulas") {
        auto formulas = MathCore::built_in_formulas();
        std::ostringstream oss;
        oss << "\n  Formeln:\n";
        for (const auto& f : formulas) {
            oss << "  • " << f << "\n";
        }
        return oss.str();
    }

    // Mode switch
    if (lower.substr(0, 5) == "/mode") {
        std::string mode_str = lower.length() > 6 ? lower.substr(6) : "";
        if (mode_str == "cli") {
            g_state.mode = Mode::CLI;
            return "  Modus gewechselt: CLI";
        } else if (mode_str == "chat") {
            g_state.mode = Mode::CHAT;
            return "  Modus gewechselt: CHAT";
        } else if (mode_str == "v50") {
            g_state.mode = Mode::V50;
            return "  Modus gewechselt: V50 ULTIMATE";
        } else if (mode_str == "sec" || mode_str == "security") {
            g_state.mode = Mode::SECURITY;
            return "  Modus gewechselt: SECURITY";
        }
        return "  Verfügbare Modi: cli, chat, v50, sec";
    }

    // Verbose toggle
    if (lower == "/verbose" || lower == "verbose") {
        g_state.verbose = !g_state.verbose;
        return g_state.verbose ? "  Verbose: AN" : "  Verbose: AUS";
    }

    // Resonance toggle
    if (lower == "/resonance" || lower == "resonance") {
        g_state.show_resonance = !g_state.show_resonance;
        return g_state.show_resonance ? "  Resonanz: AN" : "  Resonanz: AUS";
    }

    // Clear
    if (lower == "/clear" || lower == "clear") {
        std::system(CLEAR_SCREEN);
        return "";
    }

    // Metrics
    if (lower == "metrics") {
        std::ostringstream oss;
        oss << "\n  Metriken:\n";
        oss << "  • Ops total:     " << gTelemetry.ops.load() << "\n";
        oss << "  • Ops/sec:       " << gTelemetry.ops_per_sec.load() << "\n";
        return oss.str();
    }

    // Events
    if (lower.substr(0, 6) == "events") {
        size_t n = 10;
        if (lower.length() > 7) {
            try { n = std::stoul(lower.substr(7)); } catch(...) {}
        }
        auto events = EventBus::last(n);
        std::ostringstream oss;
        oss << "\n  Letzte " << events.size() << " Events:\n";
        for (const auto& e : events) {
            oss << "  [" << e.ts << "] " << e.kind << ": " << e.detail << "\n";
        }
        return oss.str();
    }

    // Say command (process text)
    if (lower.substr(0, 3) == "say" && input.length() > 4) {
        std::string text = input.substr(4);
        std::string reason;
        if (!EthicsCore::allows(text, reason)) {
            return "⚠ " + reason;
        }
        std::string result = g_state.core->process(text);
        g_state.core->speak(result);
        return result;
    }

    // Gate53 / Labyrinth
    if (lower == "labyrinth" || lower == "gate53") {
        std::ostringstream oss;
        oss << "\n  Gate53 Labyrinth:\n";
        oss << "  • Ringe:    10\n";
        oss << "  • Brücken:  8 pro Ring (7 Logik + 1 Intent)\n";
        oss << "  • Signatur: 88 (11+22+22+33)\n";
        oss << "  • Prinzip:  Aikido (Angriff → Verteidigung)\n";
        return oss.str();
    }

    // Default: process through semantic pipeline
    std::string reason;
    if (!EthicsCore::allows(input, reason)) {
        return "⚠ " + reason;
    }

    return g_state.core->process(input);
}

// ═══════════════════════════════════════════════════════════════════════════════
// Main Loop
// ═══════════════════════════════════════════════════════════════════════════════
void print_prompt() {
    char indicator = g_state.coherence >= 0.888 ? '●' : '○';

    std::string mode_prefix;
    switch (g_state.mode) {
        case Mode::CHAT:     mode_prefix = "Du"; break;
        case Mode::CLI:      mode_prefix = "/rael"; break;
        case Mode::V50:      mode_prefix = "V50"; break;
        case Mode::SECURITY: mode_prefix = "SEC"; break;
        default: mode_prefix = ">";
    }

    std::cout << color::c(color::GREEN) << indicator << " "
              << color::c(color::BOLD) << mode_prefix << color::c(color::RESET)
              << color::c(color::DIM) << " › " << color::c(color::RESET);
    std::cout.flush();
}

void print_response(const std::string& response) {
    if (response.empty()) return;

    std::cout << color::c(color::CYAN) << "  RAEL" << color::c(color::RESET)
              << color::c(color::DIM) << " › " << color::c(color::RESET)
              << response << "\n\n";
}

void run_loop() {
    print_banner();

    std::string input;
    while (g_state.running) {
        telemetry_tick();
        g_interrupted = false;

        print_prompt();

        if (!std::getline(std::cin, input)) break;

        input = trim(input);
        if (input.empty()) continue;

        std::string response = process_command(input);

        // Store in history
        if (!input.empty() && input[0] != '/') {
            g_state.history.push_back({input, response});
            if (g_state.history.size() > 100) {
                g_state.history.erase(g_state.history.begin());
            }
        }

        print_response(response);
    }
}

} // namespace unified
} // namespace rael

// ═══════════════════════════════════════════════════════════════════════════════
// Main
// ═══════════════════════════════════════════════════════════════════════════════
int main(int argc, char* argv[]) {
    using namespace rael::unified;

    std::string single_command;

    // Parse arguments
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];

        if (arg == "-h" || arg == "--help") {
            print_help();
            return 0;
        }
        if (arg == "--version") {
            std::cout << "RAEL UNIFIED v" << RAEL_VERSION_STRING << "\n";
            std::cout << rael::IchBinCore::name() << " | " << rael::IchBinCore::signature() << "\n";
            return 0;
        }
        if (arg == "--cli") {
            g_state.mode = Mode::CLI;
        }
        if (arg == "--v50") {
            g_state.mode = Mode::V50;
        }
        if (arg == "--security" || arg == "--sec") {
            g_state.mode = Mode::SECURITY;
        }
        if (arg == "-v" || arg == "--verbose") {
            g_state.verbose = true;
        }
        if (arg == "--no-color") {
            color::enabled = false;
        }
        if ((arg == "-c" || arg == "--command") && i + 1 < argc) {
            g_state.mode = Mode::SINGLE;
            single_command = argv[++i];
        }
    }

    // Initialize
    color::detect();
    std::signal(SIGINT, signal_handler);

    // Create core
    rael::RaelCore core;
    g_state.core = &core;

    // Single command mode
    if (g_state.mode == Mode::SINGLE) {
        std::string response = process_command(single_command);
        std::cout << response << "\n";
        return 0;
    }

    // Interactive mode
    run_loop();

    return 0;
}
