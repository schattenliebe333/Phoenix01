#pragma once
#include <string>
#include <vector>
#include <cstdint>
#include <functional>

namespace rael {

// ═══════════════════════════════════════════════════════════════════════════
// IMPROVEMENT SOURCE TYPES (Verbesserung #9: Quellen klar trennen)
// ═══════════════════════════════════════════════════════════════════════════
enum class ImprovementSource : uint8_t {
    REFLECT      = 1,   // Aus Selbstreflexion (ReflectionEngine)
    LIVE_DEFENSE = 2,   // Aus Live-Defense-Ereignissen (Gate53/Labyrinth)
    USER_TASK    = 3,   // Vom Benutzer angefordert
    SELF_OPT     = 4,   // Selbstoptimierung (Metriken-basiert)
    LEARNING     = 5,   // Aus Lernprozessen
    AAR          = 6,   // After-Action-Review (automatisch aus Metriken)
    EXTERNAL     = 7,   // Externe Quelle (Module, Plugins)
    OTHER        = 0    // Unbekannt/Sonstige
};

// Convert enum to string
inline const char* source_to_string(ImprovementSource src) {
    switch (src) {
        case ImprovementSource::REFLECT:      return "REFLECT";
        case ImprovementSource::LIVE_DEFENSE: return "LIVE_DEFENSE";
        case ImprovementSource::USER_TASK:    return "USER_TASK";
        case ImprovementSource::SELF_OPT:     return "SELF_OPT";
        case ImprovementSource::LEARNING:     return "LEARNING";
        case ImprovementSource::AAR:          return "AAR";
        case ImprovementSource::EXTERNAL:     return "EXTERNAL";
        default:                              return "OTHER";
    }
}

// Parse string to enum
inline ImprovementSource string_to_source(const std::string& s) {
    if (s == "REFLECT")      return ImprovementSource::REFLECT;
    if (s == "LIVE_DEFENSE") return ImprovementSource::LIVE_DEFENSE;
    if (s == "USER_TASK")    return ImprovementSource::USER_TASK;
    if (s == "SELF_OPT")     return ImprovementSource::SELF_OPT;
    if (s == "LEARNING")     return ImprovementSource::LEARNING;
    if (s == "AAR")          return ImprovementSource::AAR;
    if (s == "EXTERNAL")     return ImprovementSource::EXTERNAL;
    return ImprovementSource::OTHER;
}

// ═══════════════════════════════════════════════════════════════════════════
// IMPROVEMENT STATUS
// ═══════════════════════════════════════════════════════════════════════════
enum class ImprovementStatus : uint8_t {
    PENDING  = 0,   // Warte auf Review
    APPROVED = 1,   // Vom Menschen genehmigt
    REJECTED = 2,   // Abgelehnt
    DEFERRED = 3,   // Zurückgestellt
    APPLIED  = 4,   // Angewendet (mit Rollback-Möglichkeit)
    ROLLED_BACK = 5 // Zurückgerollt
};

inline const char* status_to_string(ImprovementStatus s) {
    switch (s) {
        case ImprovementStatus::PENDING:     return "PENDING";
        case ImprovementStatus::APPROVED:    return "APPROVED";
        case ImprovementStatus::REJECTED:    return "REJECTED";
        case ImprovementStatus::DEFERRED:    return "DEFERRED";
        case ImprovementStatus::APPLIED:     return "APPLIED";
        case ImprovementStatus::ROLLED_BACK: return "ROLLED_BACK";
        default:                             return "PENDING";
    }
}

// ═══════════════════════════════════════════════════════════════════════════
// IMPROVEMENT RECORD
// ═══════════════════════════════════════════════════════════════════════════
// Global human-in-the-loop improvement record.
// IMPORTANT: Improvements are suggestions only; applying them is always a human decision.
struct Improvement {
    uint64_t id = 0;
    std::string ts;                              // ISO8601
    std::string src;                             // Source string (legacy compatibility)
    ImprovementSource source = ImprovementSource::OTHER;  // Typed source
    int importance = 5;                          // 1..10
    int risk = 3;                                // 1..10
    double confidence = 0;                       // 0..1
    std::string title;
    std::string problem;
    std::string rationale;
    std::string testplan;
    std::string code;                            // optional (snippet)
    std::string status;                          // Status string (legacy)
    ImprovementStatus typed_status = ImprovementStatus::PENDING;

    // NEU: Rollback-Information (Verbesserung #4)
    std::string rollback_code;                   // Code zum Zurückrollen
    std::string shadow_result;                   // Ergebnis der Shadow-Simulation
    bool shadow_tested = false;                  // Wurde in Shadow-Umgebung getestet?

    // NEU: Metriken-Referenz (Verbesserung #10)
    std::string triggering_metric;               // Welche Metrik hat dies ausgelöst?
    double metric_value = 0.0;                   // Wert der Metrik
    double metric_threshold = 0.0;               // Schwellwert der Metrik
};

// ═══════════════════════════════════════════════════════════════════════════
// IMPROVEMENT BUS
// ═══════════════════════════════════════════════════════════════════════════
class ImprovementBus final {
public:
    // Adds an improvement suggestion, persists it, and emits a GUI-friendly event line.
    // Returns the assigned id.
    static uint64_t emit(Improvement imp);

    // Typed emit with source enum
    static uint64_t emit(ImprovementSource source, const std::string& title,
                        const std::string& problem, int importance = 5, int risk = 3);

    // Returns the last n improvements (most recent first).
    static std::vector<Improvement> last(size_t n);

    // Filter by source
    static std::vector<Improvement> by_source(ImprovementSource source, size_t n = 100);

    // Filter by status
    static std::vector<Improvement> by_status(ImprovementStatus status, size_t n = 100);

    // Persistence path used by the core (jsonl).
    static std::string storage_path();

    // Update status of an improvement
    static bool update_status(uint64_t id, ImprovementStatus new_status);
};

// ═══════════════════════════════════════════════════════════════════════════
// AFTER-ACTION-REVIEW (AAR) ENGINE (Verbesserung #10)
// ═══════════════════════════════════════════════════════════════════════════
// Analysiert Metriken und generiert automatisch Verbesserungsvorschläge

struct AARRule {
    std::string name;
    std::string metric_name;
    double threshold;
    bool trigger_above;  // true = trigger wenn metric > threshold
    std::string suggestion_title;
    std::string suggestion_problem;
    int importance;
    int risk;
};

class AAREngine {
public:
    AAREngine();

    // Registriere eine AAR-Regel
    void add_rule(const AARRule& rule);

    // Analysiere aktuelle Metriken und generiere Verbesserungen
    void analyze();

    // Setze Callback für Custom-Metriken
    using MetricProvider = std::function<double(const std::string&)>;
    void set_metric_provider(MetricProvider provider);

private:
    std::vector<AARRule> rules_;
    MetricProvider metric_provider_;
    uint64_t last_analysis_tick_ = 0;
};

// Global AAR Engine
extern AAREngine gAAR;

} // namespace rael
