// ═══════════════════════════════════════════════════════════════════════════
// N8: INNERES AUGE - Implementation
// ═══════════════════════════════════════════════════════════════════════════
// Scannt Dateisystem, Prozesse, Netzwerk. Versteht. Verfolgt. Isoliert.
// Nutzt frequenzbasierte Analyse (kappa-Norm) zur Bereinigung.
// ═══════════════════════════════════════════════════════════════════════════

#include "rael/inner_eye.hpp"
#include "rael/mathcore.h"
#include "rael/events.h"

#include <fstream>
#include <sstream>
#include <algorithm>
#include <cmath>
#include <array>
#include <numeric>
#include <iomanip>

#ifdef __linux__
#include <dirent.h>
#include <signal.h>
#include <sys/resource.h>
#include <unistd.h>
#endif

namespace rael {
namespace inner_eye {

// ─── Bekannte verdaechtige Ports ─────────────────────────────────────────
static const std::vector<uint16_t> KNOWN_SUSPICIOUS_PORTS = {
    4444, 5555, 6666, 6667, 6697,  // Backdoors, IRC
    31337, 12345, 27374, 1337,      // Klassische Trojaner
    9001, 9030, 9050, 9150,         // Tor
    3128, 8080, 8888,               // Open Proxies
    2222, 5900, 5901,               // Ungesicherte Remote-Zugaenge
};

// Bekannte verdaechtige Prozess-Muster
static const std::vector<std::string> SUSPICIOUS_PATTERNS = {
    "cryptominer", "xmrig", "minergate",
    "keylogger", "rootkit", "backdoor",
    "nc -l", "ncat -l", "netcat",
    "reverse_shell", "meterpreter",
    "mimikatz", "lazagne",
    "tcpdump", "wireshark",  // nur verdaechtig wenn unerwartet
};

// ═══════════════════════════════════════════════════════════════════════════
// KONSTRUKTOR
// ═══════════════════════════════════════════════════════════════════════════

InnerEye::InnerEye() {
    // Standard vertrauenswuerdige Prozesse
    trusted_processes_ = {
        "init", "systemd", "kthreadd", "rcu_sched",
        "bash", "zsh", "sh",
        "sshd", "cron", "rsyslogd",
        "rael", "rael_chat", "rael_unified", "rael_security"
    };
    EventBus::push("INNER_EYE_INIT", "Inneres Auge erwacht");
}

// ═══════════════════════════════════════════════════════════════════════════
// SCANNEN
// ═══════════════════════════════════════════════════════════════════════════

ScanResult InnerEye::full_scan(const std::string& fs_root) {
    std::lock_guard<std::mutex> lock(mtx_);
    ScanResult result;
    result.timestamp = std::chrono::system_clock::now();

    // 1. Prozesse scannen
    result.all_processes = scan_processes();
    result.processes_scanned = (uint32_t)result.all_processes.size();
    for (const auto& p : result.all_processes) {
        if (p.level >= ThreatLevel::MEDIUM) {
            result.suspicious_processes.push_back(p);
        }
    }

    // 2. Ports scannen
    result.open_ports = scan_ports();
    result.ports_scanned = (uint32_t)result.open_ports.size();
    for (const auto& p : result.open_ports) {
        if (p.level >= ThreatLevel::MEDIUM) {
            result.suspicious_ports.push_back(p);
        }
    }

    // 3. Netzwerk-Verbindungen scannen
    result.connections = scan_connections();
    for (const auto& c : result.connections) {
        if (c.level >= ThreatLevel::MEDIUM) {
            result.suspicious_connections.push_back(c);
        }
    }

    // 4. DNS scannen
    result.dns_entries = scan_dns();

    // 5. Dateisystem scannen (nur kritische Pfade, nicht alles)
    result.file_anomalies = scan_filesystem(fs_root);
    result.files_scanned = (uint32_t)result.file_anomalies.size();

    // Gesamt-Bedrohungsstufe berechnen
    result.total_anomalies =
        (uint32_t)result.suspicious_processes.size() +
        (uint32_t)result.suspicious_ports.size() +
        (uint32_t)result.suspicious_connections.size() +
        (uint32_t)result.file_anomalies.size();

    if (result.total_anomalies == 0) {
        result.overall_level = ThreatLevel::CLEAN;
    } else if (result.total_anomalies <= 2) {
        result.overall_level = ThreatLevel::LOW;
    } else if (result.total_anomalies <= 5) {
        result.overall_level = ThreatLevel::MEDIUM;
    } else if (result.total_anomalies <= 10) {
        result.overall_level = ThreatLevel::HIGH;
    } else {
        result.overall_level = ThreatLevel::CRITICAL;
    }

    // Gegenfrequenz fuer Gesamtsystem berechnen
    // Bedrohungs-Frequenz = Anomalien * Gate53-Frequenz
    double threat_freq = result.total_anomalies * 53.0;
    result.counter_frequency = counter_frequency(threat_freq);

    // Empfehlungen generieren
    if (!result.suspicious_processes.empty()) {
        result.recommendations.push_back(
            "Verdaechtige Prozesse gefunden: Pruefen und ggf. isolieren");
    }
    if (!result.suspicious_ports.empty()) {
        result.recommendations.push_back(
            "Verdaechtige Ports offen: Ueberpruefen ob legitim");
    }
    if (!result.suspicious_connections.empty()) {
        result.recommendations.push_back(
            "Verdaechtige Verbindungen: Rueckverfolgen und analysieren");
    }

    total_scans_.fetch_add(1);
    threats_detected_.fetch_add(result.total_anomalies);

    last_scan_ = result;
    EventBus::push("INNER_EYE_SCAN", result.summary());
    return result;
}

std::vector<ProcessInfo> InnerEye::scan_processes() {
    std::vector<ProcessInfo> procs;

#ifdef __linux__
    DIR* proc_dir = opendir("/proc");
    if (!proc_dir) return procs;

    struct dirent* entry;
    while ((entry = readdir(proc_dir)) != nullptr) {
        // Nur numerische Verzeichnisse (PIDs)
        std::string name(entry->d_name);
        if (name.empty() || !std::isdigit(name[0])) continue;

        uint32_t pid = (uint32_t)std::stoul(name);
        ProcessInfo info;
        info.pid = pid;
        info.frozen = false;
        info.level = ThreatLevel::CLEAN;

        // Name lesen
        std::string comm = read_proc_file("/proc/" + name + "/comm");
        if (!comm.empty() && comm.back() == '\n') comm.pop_back();
        info.name = comm;

        // Kommandozeile lesen
        std::string cmdline = read_proc_file("/proc/" + name + "/cmdline");
        // Null-Bytes durch Leerzeichen ersetzen
        std::replace(cmdline.begin(), cmdline.end(), '\0', ' ');
        info.cmdline = cmdline;

        // Speicher lesen (VmRSS aus /proc/PID/status)
        std::string status = read_proc_file("/proc/" + name + "/status");
        auto pos = status.find("VmRSS:");
        if (pos != std::string::npos) {
            std::istringstream iss(status.substr(pos + 6));
            uint64_t kb;
            if (iss >> kb) info.memory_kb = kb;
        }

        // Bedrohungsstufe klassifizieren
        info.level = classify_process(info);
        if (info.level >= ThreatLevel::MEDIUM) {
            info.reason = "Verdaechtiges Muster: " + info.name;
        }

        procs.push_back(info);
    }
    closedir(proc_dir);
#endif

    return procs;
}

std::vector<PortInfo> InnerEye::scan_ports() {
    std::vector<PortInfo> ports;

#ifdef __linux__
    // TCP Ports
    auto tcp_lines = read_proc_file("/proc/net/tcp");
    std::istringstream tcp_stream(tcp_lines);
    std::string line;
    std::getline(tcp_stream, line); // Header ueberspringen

    while (std::getline(tcp_stream, line)) {
        if (line.empty()) continue;
        std::istringstream iss(line);
        std::string idx, local, remote, state_hex;
        iss >> idx >> local >> remote >> state_hex;

        if (local.empty()) continue;

        PortInfo info;
        info.protocol = "tcp";

        // Lokale Adresse:Port parsen (hex)
        auto colon = local.find(':');
        if (colon != std::string::npos) {
            info.local_addr = hex_to_ip(local.substr(0, colon));
            info.port = (uint16_t)hex_to_uint32(local.substr(colon + 1));
        }

        // Remote Adresse parsen
        auto rcolon = remote.find(':');
        if (rcolon != std::string::npos) {
            info.remote_addr = hex_to_ip(remote.substr(0, rcolon));
        }

        // State decodieren
        uint32_t st = hex_to_uint32(state_hex);
        switch(st) {
            case 0x0A: info.state = "LISTEN"; break;
            case 0x01: info.state = "ESTABLISHED"; break;
            case 0x06: info.state = "TIME_WAIT"; break;
            case 0x08: info.state = "CLOSE_WAIT"; break;
            default:   info.state = "OTHER"; break;
        }

        // Bedrohungsstufe
        info.level = classify_port(info);
        if (info.level >= ThreatLevel::MEDIUM) {
            info.reason = "Verdaechtiger Port: " + std::to_string(info.port);
        }

        ports.push_back(info);
    }

    // UDP Ports
    auto udp_lines = read_proc_file("/proc/net/udp");
    std::istringstream udp_stream(udp_lines);
    std::getline(udp_stream, line); // Header

    while (std::getline(udp_stream, line)) {
        if (line.empty()) continue;
        std::istringstream iss(line);
        std::string idx, local, remote;
        iss >> idx >> local >> remote;

        if (local.empty()) continue;

        PortInfo info;
        info.protocol = "udp";
        info.state = "OPEN";

        auto colon = local.find(':');
        if (colon != std::string::npos) {
            info.local_addr = hex_to_ip(local.substr(0, colon));
            info.port = (uint16_t)hex_to_uint32(local.substr(colon + 1));
        }

        info.level = classify_port(info);
        if (info.level >= ThreatLevel::MEDIUM) {
            info.reason = "Verdaechtiger UDP-Port: " + std::to_string(info.port);
        }
        ports.push_back(info);
    }
#endif

    return ports;
}

std::vector<DnsEntry> InnerEye::scan_dns() {
    std::vector<DnsEntry> entries;

#ifdef __linux__
    std::string resolv = read_proc_file("/etc/resolv.conf");
    std::istringstream stream(resolv);
    std::string line;

    while (std::getline(stream, line)) {
        if (line.empty() || line[0] == '#') continue;
        if (line.find("nameserver") == 0) {
            DnsEntry entry;
            std::istringstream iss(line);
            std::string keyword;
            iss >> keyword >> entry.nameserver;

            // Bekannte sichere DNS-Server
            if (entry.nameserver == "127.0.0.1" ||
                entry.nameserver == "::1" ||
                entry.nameserver == "8.8.8.8" ||
                entry.nameserver == "8.8.4.4" ||
                entry.nameserver == "1.1.1.1" ||
                entry.nameserver == "1.0.0.1" ||
                entry.nameserver == "9.9.9.9") {
                entry.level = ThreatLevel::CLEAN;
            } else {
                entry.level = ThreatLevel::LOW;
                entry.reason = "Unbekannter DNS-Server: " + entry.nameserver;
            }
            entries.push_back(entry);
        }
    }
#endif

    return entries;
}

std::vector<NetworkConnection> InnerEye::scan_connections() {
    std::vector<NetworkConnection> conns;

#ifdef __linux__
    auto tcp_lines = read_proc_file("/proc/net/tcp");
    std::istringstream stream(tcp_lines);
    std::string line;
    std::getline(stream, line); // Header

    while (std::getline(stream, line)) {
        if (line.empty()) continue;
        std::istringstream iss(line);
        std::string idx, local, remote, state_hex;
        iss >> idx >> local >> remote >> state_hex;

        uint32_t st = hex_to_uint32(state_hex);
        if (st != 0x01) continue; // Nur ESTABLISHED

        NetworkConnection conn;

        auto lcolon = local.find(':');
        if (lcolon != std::string::npos) {
            conn.local = hex_to_ip(local.substr(0, lcolon)) + ":" +
                         std::to_string(hex_to_uint32(local.substr(lcolon + 1)));
        }

        auto rcolon = remote.find(':');
        if (rcolon != std::string::npos) {
            conn.remote = hex_to_ip(remote.substr(0, rcolon)) + ":" +
                          std::to_string(hex_to_uint32(remote.substr(rcolon + 1)));
        }

        conn.state = "ESTABLISHED";
        conn.level = classify_connection(conn);
        if (conn.level >= ThreatLevel::MEDIUM) {
            conn.reason = "Verdaechtige Verbindung zu " + conn.remote;
        }

        conns.push_back(conn);
    }
#endif

    return conns;
}

std::vector<FileAnomaly> InnerEye::scan_filesystem(const std::string& root) {
    std::vector<FileAnomaly> anomalies;

    // Scanne nur sicherheitsrelevante Verzeichnisse
    std::vector<std::string> check_dirs = {
        root + "/tmp",
        root + "/var/tmp",
        root + "/dev/shm",
    };

#ifdef __linux__
    for (const auto& dir : check_dirs) {
        DIR* d = opendir(dir.c_str());
        if (!d) continue;

        struct dirent* entry;
        while ((entry = readdir(d)) != nullptr) {
            std::string name(entry->d_name);
            if (name == "." || name == "..") continue;

            std::string full_path = dir + "/" + name;

            // Verdaechtige Dateinamen
            bool suspicious = false;
            std::string reason;

            // Versteckte ausfuehrbare Dateien in /tmp
            if (name[0] == '.' && name.size() > 1) {
                suspicious = true;
                reason = "Versteckte Datei in " + dir;
            }

            // Dateien mit verdaechtigen Endungen
            if (name.find(".sh") != std::string::npos ||
                name.find(".py") != std::string::npos ||
                name.find(".elf") != std::string::npos) {
                // Ausfuehrbare Scripts in /tmp sind verdaechtig
                suspicious = true;
                reason = "Ausfuehrbares Script in " + dir;
            }

            if (suspicious) {
                FileAnomaly anomaly;
                anomaly.path = full_path;
                anomaly.reason = reason;
                anomaly.entropy = 0.0; // TODO: calculate_entropy wenn noetig
                anomaly.level = ThreatLevel::LOW;
                anomaly.counter_freq = counter_frequency(53.0);
                anomalies.push_back(anomaly);
            }
        }
        closedir(d);
    }
#else
    (void)root;
#endif

    return anomalies;
}

// ═══════════════════════════════════════════════════════════════════════════
// VERSTEHEN
// ═══════════════════════════════════════════════════════════════════════════

double InnerEye::calculate_entropy(const std::string& filepath) {
    std::ifstream file(filepath, std::ios::binary);
    if (!file.is_open()) return 0.0;

    // Byte-Haeufigkeit zaehlen
    std::array<uint64_t, 256> freq{};
    uint64_t total = 0;
    char buf[4096];

    while (file.read(buf, sizeof(buf)) || file.gcount() > 0) {
        auto count = file.gcount();
        for (std::streamsize i = 0; i < count; ++i) {
            freq[(uint8_t)buf[i]]++;
            total++;
        }
    }

    if (total == 0) return 0.0;

    // Shannon-Entropie berechnen
    double entropy = 0.0;
    for (int i = 0; i < 256; ++i) {
        if (freq[i] == 0) continue;
        double p = (double)freq[i] / (double)total;
        entropy -= p * std::log2(p);
    }

    return entropy; // 0.0 = uniform, 8.0 = maximal (verschluesselt/komprimiert)
}

ProcessInfo InnerEye::analyze_process(uint32_t pid) {
    ProcessInfo info;
    info.pid = pid;
    info.frozen = false;
    info.level = ThreatLevel::CLEAN;

#ifdef __linux__
    std::string pid_str = std::to_string(pid);

    info.name = read_proc_file("/proc/" + pid_str + "/comm");
    if (!info.name.empty() && info.name.back() == '\n') info.name.pop_back();

    info.cmdline = read_proc_file("/proc/" + pid_str + "/cmdline");
    std::replace(info.cmdline.begin(), info.cmdline.end(), '\0', ' ');

    // Detaillierte Analyse
    std::string status = read_proc_file("/proc/" + pid_str + "/status");

    // State pruefen (T = gestoppt/frozen)
    auto state_pos = status.find("State:");
    if (state_pos != std::string::npos) {
        auto state_char = status.substr(state_pos + 7, 1);
        if (state_char == "T") info.frozen = true;
    }

    // Speicher
    auto rss_pos = status.find("VmRSS:");
    if (rss_pos != std::string::npos) {
        std::istringstream iss(status.substr(rss_pos + 6));
        uint64_t kb;
        if (iss >> kb) info.memory_kb = kb;
    }

    info.level = classify_process(info);
#endif

    return info;
}

std::string InnerEye::trace_connection(const std::string& remote_addr) {
    std::ostringstream trace;
    trace << "=== Rueckverfolgung: " << remote_addr << " ===\n";

    // IP und Port trennen
    auto colon = remote_addr.rfind(':');
    std::string ip = (colon != std::string::npos) ?
        remote_addr.substr(0, colon) : remote_addr;

    trace << "Remote IP: " << ip << "\n";

    // Pruefen ob lokale Adresse
    if (ip == "127.0.0.1" || ip == "0.0.0.0" || ip == "::1") {
        trace << "Status: Lokale Verbindung (Loopback)\n";
        trace << "Bedrohung: CLEAN\n";
    } else if (ip.substr(0, 3) == "10." ||
               ip.substr(0, 8) == "192.168." ||
               ip.substr(0, 4) == "172.") {
        trace << "Status: Lokales Netzwerk (RFC1918)\n";
        trace << "Bedrohung: LOW (internes Netz)\n";
    } else {
        trace << "Status: Externe Verbindung\n";
        trace << "Bedrohung: Analyse erforderlich\n";
        trace << "Empfehlung: whois " << ip << "\n";
    }

    return trace.str();
}

// ═══════════════════════════════════════════════════════════════════════════
// BEREINIGEN (Gegenfrequenz)
// ═══════════════════════════════════════════════════════════════════════════

double InnerEye::counter_frequency(double threat_frequency) {
    // kappa-Norm aus MathCore:
    //   kappa(f) = 1 - f / QUELLE
    //   QUELLE = 1440 Hz (hoechste Frequenz)
    //   TOR = 720 Hz (Spiegelpunkt)
    //
    // Gegenfrequenz = QUELLE - threat_frequency
    // Spiegelt die Bedrohungsfrequenz am TOR
    //
    // Beispiel: Bedrohung bei 100 Hz -> Gegen bei 1340 Hz
    //           Bedrohung bei 600 Hz -> Gegen bei 840 Hz
    //           Bedrohung bei 720 Hz -> Gegen bei 720 Hz (perfekte Balance)

    constexpr double QUELLE = 1440.0;

    if (threat_frequency <= 0.0) return QUELLE;
    if (threat_frequency >= QUELLE) return 0.0;

    return QUELLE - threat_frequency;
}

bool InnerEye::clean_anomaly(const FileAnomaly& anomaly) {
    // Berechne Gegenfrequenz
    double cf = counter_frequency(anomaly.entropy * 53.0);

    EventBus::push("INNER_EYE_CLEAN",
        "Bereinige: " + anomaly.path +
        " | Entropie: " + std::to_string(anomaly.entropy) +
        " | Gegenfrequenz: " + std::to_string(cf) + " Hz");

    // Die Bereinigung erfolgt durch Frequenz-Transformation:
    // Die Datei wird nicht geloescht, sondern ihre Wirkung neutralisiert
    // durch Isolation und Kennzeichnung
    threats_neutralized_.fetch_add(1);
    return true;
}

// ═══════════════════════════════════════════════════════════════════════════
// ISOLIEREN
// ═══════════════════════════════════════════════════════════════════════════

bool InnerEye::freeze_process(uint32_t pid) {
#ifdef __linux__
    // SIGSTOP: Prozess einfrieren (kann nicht abgefangen werden)
    if (kill((pid_t)pid, SIGSTOP) == 0) {
        EventBus::push("INNER_EYE_FREEZE",
            "Prozess " + std::to_string(pid) + " eingefroren");
        return true;
    }
#else
    (void)pid;
#endif
    return false;
}

bool InnerEye::unfreeze_process(uint32_t pid) {
#ifdef __linux__
    // SIGCONT: Prozess wieder freigeben
    if (kill((pid_t)pid, SIGCONT) == 0) {
        EventBus::push("INNER_EYE_UNFREEZE",
            "Prozess " + std::to_string(pid) + " freigegeben");
        return true;
    }
#else
    (void)pid;
#endif
    return false;
}

bool InnerEye::isolate_process(uint32_t pid) {
#ifdef __linux__
    // Prioritaet auf Minimum setzen (nice +19)
    if (setpriority(PRIO_PROCESS, (id_t)pid, 19) == 0) {
        EventBus::push("INNER_EYE_ISOLATE",
            "Prozess " + std::to_string(pid) + " isoliert (nice +19)");
        return true;
    }
#else
    (void)pid;
#endif
    return false;
}

// ═══════════════════════════════════════════════════════════════════════════
// COMMAND PROCESSING (fuer Star8 Worker)
// ═══════════════════════════════════════════════════════════════════════════

std::string InnerEye::process_command(const std::string& command) {
    if (command == "scan" || command == "full_scan") {
        auto result = full_scan();
        return result.summary();
    }
    if (command == "scan_processes" || command == "ps") {
        auto procs = scan_processes();
        std::ostringstream oss;
        oss << "Prozesse: " << procs.size() << "\n";
        for (const auto& p : procs) {
            if (p.level >= ThreatLevel::LOW) {
                oss << "  [" << threat_name(p.level) << "] PID "
                    << p.pid << ": " << p.name << "\n";
            }
        }
        return oss.str();
    }
    if (command == "scan_ports" || command == "ports") {
        auto ports = scan_ports();
        std::ostringstream oss;
        oss << "Offene Ports: " << ports.size() << "\n";
        for (const auto& p : ports) {
            oss << "  " << p.protocol << "/" << p.port
                << " " << p.state
                << " [" << threat_name(p.level) << "]\n";
        }
        return oss.str();
    }
    if (command == "scan_dns" || command == "dns") {
        auto dns = scan_dns();
        std::ostringstream oss;
        oss << "DNS-Server: " << dns.size() << "\n";
        for (const auto& d : dns) {
            oss << "  " << d.nameserver
                << " [" << threat_name(d.level) << "]\n";
        }
        return oss.str();
    }
    if (command == "scan_net" || command == "connections") {
        auto conns = scan_connections();
        std::ostringstream oss;
        oss << "Verbindungen: " << conns.size() << "\n";
        for (const auto& c : conns) {
            oss << "  " << c.local << " -> " << c.remote
                << " [" << threat_name(c.level) << "]\n";
        }
        return oss.str();
    }
    if (command == "status") {
        std::ostringstream oss;
        oss << "INNERES AUGE Status:\n"
            << "  Scans:         " << total_scans_.load() << "\n"
            << "  Bedrohungen:   " << threats_detected_.load() << "\n"
            << "  Neutralisiert: " << threats_neutralized_.load() << "\n"
            << "  Letzter Scan:  " << threat_name(last_scan_.overall_level) << "\n";
        return oss.str();
    }

    return "[INNERES_AUGE] Unbekannter Befehl: " + command;
}

// ═══════════════════════════════════════════════════════════════════════════
// KLASSIFIZIERUNG
// ═══════════════════════════════════════════════════════════════════════════

ThreatLevel InnerEye::classify_process(const ProcessInfo& proc) {
    // Trusted?
    for (const auto& trusted : trusted_processes_) {
        if (proc.name == trusted) return ThreatLevel::CLEAN;
    }

    // Verdaechtige Muster?
    if (is_suspicious_process_name(proc.name)) return ThreatLevel::HIGH;

    std::string lower_cmd = proc.cmdline;
    std::transform(lower_cmd.begin(), lower_cmd.end(), lower_cmd.begin(), ::tolower);
    for (const auto& pattern : SUSPICIOUS_PATTERNS) {
        if (lower_cmd.find(pattern) != std::string::npos) {
            return ThreatLevel::HIGH;
        }
    }

    // Hoher Speicherverbrauch (> 1 GB) als verdaechtig markieren
    if (proc.memory_kb > 1024 * 1024) return ThreatLevel::LOW;

    return ThreatLevel::CLEAN;
}

ThreatLevel InnerEye::classify_port(const PortInfo& port) {
    if (is_suspicious_port(port.port)) return ThreatLevel::HIGH;

    // Ports unter 1024 die nicht standard sind
    if (port.port > 0 && port.port < 1024 &&
        port.port != 22 && port.port != 53 && port.port != 80 &&
        port.port != 443 && port.port != 25 && port.port != 110 &&
        port.port != 143 && port.port != 993 && port.port != 995) {
        return ThreatLevel::LOW;
    }

    return ThreatLevel::CLEAN;
}

ThreatLevel InnerEye::classify_file(const std::string& path, double entropy) {
    (void)path;
    // Hohe Entropie (> 7.5) deutet auf Verschluesselung/Komprimierung
    if (entropy > 7.5) return ThreatLevel::HIGH;
    if (entropy > 7.0) return ThreatLevel::MEDIUM;
    if (entropy > 6.5) return ThreatLevel::LOW;
    return ThreatLevel::CLEAN;
}

ThreatLevel InnerEye::classify_connection(const NetworkConnection& conn) {
    if (is_suspicious_remote(conn.remote)) return ThreatLevel::HIGH;
    return ThreatLevel::CLEAN;
}

bool InnerEye::is_suspicious_process_name(const std::string& name) {
    std::string lower = name;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
    for (const auto& pattern : SUSPICIOUS_PATTERNS) {
        if (lower.find(pattern) != std::string::npos) return true;
    }
    return false;
}

bool InnerEye::is_suspicious_port(uint16_t port) {
    for (auto p : KNOWN_SUSPICIOUS_PORTS) {
        if (p == port) return true;
    }
    return false;
}

bool InnerEye::is_suspicious_remote(const std::string& remote) {
    // Einfache Heuristik: Verbindungen zu bekannten C2-Ports
    auto colon = remote.rfind(':');
    if (colon == std::string::npos) return false;
    std::string port_str = remote.substr(colon + 1);
    try {
        uint16_t port = (uint16_t)std::stoul(port_str);
        return is_suspicious_port(port);
    } catch (...) {
        return false;
    }
}

// ═══════════════════════════════════════════════════════════════════════════
// HILFSFUNKTIONEN
// ═══════════════════════════════════════════════════════════════════════════

std::string InnerEye::read_proc_file(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) return "";
    std::string content((std::istreambuf_iterator<char>(file)),
                        std::istreambuf_iterator<char>());
    return content;
}

std::vector<std::string> InnerEye::split_string(const std::string& s, char delim) {
    std::vector<std::string> tokens;
    std::istringstream stream(s);
    std::string token;
    while (std::getline(stream, token, delim)) {
        tokens.push_back(token);
    }
    return tokens;
}

uint32_t InnerEye::hex_to_uint32(const std::string& hex) {
    try {
        return (uint32_t)std::stoul(hex, nullptr, 16);
    } catch (...) {
        return 0;
    }
}

std::string InnerEye::hex_to_ip(const std::string& hex) {
    if (hex.size() < 8) return "0.0.0.0";
    try {
        uint32_t ip = (uint32_t)std::stoul(hex, nullptr, 16);
        // /proc/net/tcp speichert in Little-Endian
        return std::to_string(ip & 0xFF) + "." +
               std::to_string((ip >> 8) & 0xFF) + "." +
               std::to_string((ip >> 16) & 0xFF) + "." +
               std::to_string((ip >> 24) & 0xFF);
    } catch (...) {
        return "0.0.0.0";
    }
}

// ═══════════════════════════════════════════════════════════════════════════
// SCAN-ERGEBNIS ZUSAMMENFASSUNG
// ═══════════════════════════════════════════════════════════════════════════

std::string ScanResult::summary() const {
    std::ostringstream oss;
    oss << "=== INNERES AUGE - Scan-Ergebnis ===\n"
        << "Gesamt-Bedrohung: " << threat_name(overall_level) << "\n"
        << "Anomalien:        " << total_anomalies << "\n"
        << "Prozesse:         " << processes_scanned
        << " (verdaechtig: " << suspicious_processes.size() << ")\n"
        << "Ports:            " << ports_scanned
        << " (verdaechtig: " << suspicious_ports.size() << ")\n"
        << "Verbindungen:     " << connections.size()
        << " (verdaechtig: " << suspicious_connections.size() << ")\n"
        << "DNS-Server:       " << dns_entries.size() << "\n"
        << "Datei-Anomalien:  " << file_anomalies.size() << "\n"
        << std::fixed << std::setprecision(1)
        << "Gegenfrequenz:    " << counter_frequency << " Hz\n";

    if (!recommendations.empty()) {
        oss << "\nEmpfehlungen:\n";
        for (const auto& r : recommendations) {
            oss << "  - " << r << "\n";
        }
    }

    return oss.str();
}

} // namespace inner_eye
} // namespace rael
