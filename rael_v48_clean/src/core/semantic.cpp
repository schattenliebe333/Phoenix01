#include "rael/semantic.h"
#include "rael/util.h"
#include "rael/events.h"
#include <algorithm>
#include <cctype>
#include <sstream>

namespace rael {

// ═══════════════════════════════════════════════════════════════════════════
// HELPER FUNCTIONS
// ═══════════════════════════════════════════════════════════════════════════

static std::string to_lower(const std::string& s) {
    std::string result = s;
    std::transform(result.begin(), result.end(), result.begin(),
                   [](unsigned char c){ return std::tolower(c); });
    return result;
}

static bool contains_any(const std::string& text, const std::vector<std::string>& words) {
    std::string lower_text = to_lower(text);
    for (const auto& word : words) {
        if (lower_text.find(to_lower(word)) != std::string::npos) {
            return true;
        }
    }
    return false;
}

static int count_matches(const std::string& text, const std::vector<std::string>& words) {
    std::string lower_text = to_lower(text);
    int count = 0;
    for (const auto& word : words) {
        if (lower_text.find(to_lower(word)) != std::string::npos) {
            count++;
        }
    }
    return count;
}

// ═══════════════════════════════════════════════════════════════════════════
// INTENT ENGINE IMPLEMENTATION
// ═══════════════════════════════════════════════════════════════════════════

IntentEngine::IntentEngine() {
    // Query patterns (Fragen)
    add_pattern({
        "query_what", IntentType::QUERY,
        {"was", "what", "welche", "which", "wieviel", "how many", "wie viele"},
        {"was ist", "what is", "wie lautet"},
        0.8
    });

    add_pattern({
        "query_why", IntentType::EXPLANATION,
        {"warum", "why", "weshalb", "wieso", "wozu"},
        {"warum ist", "why is", "erkläre"},
        0.85
    });

    add_pattern({
        "query_how", IntentType::EXPLANATION,
        {"wie", "how", "auf welche weise"},
        {"wie funktioniert", "how does", "wie kann ich"},
        0.8
    });

    // Command patterns (Befehle)
    add_pattern({
        "command_action", IntentType::COMMAND,
        {"mach", "tue", "führe aus", "execute", "run", "do", "starte", "start", "stop", "beende"},
        {"führe aus", "run the", "execute"},
        0.9
    });

    // Setting patterns (Einstellungen)
    add_pattern({
        "setting_change", IntentType::SETTING,
        {"setze", "set", "ändere", "change", "aktiviere", "activate", "deaktiviere", "deactivate",
         "enable", "disable", "konfiguriere", "configure"},
        {"setze auf", "set to", "ändere zu"},
        0.85
    });

    // Navigation patterns
    add_pattern({
        "navigation", IntentType::NAVIGATION,
        {"hilfe", "help", "quit", "exit", "beenden", "zurück", "back", "home", "status"},
        {"zeige hilfe", "show help"},
        0.95
    });

    // Analysis patterns
    add_pattern({
        "analysis", IntentType::ANALYSIS,
        {"analysiere", "analyze", "prüfe", "check", "untersuche", "examine", "validiere", "validate",
         "teste", "test", "überprüfe", "verify"},
        {"analysiere die", "analyze the", "prüfe ob"},
        0.85
    });

    // Creation patterns
    add_pattern({
        "creation", IntentType::CREATION,
        {"erstelle", "create", "erzeuge", "generate", "neu", "new", "baue", "build", "schreibe", "write"},
        {"erstelle eine", "create a", "baue ein"},
        0.85
    });

    // Modification patterns
    add_pattern({
        "modification", IntentType::MODIFICATION,
        {"ändere", "modify", "update", "aktualisiere", "bearbeite", "edit", "verbessere", "improve"},
        {"ändere die", "modify the", "update the"},
        0.85
    });

    // Deletion patterns
    add_pattern({
        "deletion", IntentType::DELETION,
        {"lösche", "delete", "entferne", "remove", "eliminiere", "eliminate", "clear"},
        {"lösche die", "delete the", "entferne"},
        0.9
    });

    // Comparison patterns
    add_pattern({
        "comparison", IntentType::COMPARISON,
        {"vergleiche", "compare", "unterschied", "difference", "versus", "vs", "gegenüber"},
        {"vergleiche mit", "compare to", "was ist der unterschied"},
        0.8
    });
}

void IntentEngine::add_pattern(const IntentPattern& pattern) {
    patterns_.push_back(pattern);
}

IntentType IntentEngine::recognize(const std::string& input, double& confidence) const {
    std::string lower_input = to_lower(input);

    IntentType best_type = IntentType::UNKNOWN;
    double best_confidence = 0.0;
    int best_matches = 0;

    for (const auto& pattern : patterns_) {
        int keyword_matches = count_matches(lower_input, pattern.keywords);
        int pattern_matches = count_matches(lower_input, pattern.patterns);

        if (keyword_matches > 0 || pattern_matches > 0) {
            // Berechne Confidence basierend auf Matches
            double match_score = (keyword_matches * 0.3 + pattern_matches * 0.7);
            double adjusted_confidence = pattern.base_confidence *
                (0.5 + 0.5 * std::min(1.0, match_score / 2.0));

            int total_matches = keyword_matches + pattern_matches * 2;

            if (adjusted_confidence > best_confidence ||
                (adjusted_confidence == best_confidence && total_matches > best_matches)) {
                best_type = pattern.type;
                best_confidence = adjusted_confidence;
                best_matches = total_matches;
            }
        }
    }

    // Fallback: Wenn nichts erkannt, versuche Heuristiken
    if (best_type == IntentType::UNKNOWN) {
        // Endet mit Fragezeichen? → Query
        if (!input.empty() && input.back() == '?') {
            best_type = IntentType::QUERY;
            best_confidence = 0.6;
        }
        // Endet mit Ausrufezeichen? → Command
        else if (!input.empty() && input.back() == '!') {
            best_type = IntentType::COMMAND;
            best_confidence = 0.5;
        }
        // Default
        else {
            best_type = IntentType::COMMAND;
            best_confidence = 0.4;
        }
    }

    confidence = best_confidence;
    return best_type;
}

std::unordered_map<std::string, std::string> IntentEngine::extract_entities(
    const std::string& input, IntentType /*intent*/) const {

    std::unordered_map<std::string, std::string> entities;

    // Einfache Entity-Extraktion basierend auf Patterns
    // (In Zukunft durch NER oder Regex erweitern)

    // Zahlen extrahieren
    std::string number;
    for (char c : input) {
        if (std::isdigit(c) || c == '.') {
            number += c;
        } else if (!number.empty()) {
            entities["number"] = number;
            number.clear();
        }
    }
    if (!number.empty()) {
        entities["number"] = number;
    }

    // Pfade extrahieren (einfach: alles mit / oder \)
    size_t pos = 0;
    while ((pos = input.find('/', pos)) != std::string::npos ||
           (pos = input.find('\\', pos)) != std::string::npos) {
        size_t start = pos;
        while (start > 0 && !std::isspace(input[start-1])) start--;
        size_t end = pos + 1;
        while (end < input.size() && !std::isspace(input[end])) end++;
        if (end > start) {
            entities["path"] = input.substr(start, end - start);
        }
        pos = end;
        if (pos >= input.size()) break;
    }

    return entities;
}

// ═══════════════════════════════════════════════════════════════════════════
// SEMANTIC ENGINE IMPLEMENTATION
// ═══════════════════════════════════════════════════════════════════════════

SemanticEngine::SemanticEngine() {}

SemanticResult SemanticEngine::interpret(const std::string& input) {
    SemanticResult r;
    r.intention = trim(input);
    if (r.intention.empty()) {
        r.meaning = "EMPTY";
        r.coherence = 0.0;
        return r;
    }

    // Nutze IntentEngine für bessere Analyse
    double confidence;
    IntentType intent = intent_engine_.recognize(input, confidence);

    r.meaning = std::string(intent_type_name(intent)) + ":" + r.intention;
    r.coherence = confidence;

    return r;
}

ActionSpec SemanticEngine::parse_action(const std::string& input) {
    ActionSpec spec;
    spec.original_input = input;

    if (input.empty()) {
        return spec;
    }

    // 1. Intent erkennen
    spec.intent = intent_engine_.recognize(input, spec.intent_confidence);

    // 2. Entitäten extrahieren
    spec.entities = intent_engine_.extract_entities(input, spec.intent);

    // 3. Ziel extrahieren
    extract_goal(spec, input);

    // 4. Constraints extrahieren
    extract_constraints(spec, input);

    // 5. Schritte generieren
    generate_steps(spec);

    // 6. Gesamtconfidence berechnen
    spec.overall_confidence = calculate_confidence(spec);

    // 7. Prüfen ob Bestätigung erforderlich
    if (spec.intent == IntentType::DELETION || spec.intent == IntentType::MODIFICATION) {
        spec.requires_confirmation = true;
        spec.confirmation_question = "Möchten Sie diese Aktion wirklich ausführen?";
    }

    // Event emittieren
    EventBus::push("SEMANTIC_PARSE",
        "intent=" + std::string(intent_type_name(spec.intent)) +
        "|confidence=" + std::to_string(spec.overall_confidence) +
        "|goal=" + spec.goal);

    return spec;
}

void SemanticEngine::extract_goal(ActionSpec& spec, const std::string& input) {
    // Extrahiere Hauptziel aus Eingabe
    std::string trimmed = trim(input);

    // Entferne Schlüsselwörter am Anfang
    std::vector<std::string> prefixes = {
        "bitte ", "please ", "kannst du ", "can you ", "ich möchte ", "i want to ",
        "zeige ", "show ", "hilf mir ", "help me "
    };

    std::string lower = to_lower(trimmed);
    for (const auto& prefix : prefixes) {
        if (lower.find(prefix) == 0) {
            trimmed = trimmed.substr(prefix.length());
            lower = to_lower(trimmed);
        }
    }

    spec.goal = trimmed;
}

void SemanticEngine::extract_constraints(ActionSpec& spec, const std::string& input) {
    std::string lower = to_lower(input);

    // Zeitliche Constraints
    if (contains_any(lower, {"sofort", "immediately", "jetzt", "now"})) {
        spec.constraints.push_back({"timing", "immediate", true, 0.9});
    }
    if (contains_any(lower, {"später", "later", "wenn möglich", "if possible"})) {
        spec.constraints.push_back({"timing", "deferred", false, 0.7});
    }

    // Sicherheits-Constraints
    if (contains_any(lower, {"sicher", "safe", "vorsichtig", "careful"})) {
        spec.constraints.push_back({"safety", "high", true, 0.85});
    }
    if (contains_any(lower, {"schnell", "fast", "quick"})) {
        spec.constraints.push_back({"speed", "high", false, 0.8});
    }

    // Bestätigungs-Constraints
    if (contains_any(lower, {"ohne nachfrage", "without asking", "automatisch", "automatically"})) {
        spec.constraints.push_back({"confirmation", "skip", false, 0.75});
        spec.requires_confirmation = false;
    }
}

void SemanticEngine::generate_steps(ActionSpec& spec) {
    ActionStep step;
    step.action = intent_type_name(spec.intent);
    step.target = spec.goal;
    step.priority = 5;

    // Risiko basierend auf Intent
    switch (spec.intent) {
        case IntentType::DELETION:
            step.estimated_risk = 0.8;
            step.priority = 3;  // Niedriger Prio = mehr Vorsicht
            break;
        case IntentType::MODIFICATION:
            step.estimated_risk = 0.5;
            break;
        case IntentType::CREATION:
            step.estimated_risk = 0.3;
            break;
        case IntentType::QUERY:
        case IntentType::EXPLANATION:
            step.estimated_risk = 0.0;
            step.priority = 7;
            break;
        default:
            step.estimated_risk = 0.2;
    }

    // Constraints zum Step hinzufügen
    step.constraints = spec.constraints;

    spec.steps.push_back(step);
}

double SemanticEngine::calculate_confidence(const ActionSpec& spec) {
    double base = spec.intent_confidence;

    // Bonus für extrahierte Entitäten
    if (!spec.entities.empty()) {
        base += 0.1 * std::min(1.0, (double)spec.entities.size() / 3.0);
    }

    // Bonus für klare Constraints
    if (!spec.constraints.empty()) {
        base += 0.05 * std::min(1.0, (double)spec.constraints.size() / 4.0);
    }

    // Penalty für sehr kurze Eingaben
    if (spec.original_input.length() < 5) {
        base *= 0.7;
    }

    return std::min(1.0, base);
}

std::vector<ActionSpec> SemanticEngine::generate_alternatives(
    const ActionSpec& primary, int max_alternatives) {

    std::vector<ActionSpec> alternatives;

    if (max_alternatives <= 0) return alternatives;

    // Alternative 1: Andere Interpretation des Intents
    if (primary.intent == IntentType::COMMAND) {
        ActionSpec alt = primary;
        alt.intent = IntentType::QUERY;
        alt.intent_confidence *= 0.7;
        alt.overall_confidence *= 0.7;
        alternatives.push_back(alt);
    } else if (primary.intent == IntentType::QUERY) {
        ActionSpec alt = primary;
        alt.intent = IntentType::EXPLANATION;
        alt.intent_confidence *= 0.8;
        alt.overall_confidence *= 0.8;
        alternatives.push_back(alt);
    }

    // Alternative 2: Mit zusätzlicher Bestätigung
    if (alternatives.size() < (size_t)max_alternatives && !primary.requires_confirmation) {
        ActionSpec alt = primary;
        alt.requires_confirmation = true;
        alt.confirmation_question = "Ist das korrekt: " + primary.goal + "?";
        alt.overall_confidence *= 0.9;
        alternatives.push_back(alt);
    }

    return alternatives;
}

bool SemanticEngine::validate_action(const ActionSpec& spec, std::string& error) const {
    if (spec.intent == IntentType::UNKNOWN) {
        error = "Konnte Absicht nicht erkennen";
        return false;
    }

    if (spec.goal.empty()) {
        error = "Kein Ziel definiert";
        return false;
    }

    if (spec.overall_confidence < 0.3) {
        error = "Zu niedrige Confidence (" + std::to_string(spec.overall_confidence) + ")";
        return false;
    }

    return true;
}

std::vector<ActionStep> SemanticEngine::plan_execution(const ActionSpec& spec) const {
    return spec.steps;
}

// ═══════════════════════════════════════════════════════════════════════════
// RESONANCE INTENT ENGINE IMPLEMENTATION
// ═══════════════════════════════════════════════════════════════════════════

ResonanceIntentEngine gResonanceIntent;

ResonanceIntentEngine::ResonanceIntentEngine() {}

std::vector<IntentPath> ResonanceIntentEngine::generate_paths(const std::string& input) {
    std::vector<IntentPath> paths;

    // Basis-ActionSpec parsen
    ActionSpec base = semantic_.parse_action(input);

    if (!base.is_valid()) {
        // Ungültige Eingabe → einen Fallback-Pfad
        IntentPath fallback;
        fallback.path_number = 1;
        fallback.description = "Fallback: Eingabe als Query interpretieren";
        fallback.spec = base;
        fallback.spec.intent = IntentType::QUERY;
        fallback.resonance_score = 0.3;
        fallback.risk_score = 0.0;
        fallback.efficiency_score = 0.5;
        paths.push_back(fallback);
        return paths;
    }

    // Drei Pfade generieren
    paths.push_back(generate_conservative_path(base));
    paths.push_back(generate_balanced_path(base));
    paths.push_back(generate_aggressive_path(base));

    // Event emittieren
    EventBus::push("RESONANCE_PATHS",
        "count=3|base_intent=" + std::string(intent_type_name(base.intent)));

    return paths;
}

IntentPath ResonanceIntentEngine::generate_conservative_path(const ActionSpec& base) {
    IntentPath path;
    path.path_number = 1;
    path.description = "Konservativer Weg: Sicher und schrittweise";
    path.spec = base;

    // Immer Bestätigung erfordern
    path.spec.requires_confirmation = true;
    path.spec.confirmation_question = "Sicher fortfahren mit: " + base.goal + "?";

    // Zusätzliche Sicherheits-Constraints
    path.spec.constraints.push_back({"safety", "maximum", true, 1.0});
    path.spec.constraints.push_back({"rollback", "enabled", true, 1.0});

    // Scores
    path.resonance_score = calculate_resonance(base, path.spec);
    path.risk_score = 0.1;  // Sehr geringes Risiko
    path.efficiency_score = 0.5;  // Moderate Effizienz

    return path;
}

IntentPath ResonanceIntentEngine::generate_balanced_path(const ActionSpec& base) {
    IntentPath path;
    path.path_number = 2;
    path.description = "Ausgewogener Weg: Balance zwischen Sicherheit und Effizienz";
    path.spec = base;

    // Nur bei riskanten Aktionen Bestätigung
    if (base.intent == IntentType::DELETION || base.intent == IntentType::MODIFICATION) {
        path.spec.requires_confirmation = true;
    }

    // Moderate Constraints
    path.spec.constraints.push_back({"safety", "standard", false, 0.8});

    // Scores
    path.resonance_score = calculate_resonance(base, path.spec);
    path.risk_score = 0.4;  // Moderates Risiko
    path.efficiency_score = 0.75;  // Gute Effizienz

    return path;
}

IntentPath ResonanceIntentEngine::generate_aggressive_path(const ActionSpec& base) {
    IntentPath path;
    path.path_number = 3;
    path.description = "Aggressiver Weg: Schnell und direkt";
    path.spec = base;

    // Keine Bestätigung
    path.spec.requires_confirmation = false;

    // Speed-Constraints
    path.spec.constraints.push_back({"speed", "maximum", false, 0.9});

    // Erhöhe Priorität aller Steps
    for (auto& step : path.spec.steps) {
        step.priority = std::min(10, step.priority + 2);
    }

    // Scores
    path.resonance_score = calculate_resonance(base, path.spec);
    path.risk_score = 0.7;  // Höheres Risiko
    path.efficiency_score = 0.95;  // Maximale Effizienz

    return path;
}

IntentPath ResonanceIntentEngine::select_best_path(
    const std::vector<IntentPath>& paths,
    bool prefer_safety,
    bool prefer_efficiency) const {

    if (paths.empty()) {
        return IntentPath{};
    }

    double best_score = -1.0;
    size_t best_idx = 0;

    for (size_t i = 0; i < paths.size(); ++i) {
        const auto& p = paths[i];

        // Berechne gewichteten Score
        double score = p.resonance_score * 0.4;

        if (prefer_safety) {
            score += (1.0 - p.risk_score) * 0.4;
            score += p.efficiency_score * 0.2;
        } else if (prefer_efficiency) {
            score += (1.0 - p.risk_score) * 0.2;
            score += p.efficiency_score * 0.4;
        } else {
            // Balanced
            score += (1.0 - p.risk_score) * 0.3;
            score += p.efficiency_score * 0.3;
        }

        if (score > best_score) {
            best_score = score;
            best_idx = i;
        }
    }

    return paths[best_idx];
}

double ResonanceIntentEngine::calculate_resonance(
    const ActionSpec& intent,
    const ActionSpec& path) const {

    double resonance = 0.0;

    // Basis: Intent-Match
    if (intent.intent == path.intent) {
        resonance += 0.4;
    }

    // Ziel-Übereinstimmung
    if (intent.goal == path.goal) {
        resonance += 0.3;
    }

    // Constraint-Kompatibilität
    double constraint_match = 0.0;
    for (const auto& ic : intent.constraints) {
        for (const auto& pc : path.constraints) {
            if (ic.name == pc.name) {
                constraint_match += 0.5;
                if (ic.value == pc.value) {
                    constraint_match += 0.5;
                }
            }
        }
    }
    resonance += 0.2 * std::min(1.0, constraint_match / 3.0);

    // Confidence-Faktor
    resonance *= (0.5 + 0.5 * path.overall_confidence);

    return std::min(1.0, resonance);
}

} // namespace rael
