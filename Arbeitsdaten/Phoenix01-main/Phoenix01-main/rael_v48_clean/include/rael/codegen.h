// RAEL V48 - Code Generation + Auto-Fix (#18)
// Generates code, applies fixes, and performs refactoring
#pragma once

#include <string>
#include <vector>
#include <map>
#include <functional>
#include <optional>
#include <memory>

namespace rael {

// ═══════════════════════════════════════════════════════════════════
//  CODE TEMPLATES
// ═══════════════════════════════════════════════════════════════════

struct TemplateParameter {
    std::string name;
    std::string type;
    std::string default_value;
    std::string description;
    bool required = true;
};

struct CodeTemplate {
    std::string name;
    std::string language;
    std::string category;  // class, function, test, module, etc.
    std::string template_text;
    std::vector<TemplateParameter> parameters;
    std::string description;
};

class TemplateEngine {
public:
    // Register a template
    void register_template(const CodeTemplate& tmpl);

    // Get available templates
    std::vector<CodeTemplate> get_templates(const std::string& language = "") const;
    std::optional<CodeTemplate> get_template(const std::string& name) const;

    // Render a template with parameters
    std::string render(const std::string& template_name,
                       const std::map<std::string, std::string>& params) const;
    std::string render(const CodeTemplate& tmpl,
                       const std::map<std::string, std::string>& params) const;

    // Validate parameters
    bool validate_params(const CodeTemplate& tmpl,
                         const std::map<std::string, std::string>& params,
                         std::vector<std::string>& errors) const;

    // Load built-in templates
    void load_defaults();

private:
    std::map<std::string, CodeTemplate> templates_;
    std::string substitute(const std::string& text,
                           const std::map<std::string, std::string>& params) const;
};

// ═══════════════════════════════════════════════════════════════════
//  CODE FIXES
// ═══════════════════════════════════════════════════════════════════

struct CodeLocation {
    std::string file;
    int line = 0;
    int column = 0;
    int end_line = 0;
    int end_column = 0;
};

struct CodeIssue {
    std::string id;
    std::string message;
    std::string severity;  // error, warning, info, hint
    std::string category;  // syntax, style, security, performance
    CodeLocation location;
    std::string source;    // compiler, linter, analyzer
    std::vector<std::string> suggested_fixes;
};

struct CodeFix {
    std::string description;
    CodeLocation location;
    std::string old_text;
    std::string new_text;
    bool is_safe = true;  // Can be applied automatically
    std::string fix_id;
};

struct FixResult {
    bool success = false;
    std::string original_content;
    std::string fixed_content;
    std::vector<CodeFix> applied_fixes;
    std::vector<std::string> errors;
    int fixes_applied = 0;
};

class FixApplier {
public:
    // Apply a single fix to content
    std::string apply_fix(const std::string& content, const CodeFix& fix) const;

    // Apply multiple fixes (handles overlapping)
    FixResult apply_fixes(const std::string& content,
                          const std::vector<CodeFix>& fixes,
                          bool safe_only = true) const;

    // Apply fixes to a file
    FixResult apply_fixes_to_file(const std::string& path,
                                   const std::vector<CodeFix>& fixes,
                                   bool safe_only = true) const;

    // Preview fixes without applying
    std::string preview_fixes(const std::string& content,
                              const std::vector<CodeFix>& fixes) const;

    // Generate diff
    std::string generate_diff(const std::string& original,
                              const std::string& fixed,
                              const std::string& filename = "") const;

private:
    // Sort fixes by location (reverse order for safe application)
    std::vector<CodeFix> sort_fixes(std::vector<CodeFix> fixes) const;

    // Check if fixes overlap
    bool fixes_overlap(const CodeFix& a, const CodeFix& b) const;
};

// ═══════════════════════════════════════════════════════════════════
//  AUTO-FIX RULES
// ═══════════════════════════════════════════════════════════════════

class AutoFixRule {
public:
    virtual ~AutoFixRule() = default;
    virtual std::string id() const = 0;
    virtual std::string description() const = 0;
    virtual std::string language() const = 0;  // empty = all
    virtual std::vector<CodeFix> find_fixes(const std::string& content,
                                            const std::string& filename) const = 0;
};

class AutoFixer {
public:
    // Register a fix rule
    void register_rule(std::shared_ptr<AutoFixRule> rule);

    // Find all available fixes
    std::vector<CodeFix> find_all_fixes(const std::string& content,
                                        const std::string& filename,
                                        const std::string& language = "") const;

    // Find fixes for specific issue
    std::vector<CodeFix> find_fixes_for_issue(const CodeIssue& issue,
                                              const std::string& content) const;

    // Auto-fix common issues
    FixResult auto_fix(const std::string& content,
                       const std::string& filename,
                       const std::string& language = "") const;

    // Get available rules
    std::vector<std::string> get_rule_ids() const;

    // Load built-in rules
    void load_default_rules();

private:
    std::vector<std::shared_ptr<AutoFixRule>> rules_;
};

// ═══════════════════════════════════════════════════════════════════
//  BUILT-IN FIX RULES
// ═══════════════════════════════════════════════════════════════════

// Remove trailing whitespace
class TrailingWhitespaceRule : public AutoFixRule {
public:
    std::string id() const override { return "trailing-whitespace"; }
    std::string description() const override { return "Remove trailing whitespace"; }
    std::string language() const override { return ""; }
    std::vector<CodeFix> find_fixes(const std::string& content, const std::string& filename) const override;
};

// Fix inconsistent indentation
class IndentationRule : public AutoFixRule {
public:
    explicit IndentationRule(int spaces = 4, bool use_tabs = false)
        : spaces_(spaces), use_tabs_(use_tabs) {}
    std::string id() const override { return "indentation"; }
    std::string description() const override { return "Fix inconsistent indentation"; }
    std::string language() const override { return ""; }
    std::vector<CodeFix> find_fixes(const std::string& content, const std::string& filename) const override;
private:
    int spaces_;
    bool use_tabs_;
};

// Add missing semicolons (JS/TS)
class MissingSemicolonRule : public AutoFixRule {
public:
    std::string id() const override { return "missing-semicolon"; }
    std::string description() const override { return "Add missing semicolons"; }
    std::string language() const override { return "javascript"; }
    std::vector<CodeFix> find_fixes(const std::string& content, const std::string& filename) const override;
};

// Fix missing includes (C++)
class MissingIncludeRule : public AutoFixRule {
public:
    std::string id() const override { return "missing-include"; }
    std::string description() const override { return "Add missing #include directives"; }
    std::string language() const override { return "cpp"; }
    std::vector<CodeFix> find_fixes(const std::string& content, const std::string& filename) const override;
};

// ═══════════════════════════════════════════════════════════════════
//  REFACTORING
// ═══════════════════════════════════════════════════════════════════

struct RenameResult {
    bool success = false;
    std::map<std::string, std::string> file_changes;  // file -> new content
    int occurrences = 0;
    std::vector<std::string> errors;
};

struct ExtractResult {
    bool success = false;
    std::string extracted_code;
    std::string modified_original;
    std::string new_file_path;  // For extract to file
    std::vector<std::string> errors;
};

class Refactorer {
public:
    // Rename symbol across files
    RenameResult rename_symbol(const std::string& old_name,
                               const std::string& new_name,
                               const std::vector<std::string>& files);

    // Extract function/method
    ExtractResult extract_function(const std::string& content,
                                   int start_line, int end_line,
                                   const std::string& function_name,
                                   const std::string& language);

    // Extract variable
    ExtractResult extract_variable(const std::string& content,
                                   int line, int start_col, int end_col,
                                   const std::string& var_name,
                                   const std::string& language);

    // Inline function/variable
    std::string inline_symbol(const std::string& content,
                              const std::string& symbol_name,
                              const std::string& language);

    // Convert between styles (e.g., for loop to forEach)
    std::string convert_loop_style(const std::string& content,
                                   const std::string& from_style,
                                   const std::string& to_style,
                                   const std::string& language);

    // Add/remove imports
    std::string add_import(const std::string& content,
                           const std::string& import_statement,
                           const std::string& language);
    std::string remove_unused_imports(const std::string& content,
                                      const std::string& language);

private:
    std::vector<std::pair<int, int>> find_symbol_locations(const std::string& content,
                                                           const std::string& symbol);
};

// ═══════════════════════════════════════════════════════════════════
//  CODE GENERATOR
// ═══════════════════════════════════════════════════════════════════

struct GeneratedFile {
    std::string path;
    std::string content;
    bool overwrite = false;
};

struct GenerationRequest {
    std::string type;  // class, function, module, test, etc.
    std::string name;
    std::string language;
    std::map<std::string, std::string> options;
    std::string target_directory;
};

class CodeGenerator {
public:
    explicit CodeGenerator(TemplateEngine& templates);

    // Generate code from request
    std::vector<GeneratedFile> generate(const GenerationRequest& request) const;

    // Generate specific constructs
    std::string generate_class(const std::string& name,
                               const std::string& language,
                               const std::vector<std::string>& methods = {},
                               const std::vector<std::string>& fields = {}) const;

    std::string generate_function(const std::string& name,
                                  const std::string& language,
                                  const std::vector<std::string>& params = {},
                                  const std::string& return_type = "") const;

    std::string generate_test(const std::string& target_name,
                              const std::string& language,
                              const std::string& test_framework = "") const;

    std::string generate_interface(const std::string& name,
                                   const std::string& language,
                                   const std::vector<std::string>& methods = {}) const;

    // Generate boilerplate
    std::string generate_header_guard(const std::string& filename) const;
    std::string generate_copyright_header(const std::string& author = "",
                                          const std::string& license = "") const;

    // Generate documentation
    std::string generate_docstring(const std::string& description,
                                   const std::vector<std::string>& params,
                                   const std::string& returns,
                                   const std::string& language) const;

private:
    TemplateEngine& templates_;
};

} // namespace rael
