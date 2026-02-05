#include "rael/executor.h"
#include "rael/filesystem.h"
#include "rael/events.h"
#include "rael/sha256.h"
#include <sstream>
#include <regex>
#include <algorithm>
#include <thread>
#include <chrono>
#include <cstring>
#include <filesystem>
#include <fstream>

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include <fcntl.h>
#include <poll.h>
#endif

namespace fs = std::filesystem;

namespace rael {

// Global instances
ProcessExecutor gExecutor;
BuildSystem gBuildSystem;
TestRunner gTestRunner;

// ═══════════════════════════════════════════════════════════════════════════
// PROCESS EXECUTOR IMPLEMENTATION
// ═══════════════════════════════════════════════════════════════════════════

ProcessExecutor::ProcessExecutor() {}

ExecutionResult ProcessExecutor::run(const std::string& command) {
    ExecutionOptions opts;
    opts.timeout = default_timeout_;
    opts.working_dir = default_working_dir_;
    opts.shell = true;
    return run(command, opts);
}

ExecutionResult ProcessExecutor::run(const std::string& command, const ExecutionOptions& options) {
    return execute_internal(command, {}, options);
}

ExecutionResult ProcessExecutor::run(const std::string& program,
                                      const std::vector<std::string>& args) {
    ExecutionOptions opts;
    opts.timeout = default_timeout_;
    opts.working_dir = default_working_dir_;
    return run(program, args, opts);
}

ExecutionResult ProcessExecutor::run(const std::string& program,
                                      const std::vector<std::string>& args,
                                      const ExecutionOptions& options) {
    return execute_internal(program, args, options);
}

std::future<ExecutionResult> ProcessExecutor::run_async(const std::string& command) {
    return std::async(std::launch::async, [this, command]() {
        return run(command);
    });
}

std::future<ExecutionResult> ProcessExecutor::run_async(const std::string& command,
                                                         const ExecutionOptions& options) {
    return std::async(std::launch::async, [this, command, options]() {
        return run(command, options);
    });
}

ExecutionResult ProcessExecutor::shell(const std::string& command) {
    ExecutionOptions opts;
    opts.shell = true;
    opts.timeout = default_timeout_;
    return run(command, opts);
}

ExecutionResult ProcessExecutor::shell(const std::string& command, const ExecutionOptions& options) {
    ExecutionOptions opts = options;
    opts.shell = true;
    return run(command, opts);
}

ExecutionResult ProcessExecutor::execute_internal(const std::string& command,
                                                   const std::vector<std::string>& args,
                                                   const ExecutionOptions& options) {
    ExecutionResult result;
    auto start = std::chrono::steady_clock::now();

    // Sandbox-Check
    if (sandbox_enabled_ && !is_program_allowed(command)) {
        result.error = "Program not allowed in sandbox mode: " + command;
        result.exit_code = -1;
        return result;
    }

    // Build full command
    std::string full_command = command;
    for (const auto& arg : args) {
        full_command += " ";
        // Quote argument if it contains spaces
        if (arg.find(' ') != std::string::npos) {
            full_command += "\"" + arg + "\"";
        } else {
            full_command += arg;
        }
    }

    EventBus::push("EXEC_START", "cmd=" + full_command.substr(0, 100));

#ifdef _WIN32
    // Windows implementation using CreateProcess
    SECURITY_ATTRIBUTES sa;
    sa.nLength = sizeof(SECURITY_ATTRIBUTES);
    sa.bInheritHandle = TRUE;
    sa.lpSecurityDescriptor = nullptr;

    HANDLE stdout_read, stdout_write;
    HANDLE stderr_read, stderr_write;

    if (!CreatePipe(&stdout_read, &stdout_write, &sa, 0) ||
        !CreatePipe(&stderr_read, &stderr_write, &sa, 0)) {
        result.error = "Failed to create pipes";
        return result;
    }

    SetHandleInformation(stdout_read, HANDLE_FLAG_INHERIT, 0);
    SetHandleInformation(stderr_read, HANDLE_FLAG_INHERIT, 0);

    STARTUPINFOA si = {};
    si.cb = sizeof(STARTUPINFOA);
    si.hStdOutput = stdout_write;
    si.hStdError = options.merge_stderr ? stdout_write : stderr_write;
    si.dwFlags |= STARTF_USESTDHANDLES;

    PROCESS_INFORMATION pi = {};

    std::string cmd_line;
    if (options.shell) {
        cmd_line = "cmd.exe /c " + full_command;
    } else {
        cmd_line = full_command;
    }

    std::string working_dir = options.working_dir.empty() ? default_working_dir_ : options.working_dir;

    if (!CreateProcessA(
            nullptr,
            const_cast<char*>(cmd_line.c_str()),
            nullptr, nullptr,
            TRUE,
            CREATE_NO_WINDOW,
            nullptr,
            working_dir.empty() ? nullptr : working_dir.c_str(),
            &si, &pi)) {
        result.error = "Failed to create process";
        CloseHandle(stdout_read);
        CloseHandle(stdout_write);
        CloseHandle(stderr_read);
        CloseHandle(stderr_write);
        return result;
    }

    CloseHandle(stdout_write);
    CloseHandle(stderr_write);

    // Read output
    auto read_pipe = [](HANDLE pipe) -> std::string {
        std::string output;
        char buffer[4096];
        DWORD bytes_read;
        while (ReadFile(pipe, buffer, sizeof(buffer) - 1, &bytes_read, nullptr) && bytes_read > 0) {
            buffer[bytes_read] = '\0';
            output += buffer;
        }
        return output;
    };

    // Wait with timeout
    DWORD wait_result = WaitForSingleObject(pi.hProcess,
                                            static_cast<DWORD>(options.timeout.count()));

    if (wait_result == WAIT_TIMEOUT) {
        TerminateProcess(pi.hProcess, 1);
        result.timed_out = true;
        result.killed = true;
    }

    DWORD exit_code;
    GetExitCodeProcess(pi.hProcess, &exit_code);
    result.exit_code = static_cast<int>(exit_code);

    if (options.capture_stdout) {
        result.stdout_output = read_pipe(stdout_read);
    }
    if (options.capture_stderr && !options.merge_stderr) {
        result.stderr_output = read_pipe(stderr_read);
    }

    CloseHandle(stdout_read);
    CloseHandle(stderr_read);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

#else
    // POSIX implementation using fork/exec
    int stdout_pipe[2];
    int stderr_pipe[2];

    if (pipe(stdout_pipe) < 0 || pipe(stderr_pipe) < 0) {
        result.error = "Failed to create pipes";
        return result;
    }

    pid_t pid = fork();

    if (pid < 0) {
        result.error = "Fork failed";
        close(stdout_pipe[0]);
        close(stdout_pipe[1]);
        close(stderr_pipe[0]);
        close(stderr_pipe[1]);
        return result;
    }

    if (pid == 0) {
        // Child process
        close(stdout_pipe[0]);
        close(stderr_pipe[0]);

        dup2(stdout_pipe[1], STDOUT_FILENO);
        if (options.merge_stderr) {
            dup2(stdout_pipe[1], STDERR_FILENO);
        } else {
            dup2(stderr_pipe[1], STDERR_FILENO);
        }

        close(stdout_pipe[1]);
        close(stderr_pipe[1]);

        // Change directory if specified
        std::string working_dir = options.working_dir.empty() ? default_working_dir_ : options.working_dir;
        if (!working_dir.empty()) {
            if (chdir(working_dir.c_str()) != 0) {
                _exit(127);
            }
        }

        // Set environment variables
        for (const auto& [key, value] : options.env) {
            setenv(key.c_str(), value.c_str(), 1);
        }

        if (options.shell) {
            execlp("/bin/sh", "sh", "-c", full_command.c_str(), nullptr);
        } else {
            // Build argv
            std::vector<const char*> argv;
            argv.push_back(command.c_str());
            for (const auto& arg : args) {
                argv.push_back(arg.c_str());
            }
            argv.push_back(nullptr);
            execvp(command.c_str(), const_cast<char* const*>(argv.data()));
        }

        _exit(127);
    }

    // Parent process
    close(stdout_pipe[1]);
    close(stderr_pipe[1]);

    // Set non-blocking
    fcntl(stdout_pipe[0], F_SETFL, O_NONBLOCK);
    fcntl(stderr_pipe[0], F_SETFL, O_NONBLOCK);

    // Read output with timeout
    auto timeout_point = std::chrono::steady_clock::now() + options.timeout;
    bool finished = false;

    while (!finished) {
        // Check timeout
        if (std::chrono::steady_clock::now() > timeout_point) {
            kill(pid, SIGKILL);
            result.timed_out = true;
            result.killed = true;
            break;
        }

        // Check if process finished
        int status;
        pid_t wait_result = waitpid(pid, &status, WNOHANG);

        if (wait_result > 0) {
            if (WIFEXITED(status)) {
                result.exit_code = WEXITSTATUS(status);
            } else if (WIFSIGNALED(status)) {
                result.exit_code = -WTERMSIG(status);
                result.killed = true;
            }
            finished = true;
        }

        // Read available output
        char buffer[4096];
        ssize_t n;

        while ((n = read(stdout_pipe[0], buffer, sizeof(buffer) - 1)) > 0) {
            buffer[n] = '\0';
            result.stdout_output += buffer;
            if (options.on_output) {
                options.on_output(buffer, false);
            }
        }

        while ((n = read(stderr_pipe[0], buffer, sizeof(buffer) - 1)) > 0) {
            buffer[n] = '\0';
            result.stderr_output += buffer;
            if (options.on_output) {
                options.on_output(buffer, true);
            }
        }

        if (!finished) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    }

    // Read any remaining output
    char buffer[4096];
    ssize_t n;
    while ((n = read(stdout_pipe[0], buffer, sizeof(buffer) - 1)) > 0) {
        buffer[n] = '\0';
        result.stdout_output += buffer;
    }
    while ((n = read(stderr_pipe[0], buffer, sizeof(buffer) - 1)) > 0) {
        buffer[n] = '\0';
        result.stderr_output += buffer;
    }

    close(stdout_pipe[0]);
    close(stderr_pipe[0]);

    if (!finished) {
        waitpid(pid, nullptr, 0);
    }
#endif

    auto end = std::chrono::steady_clock::now();
    result.duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

    if (options.merge_stderr) {
        result.combined_output = result.stdout_output;
    } else {
        result.combined_output = result.stdout_output + result.stderr_output;
    }

    last_result_ = result;

    EventBus::push("EXEC_END", "exit=" + std::to_string(result.exit_code) +
                   "|duration=" + std::to_string(result.duration.count()) + "ms");

    return result;
}

BuildResult ProcessExecutor::compile(const std::string& compiler,
                                      const std::vector<std::string>& args) {
    BuildResult result;
    result.execution = run(compiler, args);
    result.success = result.execution.success();
    result.diagnostics = parse_compiler_output(result.execution.output(), compiler);
    return result;
}

std::vector<CompilerDiagnostic> ProcessExecutor::parse_compiler_output(const std::string& output,
                                                                         const std::string& compiler) {
    // Detect compiler type
    std::string comp_lower = compiler;
    std::transform(comp_lower.begin(), comp_lower.end(), comp_lower.begin(), ::tolower);

    if (comp_lower.find("cl") != std::string::npos ||
        comp_lower.find("msvc") != std::string::npos) {
        return parse_msvc_output(output);
    }
    if (comp_lower.find("clang") != std::string::npos) {
        return parse_clang_output(output);
    }
    // Default to GCC-style parsing (works for g++, gcc, clang in many cases)
    return parse_gcc_output(output);
}

std::vector<CompilerDiagnostic> ProcessExecutor::parse_gcc_output(const std::string& output) {
    std::vector<CompilerDiagnostic> diagnostics;

    // GCC/Clang format: file:line:column: error/warning: message
    std::regex pattern(R"(([^:\s]+):(\d+):(\d+):\s*(error|warning|note):\s*(.+))");

    std::istringstream stream(output);
    std::string line;

    while (std::getline(stream, line)) {
        std::smatch match;
        if (std::regex_search(line, match, pattern)) {
            CompilerDiagnostic diag;
            diag.file = match[1].str();
            diag.line = std::stoi(match[2].str());
            diag.column = std::stoi(match[3].str());

            std::string level_str = match[4].str();
            if (level_str == "error") diag.level = DiagnosticLevel::ERROR;
            else if (level_str == "warning") diag.level = DiagnosticLevel::WARNING;
            else if (level_str == "note") diag.level = DiagnosticLevel::NOTE;

            diag.message = match[5].str();
            diagnostics.push_back(diag);
        }
    }

    return diagnostics;
}

std::vector<CompilerDiagnostic> ProcessExecutor::parse_msvc_output(const std::string& output) {
    std::vector<CompilerDiagnostic> diagnostics;

    // MSVC format: file(line): error/warning CODE: message
    std::regex pattern(R"(([^(]+)\((\d+)\):\s*(error|warning)\s*([A-Z]\d+):\s*(.+))");

    std::istringstream stream(output);
    std::string line;

    while (std::getline(stream, line)) {
        std::smatch match;
        if (std::regex_search(line, match, pattern)) {
            CompilerDiagnostic diag;
            diag.file = match[1].str();
            diag.line = std::stoi(match[2].str());

            std::string level_str = match[3].str();
            if (level_str == "error") diag.level = DiagnosticLevel::ERROR;
            else if (level_str == "warning") diag.level = DiagnosticLevel::WARNING;

            diag.code = match[4].str();
            diag.message = match[5].str();
            diagnostics.push_back(diag);
        }
    }

    return diagnostics;
}

std::vector<CompilerDiagnostic> ProcessExecutor::parse_clang_output(const std::string& output) {
    // Clang uses same format as GCC
    return parse_gcc_output(output);
}

bool ProcessExecutor::program_exists(const std::string& name) {
    return !find_program(name).empty();
}

std::string ProcessExecutor::find_program(const std::string& name) const {
#ifdef _WIN32
    // Check common locations
    std::vector<std::string> paths = {"", "C:\\Windows\\System32\\", "C:\\Windows\\"};
    std::vector<std::string> exts = {"", ".exe", ".cmd", ".bat"};

    for (const auto& path : paths) {
        for (const auto& ext : exts) {
            std::string full = path + name + ext;
            if (gFileSystem.exists(full)) {
                return full;
            }
        }
    }

    // Check PATH
    char* path_env = getenv("PATH");
    if (path_env) {
        std::string path_str(path_env);
        std::istringstream stream(path_str);
        std::string dir;
        while (std::getline(stream, dir, ';')) {
            for (const auto& ext : exts) {
                std::string full = dir + "\\" + name + ext;
                if (gFileSystem.exists(full)) {
                    return full;
                }
            }
        }
    }
#else
    // Check if it's an absolute path
    if (name[0] == '/' && gFileSystem.exists(name)) {
        return name;
    }

    // Check PATH
    char* path_env = getenv("PATH");
    if (path_env) {
        std::string path_str(path_env);
        std::istringstream stream(path_str);
        std::string dir;
        while (std::getline(stream, dir, ':')) {
            std::string full = dir + "/" + name;
            if (gFileSystem.exists(full)) {
                return full;
            }
        }
    }
#endif
    return "";
}

void ProcessExecutor::set_allowed_programs(const std::vector<std::string>& programs) {
    allowed_programs_ = programs;
    sandbox_enabled_ = !programs.empty();
}

// ============================================================================
// SECURITY: Set allowed programs with absolute paths (hardened mode)
// ============================================================================
void ProcessExecutor::set_allowed_programs_secure(const std::vector<std::string>& absolute_paths) {
    allowed_programs_secure_.clear();
    for (const auto& path : absolute_paths) {
        try {
            // Only accept absolute paths
            fs::path p(path);
            if (p.is_absolute()) {
                std::string canonical = fs::weakly_canonical(p).string();
                allowed_programs_secure_.insert(canonical);
            } else {
                EventBus::push("SECURITY_WARN", "Ignoring non-absolute path in allowlist: " + path);
            }
        } catch (...) {
            EventBus::push("SECURITY_WARN", "Invalid path in allowlist: " + path);
        }
    }
    sandbox_enabled_ = true;
    use_secure_allowlist_ = true;
    EventBus::push("SECURITY", "Executor allowlist set with " +
                   std::to_string(allowed_programs_secure_.size()) + " programs");
}

// SECURITY: Set hash pins for programs (optional additional verification)
void ProcessExecutor::set_program_hashes(const std::unordered_map<std::string, std::string>& hashes) {
    program_hashes_ = hashes;
    hash_pinning_enabled_ = !hashes.empty();
    EventBus::push("SECURITY", "Hash pinning enabled for " +
                   std::to_string(hashes.size()) + " programs");
}

// SECURITY: Compute SHA-256 of a file
std::string ProcessExecutor::compute_file_hash(const std::string& path) const {
    std::ifstream f(path, std::ios::binary);
    if (!f) return "";

    std::vector<uint8_t> data((std::istreambuf_iterator<char>(f)),
                               std::istreambuf_iterator<char>());
    auto hash = SHA256::digest(data);
    return SHA256::hex(hash);
}

bool ProcessExecutor::is_program_allowed(const std::string& program) const {
    if (!sandbox_enabled_) return true;

    // ========================================================================
    // SECURITY: Use hardened mode if available
    // ========================================================================
    if (use_secure_allowlist_) {
        try {
            // Resolve the program to an absolute path
            std::string resolved_path;

            fs::path p(program);
            if (p.is_absolute()) {
                resolved_path = fs::weakly_canonical(p).string();
            } else {
                // Find in PATH
                std::string found = find_program(program);
                if (found.empty()) {
                    EventBus::push("SECURITY_BLOCK", "Program not found: " + program);
                    return false;
                }
                resolved_path = fs::weakly_canonical(found).string();
            }

            // Check if canonical path is in allowlist
            if (allowed_programs_secure_.find(resolved_path) == allowed_programs_secure_.end()) {
                EventBus::push("SECURITY_BLOCK", "Program not in secure allowlist: " + resolved_path);
                return false;
            }

            // Optional: Verify hash
            if (hash_pinning_enabled_) {
                auto it = program_hashes_.find(resolved_path);
                if (it != program_hashes_.end()) {
                    std::string actual_hash = compute_file_hash(resolved_path);
                    std::string expected_lower = it->second;
                    std::string actual_lower = actual_hash;
                    std::transform(expected_lower.begin(), expected_lower.end(),
                                   expected_lower.begin(), ::tolower);
                    std::transform(actual_lower.begin(), actual_lower.end(),
                                   actual_lower.begin(), ::tolower);

                    if (expected_lower != actual_lower) {
                        EventBus::push("SECURITY_BLOCK", "Hash mismatch for: " + resolved_path);
                        return false;
                    }
                }
            }

            return true;

        } catch (const std::exception& e) {
            EventBus::push("SECURITY_BLOCK", "Path resolution failed: " + std::string(e.what()));
            return false;
        }
    }

    // Legacy mode (basename comparison) - DEPRECATED, use set_allowed_programs_secure()
    std::string prog_name = gFileSystem.basename(program);
    for (const auto& allowed : allowed_programs_) {
        if (prog_name == allowed || program == allowed) {
            EventBus::push("SECURITY_WARN", "Using legacy allowlist (insecure): " + program);
            return true;
        }
    }
    return false;
}

std::string ProcessExecutor::detect_compiler(const std::string& name) {
    std::string lower = name;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);

    if (lower.find("cl") != std::string::npos) return "msvc";
    if (lower.find("clang") != std::string::npos) return "clang";
    if (lower.find("gcc") != std::string::npos || lower.find("g++") != std::string::npos) return "gcc";
    if (lower.find("rustc") != std::string::npos) return "rustc";

    return "unknown";
}

// ═══════════════════════════════════════════════════════════════════════════
// BUILD RESULT IMPLEMENTATION
// ═══════════════════════════════════════════════════════════════════════════

int BuildResult::error_count() const {
    int count = 0;
    for (const auto& d : diagnostics) {
        if (d.is_error()) count++;
    }
    return count;
}

int BuildResult::warning_count() const {
    int count = 0;
    for (const auto& d : diagnostics) {
        if (d.is_warning()) count++;
    }
    return count;
}

std::vector<CompilerDiagnostic> BuildResult::errors() const {
    std::vector<CompilerDiagnostic> result;
    for (const auto& d : diagnostics) {
        if (d.is_error()) result.push_back(d);
    }
    return result;
}

std::vector<CompilerDiagnostic> BuildResult::warnings() const {
    std::vector<CompilerDiagnostic> result;
    for (const auto& d : diagnostics) {
        if (d.is_warning()) result.push_back(d);
    }
    return result;
}

// ═══════════════════════════════════════════════════════════════════════════
// BUILD SYSTEM IMPLEMENTATION
// ═══════════════════════════════════════════════════════════════════════════

BuildSystemInfo BuildSystem::detect(const std::string& directory) {
    BuildSystemInfo info;
    info.project_root = directory;

    // Check for various build system config files
    if (gFileSystem.exists(gFileSystem.join(directory, "CMakeLists.txt"))) {
        info.type = BuildSystemType::CMAKE;
        info.config_file = "CMakeLists.txt";
        info.build_command = "cmake --build build";
        info.test_command = "ctest --test-dir build";
    }
    else if (gFileSystem.exists(gFileSystem.join(directory, "Cargo.toml"))) {
        info.type = BuildSystemType::CARGO;
        info.config_file = "Cargo.toml";
        info.build_command = "cargo build";
        info.test_command = "cargo test";
    }
    else if (gFileSystem.exists(gFileSystem.join(directory, "package.json"))) {
        info.type = BuildSystemType::NPM;
        info.config_file = "package.json";
        info.build_command = "npm run build";
        info.test_command = "npm test";
    }
    else if (gFileSystem.exists(gFileSystem.join(directory, "Makefile"))) {
        info.type = BuildSystemType::MAKE;
        info.config_file = "Makefile";
        info.build_command = "make";
        info.test_command = "make test";
    }
    else if (gFileSystem.exists(gFileSystem.join(directory, "go.mod"))) {
        info.type = BuildSystemType::GO_MOD;
        info.config_file = "go.mod";
        info.build_command = "go build ./...";
        info.test_command = "go test ./...";
    }
    else if (gFileSystem.exists(gFileSystem.join(directory, "pom.xml"))) {
        info.type = BuildSystemType::MAVEN;
        info.config_file = "pom.xml";
        info.build_command = "mvn compile";
        info.test_command = "mvn test";
    }
    else if (gFileSystem.exists(gFileSystem.join(directory, "build.gradle"))) {
        info.type = BuildSystemType::GRADLE;
        info.config_file = "build.gradle";
        info.build_command = "gradle build";
        info.test_command = "gradle test";
    }

    return info;
}

BuildResult BuildSystem::build(const BuildSystemInfo& info) {
    BuildResult result;
    ExecutionOptions opts;
    opts.working_dir = info.project_root;
    opts.shell = true;

    result.execution = executor_.run(info.build_command, opts);
    result.success = result.execution.success();
    result.diagnostics = executor_.parse_compiler_output(result.execution.output());

    return result;
}

BuildResult BuildSystem::build(const std::string& directory) {
    return build(detect(directory));
}

ExecutionResult BuildSystem::test(const BuildSystemInfo& info) {
    ExecutionOptions opts;
    opts.working_dir = info.project_root;
    opts.shell = true;

    return executor_.run(info.test_command, opts);
}

ExecutionResult BuildSystem::test(const std::string& directory) {
    return test(detect(directory));
}

ExecutionResult BuildSystem::clean(const BuildSystemInfo& info) {
    ExecutionOptions opts;
    opts.working_dir = info.project_root;
    opts.shell = true;

    std::string cmd;
    switch (info.type) {
        case BuildSystemType::CMAKE: cmd = "cmake --build build --target clean"; break;
        case BuildSystemType::CARGO: cmd = "cargo clean"; break;
        case BuildSystemType::NPM: cmd = "npm run clean"; break;
        case BuildSystemType::MAKE: cmd = "make clean"; break;
        case BuildSystemType::MAVEN: cmd = "mvn clean"; break;
        case BuildSystemType::GRADLE: cmd = "gradle clean"; break;
        default: cmd = ""; break;
    }

    if (cmd.empty()) {
        ExecutionResult result;
        result.error = "Clean not supported for this build system";
        return result;
    }

    return executor_.run(cmd, opts);
}

// ═══════════════════════════════════════════════════════════════════════════
// TEST RUNNER IMPLEMENTATION
// ═══════════════════════════════════════════════════════════════════════════

TestResult TestRunner::run(const std::string& directory) {
    // Detect test framework
    if (gFileSystem.exists(gFileSystem.join(directory, "pytest.ini")) ||
        gFileSystem.exists(gFileSystem.join(directory, "setup.py")) ||
        gFileSystem.exists(gFileSystem.join(directory, "pyproject.toml"))) {
        return run_pytest(directory);
    }

    if (gFileSystem.exists(gFileSystem.join(directory, "package.json"))) {
        return run_jest(directory);
    }

    if (gFileSystem.exists(gFileSystem.join(directory, "Cargo.toml"))) {
        return run_cargo_test(directory);
    }

    // Default: try ctest
    return run_ctest(directory);
}

TestResult TestRunner::run_pytest(const std::string& directory,
                                   const std::vector<std::string>& args) {
    ExecutionOptions opts;
    opts.working_dir = directory;
    opts.shell = true;

    std::string cmd = "python -m pytest -v";
    for (const auto& arg : args) {
        cmd += " " + arg;
    }

    auto result = executor_.run(cmd, opts);
    return parse_pytest_output(result);
}

TestResult TestRunner::run_jest(const std::string& directory,
                                 const std::vector<std::string>& args) {
    ExecutionOptions opts;
    opts.working_dir = directory;
    opts.shell = true;

    std::string cmd = "npx jest --verbose";
    for (const auto& arg : args) {
        cmd += " " + arg;
    }

    auto result = executor_.run(cmd, opts);
    return parse_jest_output(result);
}

TestResult TestRunner::run_cargo_test(const std::string& directory,
                                       const std::vector<std::string>& args) {
    ExecutionOptions opts;
    opts.working_dir = directory;
    opts.shell = true;

    std::string cmd = "cargo test";
    for (const auto& arg : args) {
        cmd += " " + arg;
    }

    auto result = executor_.run(cmd, opts);
    return parse_cargo_test_output(result);
}

TestResult TestRunner::run_gtest(const std::string& executable,
                                  const std::vector<std::string>& args) {
    auto result = executor_.run(executable, args);
    return parse_gtest_output(result);
}

TestResult TestRunner::run_ctest(const std::string& build_dir,
                                  const std::vector<std::string>& args) {
    ExecutionOptions opts;
    opts.working_dir = build_dir;
    opts.shell = true;

    std::string cmd = "ctest --output-on-failure";
    for (const auto& arg : args) {
        cmd += " " + arg;
    }

    TestResult result;
    result.execution = executor_.run(cmd, opts);
    result.success = result.execution.success();

    // Parse ctest output
    std::regex passed_regex(R"((\d+)\s+tests passed)");
    std::regex failed_regex(R"((\d+)\s+tests failed)");

    std::smatch match;
    std::string output = result.execution.output();

    if (std::regex_search(output, match, passed_regex)) {
        result.passed = std::stoi(match[1].str());
    }
    if (std::regex_search(output, match, failed_regex)) {
        result.failed = std::stoi(match[1].str());
    }

    return result;
}

TestResult TestRunner::parse_pytest_output(const ExecutionResult& exec) {
    TestResult result;
    result.execution = exec;
    result.success = exec.success();

    // Parse pytest output
    std::regex summary_regex(R"((\d+) passed|(\d+) failed|(\d+) skipped)");
    std::string output = exec.output();

    std::sregex_iterator iter(output.begin(), output.end(), summary_regex);
    std::sregex_iterator end;

    while (iter != end) {
        std::smatch match = *iter;
        if (match[1].matched) result.passed = std::stoi(match[1].str());
        if (match[2].matched) result.failed = std::stoi(match[2].str());
        if (match[3].matched) result.skipped = std::stoi(match[3].str());
        ++iter;
    }

    return result;
}

TestResult TestRunner::parse_jest_output(const ExecutionResult& exec) {
    TestResult result;
    result.execution = exec;
    result.success = exec.success();

    // Jest output: Tests: X passed, Y failed, Z total
    std::regex summary_regex(R"(Tests:\s+(\d+)\s+passed,?\s*(\d*)\s*failed?)");
    std::smatch match;
    std::string output = exec.output();

    if (std::regex_search(output, match, summary_regex)) {
        result.passed = std::stoi(match[1].str());
        if (match[2].matched && !match[2].str().empty()) {
            result.failed = std::stoi(match[2].str());
        }
    }

    return result;
}

TestResult TestRunner::parse_cargo_test_output(const ExecutionResult& exec) {
    TestResult result;
    result.execution = exec;
    result.success = exec.success();

    // Cargo test: test result: ok. X passed; Y failed
    std::regex summary_regex(R"((\d+) passed[;,]\s*(\d+) failed)");
    std::smatch match;
    std::string output = exec.output();

    if (std::regex_search(output, match, summary_regex)) {
        result.passed = std::stoi(match[1].str());
        result.failed = std::stoi(match[2].str());
    }

    return result;
}

TestResult TestRunner::parse_gtest_output(const ExecutionResult& exec) {
    TestResult result;
    result.execution = exec;
    result.success = exec.success();

    // GTest: [  PASSED  ] X tests. / [  FAILED  ] Y tests.
    std::regex passed_regex(R"(\[\s*PASSED\s*\]\s*(\d+)\s*test)");
    std::regex failed_regex(R"(\[\s*FAILED\s*\]\s*(\d+)\s*test)");

    std::smatch match;
    std::string output = exec.output();

    if (std::regex_search(output, match, passed_regex)) {
        result.passed = std::stoi(match[1].str());
    }
    if (std::regex_search(output, match, failed_regex)) {
        result.failed = std::stoi(match[1].str());
    }

    return result;
}

} // namespace rael
