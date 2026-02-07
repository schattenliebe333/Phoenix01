#pragma once
// ═══════════════════════════════════════════════════════════════════════════
// N8: INNERES AUGE - Aktive Cyber-Verteidigung
// ═══════════════════════════════════════════════════════════════════════════
//
// Sieht alles. Versteht alles. Verfolgt zurueck. Isoliert. Bereinigt.
//
// Faehigkeiten:
//   SCANNEN    - Dateisystem, Prozesse, Ports, DNS, Netzwerk
//   VERSTEHEN  - Entropie-Analyse, Muster-Erkennung, Bedrohungs-Klassifikation
//   BEREINIGEN - Gegenfrequenz-Neutralisierung (kappa-Norm aus MathCore)
//   ISOLIEREN  - Prozesse einfrieren/isolieren, Verbindungen dokumentieren
//   VERFOLGEN  - Rueckverfolgung von Verbindungen und Anomalien
//
// Prinzip: Wie Gate53 - Angriffsdruck wird zu Verteidigungsdruck.
//          Schadcode verliert seine Wirkungsgrundlage durch
//          frequenzbasierte Transformation.
//
// ═══════════════════════════════════════════════════════════════════════════

#include <string>
#include <vector>
#include <map>
#include <cstdint>
#include <mutex>
#include <chrono>
#include <atomic>

namespace rael {
namespace inner_eye {

// ─── Bedrohungsstufen ────────────────────────────────────────────────────

enum class ThreatLevel : uint8_t {
    CLEAN    = 0,  // Keine Bedrohung
    LOW      = 1,  // Geringes Risiko
    MEDIUM   = 2,  // Mittleres Risiko
    HIGH     = 3,  // Hohes Risiko
    CRITICAL = 4   // Kritisch - sofort handeln
};

constexpr const char* threat_name(ThreatLevel t) {
    switch(t) {
        case ThreatLevel::CLEAN:    return "CLEAN";
        case ThreatLevel::LOW:      return "LOW";
        case ThreatLevel::MEDIUM:   return "MEDIUM";
        case ThreatLevel::HIGH:     return "HIGH";
        case ThreatLevel::CRITICAL: return "CRITICAL";
    }
    return "UNKNOWN";
}

// ─── Scan-Ergebnis-Strukturen ────────────────────────────────────────────

struct FileAnomaly {
    std::string path;
    std::string reason;
    ThreatLevel level;
    double entropy;          // Shannon-Entropie (hoch = verdaechtig)
    double counter_freq;     // Berechnete Gegenfrequenz
    uint64_t size_bytes;
    std::string hash;
};

struct ProcessInfo {
    uint32_t pid;
    std::string name;
    std::string cmdline;
    std::string user;
    uint64_t memory_kb;
    double cpu_percent;
    ThreatLevel level;
    std::string reason;
    bool frozen;
};

struct PortInfo {
    uint16_t port;
    std::string protocol;    // "tcp" oder "udp"
    std::string local_addr;
    std::string remote_addr;
    std::string state;
    uint32_t pid;
    std::string process_name;
    ThreatLevel level;
    std::string reason;
};

struct DnsEntry {
    std::string nameserver;
    ThreatLevel level;
    std::string reason;
};

struct NetworkConnection {
    std::string local;
    std::string remote;
    std::string state;
    uint32_t pid;
    std::string process;
    ThreatLevel level;
    std::string reason;
};

// ─── Gesamt-Scan-Ergebnis ────────────────────────────────────────────────

struct ScanResult {
    std::chrono::system_clock::time_point timestamp;

    // Ergebnisse
    std::vector<FileAnomaly> file_anomalies;
    std::vector<ProcessInfo> all_processes;
    std::vector<ProcessInfo> suspicious_processes;
    std::vector<PortInfo> open_ports;
    std::vector<PortInfo> suspicious_ports;
    std::vector<DnsEntry> dns_entries;
    std::vector<NetworkConnection> connections;
    std::vector<NetworkConnection> suspicious_connections;

    // Zusammenfassung
    ThreatLevel overall_level = ThreatLevel::CLEAN;
    uint32_t total_anomalies = 0;
    uint32_t files_scanned = 0;
    uint32_t processes_scanned = 0;
    uint32_t ports_scanned = 0;
    double system_entropy = 0.0;
    double counter_frequency = 0.0;

    // Empfehlungen
    std::vector<std::string> recommendations;

    std::string summary() const;
};

// ─── Das Innere Auge ─────────────────────────────────────────────────────

class InnerEye {
public:
    InnerEye();

    // ═══ SCANNEN (alles sehen) ═══

    ScanResult full_scan(const std::string& fs_root = "/");
    std::vector<FileAnomaly>      scan_filesystem(const std::string& root = "/");
    std::vector<ProcessInfo>      scan_processes();
    std::vector<PortInfo>         scan_ports();
    std::vector<DnsEntry>         scan_dns();
    std::vector<NetworkConnection> scan_connections();

    // ═══ VERSTEHEN (analysieren) ═══

    double      calculate_entropy(const std::string& filepath);
    ProcessInfo analyze_process(uint32_t pid);
    std::string trace_connection(const std::string& remote_addr);

    // ═══ BEREINIGEN (Gegenfrequenz) ═══

    // kappa(f) = 1 - f/QUELLE   (QUELLE = 1440 Hz)
    // Gegenfrequenz = QUELLE - f  (spiegelt am TOR = 720 Hz)
    double counter_frequency(double threat_frequency);
    bool   clean_anomaly(const FileAnomaly& anomaly);

    // ═══ ISOLIEREN (einfrieren) ═══

    bool freeze_process(uint32_t pid);
    bool unfreeze_process(uint32_t pid);
    bool isolate_process(uint32_t pid);

    // ═══ STATUS ═══

    const ScanResult& last_scan() const { return last_scan_; }
    void add_trusted_hash(const std::string& hash, const std::string& path);
    void add_trusted_process(const std::string& name);

    uint64_t total_scans()          const { return total_scans_.load(); }
    uint64_t threats_detected()     const { return threats_detected_.load(); }
    uint64_t threats_neutralized()  const { return threats_neutralized_.load(); }

    // Verarbeite einen Scan-Befehl (von Star8 Worker aufgerufen)
    std::string process_command(const std::string& command);

private:
    // Klassifizierung
    ThreatLevel classify_process(const ProcessInfo& proc);
    ThreatLevel classify_port(const PortInfo& port);
    ThreatLevel classify_file(const std::string& path, double entropy);
    ThreatLevel classify_connection(const NetworkConnection& conn);

    // Verdaechtige Muster
    bool is_suspicious_process_name(const std::string& name);
    bool is_suspicious_port(uint16_t port);
    bool is_suspicious_remote(const std::string& remote);

    // Hilfsfunktionen
    std::string read_proc_file(const std::string& path);
    std::vector<std::string> split_string(const std::string& s, char delim);
    uint32_t hex_to_uint32(const std::string& hex);
    std::string hex_to_ip(const std::string& hex);

    // State
    ScanResult last_scan_;
    std::map<std::string, std::string> trusted_hashes_;
    std::vector<std::string> trusted_processes_;
    std::mutex mtx_;

    // Statistik
    std::atomic<uint64_t> total_scans_{0};
    std::atomic<uint64_t> threats_detected_{0};
    std::atomic<uint64_t> threats_neutralized_{0};
};

} // namespace inner_eye
} // namespace rael
