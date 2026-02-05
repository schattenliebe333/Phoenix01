#pragma once
// ═══════════════════════════════════════════════════════════════════════════
// RAEL PROCESS EXECUTOR (Verbesserung #13)
// ═══════════════════════════════════════════════════════════════════════════
// Externe Prozesse ausführen: Compiler, Tests, Git, etc.
// ═══════════════════════════════════════════════════════════════════════════

#include <string>
#include <vector>
#include <functional>
#include <chrono>
#include <cstdint>
#include <optional>
#include <future>
#include <unordered_map>
#include <unordered_set>

namespace rael {

// ═══════════════════════════════════════════════════════════════════════════
// EXECUTION RESULT - Ergebnis einer Prozessausführung
// ═══════════════════════════════════════════════════════════════════════════

struct ExecutionResult {
    int exit_code = -1;
    std::string stdout_output;
    std::string stderr_output;
    std::string combined_output;

    std::chrono::milliseconds duration{0};
    bool timed_out = false;
    bool killed = false;
    std::string error;

    // Erfolgreich (exit_code == 0)?
    bool success() const { return exit_code == 0 && !timed_out && !killed; }
    operator bool() const { return success(); }

    // Gesamte Ausgabe (stdout + stderr)
    std::string output() const {
        return combined_output.empty() ? stdout_output + stderr_output : combined_output;
    }
};

// ═══════════════════════════════════════════════════════════════════════════
// EXECUTION OPTIONS - Optionen für Prozessausführung
// ═══════════════════════════════════════════════════════════════════════════

struct ExecutionOptions {
    std::string working_dir;                    // Arbeitsverzeichnis
    std::unordered_map<std::string, std::string> env;  // Zusätzliche Umgebungsvariablen
    bool inherit_env = true;                    // Umgebung vom Parent erben?
    std::chrono::milliseconds timeout{60000};   // Timeout (60s default)
    std::string stdin_input;                    // Input für stdin
    bool capture_stdout = true;                 // stdout erfassen?
    bool capture_stderr = true;                 // stderr erfassen?
    bool merge_stderr = false;                  // stderr in stdout mergen?
    bool shell = false;                         // In Shell ausführen?

    // Callback für Live-Output
    std::function<void(const std::string&, bool)> on_output;  // (line, is_stderr)
};

// ═══════════════════════════════════════════════════════════════════════════
// COMPILER ERROR - Geparster Compiler-Fehler
// ═══════════════════════════════════════════════════════════════════════════

enum class DiagnosticLevel : uint8_t {
    ERROR = 0,
    WARNING = 1,
    NOTE = 2,
    INFO = 3
};

struct CompilerDiagnostic {
    DiagnosticLevel level = DiagnosticLevel::ERROR;
    std::string file;
    int line = 0;
    int column = 0;
    std::string message;
    std::string code;           // z.B. "C4996", "E0001"
    std::string context_line;   // Die betroffene Code-Zeile

    bool is_error() const { return level == DiagnosticLevel::ERROR; }
    bool is_warning() const { return level == DiagnosticLevel::WARNING; }
};

// ═══════════════════════════════════════════════════════════════════════════
// BUILD RESULT - Ergebnis eines Build-Prozesses
// ═══════════════════════════════════════════════════════════════════════════

struct BuildResult {
    bool success = false;
    ExecutionResult execution;
    std::vector<CompilerDiagnostic> diagnostics;

    int error_count() const;
    int warning_count() const;
    std::vector<CompilerDiagnostic> errors() const;
    std::vector<CompilerDiagnostic> warnings() const;
};

// ═══════════════════════════════════════════════════════════════════════════
// PROCESS EXECUTOR - Führt externe Prozesse aus
// ═══════════════════════════════════════════════════════════════════════════

class ProcessExecutor {
public:
    ProcessExecutor();

    // ─────────────────────────────────────────────────────────────────────────
    // Einfache Ausführung
    // ─────────────────────────────────────────────────────────────────────────

    // Befehl ausführen (String)
    ExecutionResult run(const std::string& command);

    // Befehl mit Optionen
    ExecutionResult run(const std::string& command, const ExecutionOptions& options);

    // Befehl mit Argumenten (sicherer)
    ExecutionResult run(const std::string& program, const std::vector<std::string>& args);
    ExecutionResult run(const std::string& program, const std::vector<std::string>& args,
                        const ExecutionOptions& options);

    // ─────────────────────────────────────────────────────────────────────────
    // Asynchrone Ausführung
    // ─────────────────────────────────────────────────────────────────────────

    // Starte Prozess asynchron
    std::future<ExecutionResult> run_async(const std::string& command);
    std::future<ExecutionResult> run_async(const std::string& command,
                                            const ExecutionOptions& options);

    // ─────────────────────────────────────────────────────────────────────────
    // Spezialisierte Ausführungen
    // ─────────────────────────────────────────────────────────────────────────

    // Shell-Befehl (bash/cmd)
    ExecutionResult shell(const std::string& command);
    ExecutionResult shell(const std::string& command, const ExecutionOptions& options);

    // Compiler ausführen
    BuildResult compile(const std::string& compiler, const std::vector<std::string>& args);

    // ─────────────────────────────────────────────────────────────────────────
    // Compiler-Erkennung und -Parsing
    // ─────────────────────────────────────────────────────────────────────────

    // Parse Compiler-Output
    std::vector<CompilerDiagnostic> parse_compiler_output(const std::string& output,
                                                           const std::string& compiler = "");

    // Erkenne Compiler aus Pfad/Name
    std::string detect_compiler(const std::string& name);

    // ─────────────────────────────────────────────────────────────────────────
    // Hilfsmethoden
    // ─────────────────────────────────────────────────────────────────────────

    // Prüfe ob Programm existiert
    bool program_exists(const std::string& name);

    // Finde Programm im PATH
    std::string find_program(const std::string& name);

    // Letztes Ergebnis
    const ExecutionResult& last_result() const { return last_result_; }

    // ─────────────────────────────────────────────────────────────────────────
    // Konfiguration
    // ─────────────────────────────────────────────────────────────────────────

    // Default-Timeout setzen
    void set_default_timeout(std::chrono::milliseconds timeout) {
        default_timeout_ = timeout;
    }

    // Default-Arbeitsverzeichnis
    void set_default_working_dir(const std::string& dir) {
        default_working_dir_ = dir;
    }

    // Sandbox-Modus (nur bestimmte Programme erlaubt)
    void set_allowed_programs(const std::vector<std::string>& programs);  // DEPRECATED
    bool is_program_allowed(const std::string& program) const;

    // ─────────────────────────────────────────────────────────────────────────
    // SECURITY: Hardened allowlist (absolute paths only)
    // ─────────────────────────────────────────────────────────────────────────

    // Set allowed programs using ABSOLUTE paths (recommended)
    void set_allowed_programs_secure(const std::vector<std::string>& absolute_paths);

    // Optional: Enable hash pinning for additional verification
    void set_program_hashes(const std::unordered_map<std::string, std::string>& hashes);

private:
    ExecutionResult execute_internal(const std::string& command,
                                     const std::vector<std::string>& args,
                                     const ExecutionOptions& options);

    std::vector<CompilerDiagnostic> parse_gcc_output(const std::string& output);
    std::vector<CompilerDiagnostic> parse_msvc_output(const std::string& output);
    std::vector<CompilerDiagnostic> parse_clang_output(const std::string& output);

    // SECURITY: Compute file hash for verification
    std::string compute_file_hash(const std::string& path) const;

    ExecutionResult last_result_;
    std::chrono::milliseconds default_timeout_{60000};
    std::string default_working_dir_;

    // Legacy allowlist (DEPRECATED - basename comparison is insecure)
    std::vector<std::string> allowed_programs_;
    bool sandbox_enabled_ = false;

    // SECURITY: Hardened allowlist (canonical absolute paths)
    std::unordered_set<std::string> allowed_programs_secure_;
    bool use_secure_allowlist_ = false;

    // SECURITY: Optional hash pinning
    std::unordered_map<std::string, std::string> program_hashes_;
    bool hash_pinning_enabled_ = false;
};

// ═══════════════════════════════════════════════════════════════════════════
// BUILD SYSTEM - Erkennt und führt Build-Systeme aus
// ═══════════════════════════════════════════════════════════════════════════

enum class BuildSystemType : uint8_t {
    UNKNOWN = 0,
    CMAKE = 1,
    MAKE = 2,
    NINJA = 3,
    MESON = 4,
    CARGO = 5,      // Rust
    NPM = 6,        // Node.js
    YARN = 7,
    PIP = 8,        // Python
    POETRY = 9,
    MAVEN = 10,     // Java
    GRADLE = 11,
    GO_MOD = 12,    // Go
    BAZEL = 13
};

inline const char* build_system_name(BuildSystemType t) {
    switch (t) {
        case BuildSystemType::CMAKE: return "CMake";
        case BuildSystemType::MAKE: return "Make";
        case BuildSystemType::NINJA: return "Ninja";
        case BuildSystemType::CARGO: return "Cargo";
        case BuildSystemType::NPM: return "npm";
        case BuildSystemType::YARN: return "Yarn";
        case BuildSystemType::MAVEN: return "Maven";
        case BuildSystemType::GRADLE: return "Gradle";
        case BuildSystemType::GO_MOD: return "Go Modules";
        default: return "Unknown";
    }
}

struct BuildSystemInfo {
    BuildSystemType type = BuildSystemType::UNKNOWN;
    std::string config_file;        // z.B. CMakeLists.txt, package.json
    std::string project_root;
    std::string build_command;      // z.B. "cmake --build ."
    std::string test_command;       // z.B. "ctest" oder "npm test"
    std::vector<std::string> targets;
};

class BuildSystem {
public:
    // Erkenne Build-System im Verzeichnis
    BuildSystemInfo detect(const std::string& directory);

    // Build ausführen
    BuildResult build(const BuildSystemInfo& info);
    BuildResult build(const std::string& directory);

    // Tests ausführen
    ExecutionResult test(const BuildSystemInfo& info);
    ExecutionResult test(const std::string& directory);

    // Clean
    ExecutionResult clean(const BuildSystemInfo& info);

private:
    ProcessExecutor executor_;
};

// ═══════════════════════════════════════════════════════════════════════════
// TEST RUNNER - Führt Tests aus und parst Ergebnisse
// ═══════════════════════════════════════════════════════════════════════════

struct TestCase {
    std::string name;
    std::string suite;
    bool passed = false;
    std::chrono::milliseconds duration{0};
    std::string error_message;
    std::string file;
    int line = 0;
};

struct TestResult {
    bool success = false;
    int passed = 0;
    int failed = 0;
    int skipped = 0;
    std::chrono::milliseconds total_duration{0};
    std::vector<TestCase> cases;
    ExecutionResult execution;

    int total() const { return passed + failed + skipped; }
    double pass_rate() const {
        return total() > 0 ? static_cast<double>(passed) / total() : 0.0;
    }
};

class TestRunner {
public:
    // Test-Framework erkennen und Tests ausführen
    TestResult run(const std::string& directory);

    // Spezifische Frameworks
    TestResult run_pytest(const std::string& directory, const std::vector<std::string>& args = {});
    TestResult run_jest(const std::string& directory, const std::vector<std::string>& args = {});
    TestResult run_cargo_test(const std::string& directory, const std::vector<std::string>& args = {});
    TestResult run_gtest(const std::string& executable, const std::vector<std::string>& args = {});
    TestResult run_ctest(const std::string& build_dir, const std::vector<std::string>& args = {});

private:
    TestResult parse_pytest_output(const ExecutionResult& result);
    TestResult parse_jest_output(const ExecutionResult& result);
    TestResult parse_cargo_test_output(const ExecutionResult& result);
    TestResult parse_gtest_output(const ExecutionResult& result);

    ProcessExecutor executor_;
};

// Global instances
extern ProcessExecutor gExecutor;
extern BuildSystem gBuildSystem;
extern TestRunner gTestRunner;

} // namespace rael
