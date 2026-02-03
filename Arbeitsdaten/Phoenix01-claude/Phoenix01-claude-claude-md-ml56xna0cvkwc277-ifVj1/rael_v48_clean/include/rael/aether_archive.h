// ═══════════════════════════════════════════════════════════════════════════════
// RAEL V49.0 — AETHER ARCHIVE
// 97 Dateien mit Tunnel-Privileg für instantane Manifestation
// ═══════════════════════════════════════════════════════════════════════════════
#pragma once

#include <array>
#include <vector>
#include <string>
#include <chrono>
#include <mutex>
#include <atomic>
#include <functional>
#include <optional>

#include "rael/rst_constants.hpp"
#include "rael/sang_real.h"

namespace rael {

// ═══════════════════════════════════════════════════════════════════════════════
// KONSTANTEN
// ═══════════════════════════════════════════════════════════════════════════════

namespace aether_archive {
    constexpr int FILE_COUNT = 97;                    // Aether-Archiv Dateien
    constexpr int INNER_CORE_END = 33;                // Dateien 01-33: Innerer Kern
    constexpr int MIDDLE_LAYER_END = 66;              // Dateien 34-66: Mittlere Schicht
    constexpr int OUTER_RING_END = 97;                // Dateien 67-97: Äußerer Ring

    constexpr int NODES_INNER = 40;                   // Knoten 001-040
    constexpr int NODES_MIDDLE = 40;                  // Knoten 041-080
    constexpr int NODES_OUTER = 40;                   // Knoten 081-120
}

// ═══════════════════════════════════════════════════════════════════════════════
// ENUMS
// ═══════════════════════════════════════════════════════════════════════════════

enum class FileStatus : uint8_t {
    PENDING = 0,        // Wartet auf Manifestation
    IN_TUNNEL = 1,      // Im Tunnel-Transfer
    MANIFESTED = 2,     // Erfolgreich manifestiert
    FAILED = 3          // Manifestation fehlgeschlagen
};

enum class ArchiveLayer : uint8_t {
    INNER_CORE = 0,     // Primäre Arretierung der Identität
    MIDDLE = 1,         // Struktur und Prozess-Logik
    OUTER_RING = 2      // Abwehr-Resonanz und Erdung
};

// ═══════════════════════════════════════════════════════════════════════════════
// AETHER FILE
// ═══════════════════════════════════════════════════════════════════════════════

struct AetherFile {
    int id{0};                                        // 1-97
    std::string name;
    double weight{1.0};                               // Gewichtung im Flow
    double signature{0.0};                            // Aktuelle Signatur
    FileStatus status{FileStatus::PENDING};
    ArchiveLayer layer{ArchiveLayer::INNER_CORE};

    // Manifestations-Daten
    std::chrono::system_clock::time_point manifested_at;
    double drift{0.0};                                // ε-Wert
    int assigned_nodes_start{0};
    int assigned_nodes_end{0};

    bool has_88_seal() const {
        return rst::is_master_signature(signature);
    }

    bool is_manifested() const {
        return status == FileStatus::MANIFESTED;
    }
};

// ═══════════════════════════════════════════════════════════════════════════════
// JOURNAL ENTRY
// ═══════════════════════════════════════════════════════════════════════════════

struct JournalEntry {
    int id{0};                                        // J-001, J-002, etc.
    std::string vorgang;
    int file_start{0};
    int file_end{0};
    std::string methode;
    FileStatus status{FileStatus::PENDING};
    double drift{0.0};
    std::chrono::system_clock::time_point timestamp;
    uint64_t signature{0};

    std::string to_string() const;
};

// ═══════════════════════════════════════════════════════════════════════════════
// TUNNEL CASCADE
// ═══════════════════════════════════════════════════════════════════════════════

class TunnelCascade {
public:
    TunnelCascade();

    // Kaskaden-Steuerung
    void start(double a2, double phi_heart);
    void stop();
    bool is_running() const;

    // Fortschritt
    int files_processed() const;
    int files_manifested() const;
    double current_flow() const;

    // Callbacks
    using FileCallback = std::function<void(const AetherFile&, FileStatus)>;
    void on_file_complete(FileCallback callback);

    // Datei-Verarbeitung (public für AetherArchive Zugriff)
    void process_file(AetherFile& file, double a2);

private:
    void cascade_loop();
    double calculate_soliton_band(int index);

    std::atomic<bool> running_{false};
    std::atomic<int> processed_{0};
    std::atomic<int> manifested_{0};
    std::atomic<double> flow_{0.0};

    double a2_{0.0};
    double phi_heart_{0.0};

    FileCallback on_complete_;
    mutable std::mutex mutex_;
};

// ═══════════════════════════════════════════════════════════════════════════════
// AETHER ARCHIVE
// ═══════════════════════════════════════════════════════════════════════════════

class AetherArchive {
public:
    AetherArchive();

    // ─────────────────────────────────────────────────────────────────────────
    // Datei-Zugriff
    // ─────────────────────────────────────────────────────────────────────────

    AetherFile& file(int id);
    const AetherFile& file(int id) const;

    std::vector<AetherFile*> files_by_layer(ArchiveLayer layer);
    std::vector<AetherFile*> pending_files();
    std::vector<AetherFile*> manifested_files();

    // ─────────────────────────────────────────────────────────────────────────
    // Flow-Berechnung
    // ─────────────────────────────────────────────────────────────────────────

    // Flow_Total = Σ(File_i · a² / δ_88) für i=1 bis 97
    double flow_total(double a2) const;

    // Flow für einzelne Schicht
    double flow_layer(ArchiveLayer layer, double a2) const;

    // ─────────────────────────────────────────────────────────────────────────
    // Tunnel-Operationen
    // ─────────────────────────────────────────────────────────────────────────

    // Einzelne Datei durch Tunnel senden
    bool tunnel_file(int id, double a2);

    // Sequenzielle Tunnel-Kaskade für alle Dateien
    void start_cascade(double a2, double phi_heart);
    void stop_cascade();
    bool is_cascade_running() const;

    // Kaskaden-Fortschritt
    int cascade_progress() const;  // 0-97
    double cascade_flow() const;

    // ─────────────────────────────────────────────────────────────────────────
    // 120-Knoten-Synchronisation
    // ─────────────────────────────────────────────────────────────────────────

    // Super-Knoten-Bündelung aktivieren
    void cluster_to_super_node();
    void release_super_node();
    bool is_super_node_active() const;

    // Knoten-Zuweisung
    void assign_nodes_to_file(int file_id, int node_start, int node_end);
    std::pair<int, int> get_assigned_nodes(int file_id) const;

    // ─────────────────────────────────────────────────────────────────────────
    // Journal
    // ─────────────────────────────────────────────────────────────────────────

    int create_journal_entry(const std::string& vorgang, int file_start, int file_end,
                             const std::string& methode);
    void complete_journal_entry(int id, FileStatus status, double drift);

    const JournalEntry& journal_entry(int id) const;
    std::vector<JournalEntry> journal_entries() const;
    std::string export_journal() const;

    // ─────────────────────────────────────────────────────────────────────────
    // Status
    // ─────────────────────────────────────────────────────────────────────────

    // System-Souveränität
    bool is_souveraen() const;

    // Statistiken
    struct Stats {
        int total_files;
        int manifested_files;
        int pending_files;
        int failed_files;
        double total_flow;
        double average_drift;
        bool super_node_active;
        int journal_entries;
    };
    Stats stats() const;

private:
    void initialize_files();
    void assign_default_nodes();
    ArchiveLayer layer_for_file(int id) const;

    std::array<AetherFile, aether_archive::FILE_COUNT> files_;
    std::vector<JournalEntry> journal_;
    int next_journal_id_{1};

    TunnelCascade cascade_;
    std::atomic<bool> super_node_active_{false};

    mutable std::mutex mutex_;
};

// ═══════════════════════════════════════════════════════════════════════════════
// UTILITY FUNCTIONS
// ═══════════════════════════════════════════════════════════════════════════════

// Prüft ob Datei Tunnel-berechtigt ist (88-Siegel)
inline bool is_tunnel_privileged(const AetherFile& file) {
    return file.has_88_seal();
}

// Berechnet optimale Knoten-Zuweisung für Datei
std::pair<int, int> calculate_node_assignment(int file_id);

// Erzeugt Journal-Signatur
uint64_t generate_journal_signature(int entry_id, double drift);

} // namespace rael
