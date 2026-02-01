#pragma once
// ═══════════════════════════════════════════════════════════════════════════
// RAEL CODE REVIEW GATE (Verbesserung #8)
// ═══════════════════════════════════════════════════════════════════════════
// RAEL kann Code analysieren und Risiken erklären
// Integration bleibt human-approved
// ═══════════════════════════════════════════════════════════════════════════

#include <string>
#include <vector>
#include <unordered_map>
#include <functional>
#include <regex>
#include <mutex>
#include <cstdint>

namespace rael {

// ═══════════════════════════════════════════════════════════════════════════
// RISK LEVEL
// ═══════════════════════════════════════════════════════════════════════════

enum class RiskLevel : uint8_t {
    NONE     = 0,   // Kein Risiko
    LOW      = 1,   // Niedriges Risiko
    MEDIUM   = 2,   // Mittleres Risiko
    HIGH     = 3,   // Hohes Risiko
    CRITICAL = 4    // Kritisches Risiko
};

inline const char* risk_level_name(RiskLevel r) {
    switch (r) {
        case RiskLevel::NONE:     return "NONE";
        case RiskLevel::LOW:      return "LOW";
        case RiskLevel::MEDIUM:   return "MEDIUM";
        case RiskLevel::HIGH:     return "HIGH";
        case RiskLevel::CRITICAL: return "CRITICAL";
        default:                  return "UNKNOWN";
    }
}

// ═══════════════════════════════════════════════════════════════════════════
// CODE ISSUE - Gefundenes Problem im Code
// ═══════════════════════════════════════════════════════════════════════════

struct CodeIssue {
    std::string id;              // Eindeutige ID
    std::string category;        // Kategorie (security, performance, style, etc.)
    RiskLevel risk = RiskLevel::NONE;
    std::string title;
    std::string description;
    std::string explanation;     // Warum ist das ein Problem?
    std::string recommendation;  // Was sollte geändert werden?

    // Position im Code
    int line_start = 0;
    int line_end = 0;
    int column = 0;
    std::string code_snippet;

    // Für Auto-Fix
    std::string suggested_fix;
    bool auto_fixable = false;
};

// ═══════════════════════════════════════════════════════════════════════════
// REVIEW RESULT
// ═══════════════════════════════════════════════════════════════════════════

struct ReviewResult {
    bool passed = false;              // Review bestanden?
    RiskLevel overall_risk = RiskLevel::NONE;
    std::vector<CodeIssue> issues;

    // Statistiken
    int total_lines = 0;
    int issues_found = 0;
    int critical_count = 0;
    int high_count = 0;
    int medium_count = 0;
    int low_count = 0;

    // Zusammenfassung
    std::string summary;
    std::string recommendation;

    // Human approval status
    bool requires_approval = false;
    bool approved = false;
    std::string approver;
    std::string approval_comment;
};

// ═══════════════════════════════════════════════════════════════════════════
// CODE PATTERN - Zu suchende Muster
// ═══════════════════════════════════════════════════════════════════════════

struct CodePattern {
    std::string id;
    std::string category;
    std::string pattern;          // Regex-Pattern
    RiskLevel risk;
    std::string title;
    std::string description;
    std::string recommendation;
    bool enabled = true;
};

// ═══════════════════════════════════════════════════════════════════════════
// CODE ANALYZER - Analysiert Code auf Probleme
// ═══════════════════════════════════════════════════════════════════════════

class CodeAnalyzer {
public:
    CodeAnalyzer();

    // ─────────────────────────────────────────────────────────────────────────
    // Analyse
    // ─────────────────────────────────────────────────────────────────────────

    // Analysiere Code-String
    std::vector<CodeIssue> analyze(const std::string& code, const std::string& language = "cpp");

    // Analysiere Datei
    std::vector<CodeIssue> analyze_file(const std::string& file_path);

    // ─────────────────────────────────────────────────────────────────────────
    // Pattern-Management
    // ─────────────────────────────────────────────────────────────────────────

    // Pattern hinzufügen
    void add_pattern(const CodePattern& pattern);

    // Pattern aktivieren/deaktivieren
    void enable_pattern(const std::string& id, bool enabled);

    // Alle Patterns für eine Kategorie aktivieren/deaktivieren
    void enable_category(const std::string& category, bool enabled);

    // Patterns auflisten
    std::vector<CodePattern> list_patterns() const;

private:
    void init_default_patterns();
    CodeIssue create_issue(const CodePattern& pattern, const std::string& match,
                          int line, const std::string& context);

    std::vector<CodePattern> patterns_;
};

// ═══════════════════════════════════════════════════════════════════════════
// CODE REVIEW GATE
// ═══════════════════════════════════════════════════════════════════════════

class CodeReviewGate {
public:
    CodeReviewGate();

    // ─────────────────────────────────────────────────────────────────────────
    // Review durchführen
    // ─────────────────────────────────────────────────────────────────────────

    // Code-Review durchführen
    ReviewResult review(const std::string& code, const std::string& language = "cpp");

    // Datei-Review
    ReviewResult review_file(const std::string& file_path);

    // Mehrere Dateien reviewen
    std::vector<ReviewResult> review_files(const std::vector<std::string>& file_paths);

    // ─────────────────────────────────────────────────────────────────────────
    // Approval Workflow
    // ─────────────────────────────────────────────────────────────────────────

    // Review zur Genehmigung einreichen
    std::string submit_for_approval(const ReviewResult& result, const std::string& context);

    // Review genehmigen
    bool approve(const std::string& review_id, const std::string& approver,
                 const std::string& comment = "");

    // Review ablehnen
    bool reject(const std::string& review_id, const std::string& approver,
                const std::string& reason);

    // Status abfragen
    ReviewResult get_review(const std::string& review_id) const;

    // Pending Reviews
    std::vector<std::string> pending_reviews() const;

    // ─────────────────────────────────────────────────────────────────────────
    // Konfiguration
    // ─────────────────────────────────────────────────────────────────────────

    // Setze Schwellwert ab dem Approval erforderlich ist
    void set_approval_threshold(RiskLevel level) { approval_threshold_ = level; }

    // Setze Callback für Benachrichtigungen
    using NotifyCallback = std::function<void(const std::string&, const ReviewResult&)>;
    void set_notify_callback(NotifyCallback cb) { notify_callback_ = cb; }

    // Zugriff auf Analyzer
    CodeAnalyzer& analyzer() { return analyzer_; }

private:
    ReviewResult create_result(const std::vector<CodeIssue>& issues);
    std::string generate_summary(const ReviewResult& result);

    CodeAnalyzer analyzer_;
    RiskLevel approval_threshold_ = RiskLevel::HIGH;
    NotifyCallback notify_callback_;

    std::unordered_map<std::string, ReviewResult> pending_reviews_;
    uint64_t next_review_id_ = 1;

    mutable std::mutex mutex_;
};

// Global instance
extern CodeReviewGate gCodeReview;

} // namespace rael
