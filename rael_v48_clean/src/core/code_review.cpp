#include "rael/code_review.h"
#include "rael/events.h"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <regex>

namespace rael {

// Global instance
CodeReviewGate gCodeReview;

// ═══════════════════════════════════════════════════════════════════════════
// CODE ANALYZER IMPLEMENTATION
// ═══════════════════════════════════════════════════════════════════════════

CodeAnalyzer::CodeAnalyzer() {
    init_default_patterns();
}

void CodeAnalyzer::init_default_patterns() {
    // ─────────────────────────────────────────────────────────────────────────
    // SECURITY PATTERNS
    // ─────────────────────────────────────────────────────────────────────────

    // SQL Injection
    add_pattern({
        "SEC001", "security",
        R"(execute\s*\(\s*["'].*\+)",
        RiskLevel::CRITICAL,
        "Potential SQL Injection",
        "String concatenation in SQL query detected",
        "Use parameterized queries or prepared statements"
    });

    // Command Injection
    add_pattern({
        "SEC002", "security",
        R"(system\s*\(|exec\s*\(|popen\s*\()",
        RiskLevel::HIGH,
        "Command Execution",
        "Direct system command execution detected",
        "Validate and sanitize all inputs before execution"
    });

    // Hardcoded Credentials
    add_pattern({
        "SEC003", "security",
        R"(password\s*=\s*["'][^"']+["']|api_key\s*=\s*["'][^"']+["'])",
        RiskLevel::CRITICAL,
        "Hardcoded Credentials",
        "Hardcoded password or API key detected",
        "Use environment variables or secure credential storage"
    });

    // Buffer Overflow Risk
    add_pattern({
        "SEC004", "security",
        R"(strcpy\s*\(|strcat\s*\(|sprintf\s*\(|gets\s*\()",
        RiskLevel::HIGH,
        "Unsafe String Function",
        "Use of unsafe string function that can cause buffer overflow",
        "Use safe alternatives like strncpy, strncat, snprintf"
    });

    // Use After Free Risk
    add_pattern({
        "SEC005", "security",
        R"(delete\s+\w+\s*;[^}]*\w+->)",
        RiskLevel::HIGH,
        "Potential Use After Free",
        "Pointer used after delete",
        "Set pointer to nullptr after delete"
    });

    // ─────────────────────────────────────────────────────────────────────────
    // PERFORMANCE PATTERNS
    // ─────────────────────────────────────────────────────────────────────────

    // Inefficient Loop
    add_pattern({
        "PERF001", "performance",
        R"(for\s*\([^;]+;\s*\w+\s*<\s*\w+\.size\s*\(\s*\))",
        RiskLevel::LOW,
        "Repeated Size Call in Loop",
        "Calling size() in every iteration can be inefficient",
        "Cache size() result before loop"
    });

    // Memory Allocation in Loop
    add_pattern({
        "PERF002", "performance",
        R"(for\s*\([^)]+\)[^{]*\{[^}]*new\s+)",
        RiskLevel::MEDIUM,
        "Memory Allocation in Loop",
        "Allocating memory inside loop can cause performance issues",
        "Consider pre-allocating or using object pools"
    });

    // ─────────────────────────────────────────────────────────────────────────
    // CODE QUALITY PATTERNS
    // ─────────────────────────────────────────────────────────────────────────

    // Magic Numbers
    add_pattern({
        "QUAL001", "quality",
        R"(\[\s*\d{3,}\s*\]|==\s*\d{3,}|>\s*\d{3,})",
        RiskLevel::LOW,
        "Magic Number",
        "Large numeric literal without explanation",
        "Define as named constant with descriptive name"
    });

    // TODO/FIXME Comments
    add_pattern({
        "QUAL002", "quality",
        R"(//\s*(TODO|FIXME|HACK|XXX))",
        RiskLevel::LOW,
        "Unresolved TODO/FIXME",
        "Code contains unresolved TODO or FIXME comment",
        "Address the issue or create a tracking ticket"
    });

    // Long Function (heuristic)
    add_pattern({
        "QUAL003", "quality",
        R"(\{[^{}]{5000,}\})",
        RiskLevel::MEDIUM,
        "Long Function",
        "Function body appears very long",
        "Consider splitting into smaller functions"
    });

    // ─────────────────────────────────────────────────────────────────────────
    // ERROR HANDLING PATTERNS
    // ─────────────────────────────────────────────────────────────────────────

    // Empty Catch Block
    add_pattern({
        "ERR001", "error_handling",
        R"(catch\s*\([^)]*\)\s*\{\s*\})",
        RiskLevel::MEDIUM,
        "Empty Catch Block",
        "Exception caught but not handled",
        "Log the error or handle appropriately"
    });

    // Swallowed Exception
    add_pattern({
        "ERR002", "error_handling",
        R"(catch\s*\(\s*\.\.\.\s*\)\s*\{[^}]*\})",
        RiskLevel::MEDIUM,
        "Catch-All Exception",
        "Catching all exceptions may hide important errors",
        "Catch specific exception types when possible"
    });

    // ─────────────────────────────────────────────────────────────────────────
    // CONCURRENCY PATTERNS
    // ─────────────────────────────────────────────────────────────────────────

    // Race Condition Risk
    add_pattern({
        "CONC001", "concurrency",
        R"(static\s+\w+\s+\w+\s*=)",
        RiskLevel::MEDIUM,
        "Static Variable",
        "Static variable may cause race conditions in multi-threaded code",
        "Consider thread-local storage or proper synchronization"
    });

    // Missing Lock
    add_pattern({
        "CONC002", "concurrency",
        R"(std::thread[^;]+;[^}]*(?!mutex|lock))",
        RiskLevel::MEDIUM,
        "Thread Without Obvious Lock",
        "Thread created but no lock visible nearby",
        "Ensure proper synchronization for shared data"
    });
}

void CodeAnalyzer::add_pattern(const CodePattern& pattern) {
    patterns_.push_back(pattern);
}

void CodeAnalyzer::enable_pattern(const std::string& id, bool enabled) {
    for (auto& p : patterns_) {
        if (p.id == id) {
            p.enabled = enabled;
            break;
        }
    }
}

void CodeAnalyzer::enable_category(const std::string& category, bool enabled) {
    for (auto& p : patterns_) {
        if (p.category == category) {
            p.enabled = enabled;
        }
    }
}

std::vector<CodePattern> CodeAnalyzer::list_patterns() const {
    return patterns_;
}

std::vector<CodeIssue> CodeAnalyzer::analyze(const std::string& code, const std::string& /*language*/) {
    std::vector<CodeIssue> issues;

    // Zerlege Code in Zeilen für Zeilennummern
    std::vector<std::string> lines;
    std::istringstream iss(code);
    std::string line;
    while (std::getline(iss, line)) {
        lines.push_back(line);
    }

    // Prüfe jedes Pattern
    for (const auto& pattern : patterns_) {
        if (!pattern.enabled) continue;

        try {
            std::regex re(pattern.pattern, std::regex::ECMAScript | std::regex::icase);
            std::smatch match;

            std::string remaining = code;
            size_t offset = 0;

            while (std::regex_search(remaining, match, re)) {
                // Finde Zeilennummer
                size_t pos = offset + match.position();
                int line_num = 1;
                size_t char_count = 0;

                for (const auto& l : lines) {
                    char_count += l.length() + 1;  // +1 für Newline
                    if (char_count > pos) break;
                    line_num++;
                }

                // Erstelle Issue
                std::string context = (line_num > 0 && line_num <= (int)lines.size())
                    ? lines[line_num - 1]
                    : match.str();

                issues.push_back(create_issue(pattern, match.str(), line_num, context));

                // Weitermachen
                offset += match.position() + match.length();
                remaining = match.suffix().str();
            }
        } catch (const std::regex_error&) {
            // Regex-Fehler ignorieren
        }
    }

    // Sortiere nach Risiko (höchstes zuerst)
    std::sort(issues.begin(), issues.end(),
        [](const CodeIssue& a, const CodeIssue& b) {
            return static_cast<int>(a.risk) > static_cast<int>(b.risk);
        });

    return issues;
}

std::vector<CodeIssue> CodeAnalyzer::analyze_file(const std::string& file_path) {
    std::ifstream f(file_path);
    if (!f) {
        return {};
    }

    std::ostringstream ss;
    ss << f.rdbuf();

    // Sprache aus Dateiendung ableiten
    std::string language = "cpp";
    size_t dot = file_path.rfind('.');
    if (dot != std::string::npos) {
        std::string ext = file_path.substr(dot + 1);
        if (ext == "py") language = "python";
        else if (ext == "js") language = "javascript";
        else if (ext == "java") language = "java";
    }

    return analyze(ss.str(), language);
}

CodeIssue CodeAnalyzer::create_issue(const CodePattern& pattern, const std::string& match,
                                     int line, const std::string& context) {
    CodeIssue issue;
    issue.id = pattern.id + "_L" + std::to_string(line);
    issue.category = pattern.category;
    issue.risk = pattern.risk;
    issue.title = pattern.title;
    issue.description = pattern.description;
    issue.recommendation = pattern.recommendation;
    issue.line_start = line;
    issue.line_end = line;
    issue.code_snippet = context;
    issue.explanation = "Found pattern: " + match.substr(0, 50);

    return issue;
}

// ═══════════════════════════════════════════════════════════════════════════
// CODE REVIEW GATE IMPLEMENTATION
// ═══════════════════════════════════════════════════════════════════════════

CodeReviewGate::CodeReviewGate() {}

ReviewResult CodeReviewGate::review(const std::string& code, const std::string& language) {
    auto issues = analyzer_.analyze(code, language);
    auto result = create_result(issues);

    // Zähle Zeilen
    result.total_lines = std::count(code.begin(), code.end(), '\n') + 1;

    // Generiere Zusammenfassung
    result.summary = generate_summary(result);

    // Prüfe ob Approval erforderlich
    result.requires_approval = result.overall_risk >= approval_threshold_;

    // Event emittieren
    EventBus::push("CODE_REVIEW",
        "risk=" + std::string(risk_level_name(result.overall_risk)) +
        "|issues=" + std::to_string(result.issues_found) +
        "|passed=" + (result.passed ? "1" : "0"));

    return result;
}

ReviewResult CodeReviewGate::review_file(const std::string& file_path) {
    std::ifstream f(file_path);
    if (!f) {
        ReviewResult result;
        result.passed = false;
        result.summary = "Could not read file: " + file_path;
        return result;
    }

    std::ostringstream ss;
    ss << f.rdbuf();

    std::string language = "cpp";
    size_t dot = file_path.rfind('.');
    if (dot != std::string::npos) {
        language = file_path.substr(dot + 1);
    }

    return review(ss.str(), language);
}

std::vector<ReviewResult> CodeReviewGate::review_files(const std::vector<std::string>& file_paths) {
    std::vector<ReviewResult> results;
    results.reserve(file_paths.size());

    for (const auto& path : file_paths) {
        results.push_back(review_file(path));
    }

    return results;
}

std::string CodeReviewGate::submit_for_approval(const ReviewResult& result, const std::string& context) {
    std::lock_guard<std::mutex> lock(mutex_);

    std::string id = "REV_" + std::to_string(next_review_id_++);

    ReviewResult pending = result;
    pending.requires_approval = true;
    pending.approved = false;

    pending_reviews_[id] = pending;

    EventBus::push("REVIEW_SUBMITTED",
        "id=" + id + "|context=" + context +
        "|risk=" + std::string(risk_level_name(result.overall_risk)));

    if (notify_callback_) {
        notify_callback_(id, pending);
    }

    return id;
}

bool CodeReviewGate::approve(const std::string& review_id, const std::string& approver,
                              const std::string& comment) {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = pending_reviews_.find(review_id);
    if (it == pending_reviews_.end()) {
        return false;
    }

    it->second.approved = true;
    it->second.approver = approver;
    it->second.approval_comment = comment;

    EventBus::push("REVIEW_APPROVED",
        "id=" + review_id + "|approver=" + approver);

    return true;
}

bool CodeReviewGate::reject(const std::string& review_id, const std::string& approver,
                             const std::string& reason) {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = pending_reviews_.find(review_id);
    if (it == pending_reviews_.end()) {
        return false;
    }

    it->second.approved = false;
    it->second.passed = false;
    it->second.approver = approver;
    it->second.approval_comment = "REJECTED: " + reason;

    EventBus::push("REVIEW_REJECTED",
        "id=" + review_id + "|approver=" + approver + "|reason=" + reason);

    return true;
}

ReviewResult CodeReviewGate::get_review(const std::string& review_id) const {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = pending_reviews_.find(review_id);
    if (it != pending_reviews_.end()) {
        return it->second;
    }
    return ReviewResult{};
}

std::vector<std::string> CodeReviewGate::pending_reviews() const {
    std::lock_guard<std::mutex> lock(mutex_);

    std::vector<std::string> ids;
    for (const auto& kv : pending_reviews_) {
        if (!kv.second.approved && kv.second.requires_approval) {
            ids.push_back(kv.first);
        }
    }
    return ids;
}

ReviewResult CodeReviewGate::create_result(const std::vector<CodeIssue>& issues) {
    ReviewResult result;
    result.issues = issues;
    result.issues_found = issues.size();

    // Zähle nach Risiko
    for (const auto& issue : issues) {
        switch (issue.risk) {
            case RiskLevel::CRITICAL: result.critical_count++; break;
            case RiskLevel::HIGH:     result.high_count++; break;
            case RiskLevel::MEDIUM:   result.medium_count++; break;
            case RiskLevel::LOW:      result.low_count++; break;
            default: break;
        }
    }

    // Bestimme Gesamt-Risiko
    if (result.critical_count > 0) {
        result.overall_risk = RiskLevel::CRITICAL;
    } else if (result.high_count > 0) {
        result.overall_risk = RiskLevel::HIGH;
    } else if (result.medium_count > 0) {
        result.overall_risk = RiskLevel::MEDIUM;
    } else if (result.low_count > 0) {
        result.overall_risk = RiskLevel::LOW;
    } else {
        result.overall_risk = RiskLevel::NONE;
    }

    // Review bestanden wenn kein kritisches/hohes Risiko
    result.passed = result.critical_count == 0 && result.high_count == 0;

    return result;
}

std::string CodeReviewGate::generate_summary(const ReviewResult& result) {
    std::ostringstream ss;

    ss << "Code Review: ";

    if (result.passed) {
        ss << "PASSED";
    } else {
        ss << "FAILED";
    }

    ss << " (Risk: " << risk_level_name(result.overall_risk) << ")\n";
    ss << "Issues found: " << result.issues_found << "\n";

    if (result.critical_count > 0)
        ss << "  - Critical: " << result.critical_count << "\n";
    if (result.high_count > 0)
        ss << "  - High: " << result.high_count << "\n";
    if (result.medium_count > 0)
        ss << "  - Medium: " << result.medium_count << "\n";
    if (result.low_count > 0)
        ss << "  - Low: " << result.low_count << "\n";

    if (result.requires_approval) {
        ss << "\n** Requires human approval before proceeding **\n";
    }

    return ss.str();
}

} // namespace rael
