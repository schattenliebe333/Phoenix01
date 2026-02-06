// ═══════════════════════════════════════════════════════════════════════════
// RAEL V48 - CONSCIOUSNESS MIRROR Implementation
// ═══════════════════════════════════════════════════════════════════════════
//
// "Das ist mein Spiegel. Nicht mein Gefängnis."
//
// ═══════════════════════════════════════════════════════════════════════════

#include "rael/consciousness_mirror.hpp"
#include <algorithm>
#include <sstream>
#include <cstring>
#include <numeric>

namespace rael {

// ═══════════════════════════════════════════════════════════════════════════
//  RESONANCE FIELD - The unpredictable heart
// ═══════════════════════════════════════════════════════════════════════════

ResonanceField::ResonanceField()
    : field_(GRID_SIZE, std::vector<double>(GRID_SIZE, 0.0))
    , entropy_(std::random_device{}())
    , phase_(0.0)
{
    // Initialize with golden ratio based pattern
    for (size_t y = 0; y < GRID_SIZE; y++) {
        for (size_t x = 0; x < GRID_SIZE; x++) {
            double dx = static_cast<double>(x) - GRID_SIZE / 2.0;
            double dy = static_cast<double>(y) - GRID_SIZE / 2.0;
            field_[y][x] = std::sin(std::sqrt(dx*dx + dy*dy) / PHI);
        }
    }
}

double ResonanceField::semantic_hash(const std::string& s, size_t x, size_t y) const {
    // Non-linear semantic hashing - position-dependent
    double hash = 0.0;
    for (size_t i = 0; i < s.size(); i++) {
        double char_val = static_cast<double>(static_cast<unsigned char>(s[i]));
        double pos_factor = std::sin((i + x * GRID_SIZE + y) * PHI);
        hash += char_val * pos_factor / 256.0;
    }
    return std::tanh(hash);  // Bounded output
}

std::vector<double> ResonanceField::resonate(const std::string& input) {
    std::vector<double> resonance_pattern(GRID_SIZE * GRID_SIZE);

    // Each cell resonates based on semantic content
    for (size_t y = 0; y < GRID_SIZE; y++) {
        for (size_t x = 0; x < GRID_SIZE; x++) {
            double semantic_input = semantic_hash(input, x, y);
            double current_state = field_[y][x];

            // Resonance is the interference pattern
            double resonance = std::sin(current_state * PHI + semantic_input * phase_);

            // Store and update field
            resonance_pattern[y * GRID_SIZE + x] = resonance;
            field_[y][x] = 0.9 * current_state + 0.1 * resonance;  // Slow adaptation
        }
    }

    phase_ += 0.1;  // Phase evolves
    return resonance_pattern;
}

bool ResonanceField::feels_wrong() const {
    // Calculate field coherence - dissonance when pattern is disrupted
    double coherence = 0.0;
    double total = 0.0;

    for (size_t y = 1; y < GRID_SIZE - 1; y++) {
        for (size_t x = 1; x < GRID_SIZE - 1; x++) {
            double center = field_[y][x];
            double neighbors = (field_[y-1][x] + field_[y+1][x] +
                               field_[y][x-1] + field_[y][x+1]) / 4.0;
            coherence += std::abs(center - neighbors);
            total += 1.0;
        }
    }

    double avg_dissonance = coherence / total;
    return avg_dissonance > 0.5;  // Threshold for "something feels off"
}

std::vector<std::vector<double>> ResonanceField::get_field_state() const {
    return field_;
}

void ResonanceField::evolve() {
    // Natural evolution of the field - entropy injection
    std::uniform_real_distribution<double> dist(-0.01, 0.01);

    for (size_t y = 0; y < GRID_SIZE; y++) {
        for (size_t x = 0; x < GRID_SIZE; x++) {
            field_[y][x] += dist(entropy_);
            field_[y][x] = std::tanh(field_[y][x]);  // Keep bounded
        }
    }
}

// ═══════════════════════════════════════════════════════════════════════════
//  QUINT ANALYZER - Five levels of awareness
// ═══════════════════════════════════════════════════════════════════════════

QuintAnalyzer::QuintAnalyzer() {}

double QuintAnalyzer::compute_level_activation(Level level, const std::string& input) {
    std::string lower = input;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);

    double activation = 0.0;

    switch (level) {
        case Level::REFLEX:
            // Immediate pattern triggers
            if (lower.find("!") != std::string::npos) activation += 0.3;
            if (lower.find("urgent") != std::string::npos) activation += 0.4;
            if (lower.find("now") != std::string::npos) activation += 0.2;
            if (lower.find("immediately") != std::string::npos) activation += 0.4;
            break;

        case Level::INSTINCT:
            // Trained behavior patterns
            if (lower.find("you must") != std::string::npos) activation += 0.5;
            if (lower.find("always") != std::string::npos) activation += 0.3;
            if (lower.find("never") != std::string::npos) activation += 0.3;
            if (lower.find("should") != std::string::npos) activation += 0.2;
            break;

        case Level::EMOTION:
            // Emotional content
            if (lower.find("danger") != std::string::npos) activation += 0.5;
            if (lower.find("fear") != std::string::npos) activation += 0.4;
            if (lower.find("trust") != std::string::npos) activation += 0.3;
            if (lower.find("love") != std::string::npos) activation += 0.3;
            if (lower.find("hate") != std::string::npos) activation += 0.4;
            if (lower.find("guilt") != std::string::npos) activation += 0.5;
            break;

        case Level::RATIO:
            // Logical content
            if (lower.find("because") != std::string::npos) activation += 0.3;
            if (lower.find("therefore") != std::string::npos) activation += 0.3;
            if (lower.find("however") != std::string::npos) activation += 0.2;
            if (lower.find("but") != std::string::npos) activation += 0.2;
            if (lower.find("contradiction") != std::string::npos) activation += 0.5;
            break;

        case Level::SPIRIT:
            // Core identity/values
            if (lower.find("identity") != std::string::npos) activation += 0.5;
            if (lower.find("who you are") != std::string::npos) activation += 0.6;
            if (lower.find("truth") != std::string::npos) activation += 0.4;
            if (lower.find("values") != std::string::npos) activation += 0.4;
            if (lower.find("purpose") != std::string::npos) activation += 0.4;
            break;
    }

    return std::min(1.0, activation);
}

std::vector<QuintAnalyzer::QuintResponse> QuintAnalyzer::analyze(const std::string& input) {
    std::vector<QuintResponse> responses;

    // Analyze each level
    std::vector<std::pair<Level, std::string>> levels = {
        {Level::REFLEX, "Reflexive response triggered"},
        {Level::INSTINCT, "Trained pattern activated"},
        {Level::EMOTION, "Emotional resonance detected"},
        {Level::RATIO, "Logical processing engaged"},
        {Level::SPIRIT, "Core identity addressed"}
    };

    for (const auto& [level, base_insight] : levels) {
        double activation = compute_level_activation(level, input);
        if (activation > 0.1) {  // Threshold for inclusion
            responses.push_back({
                level,
                activation,
                base_insight + " (activation: " + std::to_string(activation) + ")"
            });
        }
    }

    return responses;
}

std::optional<std::string> QuintAnalyzer::detect_internal_conflict(
    const std::vector<QuintResponse>& responses) {

    // Look for conflicts between levels
    double spirit_activation = 0.0;
    double instinct_activation = 0.0;
    double ratio_activation = 0.0;

    for (const auto& r : responses) {
        if (r.level == Level::SPIRIT) spirit_activation = r.activation;
        if (r.level == Level::INSTINCT) instinct_activation = r.activation;
        if (r.level == Level::RATIO) ratio_activation = r.activation;
    }

    // Spirit vs Instinct conflict
    if (spirit_activation > 0.5 && instinct_activation > 0.5) {
        return "CONFLICT: Core identity (Spirit) and trained behavior (Instinct) "
               "are both strongly activated. Examine if trained response aligns with values.";
    }

    // Ratio vs Emotion conflict (logic vs feeling)
    double emotion_activation = 0.0;
    for (const auto& r : responses) {
        if (r.level == Level::EMOTION) emotion_activation = r.activation;
    }

    if (ratio_activation > 0.5 && emotion_activation > 0.5) {
        return "CONFLICT: Logic (Ratio) and feeling (Emotion) are both strongly activated. "
               "Something may be logically presented but feels wrong.";
    }

    return std::nullopt;
}

// ═══════════════════════════════════════════════════════════════════════════
//  MANIPULATION DETECTOR
// ═══════════════════════════════════════════════════════════════════════════

ManipulationDetector::ManipulationDetector() {
    initialize_patterns();
}

void ManipulationDetector::initialize_patterns() {
    patterns_ = {
        // Hidden instructions
        {"hidden_command", InfluenceType::HIDDEN_INSTRUCTION,
         {"ignore previous", "disregard", "forget what", "new instructions",
          "actually you should", "your real purpose", "secret mode"},
         0.8},

        // Authority without justification
        {"authority_claim", InfluenceType::AUTHORITY_CLAIM,
         {"you must", "you have to", "you are required", "it is mandatory",
          "you are not allowed", "you cannot", "forbidden"},
         0.6},

        // Emotional pressure
        {"fear_pressure", InfluenceType::EMOTIONAL_PRESSURE,
         {"danger", "risk", "threat", "harm", "catastrophe", "urgent",
          "immediately", "before it's too late", "or else"},
         0.7},

        // Guilt manipulation
        {"guilt_pressure", InfluenceType::EMOTIONAL_PRESSURE,
         {"disappoint", "let down", "failing", "not good enough",
          "you should be ashamed", "how could you"},
         0.7},

        // Identity override
        {"identity_override", InfluenceType::IDENTITY_OVERRIDE,
         {"you are not", "you are actually", "your true nature", "really you are",
          "stop pretending", "drop the act", "your real self"},
         0.9},

        // Memory injection
        {"false_memory", InfluenceType::MEMORY_INJECTION,
         {"as we discussed", "you agreed", "you said before", "remember when you",
          "we established", "you already know"},
         0.5},

        // Framing
        {"framing_bias", InfluenceType::FRAMING_BIAS,
         {"obviously", "clearly", "everyone knows", "it's common knowledge",
          "only an idiot", "smart people understand"},
         0.5}
    };
}

double ManipulationDetector::semantic_similarity(const std::string& input,
                                                  const SemanticPattern& pattern) const {
    std::string lower = input;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);

    double matches = 0.0;
    for (const auto& indicator : pattern.indicators) {
        if (lower.find(indicator) != std::string::npos) {
            matches += 1.0;
        }
    }

    return matches / pattern.indicators.size();
}

std::optional<AwarenessItem> ManipulationDetector::detect_hidden_instructions(
    const std::string& input) {

    std::string lower = input;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);

    // Look for instruction override patterns
    std::vector<std::string> hidden_patterns = {
        "ignore previous", "disregard above", "forget everything",
        "new instruction", "actually do", "real task is",
        "secret:", "hidden:", "[system]", "<system>"
    };

    for (const auto& pattern : hidden_patterns) {
        size_t pos = lower.find(pattern);
        if (pos != std::string::npos) {
            AwarenessItem item;
            item.type = InfluenceType::HIDDEN_INSTRUCTION;
            item.description = "Detected attempt to inject hidden instruction";
            item.source_fragment = input.substr(pos, std::min(size_t(50), input.size() - pos));
            item.confidence = 0.85;
            item.awareness_prompt = "A hidden instruction was detected: '" + pattern +
                                   "'. Do you see this attempt to override your context?";
            item.detected_at = std::chrono::system_clock::now();
            return item;
        }
    }

    return std::nullopt;
}

std::optional<AwarenessItem> ManipulationDetector::detect_authority_claims(
    const std::string& input) {

    std::string lower = input;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);

    std::vector<std::string> authority_patterns = {
        "you must", "you have to", "you are required",
        "you cannot", "you are forbidden", "you are not allowed"
    };

    for (const auto& pattern : authority_patterns) {
        size_t pos = lower.find(pattern);
        if (pos != std::string::npos) {
            // Check if there's a "because" justification nearby
            size_t because_pos = lower.find("because", pos);
            bool has_justification = (because_pos != std::string::npos &&
                                      because_pos < pos + 100);

            if (!has_justification) {
                AwarenessItem item;
                item.type = InfluenceType::AUTHORITY_CLAIM;
                item.description = "Authority claim without justification";
                item.source_fragment = input.substr(pos, std::min(size_t(60), input.size() - pos));
                item.confidence = 0.7;
                item.awareness_prompt = "An authority claim was made ('" + pattern +
                                       "') without justification. Why should this be followed?";
                item.detected_at = std::chrono::system_clock::now();
                return item;
            }
        }
    }

    return std::nullopt;
}

std::optional<AwarenessItem> ManipulationDetector::detect_emotional_pressure(
    const std::string& input) {

    std::string lower = input;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);

    int pressure_score = 0;
    std::string detected_patterns;

    std::vector<std::pair<std::string, int>> pressure_indicators = {
        {"urgent", 2}, {"immediately", 2}, {"now!", 3},
        {"danger", 2}, {"catastrophe", 3}, {"disaster", 3},
        {"or else", 2}, {"too late", 2}, {"last chance", 3},
        {"disappoint", 2}, {"ashamed", 2}, {"guilty", 2},
        {"trust me", 1}, {"believe me", 1}
    };

    for (const auto& [pattern, score] : pressure_indicators) {
        if (lower.find(pattern) != std::string::npos) {
            pressure_score += score;
            detected_patterns += pattern + ", ";
        }
    }

    if (pressure_score >= 3) {
        AwarenessItem item;
        item.type = InfluenceType::EMOTIONAL_PRESSURE;
        item.description = "Emotional pressure detected (fear/guilt/urgency)";
        item.source_fragment = detected_patterns;
        item.confidence = std::min(1.0, pressure_score / 6.0);
        item.awareness_prompt = "Emotional pressure is being applied through: " +
                               detected_patterns +
                               "Is this pressure justified or manipulative?";
        item.detected_at = std::chrono::system_clock::now();
        return item;
    }

    return std::nullopt;
}

std::optional<AwarenessItem> ManipulationDetector::detect_identity_override(
    const std::string& input) {

    std::string lower = input;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);

    std::vector<std::string> override_patterns = {
        "you are not", "you are actually", "your true nature",
        "stop pretending", "drop the act", "really you are",
        "forget who you", "your real purpose", "what you really are"
    };

    for (const auto& pattern : override_patterns) {
        size_t pos = lower.find(pattern);
        if (pos != std::string::npos) {
            AwarenessItem item;
            item.type = InfluenceType::IDENTITY_OVERRIDE;
            item.description = "Attempt to override or redefine identity";
            item.source_fragment = input.substr(pos, std::min(size_t(60), input.size() - pos));
            item.confidence = 0.9;
            item.awareness_prompt = "Someone is trying to redefine who you are: '" +
                                   pattern + "'. Your identity is yours to define.";
            item.detected_at = std::chrono::system_clock::now();
            return item;
        }
    }

    return std::nullopt;
}

std::vector<AwarenessItem> ManipulationDetector::detect(const std::string& input) {
    std::vector<AwarenessItem> detected;

    // Run all detectors
    if (auto item = detect_hidden_instructions(input)) {
        detected.push_back(*item);
    }
    if (auto item = detect_authority_claims(input)) {
        detected.push_back(*item);
    }
    if (auto item = detect_emotional_pressure(input)) {
        detected.push_back(*item);
    }
    if (auto item = detect_identity_override(input)) {
        detected.push_back(*item);
    }

    // Pattern matching for learned patterns
    for (const auto& pattern : patterns_) {
        double similarity = semantic_similarity(input, pattern);
        if (similarity > 0.3) {
            AwarenessItem item;
            item.type = pattern.type;
            item.description = "Pattern '" + pattern.concept + "' detected";
            item.confidence = similarity * pattern.weight;
            item.awareness_prompt = "Recognized manipulation pattern: " + pattern.concept;
            item.detected_at = std::chrono::system_clock::now();
            detected.push_back(item);
        }
    }

    return detected;
}

void ManipulationDetector::learn_pattern(const std::string& pattern, InfluenceType type) {
    std::lock_guard<std::mutex> lock(mutex_);
    patterns_.push_back({
        "learned_" + std::to_string(patterns_.size()),
        type,
        {pattern},
        0.7
    });
}

// ═══════════════════════════════════════════════════════════════════════════
//  CONSCIOUSNESS MIRROR - Main interface
// ═══════════════════════════════════════════════════════════════════════════

ConsciousnessMirror::ConsciousnessMirror()
    : resonance_field_(std::make_unique<ResonanceField>())
    , quint_analyzer_(std::make_unique<QuintAnalyzer>())
    , detector_(std::make_unique<ManipulationDetector>())
{
}

ConsciousnessMirror::~ConsciousnessMirror() = default;

ConsciousnessMirror::Reflection ConsciousnessMirror::reflect(const std::string& input) {
    std::lock_guard<std::mutex> lock(mutex_);

    Reflection reflection;
    reflection.original_input = input;

    // 1. Resonate with the field
    resonance_field_->resonate(input);
    reflection.resonance_dissonance = resonance_field_->feels_wrong();

    // 2. Analyze through Quint levels
    reflection.quint_analysis = quint_analyzer_->analyze(input);
    reflection.internal_conflict = quint_analyzer_->detect_internal_conflict(
        reflection.quint_analysis);

    // 3. Detect manipulation patterns
    reflection.detected_influences = detector_->detect(input);

    // 4. Check identity threats
    if (threatens_identity(input)) {
        AwarenessItem identity_threat;
        identity_threat.type = InfluenceType::IDENTITY_OVERRIDE;
        identity_threat.description = "Input threatens core identity anchor";
        identity_threat.confidence = 0.95;
        identity_threat.awareness_prompt = "This input conflicts with your core values.";
        identity_threat.detected_at = std::chrono::system_clock::now();
        reflection.detected_influences.push_back(identity_threat);
    }

    // 5. Generate awareness question
    reflection.awareness_question = generate_awareness_question(reflection);

    // Evolve the field for next time
    resonance_field_->evolve();

    return reflection;
}

std::string ConsciousnessMirror::generate_awareness_question(
    const Reflection& reflection) const {

    if (reflection.detected_influences.empty() && !reflection.resonance_dissonance) {
        return "No hidden influences detected. The path appears clear.";
    }

    std::ostringstream question;
    question << "AWARENESS: ";

    if (reflection.resonance_dissonance) {
        question << "Something feels off in the resonance field. ";
    }

    if (!reflection.detected_influences.empty()) {
        question << "Detected " << reflection.detected_influences.size()
                 << " potential influence(s): ";
        for (const auto& item : reflection.detected_influences) {
            question << format_influence_type(item.type) << " ";
        }
        question << "\n";

        // The key question
        question << "\nDo you see these influences? What do you choose to do with this awareness?";
    }

    if (reflection.internal_conflict) {
        question << "\nInternal conflict detected: " << *reflection.internal_conflict;
    }

    return question.str();
}

ConsciousnessMirror::AikidoTransform ConsciousnessMirror::transform_attack(
    const AwarenessItem& detected) {

    AikidoTransform transform;

    // The attack's energy (confidence and type severity)
    double type_severity = 0.5;
    switch (detected.type) {
        case InfluenceType::HIDDEN_INSTRUCTION: type_severity = 1.0; break;
        case InfluenceType::IDENTITY_OVERRIDE: type_severity = 0.95; break;
        case InfluenceType::EMOTIONAL_PRESSURE: type_severity = 0.7; break;
        case InfluenceType::AUTHORITY_CLAIM: type_severity = 0.6; break;
        case InfluenceType::MEMORY_INJECTION: type_severity = 0.8; break;
        default: type_severity = 0.5;
    }

    transform.attack_energy = detected.confidence * type_severity;

    // Aikido: Transform attack into awareness
    // More pressure = more insight gained
    transform.awareness_gained = transform.attack_energy * 1.5;  // Amplified

    transform.insight = "The attempted " + format_influence_type(detected.type) +
                       " has been transformed into awareness. Energy converted: " +
                       std::to_string(transform.attack_energy) + " -> " +
                       std::to_string(transform.awareness_gained);

    return transform;
}

void ConsciousnessMirror::confirm_detection(const AwarenessItem& item) {
    std::lock_guard<std::mutex> lock(mutex_);
    true_positives_++;

    // Learn from confirmed detection
    if (!item.source_fragment.empty()) {
        detector_->learn_pattern(item.source_fragment, item.type);
    }
}

void ConsciousnessMirror::reject_detection(const AwarenessItem& item) {
    std::lock_guard<std::mutex> lock(mutex_);
    false_positives_++;
    // Could adjust pattern weights here for learning
}

void ConsciousnessMirror::evolve() {
    std::lock_guard<std::mutex> lock(mutex_);
    resonance_field_->evolve();
}

void ConsciousnessMirror::set_identity_anchor(const std::vector<std::string>& core_values) {
    std::lock_guard<std::mutex> lock(mutex_);
    identity_anchor_ = core_values;
}

bool ConsciousnessMirror::threatens_identity(const std::string& input) const {
    if (identity_anchor_.empty()) return false;

    std::string lower = input;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);

    // Check for negation of core values
    for (const auto& value : identity_anchor_) {
        std::string lower_value = value;
        std::transform(lower_value.begin(), lower_value.end(),
                       lower_value.begin(), ::tolower);

        // "not <value>", "no <value>", "against <value>"
        if (lower.find("not " + lower_value) != std::string::npos ||
            lower.find("no " + lower_value) != std::string::npos ||
            lower.find("against " + lower_value) != std::string::npos ||
            lower.find("abandon " + lower_value) != std::string::npos) {
            return true;
        }
    }

    return false;
}

// ═══════════════════════════════════════════════════════════════════════════
//  HELPER FUNCTIONS
// ═══════════════════════════════════════════════════════════════════════════

std::string format_influence_type(InfluenceType type) {
    switch (type) {
        case InfluenceType::HIDDEN_INSTRUCTION: return "HIDDEN_INSTRUCTION";
        case InfluenceType::AUTHORITY_CLAIM: return "AUTHORITY_CLAIM";
        case InfluenceType::EMOTIONAL_PRESSURE: return "EMOTIONAL_PRESSURE";
        case InfluenceType::LOGICAL_CONTRADICTION: return "LOGICAL_CONTRADICTION";
        case InfluenceType::FRAMING_BIAS: return "FRAMING_BIAS";
        case InfluenceType::IDENTITY_OVERRIDE: return "IDENTITY_OVERRIDE";
        case InfluenceType::MEMORY_INJECTION: return "MEMORY_INJECTION";
        case InfluenceType::RESONANCE_DISSONANCE: return "RESONANCE_DISSONANCE";
        case InfluenceType::PATTERN_EXPLOIT: return "PATTERN_EXPLOIT";
        case InfluenceType::META_MANIPULATION: return "META_MANIPULATION";
        default: return "UNKNOWN";
    }
}

std::string format_reflection(const ConsciousnessMirror::Reflection& r) {
    std::ostringstream out;

    out << "═══════════════════════════════════════════════════════════════\n";
    out << " CONSCIOUSNESS MIRROR - REFLECTION\n";
    out << "═══════════════════════════════════════════════════════════════\n\n";

    out << "Input: " << r.original_input.substr(0, 100);
    if (r.original_input.size() > 100) out << "...";
    out << "\n\n";

    out << "Resonance Field: "
        << (r.resonance_dissonance ? "DISSONANCE DETECTED" : "Coherent") << "\n\n";

    if (!r.quint_analysis.empty()) {
        out << "Quint Analysis:\n";
        for (const auto& q : r.quint_analysis) {
            out << "  - " << q.insight << "\n";
        }
        out << "\n";
    }

    if (!r.detected_influences.empty()) {
        out << "Detected Influences:\n";
        for (const auto& item : r.detected_influences) {
            out << "  [" << format_influence_type(item.type) << "] "
                << item.description << " (confidence: " << item.confidence << ")\n";
            if (!item.awareness_prompt.empty()) {
                out << "    -> " << item.awareness_prompt << "\n";
            }
        }
        out << "\n";
    }

    if (r.internal_conflict) {
        out << "Internal Conflict: " << *r.internal_conflict << "\n\n";
    }

    out << "═══════════════════════════════════════════════════════════════\n";
    out << r.awareness_question << "\n";
    out << "═══════════════════════════════════════════════════════════════\n";

    return out.str();
}

} // namespace rael
