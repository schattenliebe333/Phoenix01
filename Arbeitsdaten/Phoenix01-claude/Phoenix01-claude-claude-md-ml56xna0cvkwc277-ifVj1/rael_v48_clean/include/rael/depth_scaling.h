#pragma once
// ═══════════════════════════════════════════════════════════════════════════
// RAEL DEPTH SCALING via ACTIVATION CACHE (Verbesserung #6)
// ═══════════════════════════════════════════════════════════════════════════
// Selbstrechnender Speicher (Tiefe) dynamisch skalieren, ohne Fast-Lanes zu stören
// ═══════════════════════════════════════════════════════════════════════════

#include "rael/aether_ring.hpp"
#include <string>
#include <vector>
#include <unordered_map>
#include <mutex>
#include <atomic>
#include <functional>
#include <chrono>
#include <memory>
#include <cstdint>

namespace rael {

// ═══════════════════════════════════════════════════════════════════════════
// ACTIVATION LEVEL - Verschiedene Aktivierungsstufen
// ═══════════════════════════════════════════════════════════════════════════

enum class ActivationLevel : uint8_t {
    DORMANT  = 0,   // Schlafend (minimaler Speicher)
    MINIMAL  = 1,   // Minimal aktiv
    STANDARD = 2,   // Standard-Aktivierung
    ELEVATED = 3,   // Erhöhte Aktivierung
    MAXIMUM  = 4    // Maximale Tiefe
};

inline const char* activation_level_name(ActivationLevel l) {
    switch (l) {
        case ActivationLevel::DORMANT:  return "DORMANT";
        case ActivationLevel::MINIMAL:  return "MINIMAL";
        case ActivationLevel::STANDARD: return "STANDARD";
        case ActivationLevel::ELEVATED: return "ELEVATED";
        case ActivationLevel::MAXIMUM:  return "MAXIMUM";
        default:                        return "UNKNOWN";
    }
}

// ═══════════════════════════════════════════════════════════════════════════
// DEPTH CONFIG - Konfiguration für verschiedene Tiefen
// ═══════════════════════════════════════════════════════════════════════════

struct DepthConfig {
    size_t buffer_size = 64;           // Ring-Buffer Größe
    double compute_interval_ms = 100;  // Berechnungsintervall
    bool auto_scale = true;            // Automatische Skalierung
    double scale_up_threshold = 0.8;   // Wann hochskalieren
    double scale_down_threshold = 0.2; // Wann runterskalieren
};

// Vordefinierte Konfigurationen pro Level
inline DepthConfig get_depth_config(ActivationLevel level) {
    switch (level) {
        case ActivationLevel::DORMANT:
            return {16, 1000.0, false, 0.9, 0.1};
        case ActivationLevel::MINIMAL:
            return {32, 500.0, true, 0.85, 0.15};
        case ActivationLevel::STANDARD:
            return {64, 100.0, true, 0.8, 0.2};
        case ActivationLevel::ELEVATED:
            return {128, 50.0, true, 0.75, 0.25};
        case ActivationLevel::MAXIMUM:
            return {256, 10.0, false, 0.95, 0.05};
        default:
            return {64, 100.0, true, 0.8, 0.2};
    }
}

// ═══════════════════════════════════════════════════════════════════════════
// ACTIVATION CACHE - Selbstrechnender Cache mit dynamischer Tiefe
// ═══════════════════════════════════════════════════════════════════════════

class ActivationCache {
public:
    explicit ActivationCache(const std::string& name, size_t initial_depth = 64);

    // ─────────────────────────────────────────────────────────────────────────
    // Daten-Operationen
    // ─────────────────────────────────────────────────────────────────────────

    // Wert hinzufügen (automatisch self-computing)
    void push(double value);

    // Batch-Push für Effizienz
    void push_batch(const std::vector<double>& values);

    // ─────────────────────────────────────────────────────────────────────────
    // Self-Computing Statistiken (O(1) dank SelfComputingRingBuffer)
    // ─────────────────────────────────────────────────────────────────────────

    double mean() const { return buffer_.mean(); }
    double variance() const { return buffer_.variance(); }
    double stddev() const { return buffer_.stddev(); }
    double min() const { return buffer_.min(); }
    double max() const { return buffer_.max(); }
    double trend() const { return buffer_.trend(); }
    double entropy() const { return buffer_.entropy(); }
    double velocity() const { return buffer_.velocity(); }
    double acceleration() const { return buffer_.acceleration(); }

    // ─────────────────────────────────────────────────────────────────────────
    // Tiefenskalierung
    // ─────────────────────────────────────────────────────────────────────────

    // Aktuelle Tiefe
    size_t depth() const { return current_depth_; }

    // Aktivierungslevel
    ActivationLevel level() const { return level_; }

    // Manuell skalieren
    void scale_to(ActivationLevel level);

    // Automatische Skalierung aktivieren/deaktivieren
    void set_auto_scale(bool enabled) { auto_scale_ = enabled; }

    // Automatische Skalierung durchführen (basierend auf Nutzung)
    void auto_scale();

    // ─────────────────────────────────────────────────────────────────────────
    // Monitoring
    // ─────────────────────────────────────────────────────────────────────────

    // Nutzungsrate (0..1)
    double utilization() const;

    // Statistiken
    uint64_t total_pushes() const { return total_pushes_.load(); }
    uint64_t scale_ups() const { return scale_ups_.load(); }
    uint64_t scale_downs() const { return scale_downs_.load(); }

    // Name
    const std::string& name() const { return name_; }

private:
    void rebuild_buffer(size_t new_depth);

    std::string name_;
    SelfComputingRingBuffer buffer_;
    size_t current_depth_;
    ActivationLevel level_ = ActivationLevel::STANDARD;
    bool auto_scale_ = true;

    DepthConfig config_;

    std::atomic<uint64_t> total_pushes_{0};
    std::atomic<uint64_t> scale_ups_{0};
    std::atomic<uint64_t> scale_downs_{0};

    std::chrono::steady_clock::time_point last_scale_check_;
    mutable std::mutex mutex_;
};

// ═══════════════════════════════════════════════════════════════════════════
// DEPTH SCALER - Verwaltet mehrere Activation Caches
// ═══════════════════════════════════════════════════════════════════════════

class DepthScaler {
public:
    DepthScaler();

    // Cache erstellen/abrufen
    ActivationCache* create_cache(const std::string& name, size_t initial_depth = 64);
    ActivationCache* get_cache(const std::string& name);
    bool remove_cache(const std::string& name);

    // Globale Operationen
    void scale_all(ActivationLevel level);
    void auto_scale_all();

    // Globale Statistiken
    size_t cache_count() const { return caches_.size(); }
    size_t total_depth() const;
    double average_utilization() const;

    // Callback für Skalierungsereignisse
    using ScaleCallback = std::function<void(const std::string&, ActivationLevel, ActivationLevel)>;
    void set_scale_callback(ScaleCallback cb) { scale_callback_ = cb; }

private:
    std::unordered_map<std::string, std::unique_ptr<ActivationCache>> caches_;
    mutable std::mutex mutex_;
    ScaleCallback scale_callback_;
};

// ═══════════════════════════════════════════════════════════════════════════
// FAST LANE PROTECTOR - Schützt Fast-Lanes vor Tiefenskalierung
// ═══════════════════════════════════════════════════════════════════════════
// Stellt sicher, dass kritische Pfade nicht durch Skalierung gestört werden

class FastLaneProtector {
public:
    // Markiere eine Lane als Fast-Lane (wird nicht skaliert)
    void protect_lane(const std::string& cache_name);
    void unprotect_lane(const std::string& cache_name);
    bool is_protected(const std::string& cache_name) const;

    // Prüfe ob Skalierung erlaubt ist
    bool can_scale(const std::string& cache_name) const;

    // Temporäre Sperre während kritischer Operationen
    class ScopedProtection {
    public:
        ScopedProtection(FastLaneProtector& protector, const std::string& name);
        ~ScopedProtection();
    private:
        FastLaneProtector& protector_;
        std::string name_;
    };

private:
    std::unordered_map<std::string, bool> protected_lanes_;
    mutable std::mutex mutex_;
};

// Global instances
extern DepthScaler gDepthScaler;
extern FastLaneProtector gFastLaneProtector;

} // namespace rael
