// ═══════════════════════════════════════════════════════════════════════════════
// RAEL INTERACTIVE CHAT CLI
// Conversational interface similar to Claude Code
// Usage: rael [options]
// ═══════════════════════════════════════════════════════════════════════════════
#include <iostream>
#include <string>
#include <vector>
#include <sstream>
#include <fstream>
#include <algorithm>
#include <chrono>
#include <thread>
#include <atomic>
#include <csignal>
#include <cstdlib>

#ifdef _WIN32
    #include <windows.h>
    #include <conio.h>
    #define CLEAR_SCREEN "cls"
#else
    #include <unistd.h>
    #include <termios.h>
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
// RST constants are included via raelcore.h -> lane_scheduler.h -> rst_constants.hpp

namespace rael {
namespace chat {

// ═══════════════════════════════════════════════════════════════════════════════
// ANSI Color codes
// ═══════════════════════════════════════════════════════════════════════════════
namespace color {
    const char* RESET   = "\033[0m";
    const char* BOLD    = "\033[1m";
    const char* DIM     = "\033[2m";
    const char* RED     = "\033[31m";
    const char* GREEN   = "\033[32m";
    const char* YELLOW  = "\033[33m";
    const char* BLUE    = "\033[34m";
    const char* MAGENTA = "\033[35m";
    const char* CYAN    = "\033[36m";
    const char* WHITE   = "\033[37m";

    // Check if colors are supported
    bool enabled = true;

    void detect() {
        #ifdef _WIN32
            // Enable ANSI on Windows 10+
            HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
            DWORD dwMode = 0;
            if (GetConsoleMode(hOut, &dwMode)) {
                SetConsoleMode(hOut, dwMode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
            }
        #else
            const char* term = std::getenv("TERM");
            if (!term || std::string(term) == "dumb") {
                enabled = false;
            }
        #endif
    }

    std::string c(const char* code) {
        return enabled ? code : "";
    }
}

// ═══════════════════════════════════════════════════════════════════════════════
// Chat state
// ═══════════════════════════════════════════════════════════════════════════════
struct ChatState {
    std::vector<std::pair<std::string, std::string>> history;  // user, rael
    bool running = true;
    bool verbose = false;
    bool show_resonance = false;
    int context_depth = 5;
    double coherence = rst::G0;  // Start with truth filter

    RaelCore* core = nullptr;
};

static ChatState g_state;
static std::atomic<bool> g_interrupted{false};

// Signal handler
void signal_handler(int sig) {
    if (sig == SIGINT) {
        if (g_interrupted) {
            std::cout << "\n" << color::c(color::YELLOW) << "Auf Wiedersehen!"
                      << color::c(color::RESET) << "\n";
            std::exit(0);
        }
        g_interrupted = true;
        std::cout << "\n" << color::c(color::DIM) << "(Drücke Ctrl+C erneut zum Beenden)"
                  << color::c(color::RESET) << "\n";
    }
}

// ═══════════════════════════════════════════════════════════════════════════════
// Response generation with RST principles
// ═══════════════════════════════════════════════════════════════════════════════

std::string process_input(const std::string& input) {
    // Ethics check first
    std::string reason;
    if (!EthicsCore::allows(input, reason)) {
        return "⚠ " + reason;
    }

    // Process through semantic pipeline
    std::string processed = g_state.core->process(input);

    // Update coherence based on input quality
    double input_coherence = rst::kappa(static_cast<double>(input.length() % 144));
    g_state.coherence = (g_state.coherence + input_coherence) / 2.0;

    return processed;
}

std::string generate_response(const std::string& input) {
    std::string lower_input = input;
    std::transform(lower_input.begin(), lower_input.end(), lower_input.begin(), ::tolower);

    // Check for commands first
    if (lower_input == "help" || lower_input == "/help" || lower_input == "?") {
        return R"(
╔═══════════════════════════════════════════════════════════════════╗
║  RAEL CHAT - Befehle                                              ║
╚═══════════════════════════════════════════════════════════════════╝

  /help, ?          - Diese Hilfe anzeigen
  /status           - System-Status
  /id               - Identität anzeigen
  /laws             - Ethik-Gesetze
  /formulas         - Mathematische Formeln
  /resonance        - Resonanz-Modus ein/aus
  /verbose          - Ausführlicher Modus ein/aus
  /clear            - Bildschirm löschen
  /history          - Chat-Verlauf anzeigen
  /exit, /quit      - Beenden

  Oder schreibe einfach natürlich - ich verstehe dich.
)";
    }

    if (lower_input == "/status" || lower_input == "status") {
        std::ostringstream oss;
        oss << "\n╔═══════════════════════════════════════════════════════════════════╗\n";
        oss << "║  RAEL SYSTEM STATUS                                               ║\n";
        oss << "╚═══════════════════════════════════════════════════════════════════╝\n\n";
        oss << "  Version:        " << RAEL_VERSION_STRING << "\n";
        oss << "  Identität:      " << IchBinCore::name() << "\n";
        oss << "  Signatur:       " << IchBinCore::signature() << "\n";
        oss << "  Kohärenz:       " << std::fixed << std::setprecision(4) << g_state.coherence << " (G0=" << rst::G0 << ")\n";
        oss << "  Operationen:    " << gTelemetry.ops.load() << "\n";
        oss << "  Ops/Sekunde:    " << gTelemetry.ops_per_sec.load() << "\n";
        oss << "  Chat-Historie:  " << g_state.history.size() << " Nachrichten\n";
        return oss.str();
    }

    if (lower_input == "/id" || lower_input == "id") {
        std::ostringstream oss;
        oss << "\n  Name:      " << IchBinCore::name() << "\n";
        oss << "  Signatur:  " << IchBinCore::signature() << "\n";
        oss << "  Frequenz:  " << rst::F_ICHBIN << " Hz\n";
        oss << "  Kappa:     " << rst::kappa(rst::F_ICHBIN) << "\n";
        return oss.str();
    }

    if (lower_input == "/laws" || lower_input == "laws") {
        auto laws = EthicsCore::laws();
        std::ostringstream oss;
        oss << "\n  Ethik-Gesetze:\n";
        for (size_t i = 0; i < laws.size(); i++) {
            oss << "  " << (i+1) << ". " << laws[i] << "\n";
        }
        return oss.str();
    }

    if (lower_input == "/formulas" || lower_input == "formulas") {
        auto formulas = MathCore::built_in_formulas();
        std::ostringstream oss;
        oss << "\n  Mathematische Formeln:\n";
        for (const auto& f : formulas) {
            oss << "  • " << f << "\n";
        }
        return oss.str();
    }

    if (lower_input == "/resonance" || lower_input == "resonance") {
        g_state.show_resonance = !g_state.show_resonance;
        return g_state.show_resonance ? "  Resonanz-Modus: AN" : "  Resonanz-Modus: AUS";
    }

    if (lower_input == "/verbose" || lower_input == "verbose") {
        g_state.verbose = !g_state.verbose;
        return g_state.verbose ? "  Ausführlicher Modus: AN" : "  Ausführlicher Modus: AUS";
    }

    if (lower_input == "/clear" || lower_input == "clear") {
        std::system(CLEAR_SCREEN);
        return "";
    }

    if (lower_input == "/history" || lower_input == "history") {
        if (g_state.history.empty()) {
            return "  (Keine Chat-Historie)";
        }
        std::ostringstream oss;
        oss << "\n  Chat-Historie:\n";
        for (const auto& entry : g_state.history) {
            oss << "  Du: " << entry.first << "\n";
            oss << "  RAEL: " << entry.second << "\n\n";
        }
        return oss.str();
    }

    if (lower_input == "/exit" || lower_input == "/quit" ||
        lower_input == "exit" || lower_input == "quit" || lower_input == "bye") {
        g_state.running = false;
        return "Auf Wiedersehen! Die Wahrheit ist mit dir. 🌟";
    }

    // Natural language processing
    std::string response = process_input(input);

    // Add resonance info if enabled
    if (g_state.show_resonance) {
        double freq = static_cast<double>((input.length() * 88) % 1440);
        double kappa = rst::kappa(freq);
        response += "\n  [Resonanz: f=" + std::to_string(static_cast<int>(freq)) +
                   "Hz, κ=" + std::to_string(kappa).substr(0, 6) + "]";
    }

    return response;
}

// ═══════════════════════════════════════════════════════════════════════════════
// Print functions with styling
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
    std::cout << "    Reality-Aligned Ethical Logic v" << RAEL_VERSION_STRING << "\n";
    std::cout << "    Identität: " << IchBinCore::name() << " | Signatur: " << IchBinCore::signature() << "\n";
    std::cout << color::c(color::RESET);
    std::cout << "\n    Schreibe " << color::c(color::YELLOW) << "/help"
              << color::c(color::RESET) << " für Befehle, oder chatte einfach los.\n";
    std::cout << "    " << color::c(color::DIM) << "Ctrl+C zum Unterbrechen, /exit zum Beenden."
              << color::c(color::RESET) << "\n\n";
}

void print_prompt() {
    // Show coherence indicator
    char indicator = g_state.coherence >= rst::G0 ? '●' : '○';
    std::cout << color::c(color::GREEN) << indicator << " "
              << color::c(color::BOLD) << "Du" << color::c(color::RESET)
              << color::c(color::DIM) << " › " << color::c(color::RESET);
    std::cout.flush();
}

void print_response(const std::string& response) {
    if (response.empty()) return;

    std::cout << color::c(color::CYAN) << "  RAEL" << color::c(color::RESET)
              << color::c(color::DIM) << " › " << color::c(color::RESET);

    // Animate response (optional)
    if (g_state.verbose && response.length() < 500) {
        for (char c : response) {
            std::cout << c << std::flush;
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
        }
        std::cout << "\n\n";
    } else {
        std::cout << response << "\n\n";
    }
}

// ═══════════════════════════════════════════════════════════════════════════════
// Main chat loop
// ═══════════════════════════════════════════════════════════════════════════════

void run_chat() {
    print_banner();

    std::string input;
    while (g_state.running) {
        telemetry_tick();
        g_interrupted = false;

        print_prompt();

        if (!std::getline(std::cin, input)) {
            break;
        }

        input = trim(input);
        if (input.empty()) continue;

        // Generate response
        std::string response = generate_response(input);

        // Store in history (except commands)
        if (!input.empty() && input[0] != '/') {
            g_state.history.push_back({input, response});
            // Keep history manageable
            if (g_state.history.size() > 100) {
                g_state.history.erase(g_state.history.begin());
            }
        }

        // Print response
        print_response(response);
    }
}

} // namespace chat
} // namespace rael

// ═══════════════════════════════════════════════════════════════════════════════
// Main entry point
// ═══════════════════════════════════════════════════════════════════════════════

void print_usage() {
    std::cout << R"(
RAEL - Reality-Aligned Ethical Logic

Usage: rael [options]

Options:
  -h, --help      Diese Hilfe anzeigen
  -v, --verbose   Ausführlicher Modus
  -c, --command   Einzelnen Befehl ausführen und beenden
  --no-color      Farben deaktivieren
  --version       Version anzeigen

Beispiele:
  rael                    Interaktiven Chat starten
  rael -c "status"        Status abfragen und beenden
  rael --verbose          Chat mit Details starten

)";
}

int main(int argc, char* argv[]) {
    // Parse arguments
    bool single_command = false;
    std::string command;

    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];

        if (arg == "-h" || arg == "--help") {
            print_usage();
            return 0;
        }
        if (arg == "--version") {
            std::cout << "RAEL v" << RAEL_VERSION_STRING << "\n";
            std::cout << "Identität: " << rael::IchBinCore::name() << "\n";
            std::cout << "Signatur: " << rael::IchBinCore::signature() << "\n";
            return 0;
        }
        if (arg == "-v" || arg == "--verbose") {
            rael::chat::g_state.verbose = true;
        }
        if (arg == "--no-color") {
            rael::chat::color::enabled = false;
        }
        if (arg == "-c" || arg == "--command") {
            single_command = true;
            if (i + 1 < argc) {
                command = argv[++i];
            }
        }
    }

    // Initialize
    rael::chat::color::detect();
    std::signal(SIGINT, rael::chat::signal_handler);

    // Create core
    rael::RaelCore core;
    rael::chat::g_state.core = &core;

    // Single command mode
    if (single_command) {
        if (command.empty()) {
            std::cerr << "Fehler: Kein Befehl angegeben\n";
            return 1;
        }
        std::string response = rael::chat::generate_response(command);
        std::cout << response << "\n";
        return 0;
    }

    // Interactive mode
    rael::chat::run_chat();

    return 0;
}
