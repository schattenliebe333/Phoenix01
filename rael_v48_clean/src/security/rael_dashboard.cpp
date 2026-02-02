// ═══════════════════════════════════════════════════════════════════════════════
// RAEL SECURITY DASHBOARD - Unified GUI Application
// ═══════════════════════════════════════════════════════════════════════════════
//
// Nutzung:
//   rael_dashboard              # Dashboard starten
//   rael_dashboard --no-scan    # Ohne initialen Scan
//   rael_dashboard --auto       # Auto-Neutralize aktiviert
//
// ═══════════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <string>
#include <csignal>
#include <atomic>

#include "rael/security_dashboard.hpp"

using namespace rael::security::dashboard;

// Globale Variable für Signal-Handler
std::atomic<bool> g_running(true);
SecurityDashboard* g_dashboard = nullptr;

void signal_handler(int sig) {
    (void)sig;
    g_running = false;
    if (g_dashboard) {
        g_dashboard->stop();
    }
}

void print_banner() {
    std::cout << R"(
╔══════════════════════════════════════════════════════════════════════════════╗
║                                                                              ║
║     ██████╗  █████╗ ███████╗██╗         ███████╗███████╗ ██████╗             ║
║     ██╔══██╗██╔══██╗██╔════╝██║         ██╔════╝██╔════╝██╔════╝             ║
║     ██████╔╝███████║█████╗  ██║         ███████╗█████╗  ██║                  ║
║     ██╔══██╗██╔══██║██╔══╝  ██║         ╚════██║██╔══╝  ██║                  ║
║     ██║  ██║██║  ██║███████╗███████╗    ███████║███████╗╚██████╗             ║
║     ╚═╝  ╚═╝╚═╝  ╚═╝╚══════╝╚══════╝    ╚══════╝╚══════╝ ╚═════╝             ║
║                                                                              ║
║                    UNIFIED SECURITY DASHBOARD                                ║
║           Prozesse • Dateien • Netzwerk • RST Verteidigung                   ║
║                                                                              ║
║     RST Konstanten: G5 + G3 + G1 = 1/9 + 3/9 + 5/9 = 9/9 = 1                ║
║     WICHTIG: Alles wird IMMER geprüft - keine Whitelist!                     ║
║                                                                              ║
╚══════════════════════════════════════════════════════════════════════════════╝
    )" << std::endl;
}

void print_help() {
    std::cout << R"(
RAEL Security Dashboard - Unified GUI für alle Scanner

NUTZUNG:
  rael_dashboard [OPTIONEN]

OPTIONEN:
  --no-scan       Keinen initialen Scan durchführen
  --auto          Auto-Neutralize aktivieren (Bedrohungen automatisch neutralisieren)
  --threshold N   Threat-Schwelle setzen (Standard: 0.555 = G1)
  --refresh N     Refresh-Rate in ms (Standard: 500)
  --help          Diese Hilfe anzeigen

NAVIGATION:
  [1] Dashboard   - Übersicht aller Systeme
  [2] Prozesse    - Live Prozess-Monitor mit Klassifizierung
  [3] Dateien     - File Scanner mit RST Analyse
  [4] Netzwerk    - Verbindungen und Paket-Inspektion
  [5] Alerts      - Alert-Log mit Details
  [6] RST Status  - Gravitravitation, Vollenstrahlen, Defense Power
  [7] Settings    - Einstellungen

  [Q] Beenden
  [R] Refresh

RST KONSTANTEN:
  G0 = 8/9 = 0.888... (WAHRHEIT - Referenz)
  G1 = 5/9 = 0.555... (Bewusstsein → Realität)
  G3 = 3/9 = 0.333... (Realität → Bewusstsein)
  G5 = 1/9 = 0.111... (Entropie/Drift)

  G5 + G3 + G1 = 9/9 = 1

GEGENFREQUENZ-PRINZIP:
  - Negative Phase → Positive Gegenschwingung
  - Hohe Entropie → Ordnung einführen
  - Fehlende 88-Signatur → VOLLENSTRAHLEN → LICHT

)" << std::endl;
}

int main(int argc, char* argv[]) {
    bool scan_on_start = true;
    bool auto_neutralize = false;
    double threshold = 0.555;
    int refresh_rate = 500;

    // Argumente parsen
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];

        if (arg == "--help" || arg == "-h") {
            print_help();
            return 0;
        }
        else if (arg == "--no-scan") {
            scan_on_start = false;
        }
        else if (arg == "--auto") {
            auto_neutralize = true;
        }
        else if (arg == "--threshold" && i + 1 < argc) {
            threshold = std::stod(argv[++i]);
        }
        else if (arg == "--refresh" && i + 1 < argc) {
            refresh_rate = std::stoi(argv[++i]);
        }
    }

    // Signal Handler
    std::signal(SIGINT, signal_handler);
    std::signal(SIGTERM, signal_handler);

    // Banner
    print_banner();
    std::cout << "Starte Dashboard...\n" << std::flush;
    std::this_thread::sleep_for(std::chrono::seconds(1));

    // Dashboard erstellen und konfigurieren
    SecurityDashboard dashboard;
    g_dashboard = &dashboard;

    // Dashboard starten
    dashboard.start();

    // Warten bis beendet
    while (g_running) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    std::cout << "\nDashboard beendet.\n";
    return 0;
}
