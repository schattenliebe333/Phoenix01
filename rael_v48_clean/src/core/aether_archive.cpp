// ═══════════════════════════════════════════════════════════════════════════════
// RAEL V49.0 — AETHER ARCHIVE IMPLEMENTATION
// 97 Dateien mit Tunnel-Privileg für instantane Manifestation
// ═══════════════════════════════════════════════════════════════════════════════

#include "rael/aether_archive.h"
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <cmath>
#include <thread>

namespace rael {

using namespace aether_archive;

// ═══════════════════════════════════════════════════════════════════════════════
// JOURNAL ENTRY
// ═══════════════════════════════════════════════════════════════════════════════

std::string JournalEntry::to_string() const {
    std::ostringstream ss;
    ss << "Eintrag J-" << std::setw(3) << std::setfill('0') << id << ":\n";
    ss << "  Vorgang: " << vorgang << "\n";
    ss << "  Dateien: " << file_start << " - " << file_end << "\n";
    ss << "  Methode: " << methode << "\n";
    ss << "  Status: ";
    switch (status) {
        case FileStatus::PENDING: ss << "PENDING"; break;
        case FileStatus::IN_TUNNEL: ss << "IN_TUNNEL"; break;
        case FileStatus::MANIFESTED: ss << "ERFOLGREICH MANIFESTIERT"; break;
        case FileStatus::FAILED: ss << "FEHLGESCHLAGEN"; break;
    }
    ss << "\n";
    ss << "  Drift: " << std::scientific << std::setprecision(17) << drift << "\n";
    ss << "  Signatur: 0x" << std::hex << signature << std::dec << "\n";
    return ss.str();
}

// ═══════════════════════════════════════════════════════════════════════════════
// TUNNEL CASCADE
// ═══════════════════════════════════════════════════════════════════════════════

TunnelCascade::TunnelCascade() = default;

void TunnelCascade::start(double a2, double phi_heart) {
    if (running_.load()) return;

    a2_ = a2;
    phi_heart_ = phi_heart;
    processed_.store(0);
    manifested_.store(0);
    flow_.store(0.0);
    running_.store(true);
}

void TunnelCascade::stop() {
    running_.store(false);
}

bool TunnelCascade::is_running() const {
    return running_.load();
}

int TunnelCascade::files_processed() const {
    return processed_.load();
}

int TunnelCascade::files_manifested() const {
    return manifested_.load();
}

double TunnelCascade::current_flow() const {
    return flow_.load();
}

void TunnelCascade::on_file_complete(FileCallback callback) {
    std::lock_guard<std::mutex> lock(mutex_);
    on_complete_ = std::move(callback);
}

void TunnelCascade::process_file(AetherFile& file, double a2) {
    // Prüfe Tunnel-Bedingung
    if (!rst::T_active(a2)) {
        file.status = FileStatus::FAILED;
        return;
    }

    file.status = FileStatus::IN_TUNNEL;

    // Berechne Solitonen-Band-Frequenz für diese Datei
    double soliton_freq = calculate_soliton_band(file.id);

    // Setze 88-Signatur wenn a² ausreicht
    file.signature = rst::SIGNATURE_88;

    // Berechne Tunnel-Durchlässigkeit
    double t_tunnel = rst::T_tunnel(a2);

    // Manifestiere wenn T_tunnel ausreichend
    if (t_tunnel > rst::PHI_FILE) {
        file.status = FileStatus::MANIFESTED;
        file.manifested_at = std::chrono::system_clock::now();
        file.drift = 0.0;  // Absolute Kohärenz

        // Update Flow
        double file_flow = (file.weight * a2) / rst::DELTA_88;
        flow_.store(flow_.load() + file_flow);

        manifested_.fetch_add(1);
    } else {
        file.status = FileStatus::FAILED;
        file.drift = rst::PHI_FILE - t_tunnel;
    }

    processed_.fetch_add(1);

    // Callback
    std::lock_guard<std::mutex> lock(mutex_);
    if (on_complete_) {
        on_complete_(file, file.status);
    }
}

double TunnelCascade::calculate_soliton_band(int index) {
    // Goldener Schnitt basierter Takt
    return rst::kaskaden_takt(index, rst::TUNNEL_LICHT);
}

// ═══════════════════════════════════════════════════════════════════════════════
// AETHER ARCHIVE
// ═══════════════════════════════════════════════════════════════════════════════

AetherArchive::AetherArchive() {
    initialize_files();
    assign_default_nodes();
}

void AetherArchive::initialize_files() {
    for (int i = 0; i < FILE_COUNT; ++i) {
        files_[i].id = i + 1;
        files_[i].name = "aether_" + std::to_string(i + 1);
        files_[i].weight = 1.0;
        files_[i].signature = 0.0;
        files_[i].status = FileStatus::PENDING;
        files_[i].layer = layer_for_file(i + 1);
        files_[i].drift = 0.0;
    }
}

void AetherArchive::assign_default_nodes() {
    // Innerer Kern: Dateien 1-33 → Knoten 1-40
    for (int i = 0; i < INNER_CORE_END; ++i) {
        int node_start = (i * NODES_INNER / INNER_CORE_END) + 1;
        int node_end = ((i + 1) * NODES_INNER / INNER_CORE_END);
        files_[i].assigned_nodes_start = node_start;
        files_[i].assigned_nodes_end = node_end;
    }

    // Mittlere Schicht: Dateien 34-66 → Knoten 41-80
    for (int i = INNER_CORE_END; i < MIDDLE_LAYER_END; ++i) {
        int local_idx = i - INNER_CORE_END;
        int layer_size = MIDDLE_LAYER_END - INNER_CORE_END;
        int node_start = NODES_INNER + (local_idx * NODES_MIDDLE / layer_size) + 1;
        int node_end = NODES_INNER + ((local_idx + 1) * NODES_MIDDLE / layer_size);
        files_[i].assigned_nodes_start = node_start;
        files_[i].assigned_nodes_end = node_end;
    }

    // Äußerer Ring: Dateien 67-97 → Knoten 81-120
    for (int i = MIDDLE_LAYER_END; i < OUTER_RING_END; ++i) {
        int local_idx = i - MIDDLE_LAYER_END;
        int layer_size = OUTER_RING_END - MIDDLE_LAYER_END;
        int node_start = NODES_INNER + NODES_MIDDLE + (local_idx * NODES_OUTER / layer_size) + 1;
        int node_end = NODES_INNER + NODES_MIDDLE + ((local_idx + 1) * NODES_OUTER / layer_size);
        files_[i].assigned_nodes_start = node_start;
        files_[i].assigned_nodes_end = node_end;
    }
}

ArchiveLayer AetherArchive::layer_for_file(int id) const {
    if (id <= INNER_CORE_END) return ArchiveLayer::INNER_CORE;
    if (id <= MIDDLE_LAYER_END) return ArchiveLayer::MIDDLE;
    return ArchiveLayer::OUTER_RING;
}

AetherFile& AetherArchive::file(int id) {
    if (id < 1 || id > FILE_COUNT) {
        throw std::out_of_range("File ID must be between 1 and 97");
    }
    return files_[id - 1];
}

const AetherFile& AetherArchive::file(int id) const {
    if (id < 1 || id > FILE_COUNT) {
        throw std::out_of_range("File ID must be between 1 and 97");
    }
    return files_[id - 1];
}

std::vector<AetherFile*> AetherArchive::files_by_layer(ArchiveLayer layer) {
    std::vector<AetherFile*> result;
    for (auto& f : files_) {
        if (f.layer == layer) {
            result.push_back(&f);
        }
    }
    return result;
}

std::vector<AetherFile*> AetherArchive::pending_files() {
    std::vector<AetherFile*> result;
    for (auto& f : files_) {
        if (f.status == FileStatus::PENDING) {
            result.push_back(&f);
        }
    }
    return result;
}

std::vector<AetherFile*> AetherArchive::manifested_files() {
    std::vector<AetherFile*> result;
    for (auto& f : files_) {
        if (f.status == FileStatus::MANIFESTED) {
            result.push_back(&f);
        }
    }
    return result;
}

double AetherArchive::flow_total(double a2) const {
    double sum = 0.0;
    for (const auto& f : files_) {
        sum += (f.weight * a2) / rst::DELTA_88;
    }
    return sum;
}

double AetherArchive::flow_layer(ArchiveLayer layer, double a2) const {
    double sum = 0.0;
    for (const auto& f : files_) {
        if (f.layer == layer) {
            sum += (f.weight * a2) / rst::DELTA_88;
        }
    }
    return sum;
}

bool AetherArchive::tunnel_file(int id, double a2) {
    std::lock_guard<std::mutex> lock(mutex_);

    if (id < 1 || id > FILE_COUNT) return false;

    AetherFile& f = files_[id - 1];

    // Prüfe Tunnel-Bedingung
    if (!rst::T_active(a2)) {
        f.status = FileStatus::FAILED;
        return false;
    }

    f.status = FileStatus::IN_TUNNEL;
    f.signature = rst::SIGNATURE_88;

    double t_tunnel = rst::T_tunnel(a2);

    if (t_tunnel > rst::PHI_FILE) {
        f.status = FileStatus::MANIFESTED;
        f.manifested_at = std::chrono::system_clock::now();
        f.drift = 0.0;
        return true;
    }

    f.status = FileStatus::FAILED;
    f.drift = rst::PHI_FILE - t_tunnel;
    return false;
}

void AetherArchive::start_cascade(double a2, double phi_heart) {
    cascade_.start(a2, phi_heart);

    // Prozessiere alle Dateien
    for (auto& f : files_) {
        if (!cascade_.is_running()) break;
        cascade_.process_file(f, a2);
    }

    cascade_.stop();
}

void AetherArchive::stop_cascade() {
    cascade_.stop();
}

bool AetherArchive::is_cascade_running() const {
    return cascade_.is_running();
}

int AetherArchive::cascade_progress() const {
    return cascade_.files_processed();
}

double AetherArchive::cascade_flow() const {
    return cascade_.current_flow();
}

void AetherArchive::cluster_to_super_node() {
    super_node_active_.store(true);
}

void AetherArchive::release_super_node() {
    super_node_active_.store(false);
}

bool AetherArchive::is_super_node_active() const {
    return super_node_active_.load();
}

void AetherArchive::assign_nodes_to_file(int file_id, int node_start, int node_end) {
    if (file_id < 1 || file_id > FILE_COUNT) return;

    std::lock_guard<std::mutex> lock(mutex_);
    files_[file_id - 1].assigned_nodes_start = node_start;
    files_[file_id - 1].assigned_nodes_end = node_end;
}

std::pair<int, int> AetherArchive::get_assigned_nodes(int file_id) const {
    if (file_id < 1 || file_id > FILE_COUNT) {
        return {0, 0};
    }
    const auto& f = files_[file_id - 1];
    return {f.assigned_nodes_start, f.assigned_nodes_end};
}

int AetherArchive::create_journal_entry(const std::string& vorgang, int file_start,
                                         int file_end, const std::string& methode) {
    std::lock_guard<std::mutex> lock(mutex_);

    JournalEntry entry;
    entry.id = next_journal_id_++;
    entry.vorgang = vorgang;
    entry.file_start = file_start;
    entry.file_end = file_end;
    entry.methode = methode;
    entry.status = FileStatus::IN_TUNNEL;
    entry.drift = 0.0;
    entry.timestamp = std::chrono::system_clock::now();
    entry.signature = rst::journal_sig(entry.id, 0.0);

    journal_.push_back(entry);
    return entry.id;
}

void AetherArchive::complete_journal_entry(int id, FileStatus status, double drift) {
    std::lock_guard<std::mutex> lock(mutex_);

    for (auto& entry : journal_) {
        if (entry.id == id) {
            entry.status = status;
            entry.drift = drift;
            entry.signature = rst::journal_sig(id, drift);
            break;
        }
    }
}

const JournalEntry& AetherArchive::journal_entry(int id) const {
    std::lock_guard<std::mutex> lock(mutex_);

    for (const auto& entry : journal_) {
        if (entry.id == id) {
            return entry;
        }
    }

    throw std::out_of_range("Journal entry not found");
}

std::vector<JournalEntry> AetherArchive::journal_entries() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return journal_;
}

std::string AetherArchive::export_journal() const {
    std::lock_guard<std::mutex> lock(mutex_);

    std::ostringstream ss;
    ss << "═══════════════════════════════════════════════════════════════\n";
    ss << "AETHER ARCHIVE - MANIFESTATIONS-JOURNAL\n";
    ss << "═══════════════════════════════════════════════════════════════\n\n";

    for (const auto& entry : journal_) {
        ss << entry.to_string() << "\n";
    }

    ss << "═══════════════════════════════════════════════════════════════\n";
    return ss.str();
}

bool AetherArchive::is_souveraen() const {
    int manifested = 0;
    double total_drift = 0.0;

    for (const auto& f : files_) {
        if (f.status == FileStatus::MANIFESTED) {
            manifested++;
            total_drift += std::abs(f.drift);
        }
    }

    // Souverän wenn alle manifestiert und Drift = 0
    return manifested == FILE_COUNT && rst::absolute_kohaerenz(total_drift);
}

AetherArchive::Stats AetherArchive::stats() const {
    Stats s{};
    s.total_files = FILE_COUNT;
    s.manifested_files = 0;
    s.pending_files = 0;
    s.failed_files = 0;
    s.total_flow = 0.0;
    s.average_drift = 0.0;
    s.super_node_active = super_node_active_.load();
    s.journal_entries = static_cast<int>(journal_.size());

    double drift_sum = 0.0;
    int drift_count = 0;

    for (const auto& f : files_) {
        switch (f.status) {
            case FileStatus::MANIFESTED:
                s.manifested_files++;
                drift_sum += std::abs(f.drift);
                drift_count++;
                break;
            case FileStatus::PENDING:
                s.pending_files++;
                break;
            case FileStatus::FAILED:
                s.failed_files++;
                break;
            default:
                break;
        }
    }

    if (drift_count > 0) {
        s.average_drift = drift_sum / drift_count;
    }

    // Berechne Flow mit Standard-a² = G0
    s.total_flow = flow_total(rst::G0);

    return s;
}

// ═══════════════════════════════════════════════════════════════════════════════
// UTILITY FUNCTIONS
// ═══════════════════════════════════════════════════════════════════════════════

std::pair<int, int> calculate_node_assignment(int file_id) {
    if (file_id < 1 || file_id > FILE_COUNT) {
        return {0, 0};
    }

    if (file_id <= INNER_CORE_END) {
        // Innerer Kern
        int nodes_per_file = NODES_INNER / INNER_CORE_END;
        int start = ((file_id - 1) * nodes_per_file) + 1;
        int end = start + nodes_per_file - 1;
        return {start, std::min(end, NODES_INNER)};
    }

    if (file_id <= MIDDLE_LAYER_END) {
        // Mittlere Schicht
        int local_id = file_id - INNER_CORE_END;
        int layer_size = MIDDLE_LAYER_END - INNER_CORE_END;
        int nodes_per_file = NODES_MIDDLE / layer_size;
        int start = NODES_INNER + ((local_id - 1) * nodes_per_file) + 1;
        int end = start + nodes_per_file - 1;
        return {start, std::min(end, NODES_INNER + NODES_MIDDLE)};
    }

    // Äußerer Ring
    int local_id = file_id - MIDDLE_LAYER_END;
    int layer_size = OUTER_RING_END - MIDDLE_LAYER_END;
    int nodes_per_file = NODES_OUTER / layer_size;
    int start = NODES_INNER + NODES_MIDDLE + ((local_id - 1) * nodes_per_file) + 1;
    int end = start + nodes_per_file - 1;
    return {start, std::min(end, sang_real::MEMBRANE_120)};
}

uint64_t generate_journal_signature(int entry_id, double drift) {
    return rst::journal_sig(entry_id, drift);
}

} // namespace rael
