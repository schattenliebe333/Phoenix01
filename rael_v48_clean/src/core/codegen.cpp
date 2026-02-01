// RAEL V48 - Code Generation + Auto-Fix (#18)
// Implementation of code generation and auto-fix functionality

#include "rael/codegen.h"
#include <sstream>
#include <fstream>
#include <algorithm>
#include <regex>
#include <cctype>
#include <set>

namespace rael {

// ═══════════════════════════════════════════════════════════════════
//  TEMPLATE ENGINE IMPLEMENTATION
// ═══════════════════════════════════════════════════════════════════

void TemplateEngine::register_template(const CodeTemplate& tmpl) {
    templates_[tmpl.name] = tmpl;
}

std::vector<CodeTemplate> TemplateEngine::get_templates(const std::string& language) const {
    std::vector<CodeTemplate> result;
    for (const auto& [name, tmpl] : templates_) {
        if (language.empty() || tmpl.language == language) {
            result.push_back(tmpl);
        }
    }
    return result;
}

std::optional<CodeTemplate> TemplateEngine::get_template(const std::string& name) const {
    auto it = templates_.find(name);
    if (it != templates_.end()) {
        return it->second;
    }
    return std::nullopt;
}

std::string TemplateEngine::substitute(const std::string& text,
                                       const std::map<std::string, std::string>& params) const {
    std::string result = text;

    for (const auto& [key, value] : params) {
        // Replace {{key}} patterns
        std::string pattern = "{{" + key + "}}";
        size_t pos = 0;
        while ((pos = result.find(pattern, pos)) != std::string::npos) {
            result.replace(pos, pattern.length(), value);
            pos += value.length();
        }

        // Also replace ${key} patterns
        pattern = "${" + key + "}";
        pos = 0;
        while ((pos = result.find(pattern, pos)) != std::string::npos) {
            result.replace(pos, pattern.length(), value);
            pos += value.length();
        }
    }

    return result;
}

std::string TemplateEngine::render(const std::string& template_name,
                                   const std::map<std::string, std::string>& params) const {
    auto tmpl = get_template(template_name);
    if (!tmpl) return "";
    return render(*tmpl, params);
}

std::string TemplateEngine::render(const CodeTemplate& tmpl,
                                   const std::map<std::string, std::string>& params) const {
    // Add defaults for missing optional params
    std::map<std::string, std::string> full_params = params;
    for (const auto& p : tmpl.parameters) {
        if (full_params.find(p.name) == full_params.end() && !p.default_value.empty()) {
            full_params[p.name] = p.default_value;
        }
    }

    return substitute(tmpl.template_text, full_params);
}

bool TemplateEngine::validate_params(const CodeTemplate& tmpl,
                                     const std::map<std::string, std::string>& params,
                                     std::vector<std::string>& errors) const {
    errors.clear();
    bool valid = true;

    for (const auto& p : tmpl.parameters) {
        if (p.required && params.find(p.name) == params.end() && p.default_value.empty()) {
            errors.push_back("Missing required parameter: " + p.name);
            valid = false;
        }
    }

    return valid;
}

void TemplateEngine::load_defaults() {
    // C++ class template
    CodeTemplate cpp_class;
    cpp_class.name = "cpp-class";
    cpp_class.language = "cpp";
    cpp_class.category = "class";
    cpp_class.description = "C++ class with header and implementation";
    cpp_class.template_text = R"(#pragma once

#include <string>
#include <vector>

namespace {{namespace}} {

class {{class_name}} {
public:
    {{class_name}}();
    ~{{class_name}}();

    // Copy/Move
    {{class_name}}(const {{class_name}}&) = default;
    {{class_name}}& operator=(const {{class_name}}&) = default;
    {{class_name}}({{class_name}}&&) noexcept = default;
    {{class_name}}& operator=({{class_name}}&&) noexcept = default;

{{methods}}

private:
{{fields}}
};

} // namespace {{namespace}}
)";
    cpp_class.parameters = {
        {"namespace", "string", "rael", "Namespace", false},
        {"class_name", "string", "", "Class name", true},
        {"methods", "string", "", "Method declarations", false},
        {"fields", "string", "", "Field declarations", false}
    };
    register_template(cpp_class);

    // C++ function template
    CodeTemplate cpp_func;
    cpp_func.name = "cpp-function";
    cpp_func.language = "cpp";
    cpp_func.category = "function";
    cpp_func.description = "C++ function";
    cpp_func.template_text = R"({{return_type}} {{function_name}}({{params}}) {
    {{body}}
}
)";
    cpp_func.parameters = {
        {"return_type", "string", "void", "Return type", false},
        {"function_name", "string", "", "Function name", true},
        {"params", "string", "", "Parameters", false},
        {"body", "string", "// TODO: implement", "Function body", false}
    };
    register_template(cpp_func);

    // Python class template
    CodeTemplate py_class;
    py_class.name = "python-class";
    py_class.language = "python";
    py_class.category = "class";
    py_class.description = "Python class";
    py_class.template_text = R"(class {{class_name}}:
    """{{description}}"""

    def __init__(self{{init_params}}):
        """Initialize {{class_name}}."""
{{init_body}}

{{methods}}
)";
    py_class.parameters = {
        {"class_name", "string", "", "Class name", true},
        {"description", "string", "A class", "Class description", false},
        {"init_params", "string", "", "Init parameters", false},
        {"init_body", "string", "        pass", "Init body", false},
        {"methods", "string", "", "Methods", false}
    };
    register_template(py_class);

    // Python function template
    CodeTemplate py_func;
    py_func.name = "python-function";
    py_func.language = "python";
    py_func.category = "function";
    py_func.description = "Python function with docstring";
    py_func.template_text = R"(def {{function_name}}({{params}}){{return_annotation}}:
    """{{description}}

    Args:
{{param_docs}}
    Returns:
        {{return_doc}}
    """
    {{body}}
)";
    py_func.parameters = {
        {"function_name", "string", "", "Function name", true},
        {"params", "string", "", "Parameters", false},
        {"return_annotation", "string", "", "Return type annotation", false},
        {"description", "string", "Function description", "Description", false},
        {"param_docs", "string", "", "Parameter documentation", false},
        {"return_doc", "string", "None", "Return documentation", false},
        {"body", "string", "pass", "Function body", false}
    };
    register_template(py_func);

    // JavaScript/TypeScript class template
    CodeTemplate js_class;
    js_class.name = "js-class";
    js_class.language = "javascript";
    js_class.category = "class";
    js_class.description = "JavaScript/TypeScript class";
    js_class.template_text = R"(/**
 * {{description}}
 */
{{export}}class {{class_name}} {
    {{fields}}

    constructor({{constructor_params}}) {
        {{constructor_body}}
    }

{{methods}}
}
)";
    js_class.parameters = {
        {"class_name", "string", "", "Class name", true},
        {"description", "string", "A class", "Class description", false},
        {"export", "string", "export ", "Export keyword", false},
        {"fields", "string", "", "Class fields", false},
        {"constructor_params", "string", "", "Constructor parameters", false},
        {"constructor_body", "string", "", "Constructor body", false},
        {"methods", "string", "", "Methods", false}
    };
    register_template(js_class);

    // Test template (generic)
    CodeTemplate test_tmpl;
    test_tmpl.name = "test";
    test_tmpl.language = "";
    test_tmpl.category = "test";
    test_tmpl.description = "Test file template";
    test_tmpl.template_text = R"({{imports}}

{{test_class_start}}
    {{setup}}

    {{test_cases}}
{{test_class_end}}
)";
    test_tmpl.parameters = {
        {"imports", "string", "", "Import statements", false},
        {"test_class_start", "string", "", "Test class/describe start", false},
        {"setup", "string", "", "Setup/beforeEach", false},
        {"test_cases", "string", "", "Test cases", false},
        {"test_class_end", "string", "", "Test class end", false}
    };
    register_template(test_tmpl);

    // Rust struct template
    CodeTemplate rust_struct;
    rust_struct.name = "rust-struct";
    rust_struct.language = "rust";
    rust_struct.category = "struct";
    rust_struct.description = "Rust struct with impl";
    rust_struct.template_text = R"(/// {{description}}
#[derive(Debug, Clone{{derives}})]
pub struct {{struct_name}} {
{{fields}}
}

impl {{struct_name}} {
    /// Creates a new {{struct_name}}.
    pub fn new({{new_params}}) -> Self {
        Self {
{{new_body}}
        }
    }

{{methods}}
}
)";
    rust_struct.parameters = {
        {"struct_name", "string", "", "Struct name", true},
        {"description", "string", "A struct", "Description", false},
        {"derives", "string", "", "Additional derives", false},
        {"fields", "string", "", "Struct fields", false},
        {"new_params", "string", "", "Constructor parameters", false},
        {"new_body", "string", "", "Constructor body", false},
        {"methods", "string", "", "Methods", false}
    };
    register_template(rust_struct);
}

// ═══════════════════════════════════════════════════════════════════
//  FIX APPLIER IMPLEMENTATION
// ═══════════════════════════════════════════════════════════════════

std::vector<CodeFix> FixApplier::sort_fixes(std::vector<CodeFix> fixes) const {
    // Sort by location, reverse order (end of file first)
    std::sort(fixes.begin(), fixes.end(), [](const CodeFix& a, const CodeFix& b) {
        if (a.location.line != b.location.line) {
            return a.location.line > b.location.line;
        }
        return a.location.column > b.location.column;
    });
    return fixes;
}

bool FixApplier::fixes_overlap(const CodeFix& a, const CodeFix& b) const {
    // Check if two fixes affect overlapping regions
    if (a.location.file != b.location.file) return false;

    int a_start = a.location.line * 10000 + a.location.column;
    int a_end = a.location.end_line * 10000 + a.location.end_column;
    int b_start = b.location.line * 10000 + b.location.column;
    int b_end = b.location.end_line * 10000 + b.location.end_column;

    return !(a_end <= b_start || b_end <= a_start);
}

std::string FixApplier::apply_fix(const std::string& content, const CodeFix& fix) const {
    std::istringstream iss(content);
    std::ostringstream oss;
    std::string line;
    int line_num = 1;

    while (std::getline(iss, line)) {
        if (line_num == fix.location.line) {
            if (fix.location.end_line == fix.location.line || fix.location.end_line == 0) {
                // Single line fix
                int start_col = fix.location.column > 0 ? fix.location.column - 1 : 0;
                int end_col = fix.location.end_column > 0 ? fix.location.end_column - 1 : (int)line.length();

                if (start_col <= (int)line.length()) {
                    std::string before = line.substr(0, start_col);
                    std::string after = end_col <= (int)line.length() ? line.substr(end_col) : "";
                    oss << before << fix.new_text << after << "\n";
                } else {
                    oss << line << "\n";
                }
            } else {
                // Multi-line fix - more complex
                std::string before = line.substr(0, fix.location.column > 0 ? fix.location.column - 1 : 0);
                oss << before << fix.new_text;

                // Skip lines until end_line
                while (line_num < fix.location.end_line && std::getline(iss, line)) {
                    ++line_num;
                }

                // Add remaining part of end line
                if (fix.location.end_column > 0 && fix.location.end_column <= (int)line.length()) {
                    oss << line.substr(fix.location.end_column - 1);
                }
                oss << "\n";
            }
        } else {
            oss << line << "\n";
        }
        ++line_num;
    }

    return oss.str();
}

FixResult FixApplier::apply_fixes(const std::string& content,
                                   const std::vector<CodeFix>& fixes,
                                   bool safe_only) const {
    FixResult result;
    result.original_content = content;
    result.fixed_content = content;

    // Filter and sort fixes
    std::vector<CodeFix> applicable;
    for (const auto& fix : fixes) {
        if (!safe_only || fix.is_safe) {
            applicable.push_back(fix);
        }
    }

    // Sort by position (reverse order for safe application)
    applicable = sort_fixes(applicable);

    // Remove overlapping fixes (keep first = highest position)
    std::vector<CodeFix> non_overlapping;
    for (const auto& fix : applicable) {
        bool overlaps = false;
        for (const auto& existing : non_overlapping) {
            if (fixes_overlap(fix, existing)) {
                overlaps = true;
                break;
            }
        }
        if (!overlaps) {
            non_overlapping.push_back(fix);
        }
    }

    // Apply fixes
    for (const auto& fix : non_overlapping) {
        result.fixed_content = apply_fix(result.fixed_content, fix);
        result.applied_fixes.push_back(fix);
        result.fixes_applied++;
    }

    result.success = true;
    return result;
}

FixResult FixApplier::apply_fixes_to_file(const std::string& path,
                                           const std::vector<CodeFix>& fixes,
                                           bool safe_only) const {
    std::ifstream ifs(path);
    if (!ifs) {
        FixResult result;
        result.success = false;
        result.errors.push_back("Cannot read file: " + path);
        return result;
    }

    std::stringstream ss;
    ss << ifs.rdbuf();
    std::string content = ss.str();
    ifs.close();

    auto result = apply_fixes(content, fixes, safe_only);

    if (result.success && result.fixes_applied > 0) {
        std::ofstream ofs(path);
        if (ofs) {
            ofs << result.fixed_content;
        } else {
            result.success = false;
            result.errors.push_back("Cannot write file: " + path);
        }
    }

    return result;
}

std::string FixApplier::preview_fixes(const std::string& content,
                                      const std::vector<CodeFix>& fixes) const {
    auto result = apply_fixes(content, fixes, false);
    return result.fixed_content;
}

std::string FixApplier::generate_diff(const std::string& original,
                                      const std::string& fixed,
                                      const std::string& filename) const {
    std::ostringstream oss;
    oss << "--- " << (filename.empty() ? "a/file" : "a/" + filename) << "\n";
    oss << "+++ " << (filename.empty() ? "b/file" : "b/" + filename) << "\n";

    std::istringstream orig_iss(original);
    std::istringstream fix_iss(fixed);
    std::string orig_line, fix_line;
    int line_num = 0;

    while (true) {
        bool has_orig = static_cast<bool>(std::getline(orig_iss, orig_line));
        bool has_fix = static_cast<bool>(std::getline(fix_iss, fix_line));

        if (!has_orig && !has_fix) break;

        ++line_num;

        if (has_orig && has_fix && orig_line == fix_line) {
            oss << " " << orig_line << "\n";
        } else {
            if (has_orig && orig_line != fix_line) {
                oss << "-" << orig_line << "\n";
            }
            if (has_fix && orig_line != fix_line) {
                oss << "+" << fix_line << "\n";
            }
        }
    }

    return oss.str();
}

// ═══════════════════════════════════════════════════════════════════
//  AUTO FIXER IMPLEMENTATION
// ═══════════════════════════════════════════════════════════════════

void AutoFixer::register_rule(std::shared_ptr<AutoFixRule> rule) {
    rules_.push_back(rule);
}

std::vector<CodeFix> AutoFixer::find_all_fixes(const std::string& content,
                                               const std::string& filename,
                                               const std::string& language) const {
    std::vector<CodeFix> all_fixes;

    for (const auto& rule : rules_) {
        if (language.empty() || rule->language().empty() || rule->language() == language) {
            auto fixes = rule->find_fixes(content, filename);
            all_fixes.insert(all_fixes.end(), fixes.begin(), fixes.end());
        }
    }

    return all_fixes;
}

FixResult AutoFixer::auto_fix(const std::string& content,
                              const std::string& filename,
                              const std::string& language) const {
    auto fixes = find_all_fixes(content, filename, language);
    FixApplier applier;
    return applier.apply_fixes(content, fixes, true);
}

std::vector<std::string> AutoFixer::get_rule_ids() const {
    std::vector<std::string> ids;
    for (const auto& rule : rules_) {
        ids.push_back(rule->id());
    }
    return ids;
}

void AutoFixer::load_default_rules() {
    register_rule(std::make_shared<TrailingWhitespaceRule>());
    register_rule(std::make_shared<IndentationRule>());
    register_rule(std::make_shared<MissingSemicolonRule>());
    register_rule(std::make_shared<MissingIncludeRule>());
}

// ═══════════════════════════════════════════════════════════════════
//  BUILT-IN RULES IMPLEMENTATION
// ═══════════════════════════════════════════════════════════════════

std::vector<CodeFix> TrailingWhitespaceRule::find_fixes(const std::string& content,
                                                        const std::string& filename) const {
    std::vector<CodeFix> fixes;
    std::istringstream iss(content);
    std::string line;
    int line_num = 0;

    while (std::getline(iss, line)) {
        ++line_num;

        // Check for trailing whitespace
        size_t last_non_ws = line.find_last_not_of(" \t\r");
        if (last_non_ws != std::string::npos && last_non_ws < line.length() - 1) {
            CodeFix fix;
            fix.description = "Remove trailing whitespace";
            fix.fix_id = "trailing-whitespace";
            fix.location.file = filename;
            fix.location.line = line_num;
            fix.location.column = static_cast<int>(last_non_ws + 2);
            fix.location.end_line = line_num;
            fix.location.end_column = static_cast<int>(line.length() + 1);
            fix.old_text = line.substr(last_non_ws + 1);
            fix.new_text = "";
            fix.is_safe = true;
            fixes.push_back(fix);
        } else if (last_non_ws == std::string::npos && !line.empty()) {
            // Line is all whitespace
            CodeFix fix;
            fix.description = "Remove trailing whitespace";
            fix.fix_id = "trailing-whitespace";
            fix.location.file = filename;
            fix.location.line = line_num;
            fix.location.column = 1;
            fix.location.end_line = line_num;
            fix.location.end_column = static_cast<int>(line.length() + 1);
            fix.old_text = line;
            fix.new_text = "";
            fix.is_safe = true;
            fixes.push_back(fix);
        }
    }

    return fixes;
}

std::vector<CodeFix> IndentationRule::find_fixes(const std::string& content,
                                                 const std::string& filename) const {
    std::vector<CodeFix> fixes;
    std::istringstream iss(content);
    std::string line;
    int line_num = 0;

    std::string target_indent = use_tabs_ ? "\t" : std::string(spaces_, ' ');

    while (std::getline(iss, line)) {
        ++line_num;
        if (line.empty()) continue;

        // Find leading whitespace
        size_t first_non_ws = line.find_first_not_of(" \t");
        if (first_non_ws == std::string::npos || first_non_ws == 0) continue;

        std::string leading = line.substr(0, first_non_ws);

        // Check for mixed tabs and spaces
        bool has_tabs = leading.find('\t') != std::string::npos;
        bool has_spaces = leading.find(' ') != std::string::npos;

        if (has_tabs && has_spaces) {
            // Convert to consistent indentation
            int indent_level = 0;
            for (char c : leading) {
                if (c == '\t') indent_level++;
                else indent_level += 1;  // Count spaces
            }
            indent_level /= spaces_;

            std::string new_indent;
            for (int i = 0; i < indent_level; ++i) {
                new_indent += target_indent;
            }

            CodeFix fix;
            fix.description = "Fix inconsistent indentation";
            fix.fix_id = "indentation";
            fix.location.file = filename;
            fix.location.line = line_num;
            fix.location.column = 1;
            fix.location.end_line = line_num;
            fix.location.end_column = static_cast<int>(first_non_ws + 1);
            fix.old_text = leading;
            fix.new_text = new_indent;
            fix.is_safe = true;
            fixes.push_back(fix);
        }
    }

    return fixes;
}

std::vector<CodeFix> MissingSemicolonRule::find_fixes(const std::string& content,
                                                      const std::string& filename) const {
    std::vector<CodeFix> fixes;
    // Note: This is a simplified implementation
    // A full implementation would need proper JS parsing

    std::istringstream iss(content);
    std::string line;
    int line_num = 0;

    while (std::getline(iss, line)) {
        ++line_num;

        // Skip empty lines, comments, and lines ending with specific chars
        std::string trimmed = line;
        size_t end = trimmed.find_last_not_of(" \t\r");
        if (end == std::string::npos) continue;
        trimmed = trimmed.substr(0, end + 1);

        if (trimmed.empty()) continue;
        char last = trimmed.back();

        // Skip lines that don't need semicolons
        if (last == '{' || last == '}' || last == ';' || last == ',' ||
            last == '(' || last == '[' || last == ':' || last == '/' ||
            trimmed.find("//") != std::string::npos) continue;

        // Skip control structures
        if (trimmed.find("if ") == 0 || trimmed.find("if(") == 0 ||
            trimmed.find("else") == 0 || trimmed.find("for ") == 0 ||
            trimmed.find("for(") == 0 || trimmed.find("while ") == 0 ||
            trimmed.find("while(") == 0 || trimmed.find("switch ") == 0 ||
            trimmed.find("function ") == 0 || trimmed.find("class ") == 0 ||
            trimmed.find("try") == 0 || trimmed.find("catch") == 0) continue;

        // Potential missing semicolon
        // (This is a heuristic - proper parsing would be better)
        if (std::isalnum(last) || last == ')' || last == ']' || last == '"' || last == '\'') {
            CodeFix fix;
            fix.description = "Add missing semicolon";
            fix.fix_id = "missing-semicolon";
            fix.location.file = filename;
            fix.location.line = line_num;
            fix.location.column = static_cast<int>(trimmed.length() + 1);
            fix.location.end_line = line_num;
            fix.location.end_column = static_cast<int>(trimmed.length() + 1);
            fix.old_text = "";
            fix.new_text = ";";
            fix.is_safe = false;  // Not safe without proper parsing
            fixes.push_back(fix);
        }
    }

    return fixes;
}

std::vector<CodeFix> MissingIncludeRule::find_fixes(const std::string& content,
                                                    const std::string& filename) const {
    std::vector<CodeFix> fixes;

    // Map of common types to their headers
    static const std::map<std::string, std::string> type_to_header = {
        {"std::string", "<string>"},
        {"std::vector", "<vector>"},
        {"std::map", "<map>"},
        {"std::set", "<set>"},
        {"std::unordered_map", "<unordered_map>"},
        {"std::unordered_set", "<unordered_set>"},
        {"std::optional", "<optional>"},
        {"std::variant", "<variant>"},
        {"std::array", "<array>"},
        {"std::deque", "<deque>"},
        {"std::list", "<list>"},
        {"std::queue", "<queue>"},
        {"std::stack", "<stack>"},
        {"std::pair", "<utility>"},
        {"std::tuple", "<tuple>"},
        {"std::unique_ptr", "<memory>"},
        {"std::shared_ptr", "<memory>"},
        {"std::weak_ptr", "<memory>"},
        {"std::function", "<functional>"},
        {"std::thread", "<thread>"},
        {"std::mutex", "<mutex>"},
        {"std::atomic", "<atomic>"},
        {"std::chrono", "<chrono>"},
        {"std::regex", "<regex>"},
        {"std::fstream", "<fstream>"},
        {"std::ifstream", "<fstream>"},
        {"std::ofstream", "<fstream>"},
        {"std::stringstream", "<sstream>"},
        {"std::istringstream", "<sstream>"},
        {"std::ostringstream", "<sstream>"},
        {"std::cout", "<iostream>"},
        {"std::cin", "<iostream>"},
        {"std::cerr", "<iostream>"},
        {"std::endl", "<iostream>"},
        {"size_t", "<cstddef>"},
        {"int8_t", "<cstdint>"},
        {"int16_t", "<cstdint>"},
        {"int32_t", "<cstdint>"},
        {"int64_t", "<cstdint>"},
        {"uint8_t", "<cstdint>"},
        {"uint16_t", "<cstdint>"},
        {"uint32_t", "<cstdint>"},
        {"uint64_t", "<cstdint>"}
    };

    // Find existing includes
    std::set<std::string> existing_includes;
    std::regex include_re(R"(#include\s*[<"]([^>"]+)[>"])");
    auto begin = std::sregex_iterator(content.begin(), content.end(), include_re);
    auto end = std::sregex_iterator();
    for (auto it = begin; it != end; ++it) {
        existing_includes.insert((*it)[1].str());
    }

    // Find last include line
    int last_include_line = 0;
    std::istringstream iss(content);
    std::string line;
    int line_num = 0;
    while (std::getline(iss, line)) {
        ++line_num;
        if (line.find("#include") != std::string::npos) {
            last_include_line = line_num;
        }
    }

    // Check for missing includes
    std::set<std::string> needed_includes;
    for (const auto& [type, header] : type_to_header) {
        if (content.find(type) != std::string::npos) {
            std::string header_name = header.substr(1, header.length() - 2);
            if (existing_includes.find(header_name) == existing_includes.end()) {
                needed_includes.insert(header);
            }
        }
    }

    // Generate fixes for missing includes
    for (const auto& header : needed_includes) {
        CodeFix fix;
        fix.description = "Add missing include: " + header;
        fix.fix_id = "missing-include";
        fix.location.file = filename;
        fix.location.line = last_include_line > 0 ? last_include_line : 1;
        fix.location.column = 1;
        fix.location.end_line = fix.location.line;
        fix.location.end_column = 1;
        fix.old_text = "";
        fix.new_text = "#include " + header + "\n";
        fix.is_safe = true;
        fixes.push_back(fix);
    }

    return fixes;
}

// ═══════════════════════════════════════════════════════════════════
//  REFACTORER IMPLEMENTATION
// ═══════════════════════════════════════════════════════════════════

std::vector<std::pair<int, int>> Refactorer::find_symbol_locations(const std::string& content,
                                                                   const std::string& symbol) {
    std::vector<std::pair<int, int>> locations;

    // Use word boundary regex to find symbol
    std::regex re("\\b" + symbol + "\\b");
    auto begin = std::sregex_iterator(content.begin(), content.end(), re);
    auto end = std::sregex_iterator();

    for (auto it = begin; it != end; ++it) {
        int pos = static_cast<int>(it->position());

        // Convert position to line:column
        int line = 1, col = 1;
        for (int i = 0; i < pos; ++i) {
            if (content[i] == '\n') {
                ++line;
                col = 1;
            } else {
                ++col;
            }
        }
        locations.push_back({line, col});
    }

    return locations;
}

RenameResult Refactorer::rename_symbol(const std::string& old_name,
                                       const std::string& new_name,
                                       const std::vector<std::string>& files) {
    RenameResult result;
    result.success = true;

    std::regex re("\\b" + old_name + "\\b");

    for (const auto& file : files) {
        std::ifstream ifs(file);
        if (!ifs) {
            result.errors.push_back("Cannot read: " + file);
            continue;
        }

        std::stringstream ss;
        ss << ifs.rdbuf();
        std::string content = ss.str();
        ifs.close();

        // Count occurrences
        auto begin = std::sregex_iterator(content.begin(), content.end(), re);
        auto end = std::sregex_iterator();
        int count = std::distance(begin, end);

        if (count > 0) {
            std::string new_content = std::regex_replace(content, re, new_name);
            result.file_changes[file] = new_content;
            result.occurrences += count;
        }
    }

    return result;
}

ExtractResult Refactorer::extract_function(const std::string& content,
                                           int start_line, int end_line,
                                           const std::string& function_name,
                                           const std::string& language) {
    ExtractResult result;

    std::istringstream iss(content);
    std::ostringstream extracted;
    std::ostringstream modified;
    std::string line;
    int line_num = 0;

    while (std::getline(iss, line)) {
        ++line_num;

        if (line_num >= start_line && line_num <= end_line) {
            extracted << line << "\n";
            if (line_num == start_line) {
                // Insert function call
                std::string indent;
                for (char c : line) {
                    if (c == ' ' || c == '\t') indent += c;
                    else break;
                }
                modified << indent << function_name << "();\n";
            }
        } else {
            modified << line << "\n";
        }
    }

    // Create function wrapper based on language
    std::ostringstream func;
    if (language == "cpp" || language == "c") {
        func << "void " << function_name << "() {\n";
        func << extracted.str();
        func << "}\n";
    } else if (language == "python") {
        func << "def " << function_name << "():\n";
        // Add indent to extracted code
        std::istringstream ext_iss(extracted.str());
        std::string ext_line;
        while (std::getline(ext_iss, ext_line)) {
            func << "    " << ext_line << "\n";
        }
    } else if (language == "javascript" || language == "typescript") {
        func << "function " << function_name << "() {\n";
        func << extracted.str();
        func << "}\n";
    } else {
        func << extracted.str();
    }

    result.success = true;
    result.extracted_code = func.str();
    result.modified_original = modified.str();

    return result;
}

std::string Refactorer::add_import(const std::string& content,
                                   const std::string& import_statement,
                                   const std::string& language) {
    // Find appropriate location for import
    std::istringstream iss(content);
    std::ostringstream oss;
    std::string line;
    bool import_added = false;
    int last_import_line = 0;
    int line_num = 0;

    // First pass: find last import line
    while (std::getline(iss, line)) {
        ++line_num;
        if (language == "python") {
            if (line.find("import ") == 0 || line.find("from ") == 0) {
                last_import_line = line_num;
            }
        } else if (language == "javascript" || language == "typescript") {
            if (line.find("import ") == 0 || line.find("require(") != std::string::npos) {
                last_import_line = line_num;
            }
        } else if (language == "cpp" || language == "c") {
            if (line.find("#include") == 0) {
                last_import_line = line_num;
            }
        }
    }

    // Second pass: insert import
    iss.clear();
    iss.str(content);
    line_num = 0;

    while (std::getline(iss, line)) {
        ++line_num;
        oss << line << "\n";

        if (line_num == last_import_line && !import_added) {
            oss << import_statement << "\n";
            import_added = true;
        }
    }

    // If no imports found, add at beginning
    if (!import_added) {
        return import_statement + "\n" + content;
    }

    return oss.str();
}

// ═══════════════════════════════════════════════════════════════════
//  CODE GENERATOR IMPLEMENTATION
// ═══════════════════════════════════════════════════════════════════

CodeGenerator::CodeGenerator(TemplateEngine& templates) : templates_(templates) {}

std::vector<GeneratedFile> CodeGenerator::generate(const GenerationRequest& request) const {
    std::vector<GeneratedFile> files;

    if (request.type == "class") {
        GeneratedFile gf;
        gf.path = request.target_directory + "/" + request.name + (request.language == "cpp" ? ".h" : ".py");
        gf.content = generate_class(request.name, request.language);
        files.push_back(gf);
    } else if (request.type == "function") {
        GeneratedFile gf;
        gf.content = generate_function(request.name, request.language);
        files.push_back(gf);
    } else if (request.type == "test") {
        GeneratedFile gf;
        gf.path = request.target_directory + "/test_" + request.name + (request.language == "python" ? ".py" : ".cpp");
        gf.content = generate_test(request.name, request.language);
        files.push_back(gf);
    }

    return files;
}

std::string CodeGenerator::generate_class(const std::string& name,
                                          const std::string& language,
                                          const std::vector<std::string>& methods,
                                          const std::vector<std::string>& fields) const {
    std::map<std::string, std::string> params;
    params["class_name"] = name;

    std::string methods_str;
    for (const auto& m : methods) {
        methods_str += "    " + m + "\n";
    }
    params["methods"] = methods_str;

    std::string fields_str;
    for (const auto& f : fields) {
        fields_str += "    " + f + "\n";
    }
    params["fields"] = fields_str;

    std::string tmpl_name;
    if (language == "cpp" || language == "c++") {
        tmpl_name = "cpp-class";
    } else if (language == "python") {
        tmpl_name = "python-class";
    } else if (language == "javascript" || language == "typescript") {
        tmpl_name = "js-class";
    } else if (language == "rust") {
        tmpl_name = "rust-struct";
        params["struct_name"] = name;
    } else {
        return "// Unsupported language: " + language;
    }

    return templates_.render(tmpl_name, params);
}

std::string CodeGenerator::generate_function(const std::string& name,
                                             const std::string& language,
                                             const std::vector<std::string>& params_list,
                                             const std::string& return_type) const {
    std::map<std::string, std::string> params;
    params["function_name"] = name;
    params["return_type"] = return_type.empty() ? "void" : return_type;

    std::string params_str;
    for (size_t i = 0; i < params_list.size(); ++i) {
        if (i > 0) params_str += ", ";
        params_str += params_list[i];
    }
    params["params"] = params_str;

    std::string tmpl_name;
    if (language == "cpp" || language == "c++") {
        tmpl_name = "cpp-function";
    } else if (language == "python") {
        tmpl_name = "python-function";
    } else {
        // Generic fallback
        std::ostringstream oss;
        oss << "function " << name << "(" << params_str << ") {\n";
        oss << "    // TODO: implement\n";
        oss << "}\n";
        return oss.str();
    }

    return templates_.render(tmpl_name, params);
}

std::string CodeGenerator::generate_test(const std::string& target_name,
                                         const std::string& language,
                                         const std::string& test_framework) const {
    std::ostringstream oss;

    if (language == "python") {
        oss << "import pytest\n\n";
        oss << "class Test" << target_name << ":\n";
        oss << "    def test_basic(self):\n";
        oss << "        # TODO: implement test\n";
        oss << "        assert True\n";
    } else if (language == "cpp" || language == "c++") {
        oss << "#include <gtest/gtest.h>\n\n";
        oss << "TEST(" << target_name << "Test, Basic) {\n";
        oss << "    // TODO: implement test\n";
        oss << "    EXPECT_TRUE(true);\n";
        oss << "}\n";
    } else if (language == "javascript" || language == "typescript") {
        oss << "describe('" << target_name << "', () => {\n";
        oss << "    test('should work', () => {\n";
        oss << "        // TODO: implement test\n";
        oss << "        expect(true).toBe(true);\n";
        oss << "    });\n";
        oss << "});\n";
    } else if (language == "rust") {
        oss << "#[cfg(test)]\n";
        oss << "mod tests {\n";
        oss << "    use super::*;\n\n";
        oss << "    #[test]\n";
        oss << "    fn test_basic() {\n";
        oss << "        // TODO: implement test\n";
        oss << "        assert!(true);\n";
        oss << "    }\n";
        oss << "}\n";
    }

    return oss.str();
}

std::string CodeGenerator::generate_header_guard(const std::string& filename) const {
    std::string guard = filename;
    std::transform(guard.begin(), guard.end(), guard.begin(), ::toupper);
    std::replace(guard.begin(), guard.end(), '.', '_');
    std::replace(guard.begin(), guard.end(), '/', '_');
    std::replace(guard.begin(), guard.end(), '-', '_');
    return guard + "_";
}

std::string CodeGenerator::generate_copyright_header(const std::string& author,
                                                     const std::string& license) const {
    std::ostringstream oss;
    oss << "/*\n";
    if (!author.empty()) {
        oss << " * Copyright (c) " << author << "\n";
    }
    if (!license.empty()) {
        oss << " * License: " << license << "\n";
    }
    oss << " */\n";
    return oss.str();
}

std::string CodeGenerator::generate_docstring(const std::string& description,
                                              const std::vector<std::string>& params,
                                              const std::string& returns,
                                              const std::string& language) const {
    std::ostringstream oss;

    if (language == "python") {
        oss << "    \"\"\"\n";
        oss << "    " << description << "\n\n";
        if (!params.empty()) {
            oss << "    Args:\n";
            for (const auto& p : params) {
                oss << "        " << p << ": Description\n";
            }
            oss << "\n";
        }
        oss << "    Returns:\n";
        oss << "        " << (returns.empty() ? "None" : returns) << "\n";
        oss << "    \"\"\"\n";
    } else if (language == "cpp" || language == "c++" || language == "c") {
        oss << "/**\n";
        oss << " * @brief " << description << "\n";
        for (const auto& p : params) {
            oss << " * @param " << p << " Description\n";
        }
        oss << " * @return " << (returns.empty() ? "void" : returns) << "\n";
        oss << " */\n";
    } else if (language == "javascript" || language == "typescript") {
        oss << "/**\n";
        oss << " * " << description << "\n";
        for (const auto& p : params) {
            oss << " * @param {*} " << p << " - Description\n";
        }
        oss << " * @returns {" << (returns.empty() ? "void" : returns) << "}\n";
        oss << " */\n";
    }

    return oss.str();
}

} // namespace rael
