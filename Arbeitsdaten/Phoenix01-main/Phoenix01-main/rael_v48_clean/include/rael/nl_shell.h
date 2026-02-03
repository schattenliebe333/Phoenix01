// RAEL V49 - Natural Language Shell (#38)
// Natural language command interpretation and execution
#pragma once

#include <string>
#include <vector>
#include <map>
#include <set>
#include <memory>
#include <functional>
#include <optional>
#include <mutex>
#include <variant>
#include <regex>
#include <future>
#include <unistd.h>

namespace rael {

// ═══════════════════════════════════════════════════════════════════════════
//  COMMAND TYPES
// ═══════════════════════════════════════════════════════════════════════════

enum class CommandCategory {
    FILE_SYSTEM,     // File operations (create, delete, move, copy)
    NAVIGATION,      // Directory navigation (cd, pwd, ls)
    SEARCH,          // Search operations (find, grep)
    PROCESS,         // Process management (run, kill, status)
    NETWORK,         // Network operations (download, upload, ping)
    GIT,             // Version control
    BUILD,           // Build/compile operations
    SYSTEM,          // System commands (date, time, env)
    HELP,            // Help and info
    RAEL,            // RAEL-specific commands
    CUSTOM,          // User-defined commands
    UNKNOWN
};

struct ParsedSlot {
    std::string name;
    std::string value;
    std::string type;  // path, string, number, flag, etc.
    bool required = false;
    double confidence = 1.0;
};

struct ParsedCommand {
    std::string original_input;
    std::string canonical_form;
    CommandCategory category = CommandCategory::UNKNOWN;
    std::string action;
    std::vector<ParsedSlot> slots;
    std::map<std::string, std::string> flags;
    double confidence = 0;
    std::vector<std::string> alternatives;
    bool requires_confirmation = false;
    std::string confirmation_message;
};

struct CommandResult {
    bool success = false;
    std::string output;
    std::string error;
    int exit_code = 0;
    std::chrono::milliseconds duration;
    std::string suggested_followup;
};

// ═══════════════════════════════════════════════════════════════════════════
//  INTENT RECOGNITION
// ═══════════════════════════════════════════════════════════════════════════

struct Intent {
    std::string name;
    std::string description;
    std::vector<std::string> examples;
    std::vector<std::string> required_slots;
    std::vector<std::string> optional_slots;
    CommandCategory category;
    std::function<std::string(const ParsedCommand&)> generator;
};

class IntentRecognizer {
public:
    IntentRecognizer();

    // Register intents
    void register_intent(const Intent& intent);
    void unregister_intent(const std::string& name);
    std::vector<Intent> list_intents() const;

    // Recognition
    std::vector<std::pair<std::string, double>> recognize(const std::string& input);
    std::optional<Intent> get_best_intent(const std::string& input);

    // Slot extraction
    std::vector<ParsedSlot> extract_slots(const std::string& input, const Intent& intent);

    // Built-in intents
    void register_file_intents();
    void register_navigation_intents();
    void register_search_intents();
    void register_git_intents();
    void register_system_intents();

private:
    double compute_similarity(const std::string& input, const std::string& example);
    std::vector<std::string> tokenize(const std::string& text);
    std::string normalize(const std::string& text);

    std::map<std::string, Intent> intents_;
    mutable std::mutex mutex_;
};

// ═══════════════════════════════════════════════════════════════════════════
//  ENTITY EXTRACTION
// ═══════════════════════════════════════════════════════════════════════════

enum class EntityType {
    PATH,
    FILENAME,
    EXTENSION,
    PATTERN,
    NUMBER,
    SIZE,
    DATE,
    TIME,
    DURATION,
    URL,
    EMAIL,
    BRANCH_NAME,
    COMMIT_HASH,
    PERMISSION,
    USER,
    PROCESS_NAME,
    SIGNAL,
    VARIABLE,
    CUSTOM
};

struct Entity {
    std::string value;
    std::string normalized;
    EntityType type;
    size_t start_pos;
    size_t end_pos;
    double confidence;
};

class EntityExtractor {
public:
    EntityExtractor();

    // Extract entities from text
    std::vector<Entity> extract(const std::string& text);
    std::vector<Entity> extract_type(const std::string& text, EntityType type);

    // Specific extractors
    std::vector<Entity> extract_paths(const std::string& text);
    std::vector<Entity> extract_numbers(const std::string& text);
    std::vector<Entity> extract_patterns(const std::string& text);
    std::vector<Entity> extract_urls(const std::string& text);

    // Normalization
    std::string normalize_path(const std::string& path);
    std::string expand_path(const std::string& path);

    // Custom patterns
    void add_pattern(EntityType type, const std::string& regex);

private:
    std::map<EntityType, std::vector<std::regex>> patterns_;
    std::string home_dir_;
    std::string current_dir_;
};

// ═══════════════════════════════════════════════════════════════════════════
//  COMMAND GENERATOR
// ═══════════════════════════════════════════════════════════════════════════

class CommandGenerator {
public:
    CommandGenerator();

    // Generate shell command from parsed command
    std::string generate(const ParsedCommand& cmd);

    // Category-specific generators
    std::string generate_file_command(const ParsedCommand& cmd);
    std::string generate_navigation_command(const ParsedCommand& cmd);
    std::string generate_search_command(const ParsedCommand& cmd);
    std::string generate_git_command(const ParsedCommand& cmd);
    std::string generate_process_command(const ParsedCommand& cmd);
    std::string generate_network_command(const ParsedCommand& cmd);
    std::string generate_system_command(const ParsedCommand& cmd);

    // Command templates
    void add_template(const std::string& action, const std::string& template_str);
    std::string expand_template(const std::string& template_str,
                                 const std::map<std::string, std::string>& vars);

    // Safety
    bool is_dangerous(const std::string& command) const;
    std::string sanitize(const std::string& input) const;

private:
    std::map<std::string, std::string> templates_;
    std::vector<std::regex> dangerous_patterns_;
};

// ═══════════════════════════════════════════════════════════════════════════
//  CONTEXT MANAGER
// ═══════════════════════════════════════════════════════════════════════════

struct ConversationContext {
    std::string current_directory;
    std::vector<std::string> recent_files;
    std::vector<std::string> recent_commands;
    std::map<std::string, std::string> variables;
    std::map<std::string, std::string> aliases;
    std::string last_error;
    std::string last_output;
    std::string git_branch;
    bool in_git_repo = false;
};

class ContextManager {
public:
    ContextManager();

    // Context access
    ConversationContext& context();
    const ConversationContext& context() const;

    // Update from environment
    void refresh();
    void update_from_result(const CommandResult& result);

    // Pronoun resolution
    std::string resolve_pronoun(const std::string& pronoun);
    std::string resolve_reference(const std::string& text);

    // History
    void add_command(const std::string& cmd);
    std::vector<std::string> get_history(int n = 10) const;
    std::optional<std::string> find_similar_command(const std::string& partial) const;

    // Variables
    void set_variable(const std::string& name, const std::string& value);
    std::optional<std::string> get_variable(const std::string& name) const;
    std::string expand_variables(const std::string& text) const;

    // Aliases
    void set_alias(const std::string& name, const std::string& command);
    std::optional<std::string> get_alias(const std::string& name) const;

private:
    ConversationContext context_;
    mutable std::mutex mutex_;
};

// ═══════════════════════════════════════════════════════════════════════════
//  DISAMBIGUATION
// ═══════════════════════════════════════════════════════════════════════════

struct DisambiguationOption {
    std::string description;
    std::string command;
    double score;
};

class Disambiguator {
public:
    Disambiguator();

    // Check if disambiguation needed
    bool needs_disambiguation(const ParsedCommand& cmd) const;

    // Get options
    std::vector<DisambiguationOption> get_options(const ParsedCommand& cmd);

    // Generate clarification question
    std::string generate_question(const ParsedCommand& cmd);

    // Path disambiguation
    std::vector<std::string> suggest_paths(const std::string& partial);

    // Command disambiguation
    std::vector<std::string> suggest_commands(const std::string& partial);

private:
    std::vector<std::string> list_directory(const std::string& dir);
};

// ═══════════════════════════════════════════════════════════════════════════
//  FEEDBACK LEARNING
// ═══════════════════════════════════════════════════════════════════════════

struct FeedbackEntry {
    std::string input;
    std::string generated_command;
    std::string corrected_command;
    bool was_correct;
    std::chrono::system_clock::time_point timestamp;
};

class FeedbackLearner {
public:
    FeedbackLearner();

    // Record feedback
    void record(const std::string& input, const std::string& generated,
                bool correct, const std::string& correction = "");

    // Learning
    void learn_from_feedback();
    std::optional<std::string> get_learned_mapping(const std::string& input);

    // Statistics
    double get_accuracy() const;
    size_t feedback_count() const;

    // Persistence
    bool save(const std::string& path);
    bool load(const std::string& path);

private:
    std::vector<FeedbackEntry> feedback_;
    std::map<std::string, std::string> learned_mappings_;
    mutable std::mutex mutex_;
};

// ═══════════════════════════════════════════════════════════════════════════
//  COMMAND EXECUTOR
// ═══════════════════════════════════════════════════════════════════════════

struct ExecutionConfig {
    bool dry_run = false;
    bool capture_output = true;
    std::chrono::seconds timeout{60};
    std::string working_directory;
    std::map<std::string, std::string> environment;
    bool interactive = false;
};

class CommandExecutor {
public:
    CommandExecutor();

    // Execute command
    CommandResult execute(const std::string& command,
                          const ExecutionConfig& config = {});

    // Safe execution (with confirmation for dangerous commands)
    CommandResult safe_execute(const std::string& command,
                               std::function<bool(const std::string&)> confirm);

    // Background execution
    std::string execute_background(const std::string& command);
    std::optional<CommandResult> get_background_result(const std::string& job_id);
    bool cancel_background(const std::string& job_id);

    // Command validation
    bool validate_command(const std::string& command) const;

private:
    std::map<std::string, std::future<CommandResult>> background_jobs_;
    mutable std::mutex mutex_;
};

// ═══════════════════════════════════════════════════════════════════════════
//  AUTOCOMPLETE
// ═══════════════════════════════════════════════════════════════════════════

struct CompletionItem {
    std::string text;
    std::string display;
    std::string description;
    double score;
    enum class Type { PATH, COMMAND, FLAG, KEYWORD, HISTORY } type;
};

class AutoComplete {
public:
    AutoComplete();

    // Get completions
    std::vector<CompletionItem> complete(const std::string& partial,
                                          const ContextManager& ctx);

    // Specific completions
    std::vector<CompletionItem> complete_path(const std::string& partial);
    std::vector<CompletionItem> complete_command(const std::string& partial);
    std::vector<CompletionItem> complete_from_history(const std::string& partial,
                                                       const std::vector<std::string>& history);

    // Register custom completions
    void add_keyword(const std::string& keyword, const std::string& description);
    void add_completion_source(std::function<std::vector<CompletionItem>(const std::string&)> source);

private:
    std::map<std::string, std::string> keywords_;
    std::vector<std::function<std::vector<CompletionItem>(const std::string&)>> sources_;
};

// ═══════════════════════════════════════════════════════════════════════════
//  NL SHELL
// ═══════════════════════════════════════════════════════════════════════════

struct NLShellConfig {
    double confidence_threshold = 0.7;
    bool require_confirmation_for_dangerous = true;
    bool enable_learning = true;
    bool verbose = false;
    std::string history_file;
    size_t max_history = 1000;
};

class NLShell {
public:
    NLShell(const NLShellConfig& config = {});
    ~NLShell();

    // Singleton
    static NLShell& instance();

    // Main interface
    ParsedCommand parse(const std::string& input);
    std::string translate(const std::string& input);
    CommandResult interpret(const std::string& input);
    CommandResult execute(const std::string& input);

    // Interactive mode
    void run_interactive();
    void process_line(const std::string& line);

    // Explanation
    std::string explain(const ParsedCommand& cmd);
    std::string explain_command(const std::string& command);

    // Suggestions
    std::vector<std::string> suggest(const std::string& partial);
    std::string suggest_correction(const std::string& failed_cmd);

    // Components
    IntentRecognizer& intents();
    EntityExtractor& entities();
    CommandGenerator& generator();
    ContextManager& context();
    Disambiguator& disambiguator();
    AutoComplete& autocomplete();
    FeedbackLearner& learner();

    // Configuration
    void set_config(const NLShellConfig& config);
    NLShellConfig get_config() const;

    // Callbacks
    using ConfirmCallback = std::function<bool(const std::string&)>;
    using OutputCallback = std::function<void(const std::string&)>;

    void set_confirm_callback(ConfirmCallback cb);
    void set_output_callback(OutputCallback cb);

    // History
    bool save_history(const std::string& path = "");
    bool load_history(const std::string& path = "");

    // Stats
    struct Stats {
        size_t total_commands;
        size_t successful_commands;
        size_t failed_commands;
        size_t disambiguations;
        size_t corrections;
        double avg_confidence;
    };
    Stats get_stats() const;

private:
    bool should_confirm(const ParsedCommand& cmd) const;
    void log_command(const std::string& input, const std::string& generated,
                     const CommandResult& result);

    NLShellConfig config_;
    std::unique_ptr<IntentRecognizer> intents_;
    std::unique_ptr<EntityExtractor> entities_;
    std::unique_ptr<CommandGenerator> generator_;
    std::unique_ptr<ContextManager> context_;
    std::unique_ptr<Disambiguator> disambiguator_;
    std::unique_ptr<CommandExecutor> executor_;
    std::unique_ptr<AutoComplete> autocomplete_;
    std::unique_ptr<FeedbackLearner> learner_;

    ConfirmCallback confirm_cb_;
    OutputCallback output_cb_;

    Stats stats_;
    mutable std::mutex mutex_;
};

// ═══════════════════════════════════════════════════════════════════════════
//  NATURAL LANGUAGE PATTERNS
// ═══════════════════════════════════════════════════════════════════════════

namespace nlpatterns {

// Common action verbs by category
const std::vector<std::string> FILE_ACTIONS = {
    "create", "make", "new", "touch",
    "delete", "remove", "rm", "erase",
    "copy", "cp", "duplicate",
    "move", "mv", "rename",
    "open", "edit", "view", "show", "cat", "read",
    "write", "save", "append"
};

const std::vector<std::string> NAV_ACTIONS = {
    "go", "cd", "change", "switch", "navigate",
    "list", "ls", "show", "display",
    "where", "pwd", "current"
};

const std::vector<std::string> SEARCH_ACTIONS = {
    "find", "search", "look", "locate", "grep",
    "filter", "match", "contain"
};

const std::vector<std::string> GIT_ACTIONS = {
    "commit", "push", "pull", "fetch",
    "branch", "checkout", "merge", "rebase",
    "status", "log", "diff", "add", "stage",
    "clone", "init", "stash"
};

// Prepositions and connectors
const std::vector<std::string> LOCATION_PREPS = {
    "in", "into", "to", "from", "at", "under", "inside"
};

const std::vector<std::string> CONTENT_PREPS = {
    "with", "containing", "named", "called", "matching"
};

} // namespace nlpatterns

// ═══════════════════════════════════════════════════════════════════════════
//  GLOBAL ACCESS
// ═══════════════════════════════════════════════════════════════════════════

NLShell& nlsh();

} // namespace rael
