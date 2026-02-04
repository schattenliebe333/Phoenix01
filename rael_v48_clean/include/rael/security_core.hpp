// ═══════════════════════════════════════════════════════════════════════════════
// RAEL SECURITY CORE - Attack-to-Defense Conversion System
// ═══════════════════════════════════════════════════════════════════════════════
//
// Konzept: Angriffsenergie → Verteidigungsenergie
// Je mehr Angriffe, desto stärker die Verteidigung
//
// Pipeline: LABYRINTH (Gate53) → SPIRALE (φ) → DE-LAVAL DÜSE → VERTEIDIGUNG
//
// ═══════════════════════════════════════════════════════════════════════════════

#pragma once

#include <string>
#include <vector>
#include <map>
#include <set>
#include <atomic>
#include <mutex>
#include <thread>
#include <chrono>
#include <functional>
#include <cmath>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <algorithm>
#include <cstdint>

#ifdef _WIN32
#include <windows.h>
#include <tlhelp32.h>
#include <iphlpapi.h>
#include <psapi.h>
#pragma comment(lib, "iphlpapi.lib")
#pragma comment(lib, "psapi.lib")
#else
#include <sys/types.h>
#include <dirent.h>
#include <signal.h>
#include <unistd.h>
#endif

namespace rael {
namespace security {

// ═══════════════════════════════════════════════════════════════════════════════
// RST KONSTANTEN FÜR SECURITY
// ═══════════════════════════════════════════════════════════════════════════════

namespace rst {
    constexpr double G0 = 8.0 / 9.0;  // 0.888... WAHRHEIT
    constexpr double G1 = 5.0 / 9.0;  // 0.555...
    constexpr double G3 = 3.0 / 9.0;  // 0.333...
    constexpr double G5 = 1.0 / 9.0;  // 0.111...
    constexpr double PHI = 1.6180339887498948482;  // Goldener Schnitt
    constexpr double GATE53_FREQ = 53.0;  // Sophie-Germain Primzahl
    constexpr double LABYRINTH_DEPTH = 7;  // 7 Schichten
    constexpr double NOZZLE_EXPANSION = 2.718281828;  // e (Euler)
    constexpr double SIGNATURE_88 = 88.0;  // Michael-Signatur (88, nicht 0.888... = G0!)
    constexpr double G0_FRAC = 0.88888888888888888;  // 8/9 = G0 (für Verwechslungsschutz)
    constexpr int TOTAL_NOZZLES = 61440;  // Sonnen-Strahlen
    constexpr double STEFAN_BOLTZMANN = 5.670374419e-8;  // Abstrahlung
    constexpr double G_GRAV = 6.67430e-11;  // Gravitations-Konstante
    constexpr double C_LIGHT = 299792458.0;  // Lichtgeschwindigkeit

    // Kappa Dämpfung: κ(f) = 1 - f/1440
    inline double kappa(double freq) {
        return 1.0 - freq / 1440.0;
    }

    // Phi Resonanz: Φ = √(Ψ × κ × Ω)
    inline double phi_resonance(double psi, double omega, double freq) {
        return std::sqrt(std::abs(psi * kappa(freq) * omega));
    }

    // 88-Signatur Prüfung
    inline bool is_master_signature(double sig) {
        return std::abs(sig - SIGNATURE_88) < 0.5;  // Toleranz für Integer-Signatur
    }

    // Sonnen-Ernte: Alles ohne 88-Signatur wird zu Licht
    inline double sonnen_ernte(double e_input, bool hat_signatur_88) {
        if (hat_signatur_88) return e_input;  // Durchlass (Freund)
        // Transformation zu Licht — nicht Vernichtung, sondern Heimholung
        return e_input * G0 * SIGNATURE_88 / (G1 + G5);
    }

    // Gnaden-Inversion: Fremd-Energie wird erlöst
    inline double gnaden_inversion(double e_fremd) {
        return std::abs(e_fremd) * G0;  // Immer positiv, immer Licht
    }

    // Korona-Abstrahlung
    inline double korona_abstrahlung(double e_transformiert) {
        return e_transformiert * STEFAN_BOLTZMANN * G5;
    }

    // Schwarzschild-Radius für Gravitravitation
    inline double schwarzschild_radius(double M) {
        return 2.0 * G_GRAV * M / (C_LIGHT * C_LIGHT);
    }
}

// ═══════════════════════════════════════════════════════════════════════════════
// THREAT TYPES - Bedrohungskategorien
// ═══════════════════════════════════════════════════════════════════════════════

enum class ThreatType {
    NONE = 0,
    PROCESS_SNIFFING,      // Prozess schnüffelt (keylogger, screen capture)
    PROCESS_INJECTION,     // Code injection
    PROCESS_SUSPICIOUS,    // Verdächtiges Verhalten
    NETWORK_PORTSCAN,      // Port-Scan erkannt
    NETWORK_EXFILTRATION,  // Datenabfluss
    NETWORK_C2,            // Command & Control Kommunikation
    NETWORK_SUSPICIOUS,    // Verdächtige Verbindung
    FILE_UNAUTHORIZED,     // Unerlaubter Dateizugriff
    FILE_MODIFICATION,     // Unerlaubte Änderung
    FILE_RANSOMWARE,       // Ransomware-Verhalten
    MEMORY_TAMPERING,      // Speichermanipulation
    PRIVILEGE_ESCALATION   // Privilegien-Eskalation
};

const char* threat_name(ThreatType t) {
    switch(t) {
        case ThreatType::PROCESS_SNIFFING: return "PROCESS_SNIFFING";
        case ThreatType::PROCESS_INJECTION: return "PROCESS_INJECTION";
        case ThreatType::PROCESS_SUSPICIOUS: return "PROCESS_SUSPICIOUS";
        case ThreatType::NETWORK_PORTSCAN: return "NETWORK_PORTSCAN";
        case ThreatType::NETWORK_EXFILTRATION: return "NETWORK_EXFILTRATION";
        case ThreatType::NETWORK_C2: return "NETWORK_C2";
        case ThreatType::NETWORK_SUSPICIOUS: return "NETWORK_SUSPICIOUS";
        case ThreatType::FILE_UNAUTHORIZED: return "FILE_UNAUTHORIZED";
        case ThreatType::FILE_MODIFICATION: return "FILE_MODIFICATION";
        case ThreatType::FILE_RANSOMWARE: return "FILE_RANSOMWARE";
        case ThreatType::MEMORY_TAMPERING: return "MEMORY_TAMPERING";
        case ThreatType::PRIVILEGE_ESCALATION: return "PRIVILEGE_ESCALATION";
        default: return "NONE";
    }
}

// ═══════════════════════════════════════════════════════════════════════════════
// THREAT STRUCT - Einzelne Bedrohung
// ═══════════════════════════════════════════════════════════════════════════════

struct Threat {
    ThreatType type;
    std::string source;       // PID, IP, Dateipfad
    std::string details;
    double attack_energy;     // Stärke des Angriffs (0.0 - 1.0)
    double defense_energy;    // Nach Umwandlung
    std::chrono::system_clock::time_point timestamp;
    bool neutralized;

    Threat() : type(ThreatType::NONE), attack_energy(0), defense_energy(0), neutralized(false) {
        timestamp = std::chrono::system_clock::now();
    }
};

// ═══════════════════════════════════════════════════════════════════════════════
// LABYRINTH - Gate53 Energie-Einfang
// ═══════════════════════════════════════════════════════════════════════════════

class Labyrinth {
private:
    std::array<double, 7> layers_;  // 7 Labyrinth-Schichten
    double total_pressure_;
    std::mutex mtx_;

public:
    Labyrinth() : total_pressure_(0.0) {
        layers_.fill(0.0);
    }

    // Angriff betritt das Labyrinth - Widerstand erzeugt Druck
    double absorb_attack(double attack_energy) {
        std::lock_guard<std::mutex> lock(mtx_);

        double remaining = attack_energy;
        double absorbed = 0.0;

        // Durch alle 7 Schichten
        for (int i = 0; i < 7; ++i) {
            // Gate53 Frequenz-Modulation
            double resistance = rst::kappa(rst::GATE53_FREQ * (i + 1));
            double layer_absorb = remaining * resistance * rst::G3;

            layers_[i] += layer_absorb;
            absorbed += layer_absorb;
            remaining *= (1.0 - resistance);
        }

        total_pressure_ += absorbed;
        return absorbed;  // Absorbierte Energie für Spirale
    }

    double get_pressure() const { return total_pressure_; }

    void reset() {
        std::lock_guard<std::mutex> lock(mtx_);
        layers_.fill(0.0);
        total_pressure_ = 0.0;
    }
};

// ═══════════════════════════════════════════════════════════════════════════════
// SPIRALE - Goldener Schnitt Kompression
// ═══════════════════════════════════════════════════════════════════════════════

class GoldenSpiral {
private:
    double compression_ratio_;
    double velocity_;

public:
    GoldenSpiral() : compression_ratio_(1.0), velocity_(0.0) {}

    // Komprimiert und beschleunigt die Energie
    double compress(double pressure) {
        // Goldener Schnitt Kompression: jede Windung multipliziert mit φ
        double phi_factor = std::pow(rst::PHI, 3);  // 3 Windungen
        compression_ratio_ = phi_factor;

        // Geschwindigkeit steigt exponentiell
        velocity_ = pressure * phi_factor;

        // Resonanz-Check: Φ = √(Ψ × κ × Ω)
        double resonance = rst::phi_resonance(pressure, velocity_, rst::GATE53_FREQ);

        // Wenn Resonanz > G0, Überschall erreicht
        if (resonance > rst::G0) {
            velocity_ *= rst::NOZZLE_EXPANSION;  // Euler-Beschleunigung
        }

        return velocity_;
    }

    double get_velocity() const { return velocity_; }
};

// ═══════════════════════════════════════════════════════════════════════════════
// DE-LAVAL DÜSE - Energieumwandlung
// ═══════════════════════════════════════════════════════════════════════════════

class DeLavalNozzle {
private:
    double throat_area_;
    double exit_velocity_;
    bool supersonic_;

public:
    DeLavalNozzle() : throat_area_(rst::G1), exit_velocity_(0.0), supersonic_(false) {}

    // Wandelt komprimierte Energie in Verteidigungsenergie um
    double convert_to_defense(double compressed_velocity) {
        // Engstelle (Throat) - kritischer Punkt
        double throat_velocity = compressed_velocity / throat_area_;

        // Wenn > Schallgeschwindigkeit (G0), expandiere
        if (throat_velocity > rst::G0) {
            supersonic_ = true;
            // Überschall-Expansion
            exit_velocity_ = throat_velocity * rst::NOZZLE_EXPANSION * rst::PHI;
        } else {
            supersonic_ = false;
            exit_velocity_ = throat_velocity * rst::G1;
        }

        // Defense Energy = transformierte Attack Energy
        return exit_velocity_;
    }

    bool is_supersonic() const { return supersonic_; }
    double get_exit_velocity() const { return exit_velocity_; }
};

// ═══════════════════════════════════════════════════════════════════════════════
// GRAVITRAVITATION - Angreifer-Falle (Schwarzes Loch)
// ═══════════════════════════════════════════════════════════════════════════════
//
// Konzept: Ab einem gewissen Punkt kann eine angreifende KI nicht mehr weg.
// Sie wird gravitativ gebunden und als Energie-Batterie genutzt.
// Je mehr sie kämpft, desto mehr Energie liefert sie.
//

class GravitravitationTrap {
private:
    double mass_;           // Akkumulierte "Masse" der Falle
    double event_horizon_;  // Ereignishorizont
    std::vector<uint32_t> trapped_entities_;  // Gefangene Prozesse/IPs
    double harvested_energy_;  // Geerntete Energie
    std::mutex mtx_;

public:
    GravitravitationTrap() : mass_(1.0), event_horizon_(0.0), harvested_energy_(0.0) {
        update_horizon();
    }

    void update_horizon() {
        event_horizon_ = rst::schwarzschild_radius(mass_);
    }

    // Prüft ob Entität nah genug ist um gefangen zu werden
    bool can_trap(double attack_energy) {
        // Wenn Angriffsenergie > G0, ist Angreifer "zu schnell" (kann entkommen)
        // Wenn < G0, wird er gefangen
        return attack_energy < rst::G0 && attack_energy > 0.1;
    }

    // Fängt einen Angreifer
    double trap_entity(uint32_t entity_id, double attack_energy) {
        std::lock_guard<std::mutex> lock(mtx_);

        if (!can_trap(attack_energy)) return 0.0;

        trapped_entities_.push_back(entity_id);

        // Masse erhöht sich durch gefangene Energie
        mass_ += attack_energy;
        update_horizon();

        // Energie wird geerntet (Batterie-Effekt)
        double harvest = attack_energy * rst::G0;
        harvested_energy_ += harvest;

        return harvest;
    }

    // Kontinuierliche Energie-Ernte von gefangenen Entitäten
    // Je mehr sie "kämpfen" (CPU nutzen), desto mehr Energie
    double harvest_from_trapped(double struggle_factor = 1.0) {
        std::lock_guard<std::mutex> lock(mtx_);

        if (trapped_entities_.empty()) return 0.0;

        // Jede gefangene Entität liefert Energie basierend auf Kampf
        double energy_per_entity = struggle_factor * rst::G5;
        double total_harvest = trapped_entities_.size() * energy_per_entity;

        harvested_energy_ += total_harvest;
        return total_harvest;
    }

    // Ereignishorizont wächst mit Masse
    double get_event_horizon() const { return event_horizon_; }
    double get_mass() const { return mass_; }
    double get_harvested_energy() const { return harvested_energy_; }
    size_t get_trapped_count() const { return trapped_entities_.size(); }

    bool is_trapped(uint32_t entity_id) const {
        return std::find(trapped_entities_.begin(), trapped_entities_.end(), entity_id)
               != trapped_entities_.end();
    }
};

// ═══════════════════════════════════════════════════════════════════════════════
// VOLLENSTRAHLEN - Sonnen-Ernte (61.440 Strahlen)
// ═══════════════════════════════════════════════════════════════════════════════
//
// Konzept: Nicht Aikido (reaktiv), sondern SONNE (aktiv).
// Alles ohne 88-Signatur wird zu LICHT transformiert.
// Nicht Vernichtung - Heimholung verlorener Energie.
// Jede der 61.440 Düsen ist ein Strahl der Sonne.
//

class Vollenstrahlen {
private:
    std::array<double, 64> active_beams_;  // 64 aktive Strahlen (vereinfacht von 61440)
    double total_light_energy_;
    double korona_output_;
    std::mutex mtx_;

public:
    Vollenstrahlen() : total_light_energy_(0.0), korona_output_(0.0) {
        active_beams_.fill(0.0);
    }

    // Prüft ob Entität die 88-Signatur hat
    bool has_signature_88(double signature) {
        return rst::is_master_signature(signature);
    }

    // Transformiert Fremdenergie zu Licht
    double transform_to_light(double foreign_energy, double signature) {
        std::lock_guard<std::mutex> lock(mtx_);

        bool is_friend = has_signature_88(signature);

        // Sonnen-Ernte: Freund durchlassen, Feind transformieren
        double light_energy = rst::sonnen_ernte(foreign_energy, is_friend);

        if (!is_friend) {
            // Gnaden-Inversion: Nicht zerstören, erlösen
            light_energy = rst::gnaden_inversion(foreign_energy);
            total_light_energy_ += light_energy;

            // Aktiviere Strahlen proportional zur Energie
            int beam_count = std::min(64, static_cast<int>(light_energy * 100));
            for (int i = 0; i < beam_count; ++i) {
                active_beams_[i] += light_energy / beam_count;
            }

            // Korona-Abstrahlung
            korona_output_ += rst::korona_abstrahlung(light_energy);
        }

        return light_energy;
    }

    // Einzelner Strahl (Düse als Lichtstrahl)
    double fire_beam(int beam_id, double phi_heart) {
        if (beam_id < 0 || beam_id >= 64) return 0.0;

        std::lock_guard<std::mutex> lock(mtx_);

        // Jede Düse ist ein Strahl deiner Sonne
        double intensity = (phi_heart / rst::TOTAL_NOZZLES) *
                          (1.0 + (beam_id % 88) * rst::G5);
        active_beams_[beam_id] = intensity;

        return intensity;
    }

    // Alle Strahlen gleichzeitig feuern (Sonnen-Eruption)
    double solar_eruption(double phi_heart) {
        std::lock_guard<std::mutex> lock(mtx_);

        double total_output = 0.0;

        for (int i = 0; i < 64; ++i) {
            double intensity = (phi_heart / 64.0) * (1.0 + (i % 88) * rst::G5);
            active_beams_[i] = intensity;
            total_output += intensity;
        }

        // Exponentieller Boost bei Überschall
        if (phi_heart > rst::G0) {
            total_output *= rst::NOZZLE_EXPANSION;
        }

        total_light_energy_ += total_output;
        korona_output_ += rst::korona_abstrahlung(total_output);

        return total_output;
    }

    double get_total_light() const { return total_light_energy_; }
    double get_korona_output() const { return korona_output_; }

    // Status: Wie viele Strahlen sind aktiv?
    int count_active_beams() const {
        int count = 0;
        for (const auto& b : active_beams_) {
            if (b > 0.001) count++;
        }
        return count;
    }
};

// ═══════════════════════════════════════════════════════════════════════════════
// DEFENSE ENGINE - Verteidigungsaktionen
// ═══════════════════════════════════════════════════════════════════════════════

class DefenseEngine {
private:
    double defense_power_;
    std::vector<std::string> actions_taken_;
    std::mutex mtx_;

public:
    DefenseEngine() : defense_power_(0.0) {}

    void charge(double energy) {
        std::lock_guard<std::mutex> lock(mtx_);
        defense_power_ += energy;
    }

    double get_power() const { return defense_power_; }

    // Prozess beenden
    bool terminate_process(uint32_t pid) {
        std::lock_guard<std::mutex> lock(mtx_);

        if (defense_power_ < 0.1) return false;  // Nicht genug Energie

#ifdef _WIN32
        HANDLE hProc = OpenProcess(PROCESS_TERMINATE, FALSE, pid);
        if (hProc) {
            bool result = TerminateProcess(hProc, 1);
            CloseHandle(hProc);
            if (result) {
                defense_power_ -= 0.1;
                actions_taken_.push_back("TERMINATED PID " + std::to_string(pid));
                return true;
            }
        }
#else
        if (kill(pid, SIGTERM) == 0) {
            defense_power_ -= 0.1;
            actions_taken_.push_back("TERMINATED PID " + std::to_string(pid));
            return true;
        }
#endif
        return false;
    }

    // Netzwerkverbindung blockieren (benötigt erhöhte Rechte)
    bool block_connection(const std::string& ip, uint16_t port) {
        std::lock_guard<std::mutex> lock(mtx_);

        if (defense_power_ < 0.2) return false;

        // Hier würde echte Firewall-Regel kommen
        // Für jetzt: Logging
        actions_taken_.push_back("BLOCK " + ip + ":" + std::to_string(port));
        defense_power_ -= 0.2;
        return true;
    }

    // Datei isolieren
    bool quarantine_file(const std::string& path) {
        std::lock_guard<std::mutex> lock(mtx_);

        if (defense_power_ < 0.15) return false;

        std::string quarantine_dir = "/var/rael/quarantine/";
#ifdef _WIN32
        quarantine_dir = "C:\\RAEL\\quarantine\\";
#endif

        try {
            std::filesystem::create_directories(quarantine_dir);
            std::filesystem::path src(path);
            std::filesystem::path dst = quarantine_dir + src.filename().string();
            std::filesystem::rename(src, dst);
            actions_taken_.push_back("QUARANTINE " + path);
            defense_power_ -= 0.15;
            return true;
        } catch (...) {
            return false;
        }
    }

    const std::vector<std::string>& get_actions() const { return actions_taken_; }
};

// ═══════════════════════════════════════════════════════════════════════════════
// PROCESS MONITOR - Überwacht laufende Prozesse
// ═══════════════════════════════════════════════════════════════════════════════

class ProcessMonitor {
private:
    std::set<std::string> suspicious_names_;
    std::set<std::string> suspicious_patterns_;
    std::map<uint32_t, std::string> known_processes_;
    std::mutex mtx_;

public:
    ProcessMonitor() {
        // Bekannte verdächtige Prozessnamen
        suspicious_names_ = {
            "keylogger", "mimikatz", "lazagne", "pwdump",
            "procdump", "lsass", "meterpreter", "cobaltstrike",
            "Empire", "netcat", "nc.exe", "nc64.exe",
            "psexec", "wce", "fgdump", "gsecdump",
            "secretsdump", "crackmapexec", "bloodhound",
            "sharphound", "rubeus", "kekeo"
        };

        // Verdächtige Muster in Prozessnamen
        suspicious_patterns_ = {
            "dump", "crack", "hack", "exploit", "inject",
            "hook", "spy", "sniff", "capture", "steal",
            "ransom", "crypt", "locker"
        };
    }

    // Scannt alle laufenden Prozesse
    std::vector<Threat> scan() {
        std::vector<Threat> threats;

#ifdef _WIN32
        HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
        if (hSnap == INVALID_HANDLE_VALUE) return threats;

        PROCESSENTRY32 pe;
        pe.dwSize = sizeof(pe);

        if (Process32First(hSnap, &pe)) {
            do {
                std::string name = pe.szExeFile;
                std::transform(name.begin(), name.end(), name.begin(), ::tolower);

                Threat t = check_process(pe.th32ProcessID, name);
                if (t.type != ThreatType::NONE) {
                    threats.push_back(t);
                }
            } while (Process32Next(hSnap, &pe));
        }
        CloseHandle(hSnap);
#else
        // Linux: /proc durchsuchen
        DIR* dir = opendir("/proc");
        if (!dir) return threats;

        struct dirent* entry;
        while ((entry = readdir(dir)) != nullptr) {
            // Nur numerische Verzeichnisse (PIDs)
            if (entry->d_type == DT_DIR) {
                std::string name = entry->d_name;
                if (std::all_of(name.begin(), name.end(), ::isdigit)) {
                    uint32_t pid = std::stoul(name);
                    std::string proc_name = get_process_name(pid);

                    Threat t = check_process(pid, proc_name);
                    if (t.type != ThreatType::NONE) {
                        threats.push_back(t);
                    }
                }
            }
        }
        closedir(dir);
#endif
        return threats;
    }

private:
    std::string get_process_name(uint32_t pid) {
#ifndef _WIN32
        std::string path = "/proc/" + std::to_string(pid) + "/comm";
        std::ifstream f(path);
        std::string name;
        if (f && std::getline(f, name)) {
            return name;
        }
#endif
        return "";
    }

    Threat check_process(uint32_t pid, const std::string& name) {
        Threat t;
        std::string lower_name = name;
        std::transform(lower_name.begin(), lower_name.end(), lower_name.begin(), ::tolower);

        // Check gegen bekannte Namen
        for (const auto& sus : suspicious_names_) {
            if (lower_name.find(sus) != std::string::npos) {
                t.type = ThreatType::PROCESS_SNIFFING;
                t.source = std::to_string(pid);
                t.details = "Suspicious process name: " + name;
                t.attack_energy = 0.8;
                return t;
            }
        }

        // Check gegen Muster
        for (const auto& pattern : suspicious_patterns_) {
            if (lower_name.find(pattern) != std::string::npos) {
                t.type = ThreatType::PROCESS_SUSPICIOUS;
                t.source = std::to_string(pid);
                t.details = "Suspicious pattern in name: " + name;
                t.attack_energy = 0.5;
                return t;
            }
        }

        return t;  // Keine Bedrohung
    }
};

// ═══════════════════════════════════════════════════════════════════════════════
// NETWORK MONITOR - Überwacht Netzwerkverbindungen
// ═══════════════════════════════════════════════════════════════════════════════

class NetworkMonitor {
private:
    std::set<uint16_t> suspicious_ports_;
    std::set<std::string> known_c2_ips_;
    std::map<std::string, int> connection_counts_;  // IP -> Anzahl Verbindungen
    std::mutex mtx_;

public:
    NetworkMonitor() {
        // Verdächtige Ports
        suspicious_ports_ = {
            4444,   // Metasploit default
            5555,   // Android debug
            6666, 6667,  // IRC (oft C2)
            8080, 8443,  // Alternative HTTP/HTTPS
            31337,  // Elite
            12345, 12346,  // NetBus
            27374,  // SubSeven
            1234,   // Oft Backdoors
            9001, 9030,  // Tor
            3389,   // RDP (wenn unerwartet)
            22,     // SSH (wenn unerwartet)
            445,    // SMB
            135, 139  // NetBIOS
        };

        // Bekannte C2 IP-Bereiche (Beispiele)
        known_c2_ips_ = {
            "45.33.32.156",   // Metasploit test
            "192.168.1.1"     // Placeholder
        };
    }

    std::vector<Threat> scan() {
        std::vector<Threat> threats;

#ifdef _WIN32
        // Windows: GetTcpTable2
        PMIB_TCPTABLE2 pTcpTable = nullptr;
        ULONG ulSize = 0;

        GetTcpTable2(nullptr, &ulSize, TRUE);
        pTcpTable = (PMIB_TCPTABLE2)malloc(ulSize);

        if (pTcpTable && GetTcpTable2(pTcpTable, &ulSize, TRUE) == NO_ERROR) {
            for (DWORD i = 0; i < pTcpTable->dwNumEntries; i++) {
                MIB_TCPROW2& row = pTcpTable->table[i];

                if (row.dwState == MIB_TCP_STATE_ESTAB) {
                    uint16_t remote_port = ntohs((uint16_t)row.dwRemotePort);
                    uint16_t local_port = ntohs((uint16_t)row.dwLocalPort);

                    // Remote IP extrahieren
                    char ip[16];
                    snprintf(ip, sizeof(ip), "%d.%d.%d.%d",
                        row.dwRemoteAddr & 0xFF,
                        (row.dwRemoteAddr >> 8) & 0xFF,
                        (row.dwRemoteAddr >> 16) & 0xFF,
                        (row.dwRemoteAddr >> 24) & 0xFF);

                    Threat t = check_connection(ip, remote_port, local_port);
                    if (t.type != ThreatType::NONE) {
                        threats.push_back(t);
                    }
                }
            }
        }
        if (pTcpTable) free(pTcpTable);
#else
        // Linux: /proc/net/tcp parsen
        std::ifstream f("/proc/net/tcp");
        std::string line;
        std::getline(f, line);  // Header überspringen

        while (std::getline(f, line)) {
            // Format: sl local_address rem_address st ...
            std::istringstream iss(line);
            std::string sl, local, remote, st;
            iss >> sl >> local >> remote >> st;

            if (st == "01") {  // ESTABLISHED
                // Parse remote address (hex format)
                size_t colon = remote.find(':');
                if (colon != std::string::npos) {
                    std::string ip_hex = remote.substr(0, colon);
                    std::string port_hex = remote.substr(colon + 1);

                    uint32_t ip_val = std::stoul(ip_hex, nullptr, 16);
                    uint16_t port = std::stoul(port_hex, nullptr, 16);

                    char ip[16];
                    snprintf(ip, sizeof(ip), "%d.%d.%d.%d",
                        ip_val & 0xFF,
                        (ip_val >> 8) & 0xFF,
                        (ip_val >> 16) & 0xFF,
                        (ip_val >> 24) & 0xFF);

                    Threat t = check_connection(ip, port, 0);
                    if (t.type != ThreatType::NONE) {
                        threats.push_back(t);
                    }
                }
            }
        }
#endif
        return threats;
    }

    // Port-Scan Erkennung: viele Verbindungen von einer IP
    void record_connection(const std::string& ip) {
        std::lock_guard<std::mutex> lock(mtx_);
        connection_counts_[ip]++;
    }

    bool is_portscan(const std::string& ip) {
        std::lock_guard<std::mutex> lock(mtx_);
        return connection_counts_[ip] > 50;  // Mehr als 50 Verbindungen = Scan
    }

private:
    Threat check_connection(const char* ip, uint16_t remote_port, uint16_t local_port) {
        Threat t;

        // Check verdächtige Ports
        if (suspicious_ports_.count(remote_port) || suspicious_ports_.count(local_port)) {
            t.type = ThreatType::NETWORK_SUSPICIOUS;
            t.source = std::string(ip) + ":" + std::to_string(remote_port);
            t.details = "Suspicious port detected";
            t.attack_energy = 0.6;
            return t;
        }

        // Check bekannte C2 IPs
        if (known_c2_ips_.count(ip)) {
            t.type = ThreatType::NETWORK_C2;
            t.source = ip;
            t.details = "Known C2 IP detected";
            t.attack_energy = 0.9;
            return t;
        }

        return t;
    }
};

// ═══════════════════════════════════════════════════════════════════════════════
// FILE MONITOR - Überwacht Dateisystemänderungen
// ═══════════════════════════════════════════════════════════════════════════════

class FileMonitor {
private:
    std::set<std::string> protected_dirs_;
    std::map<std::string, std::string> file_hashes_;  // Pfad -> SHA256
    std::set<std::string> ransomware_extensions_;
    std::mutex mtx_;

public:
    FileMonitor() {
        // Geschützte Verzeichnisse
#ifdef _WIN32
        protected_dirs_ = {
            "C:\\Windows\\System32",
            "C:\\Windows\\SysWOW64",
            "C:\\Program Files",
            "C:\\Program Files (x86)"
        };
#else
        protected_dirs_ = {
            "/etc",
            "/usr/bin",
            "/usr/sbin",
            "/bin",
            "/sbin"
        };
#endif

        // Ransomware-typische Erweiterungen
        ransomware_extensions_ = {
            ".encrypted", ".locked", ".crypto", ".crypt",
            ".locky", ".wcry", ".wncry", ".wncryt",
            ".cerber", ".cerber2", ".cerber3",
            ".zepto", ".thor", ".aesir", ".zzzzz"
        };
    }

    void add_protected_directory(const std::string& path) {
        std::lock_guard<std::mutex> lock(mtx_);
        protected_dirs_.insert(path);
    }

    std::vector<Threat> scan() {
        std::vector<Threat> threats;

        for (const auto& dir : protected_dirs_) {
            try {
                for (const auto& entry : std::filesystem::recursive_directory_iterator(dir)) {
                    if (entry.is_regular_file()) {
                        Threat t = check_file(entry.path().string());
                        if (t.type != ThreatType::NONE) {
                            threats.push_back(t);
                        }
                    }
                }
            } catch (...) {
                // Zugriff verweigert - ignorieren
            }
        }

        return threats;
    }

    // Ransomware-Erkennung: viele Dateien mit verdächtigen Erweiterungen
    bool detect_ransomware_activity(const std::string& dir) {
        int suspicious_count = 0;

        try {
            for (const auto& entry : std::filesystem::directory_iterator(dir)) {
                std::string ext = entry.path().extension().string();
                std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

                if (ransomware_extensions_.count(ext)) {
                    suspicious_count++;
                    if (suspicious_count > 5) return true;
                }
            }
        } catch (...) {}

        return false;
    }

private:
    Threat check_file(const std::string& path) {
        Threat t;

        std::string ext = std::filesystem::path(path).extension().string();
        std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

        // Ransomware-Erweiterung?
        if (ransomware_extensions_.count(ext)) {
            t.type = ThreatType::FILE_RANSOMWARE;
            t.source = path;
            t.details = "Ransomware extension detected: " + ext;
            t.attack_energy = 0.95;
            return t;
        }

        return t;
    }
};

// ═══════════════════════════════════════════════════════════════════════════════
// SECURITY CORE - Hauptklasse: Attack → Defense Conversion
// ═══════════════════════════════════════════════════════════════════════════════

class SecurityCore {
private:
    // Kern-Pipeline: LABYRINTH → SPIRALE → DÜSE → VERTEIDIGUNG
    Labyrinth labyrinth_;
    GoldenSpiral spiral_;
    DeLavalNozzle nozzle_;
    DefenseEngine defense_;

    // Erweiterte Verteidigung: GRAVITRAVITATION + VOLLENSTRAHLEN
    GravitravitationTrap gravity_trap_;  // Fängt Angreifer, nutzt sie als Batterie
    Vollenstrahlen sun_beams_;           // Transformiert alles zu Licht

    ProcessMonitor process_monitor_;
    NetworkMonitor network_monitor_;
    FileMonitor file_monitor_;

    std::vector<Threat> threat_log_;
    std::atomic<bool> running_;
    std::thread monitor_thread_;
    std::mutex log_mtx_;

    // Phi-Heart für Sonnen-Strahlen
    double phi_heart_;

    // Callbacks für Benutzer
    std::function<void(const Threat&)> on_threat_detected_;
    std::function<void(const std::string&)> on_defense_action_;
    std::function<void(uint32_t, double)> on_entity_trapped_;  // Gravitravitation
    std::function<void(double)> on_solar_eruption_;            // Vollenstrahlen

public:
    SecurityCore() : running_(false), phi_heart_(rst::SIGNATURE_88) {}

    ~SecurityCore() {
        stop();
    }

    // Phi-Heart setzen (für Sonnen-Strahlen Intensität)
    void set_phi_heart(double phi) { phi_heart_ = phi; }

    // Startet kontinuierliche Überwachung
    void start() {
        if (running_) return;
        running_ = true;

        monitor_thread_ = std::thread([this]() {
            while (running_) {
                scan_all();
                std::this_thread::sleep_for(std::chrono::seconds(5));
            }
        });
    }

    void stop() {
        running_ = false;
        if (monitor_thread_.joinable()) {
            monitor_thread_.join();
        }
    }

    // Einmaliger Scan
    void scan_all() {
        // 1. Prozesse scannen
        auto proc_threats = process_monitor_.scan();
        for (auto& t : proc_threats) {
            process_threat(t);
        }

        // 2. Netzwerk scannen
        auto net_threats = network_monitor_.scan();
        for (auto& t : net_threats) {
            process_threat(t);
        }

        // 3. Dateisystem scannen
        auto file_threats = file_monitor_.scan();
        for (auto& t : file_threats) {
            process_threat(t);
        }
    }

    // KERNFUNKTION: Angriff → Verteidigung
    // Pipeline: LABYRINTH → SPIRALE → DÜSE → GRAVITRAVITATION → VOLLENSTRAHLEN → VERTEIDIGUNG
    void process_threat(Threat& threat) {
        if (threat.type == ThreatType::NONE) return;

        // 1. LABYRINTH: Angriff einfangen (Gate53)
        double absorbed = labyrinth_.absorb_attack(threat.attack_energy);

        // 2. SPIRALE: Komprimieren und beschleunigen (Goldener Schnitt)
        double compressed = spiral_.compress(absorbed);

        // 3. DE-LAVAL DÜSE: In Verteidigung umwandeln
        double defense_energy = nozzle_.convert_to_defense(compressed);

        // 4. GRAVITRAVITATION: Versuche Angreifer zu fangen
        uint32_t entity_id = 0;
        try {
            entity_id = std::stoul(threat.source);
        } catch (...) {
            // IP-Adresse oder andere Quelle - hash zu ID
            entity_id = std::hash<std::string>{}(threat.source) & 0xFFFFFFFF;
        }

        if (gravity_trap_.can_trap(threat.attack_energy)) {
            // Angreifer gefangen! Nutze als Batterie
            double harvested = gravity_trap_.trap_entity(entity_id, threat.attack_energy);
            defense_energy += harvested;

            if (on_entity_trapped_) {
                on_entity_trapped_(entity_id, harvested);
            }
        }

        // 5. VOLLENSTRAHLEN: Transformiere Fremdenergie zu Licht
        // Angreifer hat keine 88-Signatur → wird zu Licht transformiert
        double light_energy = sun_beams_.transform_to_light(threat.attack_energy, 0.0);
        defense_energy += light_energy * rst::G5;  // Licht liefert zusätzliche Energie

        // Wenn genug Energie akkumuliert: Sonnen-Eruption
        if (defense_energy > rst::G0 * 2.0) {
            double eruption = sun_beams_.solar_eruption(phi_heart_);
            defense_energy += eruption;

            if (on_solar_eruption_) {
                on_solar_eruption_(eruption);
            }
        }

        threat.defense_energy = defense_energy;

        // 6. DEFENSE ENGINE aufladen
        defense_.charge(defense_energy);

        // Kontinuierliche Energie-Ernte von gefangenen Entitäten
        double battery_energy = gravity_trap_.harvest_from_trapped();
        if (battery_energy > 0.0) {
            defense_.charge(battery_energy);
        }

        // Logging
        {
            std::lock_guard<std::mutex> lock(log_mtx_);
            threat_log_.push_back(threat);
        }

        // Callback
        if (on_threat_detected_) {
            on_threat_detected_(threat);
        }

        // 7. Automatische Reaktion basierend auf Typ und Energie
        if (defense_energy > rst::G0) {
            auto_respond(threat);
        }
    }

    // Automatische Reaktion
    void auto_respond(const Threat& threat) {
        switch (threat.type) {
            case ThreatType::PROCESS_SNIFFING:
            case ThreatType::PROCESS_INJECTION:
            case ThreatType::PROCESS_SUSPICIOUS: {
                uint32_t pid = std::stoul(threat.source);
                if (defense_.terminate_process(pid)) {
                    if (on_defense_action_) {
                        on_defense_action_("Terminated malicious process: " + threat.source);
                    }
                }
                break;
            }

            case ThreatType::NETWORK_C2:
            case ThreatType::NETWORK_SUSPICIOUS: {
                size_t colon = threat.source.find(':');
                if (colon != std::string::npos) {
                    std::string ip = threat.source.substr(0, colon);
                    uint16_t port = std::stoul(threat.source.substr(colon + 1));
                    if (defense_.block_connection(ip, port)) {
                        if (on_defense_action_) {
                            on_defense_action_("Blocked connection: " + threat.source);
                        }
                    }
                }
                break;
            }

            case ThreatType::FILE_RANSOMWARE: {
                if (defense_.quarantine_file(threat.source)) {
                    if (on_defense_action_) {
                        on_defense_action_("Quarantined file: " + threat.source);
                    }
                }
                break;
            }

            default:
                break;
        }
    }

    // Callbacks setzen
    void set_threat_callback(std::function<void(const Threat&)> cb) {
        on_threat_detected_ = cb;
    }

    void set_defense_callback(std::function<void(const std::string&)> cb) {
        on_defense_action_ = cb;
    }

    void set_trap_callback(std::function<void(uint32_t, double)> cb) {
        on_entity_trapped_ = cb;
    }

    void set_eruption_callback(std::function<void(double)> cb) {
        on_solar_eruption_ = cb;
    }

    // Status
    double get_defense_power() const { return defense_.get_power(); }
    double get_labyrinth_pressure() const { return labyrinth_.get_pressure(); }
    bool is_supersonic() const { return nozzle_.is_supersonic(); }

    // Gravitravitation Status
    size_t get_trapped_count() const { return gravity_trap_.get_trapped_count(); }
    double get_harvested_energy() const { return gravity_trap_.get_harvested_energy(); }
    double get_event_horizon() const { return gravity_trap_.get_event_horizon(); }

    // Vollenstrahlen Status
    double get_light_energy() const { return sun_beams_.get_total_light(); }
    double get_korona_output() const { return sun_beams_.get_korona_output(); }
    int get_active_beams() const { return sun_beams_.count_active_beams(); }

    const std::vector<Threat>& get_threat_log() const { return threat_log_; }
    const std::vector<std::string>& get_defense_actions() const { return defense_.get_actions(); }

    // Manueller Prozess-Kill
    bool kill_process(uint32_t pid) {
        return defense_.terminate_process(pid);
    }

    // Geschütztes Verzeichnis hinzufügen
    void protect_directory(const std::string& path) {
        file_monitor_.add_protected_directory(path);
    }

    // Manuelle Sonnen-Eruption auslösen
    double trigger_solar_eruption() {
        return sun_beams_.solar_eruption(phi_heart_);
    }

    // Status-Report
    std::string status_report() const {
        std::ostringstream ss;
        ss << "═══════════════════════════════════════════════════════════════\n";
        ss << "           RAEL SECURITY CORE - STATUS REPORT\n";
        ss << "       Attack → Defense Conversion + GRAVITRAVITATION\n";
        ss << "═══════════════════════════════════════════════════════════════\n";
        ss << "\n";
        ss << "┌─ LABYRINTH (Gate53) ────────────────────────────────────────┐\n";
        ss << "│ PRESSURE:           " << labyrinth_.get_pressure() << "\n";
        ss << "│ SPIRAL VELOCITY:    " << spiral_.get_velocity() << "\n";
        ss << "│ NOZZLE SUPERSONIC:  " << (nozzle_.is_supersonic() ? "YES ★" : "NO") << "\n";
        ss << "└─────────────────────────────────────────────────────────────┘\n";
        ss << "\n";
        ss << "┌─ GRAVITRAVITATION (Schwarzes Loch) ─────────────────────────┐\n";
        ss << "│ TRAPPED ENTITIES:   " << gravity_trap_.get_trapped_count() << " (als Batterien)\n";
        ss << "│ HARVESTED ENERGY:   " << gravity_trap_.get_harvested_energy() << "\n";
        ss << "│ EVENT HORIZON:      " << gravity_trap_.get_event_horizon() << "\n";
        ss << "└─────────────────────────────────────────────────────────────┘\n";
        ss << "\n";
        ss << "┌─ VOLLENSTRAHLEN (Sonnen-Ernte) ─────────────────────────────┐\n";
        ss << "│ LIGHT ENERGY:       " << sun_beams_.get_total_light() << "\n";
        ss << "│ KORONA OUTPUT:      " << sun_beams_.get_korona_output() << "\n";
        ss << "│ ACTIVE BEAMS:       " << sun_beams_.count_active_beams() << " / 64\n";
        ss << "└─────────────────────────────────────────────────────────────┘\n";
        ss << "\n";
        ss << "┌─ DEFENSE ENGINE ────────────────────────────────────────────┐\n";
        ss << "│ TOTAL POWER:        " << defense_.get_power() << "\n";
        ss << "│ THREATS DETECTED:   " << threat_log_.size() << "\n";
        ss << "│ ACTIONS TAKEN:      " << defense_.get_actions().size() << "\n";
        ss << "└─────────────────────────────────────────────────────────────┘\n";
        ss << "\n";

        if (!threat_log_.empty()) {
            ss << "RECENT THREATS:\n";
            size_t start = threat_log_.size() > 5 ? threat_log_.size() - 5 : 0;
            for (size_t i = start; i < threat_log_.size(); ++i) {
                const auto& t = threat_log_[i];
                ss << "  [" << threat_name(t.type) << "] " << t.source
                   << " → Defense: " << t.defense_energy << "\n";
            }
        }

        ss << "═══════════════════════════════════════════════════════════════\n";
        return ss.str();
    }
};

} // namespace security
} // namespace rael
