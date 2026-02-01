// RAEL V48 - Error/Debug Engine (#19)
// Stack trace parsing, exception analysis, and debugging support
#pragma once

#include <string>
#include <vector>
#include <map>
#include <optional>
#include <regex>
#include <chrono>
#include <memory>

namespace rael {

// ═══════════════════════════════════════════════════════════════════
//  STACK FRAME
// ═══════════════════════════════════════════════════════════════════

struct StackFrame {
    int index = 0;
    std::string function_name;
    std::string file_path;
    int line_number = 0;
    int column = 0;
    std::string module_name;      // Library/module name
    std::string address;          // Memory address
    std::vector<std::string> args;// Function arguments if available
    std::map<std::string, std::string> locals;  // Local variables if available
    bool is_native = false;       // Native/system code
    bool is_user_code = true;     // User's code vs library
    std::string source_line;      // Actual source code line
};

struct StackTrace {
    std::vector<StackFrame> frames;
    std::string thread_id;
    std::string thread_name;
    std::chrono::system_clock::time_point timestamp;

    // Get topmost user code frame
    std::optional<StackFrame> get_user_frame() const;

    // Get frame at index
    std::optional<StackFrame> at(size_t index) const;

    // Format as string
    std::string to_string() const;
};

// ═══════════════════════════════════════════════════════════════════
//  EXCEPTION INFO
// ═══════════════════════════════════════════════════════════════════

struct ExceptionInfo {
    std::string type;             // Exception class/type
    std::string message;          // Error message
    std::string code;             // Error code if applicable
    StackTrace stack_trace;
    std::unique_ptr<ExceptionInfo> cause;  // Chained exception (unique_ptr for incomplete type)
    std::map<std::string, std::string> context;  // Additional context
};

// ═══════════════════════════════════════════════════════════════════
//  RUNTIME ERROR
// ═══════════════════════════════════════════════════════════════════

enum class ErrorSeverity {
    INFO,
    WARNING,
    ERROR,
    FATAL,
    PANIC
};

enum class ErrorCategory {
    UNKNOWN,
    SYNTAX,
    TYPE,
    RUNTIME,
    MEMORY,
    IO,
    NETWORK,
    PERMISSION,
    ASSERTION,
    LOGIC,
    RESOURCE,
    TIMEOUT,
    INTERRUPT
};

struct RuntimeError {
    std::string id;
    ErrorSeverity severity = ErrorSeverity::ERROR;
    ErrorCategory category = ErrorCategory::UNKNOWN;
    std::string type;
    std::string message;
    std::string file;
    int line = 0;
    int column = 0;
    StackTrace stack_trace;
    std::chrono::system_clock::time_point timestamp;
    std::map<std::string, std::string> metadata;

    // Suggested fixes
    std::vector<std::string> suggestions;
};

// ═══════════════════════════════════════════════════════════════════
//  STACK TRACE PARSER
// ═══════════════════════════════════════════════════════════════════

class StackTraceParser {
public:
    // Auto-detect and parse
    static StackTrace parse(const std::string& trace_text);

    // Language-specific parsers
    static StackTrace parse_python(const std::string& trace);
    static StackTrace parse_javascript(const std::string& trace);
    static StackTrace parse_java(const std::string& trace);
    static StackTrace parse_cpp_gdb(const std::string& trace);
    static StackTrace parse_cpp_lldb(const std::string& trace);
    static StackTrace parse_rust(const std::string& trace);
    static StackTrace parse_go(const std::string& trace);
    static StackTrace parse_csharp(const std::string& trace);

    // Detect trace format
    enum class TraceFormat {
        UNKNOWN,
        PYTHON,
        JAVASCRIPT_V8,
        JAVASCRIPT_SPIDERMONKEY,
        JAVA,
        CPP_GDB,
        CPP_LLDB,
        CPP_MSVC,
        RUST,
        GO,
        CSHARP
    };
    static TraceFormat detect_format(const std::string& trace);

private:
    static std::vector<std::string> split_lines(const std::string& str);
    static std::string trim(const std::string& str);
};

// ═══════════════════════════════════════════════════════════════════
//  ERROR ANALYZER
// ═══════════════════════════════════════════════════════════════════

struct ErrorPattern {
    std::string id;
    std::regex pattern;
    ErrorCategory category;
    std::string description;
    std::vector<std::string> suggestions;
};

class ErrorAnalyzer {
public:
    ErrorAnalyzer();

    // Analyze error message
    RuntimeError analyze(const std::string& error_text,
                         const std::string& language = "") const;

    // Analyze with stack trace
    RuntimeError analyze_with_trace(const std::string& error_text,
                                    const std::string& trace_text,
                                    const std::string& language = "") const;

    // Get suggestions for error
    std::vector<std::string> get_suggestions(const RuntimeError& error) const;

    // Categorize error
    ErrorCategory categorize(const std::string& error_type,
                             const std::string& message) const;

    // Register custom patterns
    void add_pattern(const ErrorPattern& pattern);

    // Common error patterns
    void load_default_patterns();

private:
    std::vector<ErrorPattern> patterns_;

    std::string extract_error_type(const std::string& text) const;
    std::string extract_error_message(const std::string& text) const;
};

// ═══════════════════════════════════════════════════════════════════
//  SYMBOL RESOLVER
// ═══════════════════════════════════════════════════════════════════

struct SymbolInfo {
    std::string name;
    std::string demangled_name;
    std::string type;          // function, variable, class, etc.
    std::string file;
    int line = 0;
    std::string module;
    uint64_t address = 0;
    uint64_t size = 0;
};

class SymbolResolver {
public:
    explicit SymbolResolver(const std::string& executable_path = "");

    // Resolve address to symbol
    std::optional<SymbolInfo> resolve_address(uint64_t address) const;
    std::optional<SymbolInfo> resolve_address(const std::string& address_hex) const;

    // Demangle C++ names
    static std::string demangle_cpp(const std::string& mangled);

    // Load symbols from file
    bool load_symbols(const std::string& path);

    // Lookup by name
    std::optional<SymbolInfo> lookup_symbol(const std::string& name) const;

private:
    std::string executable_;
    std::map<uint64_t, SymbolInfo> symbols_;
    std::map<std::string, SymbolInfo> named_symbols_;
};

// ═══════════════════════════════════════════════════════════════════
//  DEBUGGER INTERFACE
// ═══════════════════════════════════════════════════════════════════

struct Breakpoint {
    int id = 0;
    std::string file;
    int line = 0;
    std::string condition;
    bool enabled = true;
    int hit_count = 0;
};

struct Variable {
    std::string name;
    std::string type;
    std::string value;
    bool is_local = true;
    std::vector<Variable> children;  // For structs/objects
};

class DebugSession {
public:
    virtual ~DebugSession() = default;

    // Control
    virtual bool start(const std::string& program, const std::vector<std::string>& args = {}) = 0;
    virtual bool attach(int pid) = 0;
    virtual void detach() = 0;
    virtual void stop() = 0;

    // Execution control
    virtual void continue_execution() = 0;
    virtual void step_over() = 0;
    virtual void step_into() = 0;
    virtual void step_out() = 0;
    virtual void pause() = 0;

    // Breakpoints
    virtual Breakpoint set_breakpoint(const std::string& file, int line) = 0;
    virtual bool remove_breakpoint(int id) = 0;
    virtual std::vector<Breakpoint> get_breakpoints() const = 0;

    // Inspection
    virtual StackTrace get_stack_trace() const = 0;
    virtual std::vector<Variable> get_local_variables() const = 0;
    virtual std::optional<Variable> evaluate(const std::string& expression) const = 0;

    // State
    virtual bool is_running() const = 0;
    virtual bool is_paused() const = 0;
};

// ═══════════════════════════════════════════════════════════════════
//  LOG ANALYZER
// ═══════════════════════════════════════════════════════════════════

struct LogEntry {
    std::chrono::system_clock::time_point timestamp;
    std::string level;  // DEBUG, INFO, WARN, ERROR, etc.
    std::string logger;
    std::string message;
    std::string thread;
    std::map<std::string, std::string> context;
    int line_number = 0;  // Line in log file
};

struct LogPattern {
    std::string name;
    std::regex pattern;
    std::vector<std::string> capture_groups;  // timestamp, level, message, etc.
};

class LogAnalyzer {
public:
    // Parse log file
    std::vector<LogEntry> parse_file(const std::string& path) const;

    // Parse log text
    std::vector<LogEntry> parse(const std::string& log_text) const;

    // Filter entries
    std::vector<LogEntry> filter_by_level(const std::vector<LogEntry>& entries,
                                           const std::string& min_level) const;
    std::vector<LogEntry> filter_by_time(const std::vector<LogEntry>& entries,
                                          std::chrono::system_clock::time_point start,
                                          std::chrono::system_clock::time_point end) const;
    std::vector<LogEntry> search(const std::vector<LogEntry>& entries,
                                  const std::string& query) const;

    // Find errors
    std::vector<RuntimeError> extract_errors(const std::vector<LogEntry>& entries) const;

    // Add custom log pattern
    void add_pattern(const LogPattern& pattern);
    void load_default_patterns();

private:
    std::vector<LogPattern> patterns_;
    LogEntry parse_entry(const std::string& line) const;
};

// ═══════════════════════════════════════════════════════════════════
//  CRASH DUMP ANALYZER
// ═══════════════════════════════════════════════════════════════════

struct CrashInfo {
    std::string signal_name;     // SIGSEGV, SIGABRT, etc.
    int signal_number = 0;
    std::string crash_reason;
    uint64_t fault_address = 0;
    StackTrace stack_trace;
    std::map<std::string, std::string> registers;
    std::vector<std::string> loaded_modules;
    std::chrono::system_clock::time_point timestamp;
};

class CrashDumpAnalyzer {
public:
    // Analyze crash dump
    CrashInfo analyze(const std::string& dump_path) const;

    // Analyze core dump
    CrashInfo analyze_core(const std::string& core_path,
                           const std::string& executable_path) const;

    // Generate report
    std::string generate_report(const CrashInfo& crash) const;

private:
    std::string run_command(const std::string& cmd) const;
};

// ═══════════════════════════════════════════════════════════════════
//  UTILITIES
// ═══════════════════════════════════════════════════════════════════

std::string severity_to_string(ErrorSeverity severity);
std::string category_to_string(ErrorCategory category);
ErrorSeverity string_to_severity(const std::string& str);
ErrorCategory string_to_category(const std::string& str);

} // namespace rael
