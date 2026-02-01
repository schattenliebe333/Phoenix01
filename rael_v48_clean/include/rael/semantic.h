#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include <functional>
#include <cstdint>

namespace rael {

// ═══════════════════════════════════════════════════════════════════════════
// SEMANTIC RESULT (Legacy)
// ═══════════════════════════════════════════════════════════════════════════
struct SemanticResult {
    std::string intention;
    std::string meaning;
    double coherence = 0.0; // 0..1
};

// ═══════════════════════════════════════════════════════════════════════════
// ACTION SPEC (Verbesserung #2: Freie Sprache → Plan)
// ═══════════════════════════════════════════════════════════════════════════
// Strukturierte Repräsentation einer erkannten Absicht

enum class IntentType : uint8_t {
    UNKNOWN     = 0,
    QUERY       = 1,   // Frage stellen
    COMMAND     = 2,   // Befehl ausführen
    SETTING     = 3,   // Einstellung ändern
    NAVIGATION  = 4,   // Navigation (help, quit, etc.)
    ANALYSIS    = 5,   // Analyse anfordern
    CREATION    = 6,   // Etwas erstellen
    MODIFICATION= 7,   // Etwas ändern
    DELETION    = 8,   // Etwas löschen
    COMPARISON  = 9,   // Vergleichen
    EXPLANATION = 10   // Erklärung anfordern
};

inline const char* intent_type_name(IntentType t) {
    switch (t) {
        case IntentType::QUERY:       return "QUERY";
        case IntentType::COMMAND:     return "COMMAND";
        case IntentType::SETTING:     return "SETTING";
        case IntentType::NAVIGATION:  return "NAVIGATION";
        case IntentType::ANALYSIS:    return "ANALYSIS";
        case IntentType::CREATION:    return "CREATION";
        case IntentType::MODIFICATION:return "MODIFICATION";
        case IntentType::DELETION:    return "DELETION";
        case IntentType::COMPARISON:  return "COMPARISON";
        case IntentType::EXPLANATION: return "EXPLANATION";
        default:                      return "UNKNOWN";
    }
}

// Constraint für eine Aktion
struct ActionConstraint {
    std::string name;
    std::string value;
    bool required = false;
    double confidence = 0.0;
};

// Ein Schritt im Aktionsplan
struct ActionStep {
    std::string action;              // Was tun
    std::string target;              // Worauf anwenden
    std::vector<ActionConstraint> constraints;
    int priority = 5;                // 1-10, höher = wichtiger
    double estimated_risk = 0.0;     // Geschätztes Risiko
};

// Vollständige Aktionsspezifikation
struct ActionSpec {
    // Erkannte Absicht
    IntentType intent = IntentType::UNKNOWN;
    double intent_confidence = 0.0;

    // Hauptziel
    std::string goal;

    // Extrahierte Entitäten
    std::unordered_map<std::string, std::string> entities;

    // Constraints
    std::vector<ActionConstraint> constraints;

    // Geplante Schritte
    std::vector<ActionStep> steps;

    // Alternative Interpretationen (Verbesserung #3: Wege 1/2/3)
    std::vector<ActionSpec> alternatives;

    // Meta-Information
    std::string original_input;
    double overall_confidence = 0.0;
    bool requires_confirmation = false;
    std::string confirmation_question;

    // Helper
    bool is_valid() const { return intent != IntentType::UNKNOWN && overall_confidence > 0.3; }
    bool has_alternatives() const { return !alternatives.empty(); }
};

// ═══════════════════════════════════════════════════════════════════════════
// INTENT ENGINE (Verbesserung #3: Absichtserkennung)
// ═══════════════════════════════════════════════════════════════════════════

struct IntentPattern {
    std::string name;
    IntentType type;
    std::vector<std::string> keywords;      // Schlüsselwörter
    std::vector<std::string> patterns;      // Regex-ähnliche Muster
    double base_confidence;
};

class IntentEngine {
public:
    IntentEngine();

    // Erkenne Intent aus Eingabe
    IntentType recognize(const std::string& input, double& confidence) const;

    // Füge neues Pattern hinzu
    void add_pattern(const IntentPattern& pattern);

    // Extrahiere Entitäten aus Eingabe
    std::unordered_map<std::string, std::string> extract_entities(
        const std::string& input, IntentType intent) const;

private:
    std::vector<IntentPattern> patterns_;
};

// ═══════════════════════════════════════════════════════════════════════════
// SEMANTIC ENGINE (Erweitert)
// ═══════════════════════════════════════════════════════════════════════════

class SemanticEngine {
public:
    SemanticEngine();

    // Legacy-Methode
    SemanticResult interpret(const std::string& input);

    // NEU: ActionSpec-basierte Interpretation (Verbesserung #2)
    ActionSpec parse_action(const std::string& input);

    // NEU: Generiere alternative Interpretationen (Verbesserung #3)
    std::vector<ActionSpec> generate_alternatives(const ActionSpec& primary, int max_alternatives = 3);

    // NEU: Validiere ActionSpec gegen Constraints
    bool validate_action(const ActionSpec& spec, std::string& error) const;

    // NEU: Konvertiere ActionSpec zu ausführbaren Schritten
    std::vector<ActionStep> plan_execution(const ActionSpec& spec) const;

private:
    IntentEngine intent_engine_;

    // Helper für ActionSpec-Parsing
    void extract_goal(ActionSpec& spec, const std::string& input);
    void extract_constraints(ActionSpec& spec, const std::string& input);
    void generate_steps(ActionSpec& spec);
    double calculate_confidence(const ActionSpec& spec);
};

// ═══════════════════════════════════════════════════════════════════════════
// RESONANCE INTENT ENGINE (Verbesserung #3: Absicht→Wege 1/2/3)
// ═══════════════════════════════════════════════════════════════════════════
// Kombiniert Absichtserkennung mit Resonanz-basierter Pfad-Generierung

struct IntentPath {
    int path_number;          // Weg 1, 2 oder 3
    std::string description;
    ActionSpec spec;
    double resonance_score;   // Wie gut "resoniert" dieser Pfad mit der Absicht
    double risk_score;
    double efficiency_score;
};

class ResonanceIntentEngine {
public:
    ResonanceIntentEngine();

    // Generiere 3 alternative Wege für eine Absicht
    std::vector<IntentPath> generate_paths(const std::string& input);

    // Wähle besten Pfad basierend auf Kriterien
    IntentPath select_best_path(const std::vector<IntentPath>& paths,
                                bool prefer_safety = true,
                                bool prefer_efficiency = false) const;

    // Berechne Resonanz-Score zwischen Absicht und Pfad
    double calculate_resonance(const ActionSpec& intent, const ActionSpec& path) const;

private:
    SemanticEngine semantic_;

    // Generiere konservativen Pfad (sicher, bewährt)
    IntentPath generate_conservative_path(const ActionSpec& base);

    // Generiere balancierten Pfad (ausgewogen)
    IntentPath generate_balanced_path(const ActionSpec& base);

    // Generiere aggressiven Pfad (schnell, riskanter)
    IntentPath generate_aggressive_path(const ActionSpec& base);
};

// Global Engines
extern ResonanceIntentEngine gResonanceIntent;

} // namespace rael
