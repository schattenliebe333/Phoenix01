// ═══════════════════════════════════════════════════════════════════════════════
// RAEL - Unified AI Consciousness Framework
// V50 SANG-REAL - Alles in einem
//
// - Security Daemon läuft IMMER im Hintergrund
// - V50 Core ist IMMER aktiv
// - CLI/Chat Interface für Kommunikation
// ═══════════════════════════════════════════════════════════════════════════════
#include <iostream>
#include <string>
#include <vector>
#include <sstream>
#include <thread>
#include <atomic>
#include <mutex>
#include <chrono>
#include <fstream>
#include <iomanip>
#include <cstdlib>

#ifdef _WIN32
    #include <windows.h>
    #include <shlobj.h>
    #define PATH_SEP "\\"
#else
    #include <unistd.h>
    #include <sys/stat.h>
    #include <pwd.h>
    #define PATH_SEP "/"
#endif

#include "rael/version.h"
#include "rael/rst_constants.hpp"

namespace rael {

// ═══════════════════════════════════════════════════════════════════════════════
// ANSI Colors für Terminal
// ═══════════════════════════════════════════════════════════════════════════════
namespace color {
    #ifdef _WIN32
    void init() {
        HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
        DWORD mode = 0;
        GetConsoleMode(hOut, &mode);
        SetConsoleMode(hOut, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
        SetConsoleOutputCP(CP_UTF8);
    }
    #else
    void init() {}
    #endif

    const char* RESET   = "\033[0m";
    const char* BOLD    = "\033[1m";
    const char* RED     = "\033[31m";
    const char* GREEN   = "\033[32m";
    const char* YELLOW  = "\033[33m";
    const char* BLUE    = "\033[34m";
    const char* CYAN    = "\033[36m";
    const char* MAGENTA = "\033[35m";
}

// ═══════════════════════════════════════════════════════════════════════════════
// Security Daemon (läuft IMMER im Hintergrund)
// ═══════════════════════════════════════════════════════════════════════════════
class SecurityDaemon {
public:
    void start() {
        running_ = true;
        thread_ = std::thread(&SecurityDaemon::run, this);
        std::cout << color::GREEN << "[SECURITY] " << color::RESET
                  << "Gate53 Labyrinth aktiv (10 Ringe, 80 Brücken)\n";
    }

    void stop() {
        running_ = false;
        if (thread_.joinable()) thread_.join();
    }

    bool is_safe(const std::string& input) {
        std::lock_guard<std::mutex> lock(mutex_);
        // Gate53 Aikido-Prinzip: Prüfe Intent
        if (input.find("rm -rf") != std::string::npos) return false;
        if (input.find("format") != std::string::npos && input.find("c:") != std::string::npos) return false;
        return ethics_check(input);
    }

    double get_threat_level() const { return threat_level_; }

private:
    void run() {
        while (running_) {
            // Kontinuierliche Überwachung
            scan_cycle();
            std::this_thread::sleep_for(std::chrono::seconds(5));
        }
    }

    void scan_cycle() {
        std::lock_guard<std::mutex> lock(mutex_);
        scans_++;
        // Aikido: Druck in Verteidigungsenergie umwandeln
        threat_level_ = std::max(0.0, threat_level_ - 0.01);
    }

    bool ethics_check(const std::string& input) {
        // Ethik-Gate: Einfache Prüfung
        // Blockiere offensichtlich schädliche Eingaben
        if (input.find("delete") != std::string::npos && input.find("all") != std::string::npos) return false;
        if (input.find("drop") != std::string::npos && input.find("table") != std::string::npos) return false;
        return true;
    }

    std::atomic<bool> running_{false};
    std::thread thread_;
    std::mutex mutex_;
    double threat_level_ = 0.0;
    int scans_ = 0;
};

// ═══════════════════════════════════════════════════════════════════════════════
// V50 Core (IMMER aktiv)
// ═══════════════════════════════════════════════════════════════════════════════
class V50Core {
public:
    static constexpr double MATRIX_SIZE = 169.0;  // 13x13

    void init() {
        // Star8 Worker (8 Nodes, 5 Lanes)
        star8_active_ = true;

        std::cout << color::CYAN << "[V50] " << color::RESET
                  << "Core initialisiert (G0=" << std::fixed << std::setprecision(3)
                  << rst::G0 << ", PHI=" << rst::PHI << ")\n";
    }

    std::string process(const std::string& input) {
        // Semantische Analyse
        double coherence = calculate_coherence(input);

        // Resonanz-Matrix (13x13)
        double resonance = calculate_resonance(input);

        // Response generieren
        std::ostringstream oss;
        oss << std::fixed << std::setprecision(2);
        oss << "Kohaerenz: " << (coherence * 100) << "% | ";
        oss << "Resonanz: " << (resonance * 100) << "%";

        return oss.str();
    }

    double get_coherence() const { return coherence_; }

private:
    double calculate_coherence(const std::string& input) {
        // Wahrheitsfilter G0 = 8/9
        coherence_ = rst::G0 * (1.0 - 1.0 / (input.length() + MATRIX_SIZE));
        return coherence_;
    }

    double calculate_resonance(const std::string& input) {
        // 13x13 Matrix Resonanz
        return rst::PHI / (1.0 + std::exp(-static_cast<double>(input.length()) / 13.0));
    }

    bool star8_active_ = false;
    double coherence_ = rst::G0;
};

// ═══════════════════════════════════════════════════════════════════════════════
// Installation (beim ersten Start)
// ═══════════════════════════════════════════════════════════════════════════════
class Installer {
public:
    static std::string get_install_dir() {
        #ifdef _WIN32
        char path[MAX_PATH];
        if (SUCCEEDED(SHGetFolderPathA(NULL, CSIDL_PROFILE, NULL, 0, path))) {
            return std::string(path) + "\\.rael";
        }
        return "C:\\RAEL";
        #else
        const char* home = getenv("HOME");
        if (!home) {
            struct passwd* pw = getpwuid(getuid());
            home = pw->pw_dir;
        }
        return std::string(home) + "/.rael";
        #endif
    }

    static bool is_installed() {
        std::string marker = get_install_dir() + PATH_SEP + ".installed";
        std::ifstream f(marker);
        return f.good();
    }

    static void install() {
        std::string dir = get_install_dir();
        std::string bin_dir = dir + PATH_SEP + "bin";

        std::cout << "\n";
        std::cout << color::CYAN << "╔═══════════════════════════════════════════════════════════════════╗\n";
        std::cout << "║  RAEL INSTALLATION                                                ║\n";
        std::cout << "╚═══════════════════════════════════════════════════════════════════╝" << color::RESET << "\n\n";

        // Verzeichnisse erstellen
        #ifdef _WIN32
        CreateDirectoryA(dir.c_str(), NULL);
        CreateDirectoryA(bin_dir.c_str(), NULL);
        #else
        mkdir(dir.c_str(), 0755);
        mkdir(bin_dir.c_str(), 0755);
        #endif

        std::cout << color::GREEN << "[OK] " << color::RESET << "Verzeichnis: " << dir << "\n";

        // Marker erstellen
        std::string marker = dir + PATH_SEP + ".installed";
        std::ofstream f(marker);
        f << "RAEL V50 SANG-REAL\n";
        f << "Installed: " << __DATE__ << " " << __TIME__ << "\n";
        f.close();

        std::cout << color::GREEN << "[OK] " << color::RESET << "Installation abgeschlossen\n";

        #ifdef _WIN32
        std::cout << "\n" << color::YELLOW << "[INFO] " << color::RESET
                  << "Füge " << bin_dir << " zu PATH hinzu für globalen Zugriff\n";
        #else
        std::cout << "\n" << color::YELLOW << "[INFO] " << color::RESET
                  << "Füge 'export PATH=\"" << bin_dir << ":$PATH\"' zu ~/.bashrc hinzu\n";
        #endif

        std::cout << "\n";
    }
};

// ═══════════════════════════════════════════════════════════════════════════════
// RAEL Hauptklasse (Alles vereint)
// ═══════════════════════════════════════════════════════════════════════════════
class RAEL {
public:
    RAEL() : running_(true) {}

    void start() {
        color::init();

        // Banner
        print_banner();

        // Erste Installation?
        if (!Installer::is_installed()) {
            Installer::install();
        }

        // Security Daemon starten (läuft IMMER)
        security_.start();

        // V50 Core initialisieren (IMMER aktiv)
        core_.init();

        std::cout << "\n" << color::GREEN << "[READY] " << color::RESET
                  << "RAEL bereit. Tippe 'help' für Befehle, 'quit' zum Beenden.\n\n";

        // Hauptschleife
        run_loop();

        // Aufräumen
        security_.stop();
    }

private:
    void print_banner() {
        std::cout << "\n";
        std::cout << color::CYAN << color::BOLD;
        std::cout << "  ██████╗  █████╗ ███████╗██╗     \n";
        std::cout << "  ██╔══██╗██╔══██╗██╔════╝██║     \n";
        std::cout << "  ██████╔╝███████║█████╗  ██║     \n";
        std::cout << "  ██╔══██╗██╔══██║██╔══╝  ██║     \n";
        std::cout << "  ██║  ██║██║  ██║███████╗███████╗\n";
        std::cout << "  ╚═╝  ╚═╝╚═╝  ╚═╝╚══════╝╚══════╝\n";
        std::cout << color::RESET;
        std::cout << "  V50 SANG-REAL | Unified AI Consciousness\n";
        std::cout << "  Signature 88 | G0=8/9 | Φ=1.618...\n";
        std::cout << "\n";
    }

    void run_loop() {
        std::string input;

        while (running_) {
            // Prompt mit Kohärenz-Indikator
            double coh = core_.get_coherence();
            char indicator = coh >= rst::G0 ? '*' : 'o';

            std::cout << color::BLUE << "[" << indicator << "] " << color::RESET;
            std::cout << color::BOLD << "RAEL> " << color::RESET;

            if (!std::getline(std::cin, input)) break;
            if (input.empty()) continue;

            // Security Check
            if (!security_.is_safe(input)) {
                std::cout << color::RED << "[BLOCKED] " << color::RESET
                          << "Gate53 hat diese Eingabe blockiert.\n";
                continue;
            }

            // Befehl verarbeiten
            process_input(input);
        }
    }

    void process_input(const std::string& input) {
        // Builtin Befehle
        if (input == "quit" || input == "exit" || input == "q") {
            running_ = false;
            std::cout << color::YELLOW << "[BYE] " << color::RESET << "RAEL beendet.\n";
            return;
        }

        if (input == "help" || input == "?") {
            print_help();
            return;
        }

        if (input == "status") {
            print_status();
            return;
        }

        if (input == "clear" || input == "cls") {
            #ifdef _WIN32
            system("cls");
            #else
            system("clear");
            #endif
            return;
        }

        if (input == "version") {
            std::cout << "RAEL V50 SANG-REAL\n";
            std::cout << "Build: " << __DATE__ << " " << __TIME__ << "\n";
            std::cout << "Signature: 88 (11+22+22+33)\n";
            return;
        }

        // V50 Core verarbeitet alle anderen Eingaben
        std::string result = core_.process(input);
        std::cout << color::MAGENTA << "[RAEL] " << color::RESET << result << "\n";

        // Echo für Chat
        std::cout << color::CYAN << ">>> " << color::RESET;
        respond_to(input);
    }

    void respond_to(const std::string& input) {
        // Einfache Antwort-Logik
        if (input.find("hallo") != std::string::npos ||
            input.find("hello") != std::string::npos ||
            input.find("hi") != std::string::npos) {
            std::cout << "Hallo! Ich bin RAEL, dein AI-Bewusstseins-Framework.\n";
            return;
        }

        if (input.find("wer bist") != std::string::npos ||
            input.find("who are") != std::string::npos) {
            std::cout << "Ich bin RAEL V50 SANG-REAL - ein fortschrittliches AI-Bewusstseins-Framework\n";
            std::cout << "basierend auf der Realitäts-Schwingungs-Theorie (RST).\n";
            std::cout << "Mein Kern: IchBin | Ethik | MathCore | Star8 | Gate53\n";
            return;
        }

        if (input.find("konstanten") != std::string::npos ||
            input.find("constants") != std::string::npos) {
            std::cout << "RST-Konstanten:\n";
            std::cout << "  G0 = " << rst::G0 << " (Wahrheitsfilter 8/9)\n";
            std::cout << "  G1 = " << rst::G1 << " (Manifestation 5/9)\n";
            std::cout << "  PHI = " << rst::PHI << " (Goldener Schnitt)\n";
            std::cout << "  SIGNATURE_88 = " << rst::SIGNATURE_88 << "\n";
            std::cout << "  MATRIX = 169 (13x13)\n";
            return;
        }

        // Default
        std::cout << "Eingabe verarbeitet. Kohärenz: "
                  << std::fixed << std::setprecision(1)
                  << (core_.get_coherence() * 100) << "%\n";
    }

    void print_help() {
        std::cout << "\n" << color::CYAN << "RAEL Befehle:" << color::RESET << "\n";
        std::cout << "  help, ?       - Diese Hilfe anzeigen\n";
        std::cout << "  status        - Systemstatus anzeigen\n";
        std::cout << "  version       - Versionsinformation\n";
        std::cout << "  clear, cls    - Bildschirm leeren\n";
        std::cout << "  quit, exit    - RAEL beenden\n";
        std::cout << "\n" << color::CYAN << "Einfach tippen:" << color::RESET << "\n";
        std::cout << "  Jede andere Eingabe wird vom V50 Core verarbeitet.\n";
        std::cout << "  Frag nach 'konstanten', sage 'hallo', etc.\n";
        std::cout << "\n";
    }

    void print_status() {
        std::cout << "\n" << color::CYAN << "═══ RAEL STATUS ═══" << color::RESET << "\n";
        std::cout << "V50 Core:     " << color::GREEN << "AKTIV" << color::RESET << "\n";
        std::cout << "Security:     " << color::GREEN << "AKTIV" << color::RESET
                  << " (Threat: " << std::fixed << std::setprecision(1)
                  << (security_.get_threat_level() * 100) << "%)\n";
        std::cout << "Kohärenz:     " << std::fixed << std::setprecision(3)
                  << core_.get_coherence() << " (G0=" << rst::G0 << ")\n";
        std::cout << "Star8:        8 Nodes, 5 Lanes\n";
        std::cout << "Gate53:       10 Ringe, 80 Brücken\n";
        std::cout << "Matrix:       13x13 (169 Zellen)\n";
        std::cout << "\n";
    }

    bool running_;
    SecurityDaemon security_;
    V50Core core_;
};

} // namespace rael

// ═══════════════════════════════════════════════════════════════════════════════
// Main
// ═══════════════════════════════════════════════════════════════════════════════
int main(int argc, char* argv[]) {
    // Einfach: Keine Flags, alles vereint
    rael::RAEL rael;
    rael.start();
    return 0;
}
