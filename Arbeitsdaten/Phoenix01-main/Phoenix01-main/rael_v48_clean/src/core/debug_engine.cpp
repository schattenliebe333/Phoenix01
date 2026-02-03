// RAEL V48 - Error/Debug Engine (#19)
// Implementation of stack trace parsing and error analysis

#include "rael/debug_engine.h"
#include <sstream>
#include <fstream>
#include <algorithm>
#include <cstdlib>
#include <array>
#include <iomanip>

#ifdef __GNUC__
#include <cxxabi.h>
#endif

namespace rael {

// ═══════════════════════════════════════════════════════════════════
//  UTILITIES
// ═══════════════════════════════════════════════════════════════════

std::string severity_to_string(ErrorSeverity severity) {
    switch (severity) {
        case ErrorSeverity::INFO: return "INFO";
        case ErrorSeverity::WARNING: return "WARNING";
        case ErrorSeverity::ERROR: return "ERROR";
        case ErrorSeverity::FATAL: return "FATAL";
        case ErrorSeverity::PANIC: return "PANIC";
    }
    return "UNKNOWN";
}

std::string category_to_string(ErrorCategory category) {
    switch (category) {
        case ErrorCategory::SYNTAX: return "SYNTAX";
        case ErrorCategory::TYPE: return "TYPE";
        case ErrorCategory::RUNTIME: return "RUNTIME";
        case ErrorCategory::MEMORY: return "MEMORY";
        case ErrorCategory::IO: return "IO";
        case ErrorCategory::NETWORK: return "NETWORK";
        case ErrorCategory::PERMISSION: return "PERMISSION";
        case ErrorCategory::ASSERTION: return "ASSERTION";
        case ErrorCategory::LOGIC: return "LOGIC";
        case ErrorCategory::RESOURCE: return "RESOURCE";
        case ErrorCategory::TIMEOUT: return "TIMEOUT";
        case ErrorCategory::INTERRUPT: return "INTERRUPT";
        default: return "UNKNOWN";
    }
}

ErrorSeverity string_to_severity(const std::string& str) {
    if (str == "INFO" || str == "info") return ErrorSeverity::INFO;
    if (str == "WARNING" || str == "WARN" || str == "warning" || str == "warn") return ErrorSeverity::WARNING;
    if (str == "ERROR" || str == "error") return ErrorSeverity::ERROR;
    if (str == "FATAL" || str == "fatal") return ErrorSeverity::FATAL;
    if (str == "PANIC" || str == "panic") return ErrorSeverity::PANIC;
    return ErrorSeverity::ERROR;
}

ErrorCategory string_to_category(const std::string& str) {
    if (str == "SYNTAX" || str == "SyntaxError") return ErrorCategory::SYNTAX;
    if (str == "TYPE" || str == "TypeError") return ErrorCategory::TYPE;
    if (str == "MEMORY" || str == "MemoryError" || str == "OutOfMemory") return ErrorCategory::MEMORY;
    if (str == "IO" || str == "IOError" || str == "FileNotFound") return ErrorCategory::IO;
    if (str == "NETWORK" || str == "NetworkError" || str == "ConnectionError") return ErrorCategory::NETWORK;
    if (str == "PERMISSION" || str == "PermissionError") return ErrorCategory::PERMISSION;
    if (str == "ASSERTION" || str == "AssertionError") return ErrorCategory::ASSERTION;
    return ErrorCategory::RUNTIME;
}

// ═══════════════════════════════════════════════════════════════════
//  STACK TRACE
// ═══════════════════════════════════════════════════════════════════

std::optional<StackFrame> StackTrace::get_user_frame() const {
    for (const auto& frame : frames) {
        if (frame.is_user_code && !frame.file_path.empty()) {
            return frame;
        }
    }
    return std::nullopt;
}

std::optional<StackFrame> StackTrace::at(size_t index) const {
    if (index < frames.size()) {
        return frames[index];
    }
    return std::nullopt;
}

std::string StackTrace::to_string() const {
    std::ostringstream oss;
    for (const auto& frame : frames) {
        oss << "#" << frame.index << " ";
        if (!frame.address.empty()) {
            oss << frame.address << " in ";
        }
        oss << frame.function_name;
        if (!frame.file_path.empty()) {
            oss << " at " << frame.file_path;
            if (frame.line_number > 0) {
                oss << ":" << frame.line_number;
            }
        }
        oss << "\n";
    }
    return oss.str();
}

// ═══════════════════════════════════════════════════════════════════
//  STACK TRACE PARSER
// ═══════════════════════════════════════════════════════════════════

std::vector<std::string> StackTraceParser::split_lines(const std::string& str) {
    std::vector<std::string> lines;
    std::istringstream iss(str);
    std::string line;
    while (std::getline(iss, line)) {
        lines.push_back(line);
    }
    return lines;
}

std::string StackTraceParser::trim(const std::string& str) {
    size_t start = str.find_first_not_of(" \t\r\n");
    size_t end = str.find_last_not_of(" \t\r\n");
    if (start == std::string::npos) return "";
    return str.substr(start, end - start + 1);
}

StackTraceParser::TraceFormat StackTraceParser::detect_format(const std::string& trace) {
    if (trace.find("Traceback (most recent call last)") != std::string::npos ||
        trace.find("File \"") != std::string::npos) {
        return TraceFormat::PYTHON;
    }
    if (trace.find("    at ") != std::string::npos &&
        (trace.find(".js:") != std::string::npos || trace.find(".ts:") != std::string::npos)) {
        return TraceFormat::JAVASCRIPT_V8;
    }
    if (trace.find("at java.") != std::string::npos ||
        trace.find("at org.") != std::string::npos ||
        trace.find("at com.") != std::string::npos) {
        return TraceFormat::JAVA;
    }
    if (trace.find("#0 ") != std::string::npos ||
        trace.find("(gdb)") != std::string::npos) {
        return TraceFormat::CPP_GDB;
    }
    if (trace.find("frame #") != std::string::npos ||
        trace.find("(lldb)") != std::string::npos) {
        return TraceFormat::CPP_LLDB;
    }
    if (trace.find("goroutine") != std::string::npos ||
        trace.find("runtime.") != std::string::npos) {
        return TraceFormat::GO;
    }
    if (trace.find("at System.") != std::string::npos ||
        trace.find("at Microsoft.") != std::string::npos) {
        return TraceFormat::CSHARP;
    }
    if (trace.find("stack backtrace:") != std::string::npos ||
        trace.find("note: run with `RUST_BACKTRACE=1`") != std::string::npos) {
        return TraceFormat::RUST;
    }
    return TraceFormat::UNKNOWN;
}

StackTrace StackTraceParser::parse(const std::string& trace_text) {
    TraceFormat format = detect_format(trace_text);

    switch (format) {
        case TraceFormat::PYTHON: return parse_python(trace_text);
        case TraceFormat::JAVASCRIPT_V8:
        case TraceFormat::JAVASCRIPT_SPIDERMONKEY: return parse_javascript(trace_text);
        case TraceFormat::JAVA: return parse_java(trace_text);
        case TraceFormat::CPP_GDB: return parse_cpp_gdb(trace_text);
        case TraceFormat::CPP_LLDB: return parse_cpp_lldb(trace_text);
        case TraceFormat::RUST: return parse_rust(trace_text);
        case TraceFormat::GO: return parse_go(trace_text);
        case TraceFormat::CSHARP: return parse_csharp(trace_text);
        default: break;
    }

    // Generic parsing
    StackTrace st;
    auto lines = split_lines(trace_text);
    int index = 0;
    for (const auto& line : lines) {
        if (line.empty()) continue;
        StackFrame frame;
        frame.index = index++;
        frame.function_name = trim(line);
        st.frames.push_back(frame);
    }
    return st;
}

StackTrace StackTraceParser::parse_python(const std::string& trace) {
    StackTrace st;
    auto lines = split_lines(trace);

    std::regex file_re(R"(File \"([^\"]+)\", line (\d+)(?:, in (.+))?)");
    std::regex code_re(R"(^\s{4}(.+)$)");

    int index = 0;
    for (size_t i = 0; i < lines.size(); ++i) {
        std::smatch match;
        if (std::regex_search(lines[i], match, file_re)) {
            StackFrame frame;
            frame.index = index++;
            frame.file_path = match[1].str();
            frame.line_number = std::stoi(match[2].str());
            if (match[3].matched) {
                frame.function_name = match[3].str();
            }

            // Check for source line
            if (i + 1 < lines.size() && std::regex_match(lines[i + 1], code_re)) {
                frame.source_line = trim(lines[i + 1]);
            }

            // Check if user code
            frame.is_user_code = frame.file_path.find("/site-packages/") == std::string::npos &&
                                  frame.file_path.find("/lib/python") == std::string::npos;

            st.frames.push_back(frame);
        }
    }

    return st;
}

StackTrace StackTraceParser::parse_javascript(const std::string& trace) {
    StackTrace st;
    auto lines = split_lines(trace);

    // V8 format: "    at functionName (file:line:column)"
    // or:        "    at file:line:column"
    std::regex v8_re(R"(^\s*at\s+(?:(.+?)\s+\()?([^:]+):(\d+):(\d+)\)?$)");

    int index = 0;
    for (const auto& line : lines) {
        std::smatch match;
        if (std::regex_search(line, match, v8_re)) {
            StackFrame frame;
            frame.index = index++;
            if (match[1].matched) {
                frame.function_name = match[1].str();
            } else {
                frame.function_name = "<anonymous>";
            }
            frame.file_path = match[2].str();
            frame.line_number = std::stoi(match[3].str());
            frame.column = std::stoi(match[4].str());

            // Check if user code
            frame.is_user_code = frame.file_path.find("node_modules") == std::string::npos &&
                                  frame.file_path.find("internal/") == std::string::npos;

            st.frames.push_back(frame);
        }
    }

    return st;
}

StackTrace StackTraceParser::parse_java(const std::string& trace) {
    StackTrace st;
    auto lines = split_lines(trace);

    // Java format: "    at package.Class.method(File.java:line)"
    std::regex java_re(R"(^\s*at\s+([^\(]+)\(([^:]+):(\d+)\)$)");

    int index = 0;
    for (const auto& line : lines) {
        std::smatch match;
        if (std::regex_search(line, match, java_re)) {
            StackFrame frame;
            frame.index = index++;
            frame.function_name = match[1].str();
            frame.file_path = match[2].str();
            frame.line_number = std::stoi(match[3].str());

            // Extract module from function name
            size_t last_dot = frame.function_name.rfind('.');
            if (last_dot != std::string::npos) {
                frame.module_name = frame.function_name.substr(0, last_dot);
            }

            // Check if user code
            frame.is_user_code = frame.function_name.find("java.") != 0 &&
                                  frame.function_name.find("sun.") != 0 &&
                                  frame.function_name.find("javax.") != 0;

            st.frames.push_back(frame);
        }
    }

    return st;
}

StackTrace StackTraceParser::parse_cpp_gdb(const std::string& trace) {
    StackTrace st;
    auto lines = split_lines(trace);

    // GDB format: "#0  0x00007ffff7a4c2b0 in function () at file.cpp:123"
    std::regex gdb_re(R"(^#(\d+)\s+(?:(0x[0-9a-fA-F]+)\s+in\s+)?([^\(]+)(?:\([^\)]*\))?\s*(?:at\s+([^:]+):(\d+))?)");

    for (const auto& line : lines) {
        std::smatch match;
        if (std::regex_search(line, match, gdb_re)) {
            StackFrame frame;
            frame.index = std::stoi(match[1].str());
            if (match[2].matched) {
                frame.address = match[2].str();
            }
            frame.function_name = trim(match[3].str());
            if (match[4].matched) {
                frame.file_path = match[4].str();
            }
            if (match[5].matched) {
                frame.line_number = std::stoi(match[5].str());
            }

            // Check if user code
            frame.is_user_code = !frame.file_path.empty() &&
                                  frame.file_path.find("/usr/") == std::string::npos &&
                                  frame.function_name.find("__") != 0;

            st.frames.push_back(frame);
        }
    }

    return st;
}

StackTrace StackTraceParser::parse_cpp_lldb(const std::string& trace) {
    StackTrace st;
    auto lines = split_lines(trace);

    // LLDB format: "frame #0: 0x00007fff address module`function at file:line"
    std::regex lldb_re(R"(^\s*frame\s+#(\d+):\s+(?:(0x[0-9a-fA-F]+)\s+)?(?:([^\`]+)\`)?([\w:~]+)(?:\s+at\s+([^:]+):(\d+))?)");

    for (const auto& line : lines) {
        std::smatch match;
        if (std::regex_search(line, match, lldb_re)) {
            StackFrame frame;
            frame.index = std::stoi(match[1].str());
            if (match[2].matched) {
                frame.address = match[2].str();
            }
            if (match[3].matched) {
                frame.module_name = trim(match[3].str());
            }
            frame.function_name = match[4].str();
            if (match[5].matched) {
                frame.file_path = match[5].str();
            }
            if (match[6].matched) {
                frame.line_number = std::stoi(match[6].str());
            }

            st.frames.push_back(frame);
        }
    }

    return st;
}

StackTrace StackTraceParser::parse_rust(const std::string& trace) {
    StackTrace st;
    auto lines = split_lines(trace);

    // Rust format varies, common: "   0: function_name\n             at /path/to/file.rs:123"
    std::regex frame_re(R"(^\s*(\d+):\s+(.+)$)");
    std::regex at_re(R"(^\s*at\s+(.+):(\d+)$)");

    int index = -1;
    StackFrame current_frame;

    for (const auto& line : lines) {
        std::smatch match;
        if (std::regex_search(line, match, frame_re)) {
            if (index >= 0) {
                st.frames.push_back(current_frame);
            }
            current_frame = StackFrame{};
            index = std::stoi(match[1].str());
            current_frame.index = index;
            current_frame.function_name = match[2].str();
        } else if (std::regex_search(line, match, at_re) && index >= 0) {
            current_frame.file_path = match[1].str();
            current_frame.line_number = std::stoi(match[2].str());
        }
    }

    if (index >= 0) {
        st.frames.push_back(current_frame);
    }

    return st;
}

StackTrace StackTraceParser::parse_go(const std::string& trace) {
    StackTrace st;
    auto lines = split_lines(trace);

    // Go format:
    // goroutine 1 [running]:
    // main.function(...)
    //     /path/to/file.go:123 +0x1a
    std::regex func_re(R"(^([^\s].+)$)");
    std::regex file_re(R"(^\s+(.+):(\d+)\s*)");

    int index = 0;
    for (size_t i = 0; i < lines.size(); ++i) {
        std::smatch match;
        if (std::regex_match(lines[i], match, func_re)) {
            std::string func = match[1].str();
            if (func.find("goroutine") == 0) continue;

            StackFrame frame;
            frame.index = index++;
            frame.function_name = func;

            // Check next line for file info
            if (i + 1 < lines.size()) {
                std::smatch file_match;
                if (std::regex_search(lines[i + 1], file_match, file_re)) {
                    frame.file_path = file_match[1].str();
                    frame.line_number = std::stoi(file_match[2].str());
                    ++i;
                }
            }

            frame.is_user_code = frame.function_name.find("runtime.") != 0;
            st.frames.push_back(frame);
        }
    }

    return st;
}

StackTrace StackTraceParser::parse_csharp(const std::string& trace) {
    StackTrace st;
    auto lines = split_lines(trace);

    // C# format: "   at Namespace.Class.Method() in C:\path\file.cs:line 123"
    std::regex cs_re(R"(^\s*at\s+([^\s]+)\s*(?:in\s+(.+):line\s+(\d+))?)");

    int index = 0;
    for (const auto& line : lines) {
        std::smatch match;
        if (std::regex_search(line, match, cs_re)) {
            StackFrame frame;
            frame.index = index++;
            frame.function_name = match[1].str();
            if (match[2].matched) {
                frame.file_path = match[2].str();
            }
            if (match[3].matched) {
                frame.line_number = std::stoi(match[3].str());
            }

            frame.is_user_code = frame.function_name.find("System.") != 0 &&
                                  frame.function_name.find("Microsoft.") != 0;

            st.frames.push_back(frame);
        }
    }

    return st;
}

// ═══════════════════════════════════════════════════════════════════
//  ERROR ANALYZER
// ═══════════════════════════════════════════════════════════════════

ErrorAnalyzer::ErrorAnalyzer() {
    load_default_patterns();
}

void ErrorAnalyzer::load_default_patterns() {
    // Memory errors
    add_pattern({"nullptr", std::regex(R"(null\s*pointer|nullptr|NullPointerException|NullReferenceException)", std::regex::icase),
                 ErrorCategory::MEMORY, "Null pointer dereference",
                 {"Check if the variable is initialized before use", "Add null checks"}});

    add_pattern({"segfault", std::regex(R"(segmentation\s*fault|SIGSEGV|access\s*violation)", std::regex::icase),
                 ErrorCategory::MEMORY, "Memory access violation",
                 {"Check array bounds", "Ensure pointer is valid", "Check for use-after-free"}});

    add_pattern({"oom", std::regex(R"(out\s*of\s*memory|MemoryError|bad_alloc|OOM)", std::regex::icase),
                 ErrorCategory::MEMORY, "Out of memory",
                 {"Reduce memory usage", "Check for memory leaks", "Process data in chunks"}});

    // Type errors
    add_pattern({"type", std::regex(R"(TypeError|type\s*error|incompatible\s*type)", std::regex::icase),
                 ErrorCategory::TYPE, "Type mismatch",
                 {"Check variable types", "Add type annotations", "Use type casting"}});

    // IO errors
    add_pattern({"filenotfound", std::regex(R"(FileNotFound|No\s*such\s*file|ENOENT)", std::regex::icase),
                 ErrorCategory::IO, "File not found",
                 {"Check file path", "Ensure file exists", "Check permissions"}});

    add_pattern({"permission", std::regex(R"(Permission\s*denied|EACCES|EPERM|access\s*denied)", std::regex::icase),
                 ErrorCategory::PERMISSION, "Permission denied",
                 {"Check file permissions", "Run with elevated privileges if needed"}});

    // Network errors
    add_pattern({"connection", std::regex(R"(Connection\s*refused|ECONNREFUSED|Connection\s*reset)", std::regex::icase),
                 ErrorCategory::NETWORK, "Connection error",
                 {"Check if server is running", "Verify host and port", "Check firewall"}});

    add_pattern({"timeout", std::regex(R"(timeout|ETIMEDOUT|timed\s*out)", std::regex::icase),
                 ErrorCategory::TIMEOUT, "Operation timed out",
                 {"Increase timeout value", "Check network connectivity", "Optimize operation"}});

    // Syntax errors
    add_pattern({"syntax", std::regex(R"(SyntaxError|syntax\s*error|unexpected\s*token)", std::regex::icase),
                 ErrorCategory::SYNTAX, "Syntax error",
                 {"Check for missing brackets or semicolons", "Verify syntax matches language spec"}});
}

void ErrorAnalyzer::add_pattern(const ErrorPattern& pattern) {
    patterns_.push_back(pattern);
}

RuntimeError ErrorAnalyzer::analyze(const std::string& error_text,
                                    const std::string& language) const {
    RuntimeError error;
    error.timestamp = std::chrono::system_clock::now();

    // Extract error type and message
    error.type = extract_error_type(error_text);
    error.message = extract_error_message(error_text);

    // Match against patterns
    for (const auto& pattern : patterns_) {
        if (std::regex_search(error_text, pattern.pattern)) {
            error.category = pattern.category;
            error.suggestions = pattern.suggestions;
            break;
        }
    }

    return error;
}

RuntimeError ErrorAnalyzer::analyze_with_trace(const std::string& error_text,
                                               const std::string& trace_text,
                                               const std::string& language) const {
    RuntimeError error = analyze(error_text, language);
    error.stack_trace = StackTraceParser::parse(trace_text);

    // Extract file/line from first user frame
    auto frame = error.stack_trace.get_user_frame();
    if (frame) {
        error.file = frame->file_path;
        error.line = frame->line_number;
    }

    return error;
}

std::string ErrorAnalyzer::extract_error_type(const std::string& text) const {
    // Common patterns for error type
    std::regex type_re(R"(^(\w+(?:Error|Exception|Failure))\b)");
    std::smatch match;
    if (std::regex_search(text, match, type_re)) {
        return match[1].str();
    }
    return "Error";
}

std::string ErrorAnalyzer::extract_error_message(const std::string& text) const {
    // Remove error type prefix and clean up
    std::regex msg_re(R"((?:\w+(?:Error|Exception|Failure)):\s*(.+))");
    std::smatch match;
    if (std::regex_search(text, match, msg_re)) {
        return match[1].str();
    }
    return text;
}

ErrorCategory ErrorAnalyzer::categorize(const std::string& error_type,
                                        const std::string& message) const {
    std::string combined = error_type + " " + message;

    for (const auto& pattern : patterns_) {
        if (std::regex_search(combined, pattern.pattern)) {
            return pattern.category;
        }
    }

    return ErrorCategory::RUNTIME;
}

std::vector<std::string> ErrorAnalyzer::get_suggestions(const RuntimeError& error) const {
    std::vector<std::string> suggestions = error.suggestions;

    // Add category-specific suggestions
    switch (error.category) {
        case ErrorCategory::MEMORY:
            suggestions.push_back("Use memory debugging tools (valgrind, asan)");
            break;
        case ErrorCategory::IO:
            suggestions.push_back("Check working directory");
            suggestions.push_back("Verify file encoding");
            break;
        case ErrorCategory::NETWORK:
            suggestions.push_back("Check network connectivity");
            suggestions.push_back("Verify DNS resolution");
            break;
        default:
            break;
    }

    return suggestions;
}

// ═══════════════════════════════════════════════════════════════════
//  SYMBOL RESOLVER
// ═══════════════════════════════════════════════════════════════════

SymbolResolver::SymbolResolver(const std::string& executable_path)
    : executable_(executable_path) {
    if (!executable_.empty()) {
        load_symbols(executable_);
    }
}

std::string SymbolResolver::demangle_cpp(const std::string& mangled) {
#ifdef __GNUC__
    int status = 0;
    char* demangled = abi::__cxa_demangle(mangled.c_str(), nullptr, nullptr, &status);
    if (status == 0 && demangled) {
        std::string result(demangled);
        free(demangled);
        return result;
    }
#endif
    return mangled;
}

std::optional<SymbolInfo> SymbolResolver::resolve_address(uint64_t address) const {
    auto it = symbols_.find(address);
    if (it != symbols_.end()) {
        return it->second;
    }

    // Look for containing symbol
    for (auto& [addr, sym] : symbols_) {
        if (address >= addr && address < addr + sym.size) {
            return sym;
        }
    }

    return std::nullopt;
}

std::optional<SymbolInfo> SymbolResolver::resolve_address(const std::string& address_hex) const {
    uint64_t addr = std::stoull(address_hex, nullptr, 16);
    return resolve_address(addr);
}

bool SymbolResolver::load_symbols(const std::string& path) {
    // Use nm or objdump to extract symbols
    std::string cmd = "nm -C -n \"" + path + "\" 2>/dev/null";
    std::array<char, 4096> buffer;
    std::string result;

    FILE* pipe = popen(cmd.c_str(), "r");
    if (!pipe) return false;

    while (fgets(buffer.data(), buffer.size(), pipe)) {
        result += buffer.data();
    }
    pclose(pipe);

    // Parse nm output
    std::regex nm_re(R"(^([0-9a-fA-F]+)\s+(\w)\s+(.+)$)");
    std::istringstream iss(result);
    std::string line;

    while (std::getline(iss, line)) {
        std::smatch match;
        if (std::regex_search(line, match, nm_re)) {
            SymbolInfo sym;
            sym.address = std::stoull(match[1].str(), nullptr, 16);
            sym.type = match[2].str();
            sym.name = match[3].str();
            sym.demangled_name = demangle_cpp(sym.name);

            symbols_[sym.address] = sym;
            named_symbols_[sym.name] = sym;
        }
    }

    return !symbols_.empty();
}

std::optional<SymbolInfo> SymbolResolver::lookup_symbol(const std::string& name) const {
    auto it = named_symbols_.find(name);
    if (it != named_symbols_.end()) {
        return it->second;
    }
    return std::nullopt;
}

// ═══════════════════════════════════════════════════════════════════
//  LOG ANALYZER
// ═══════════════════════════════════════════════════════════════════

void LogAnalyzer::load_default_patterns() {
    // Common log formats
    LogPattern common;
    common.name = "common";
    common.pattern = std::regex(R"(^(\d{4}-\d{2}-\d{2}[T ]\d{2}:\d{2}:\d{2}(?:\.\d+)?)\s+(\w+)\s+(?:\[([^\]]+)\]\s+)?(.+)$)");
    common.capture_groups = {"timestamp", "level", "logger", "message"};
    patterns_.push_back(common);

    // Apache/nginx access log
    LogPattern access;
    access.name = "access";
    access.pattern = std::regex(R"RE(^(\S+)\s+-\s+-\s+\[([^\]]+)\]\s+"([^"]+)"\s+(\d+)\s+(\d+))RE");
    access.capture_groups = {"ip", "timestamp", "request", "status", "size"};
    patterns_.push_back(access);
}

std::vector<LogEntry> LogAnalyzer::parse_file(const std::string& path) const {
    std::ifstream ifs(path);
    if (!ifs) return {};

    std::stringstream ss;
    ss << ifs.rdbuf();
    return parse(ss.str());
}

std::vector<LogEntry> LogAnalyzer::parse(const std::string& log_text) const {
    std::vector<LogEntry> entries;
    std::istringstream iss(log_text);
    std::string line;
    int line_num = 0;

    while (std::getline(iss, line)) {
        ++line_num;
        auto entry = parse_entry(line);
        entry.line_number = line_num;
        if (!entry.message.empty()) {
            entries.push_back(entry);
        }
    }

    return entries;
}

LogEntry LogAnalyzer::parse_entry(const std::string& line) const {
    LogEntry entry;

    for (const auto& pattern : patterns_) {
        std::smatch match;
        if (std::regex_search(line, match, pattern.pattern)) {
            for (size_t i = 0; i < pattern.capture_groups.size() && i + 1 < match.size(); ++i) {
                const auto& group = pattern.capture_groups[i];
                std::string value = match[i + 1].str();

                if (group == "timestamp") {
                    // Simple timestamp storage
                    entry.context["timestamp_str"] = value;
                } else if (group == "level") {
                    entry.level = value;
                } else if (group == "logger") {
                    entry.logger = value;
                } else if (group == "message") {
                    entry.message = value;
                } else if (group == "thread") {
                    entry.thread = value;
                } else {
                    entry.context[group] = value;
                }
            }
            break;
        }
    }

    // Fallback: use entire line as message
    if (entry.message.empty()) {
        entry.message = line;
    }

    return entry;
}

std::vector<LogEntry> LogAnalyzer::filter_by_level(const std::vector<LogEntry>& entries,
                                                    const std::string& min_level) const {
    std::vector<LogEntry> filtered;

    static const std::map<std::string, int> level_priority = {
        {"TRACE", 0}, {"DEBUG", 1}, {"INFO", 2}, {"WARN", 3}, {"WARNING", 3},
        {"ERROR", 4}, {"FATAL", 5}, {"CRITICAL", 5}
    };

    auto it = level_priority.find(min_level);
    int min_pri = (it != level_priority.end()) ? it->second : 2;

    for (const auto& entry : entries) {
        auto lit = level_priority.find(entry.level);
        int pri = (lit != level_priority.end()) ? lit->second : 2;
        if (pri >= min_pri) {
            filtered.push_back(entry);
        }
    }

    return filtered;
}

std::vector<LogEntry> LogAnalyzer::search(const std::vector<LogEntry>& entries,
                                           const std::string& query) const {
    std::vector<LogEntry> results;
    std::regex query_re(query, std::regex::icase);

    for (const auto& entry : entries) {
        if (std::regex_search(entry.message, query_re)) {
            results.push_back(entry);
        }
    }

    return results;
}

std::vector<RuntimeError> LogAnalyzer::extract_errors(const std::vector<LogEntry>& entries) const {
    std::vector<RuntimeError> errors;
    ErrorAnalyzer analyzer;

    for (const auto& entry : entries) {
        if (entry.level == "ERROR" || entry.level == "FATAL" || entry.level == "CRITICAL") {
            RuntimeError err = analyzer.analyze(entry.message);
            err.severity = string_to_severity(entry.level);
            err.metadata["logger"] = entry.logger;
            err.metadata["thread"] = entry.thread;
            errors.push_back(err);
        }
    }

    return errors;
}

} // namespace rael
