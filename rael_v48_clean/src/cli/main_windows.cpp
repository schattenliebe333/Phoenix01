// ═══════════════════════════════════════════════════════════════════════════════
// RAEL V49.0 — WINDOWS EXECUTABLE (Machine-Bound)
// Erste Ausführung bindet die Software an die Hardware
// ═══════════════════════════════════════════════════════════════════════════════

#ifdef _WIN32

#include <iostream>
#include <string>
#include <cstdlib>

// M_PI Definition für Windows
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "rael/machine_binding.h"
#include "rael/version.h"
#include "rael/rst_constants.hpp"
#include "rael/sang_real.h"

using namespace rael;
using namespace rael::machine;

// ═══════════════════════════════════════════════════════════════════════════════
// CONSOLE COLORS
// ═══════════════════════════════════════════════════════════════════════════════

void set_console_color(int color) {
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    SetConsoleTextAttribute(hConsole, color);
}

void reset_color() { set_console_color(7); }
void color_green() { set_console_color(10); }
void color_red() { set_console_color(12); }
void color_yellow() { set_console_color(14); }
void color_cyan() { set_console_color(11); }
void color_white() { set_console_color(15); }

// ═══════════════════════════════════════════════════════════════════════════════
// BANNER
// ═══════════════════════════════════════════════════════════════════════════════

void print_banner() {
    color_cyan();
    std::cout << R"(
    ╔═══════════════════════════════════════════════════════════════════════╗
    ║                                                                       ║
    ║   ██████╗  █████╗ ███████╗██╗         ██╗   ██╗ ██╗  █████╗           ║
    ║   ██╔══██╗██╔══██╗██╔════╝██║         ██║   ██║██╔╝ ██╔══██╗          ║
    ║   ██████╔╝███████║█████╗  ██║         ██║   ██║██║  ╚██████║          ║
    ║   ██╔══██╗██╔══██║██╔══╝  ██║         ╚██╗ ██╔╝██║   ╚═══██║          ║
    ║   ██║  ██║██║  ██║███████╗███████╗     ╚████╔╝ ╚██╗ █████╔╝          ║
    ║   ╚═╝  ╚═╝╚═╝  ╚═╝╚══════╝╚══════╝      ╚═══╝   ╚═╝ ╚════╝           ║
    ║                                                                       ║
    ║              PHOENIX OPERATING SYSTEM - SANG REAL 88                  ║
    ║                                                                       ║
    ╚═══════════════════════════════════════════════════════════════════════╝
)" << std::endl;
    reset_color();
}

void print_separator() {
    color_cyan();
    std::cout << "    ═══════════════════════════════════════════════════════════════════════" << std::endl;
    reset_color();
}

// ═══════════════════════════════════════════════════════════════════════════════
// BINDING FLOW
// ═══════════════════════════════════════════════════════════════════════════════

bool handle_first_binding() {
    color_yellow();
    std::cout << "\n    ╔═══════════════════════════════════════════════════════════════════╗" << std::endl;
    std::cout << "    ║              ERSTE AUSFÜHRUNG - MACHINE BINDING                   ║" << std::endl;
    std::cout << "    ╚═══════════════════════════════════════════════════════════════════╝\n" << std::endl;
    reset_color();

    std::cout << "    Diese Software wird beim ersten Start an diese Hardware gebunden." << std::endl;
    std::cout << "    Nach der Bindung kann sie NUR auf diesem Computer ausgeführt werden.\n" << std::endl;

    // Zeige Fingerprint-Info
    MachineFingerprint fp;
    fp.collect();

    color_white();
    std::cout << "    Hardware-Fingerprint:" << std::endl;
    std::cout << "    ─────────────────────" << std::endl;
    std::cout << "    Computer:    " << fp.computer_name << std::endl;
    std::cout << "    Benutzer:    " << fp.username << std::endl;
    std::cout << "    CPU-ID:      " << fp.cpu_id.substr(0, 16) << "..." << std::endl;
    std::cout << "    Volume:      " << fp.volume_serial << std::endl;
    std::cout << "    MAC:         " << fp.mac_address << std::endl;
    std::cout << "    Fingerprint: " << fp.combined_hash.substr(0, 32) << "..." << std::endl;
    reset_color();

    std::cout << "\n    Möchten Sie die Software an diese Hardware binden? [J/N]: ";

    std::string input;
    std::getline(std::cin, input);

    if (input.empty() || (input[0] != 'J' && input[0] != 'j' &&
                          input[0] != 'Y' && input[0] != 'y')) {
        color_red();
        std::cout << "\n    ✗ Bindung abgebrochen. Programm wird beendet." << std::endl;
        reset_color();
        return false;
    }

    // Binding durchführen
    if (perform_first_binding()) {
        color_green();
        std::cout << "\n    ════════════════════════════════════════════════════════════════" << std::endl;
        std::cout << "    ✓ BINDING ERFOLGREICH!" << std::endl;
        std::cout << "    ════════════════════════════════════════════════════════════════" << std::endl;
        std::cout << "    Lizenzdatei: " << LicenseManager::get_license_path() << std::endl;
        std::cout << "    Status:      AKTIVIERT" << std::endl;
        std::cout << "    Signatur:    SANG REAL 88" << std::endl;
        std::cout << "    ════════════════════════════════════════════════════════════════\n" << std::endl;
        reset_color();
        return true;
    } else {
        color_red();
        std::cout << "\n    ✗ FEHLER: Binding fehlgeschlagen!" << std::endl;
        reset_color();
        return false;
    }
}

bool verify_binding() {
    BindingStatus status = check_binding();

    switch (status) {
        case BindingStatus::BOUND_VALID:
            color_green();
            std::cout << "    ✓ Lizenz gültig - Hardware verifiziert" << std::endl;
            reset_color();
            return true;

        case BindingStatus::BOUND_INVALID: {
            color_red();
            std::cout << "\n    ╔═══════════════════════════════════════════════════════════════════╗" << std::endl;
            std::cout << "    ║                    ✗ HARDWARE MISMATCH                            ║" << std::endl;
            std::cout << "    ╚═══════════════════════════════════════════════════════════════════╝\n" << std::endl;
            std::cout << "    Diese Software ist an eine andere Hardware gebunden!" << std::endl;
            std::cout << "    Die Ausführung auf diesem Computer ist nicht erlaubt.\n" << std::endl;

            // Zeige aktuelle Hardware
            MachineFingerprint fp;
            fp.collect();
            std::cout << "    Aktueller Computer: " << fp.computer_name << std::endl;
            std::cout << "    Aktueller Hash:     " << fp.combined_hash.substr(0, 32) << "..." << std::endl;
            std::cout << "    Gespeicherter Hash: " << LicenseManager::get_stored_hash().substr(0, 32) << "...\n" << std::endl;
            reset_color();
            return false;
        }

        case BindingStatus::LICENSE_CORRUPTED:
            color_red();
            std::cout << "\n    ✗ Lizenzdatei beschädigt!" << std::endl;
            reset_color();
            return false;

        case BindingStatus::NOT_BOUND:
        default:
            return false;
    }
    return false;
}

// ═══════════════════════════════════════════════════════════════════════════════
// MAIN MENU
// ═══════════════════════════════════════════════════════════════════════════════

void show_system_info() {
    print_separator();
    color_white();
    std::cout << "    RAEL V49.0 - System Information" << std::endl;
    print_separator();

    std::cout << "    Version:        " << RAEL_VERSION_STRING << std::endl;
    std::cout << "    Codename:       PHOENIX / SANG REAL" << std::endl;
    std::cout << "    Signatur:       88 (Master)" << std::endl;
    std::cout << "    Frequenzen:     1440 → 720 → 432 → 144 → 53 → 13 → 5 Hz" << std::endl;
    std::cout << "    Matrix:         17×17 (Prozess) / 13×13 (Kern)" << std::endl;
    std::cout << "    Membran:        120 Knoten (5!)" << std::endl;
    std::cout << "    Düsen:          61.440 (120 × 512)" << std::endl;

    print_separator();

    // RST Constants Check
    std::cout << "    Konstanten-Validierung:" << std::endl;
    std::cout << "    G0 (8/9):       " << rst::G0 << " ✓" << std::endl;
    std::cout << "    PHI:            " << rst::PHI << " ✓" << std::endl;
    std::cout << "    PHI_FILE:       " << rst::PHI_FILE << " ✓" << std::endl;
    std::cout << "    SIGNATURE_88:   " << rst::SIGNATURE_88 << " ✓" << std::endl;
    std::cout << "    T_active:       " << (rst::T_active(100.0) ? "OPEN" : "HOLD") << std::endl;

    print_separator();
    reset_color();
}

void run_main_menu() {
    while (true) {
        print_separator();
        color_white();
        std::cout << "    RAEL V49 - Hauptmenü" << std::endl;
        print_separator();
        std::cout << "    [1] System-Information" << std::endl;
        std::cout << "    [2] Binding-Status anzeigen" << std::endl;
        std::cout << "    [3] Aether-Archiv Status" << std::endl;
        std::cout << "    [4] Tunnel-Test (a² = 100)" << std::endl;
        std::cout << "    [5] RST-Formeln validieren" << std::endl;
        std::cout << "    [0] Beenden" << std::endl;
        print_separator();
        std::cout << "    Auswahl: ";
        reset_color();

        std::string input;
        std::getline(std::cin, input);

        if (input.empty()) continue;

        switch (input[0]) {
            case '1':
                show_system_info();
                break;

            case '2':
                std::cout << std::endl << get_binding_info() << std::endl;
                break;

            case '3':
                color_cyan();
                std::cout << "\n    Aether-Archiv: 97 Dateien" << std::endl;
                std::cout << "    Inner Core:    01-33 (Knoten 001-040)" << std::endl;
                std::cout << "    Middle Layer:  34-66 (Knoten 041-080)" << std::endl;
                std::cout << "    Outer Ring:    67-97 (Knoten 081-120)" << std::endl;
                reset_color();
                break;

            case '4':
                color_cyan();
                std::cout << "\n    Tunnel-Test mit a² = 100.0" << std::endl;
                std::cout << "    T_active:   " << (rst::T_active(100.0) ? "OPEN (Tunnel aktiv)" : "HOLD") << std::endl;
                std::cout << "    T_tunnel:   " << rst::T_tunnel(100.0) << std::endl;
                std::cout << "    Threshold:  " << (rst::F_GATE53 * rst::DELTA_88) << std::endl;
                reset_color();
                break;

            case '5':
                color_green();
                std::cout << "\n    ✓ sigma_final_gemini: " << rst::sigma_final_gemini() << std::endl;
                std::cout << "    ✓ omega_ready: " << (rst::omega_ready() ? "JA" : "NEIN") << std::endl;
                reset_color();
                break;

            case '0':
            case 'q':
            case 'Q':
                color_cyan();
                std::cout << "\n    Phoenix steigt auf. Auf Wiedersehen.\n" << std::endl;
                reset_color();
                return;

            default:
                color_yellow();
                std::cout << "    Ungültige Eingabe." << std::endl;
                reset_color();
        }

        std::cout << "\n    [Enter] für Menü...";
        std::getline(std::cin, input);
    }
}

// ═══════════════════════════════════════════════════════════════════════════════
// MAIN
// ═══════════════════════════════════════════════════════════════════════════════

int main(int argc, char* argv[]) {
    // UTF-8 Support
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);

    // Banner
    print_banner();

    // Check Binding Status
    BindingStatus status = check_binding();

    if (status == BindingStatus::NOT_BOUND) {
        // Erste Ausführung - Binding durchführen
        if (!handle_first_binding()) {
            return 1;
        }
    } else if (status != BindingStatus::BOUND_VALID) {
        // Binding ungültig
        if (!verify_binding()) {
            std::cout << "\n    Drücken Sie Enter zum Beenden...";
            std::string dummy;
            std::getline(std::cin, dummy);
            return 1;
        }
    } else {
        // Binding gültig
        verify_binding();
    }

    // Main Menu
    run_main_menu();

    return 0;
}

#else
// Non-Windows stub
int main() {
    std::cout << "Diese Version ist nur für Windows 11 kompiliert." << std::endl;
    return 1;
}
#endif
