// RAEL V48 - LSP/IDE Integration (#20)
// Language Server Protocol implementation for IDE integration
#pragma once

#include <string>
#include <vector>
#include <map>
#include <optional>
#include <functional>
#include <memory>
#include <variant>

namespace rael {

// ═══════════════════════════════════════════════════════════════════
//  LSP TYPES
// ═══════════════════════════════════════════════════════════════════

struct Position {
    int line = 0;      // 0-based
    int character = 0; // 0-based
};

struct Range {
    Position start;
    Position end;
};

struct Location {
    std::string uri;
    Range range;
};

struct TextEdit {
    Range range;
    std::string new_text;
};

struct TextDocumentIdentifier {
    std::string uri;
};

struct TextDocumentItem {
    std::string uri;
    std::string language_id;
    int version = 0;
    std::string text;
};

struct VersionedTextDocumentIdentifier : TextDocumentIdentifier {
    int version = 0;
};

struct TextDocumentPositionParams {
    TextDocumentIdentifier text_document;
    Position position;
};

// ═══════════════════════════════════════════════════════════════════
//  DIAGNOSTICS
// ═══════════════════════════════════════════════════════════════════

enum class DiagnosticSeverity {
    Error = 1,
    Warning = 2,
    Information = 3,
    Hint = 4
};

struct DiagnosticRelatedInformation {
    Location location;
    std::string message;
};

struct Diagnostic {
    Range range;
    DiagnosticSeverity severity = DiagnosticSeverity::Error;
    std::string code;
    std::string source;
    std::string message;
    std::vector<DiagnosticRelatedInformation> related_information;
};

// ═══════════════════════════════════════════════════════════════════
//  COMPLETION
// ═══════════════════════════════════════════════════════════════════

enum class CompletionItemKind {
    Text = 1, Method = 2, Function = 3, Constructor = 4, Field = 5,
    Variable = 6, Class = 7, Interface = 8, Module = 9, Property = 10,
    Unit = 11, Value = 12, Enum = 13, Keyword = 14, Snippet = 15,
    Color = 16, File = 17, Reference = 18, Folder = 19, EnumMember = 20,
    Constant = 21, Struct = 22, Event = 23, Operator = 24, TypeParameter = 25
};

enum class InsertTextFormat {
    PlainText = 1,
    Snippet = 2
};

struct CompletionItem {
    std::string label;
    CompletionItemKind kind = CompletionItemKind::Text;
    std::string detail;
    std::string documentation;
    bool deprecated = false;
    bool preselect = false;
    std::string sort_text;
    std::string filter_text;
    std::string insert_text;
    InsertTextFormat insert_text_format = InsertTextFormat::PlainText;
    std::optional<TextEdit> text_edit;
    std::vector<TextEdit> additional_text_edits;
    std::vector<std::string> commit_characters;
};

struct CompletionList {
    bool is_incomplete = false;
    std::vector<CompletionItem> items;
};

// ═══════════════════════════════════════════════════════════════════
//  HOVER
// ═══════════════════════════════════════════════════════════════════

struct MarkupContent {
    std::string kind;  // "plaintext" or "markdown"
    std::string value;
};

struct Hover {
    MarkupContent contents;
    std::optional<Range> range;
};

// ═══════════════════════════════════════════════════════════════════
//  SYMBOLS
// ═══════════════════════════════════════════════════════════════════

enum class SymbolKind {
    File = 1, Module = 2, Namespace = 3, Package = 4, Class = 5,
    Method = 6, Property = 7, Field = 8, Constructor = 9, Enum = 10,
    Interface = 11, Function = 12, Variable = 13, Constant = 14,
    String = 15, Number = 16, Boolean = 17, Array = 18, Object = 19,
    Key = 20, Null = 21, EnumMember = 22, Struct = 23, Event = 24,
    Operator = 25, TypeParameter = 26
};

struct DocumentSymbol {
    std::string name;
    std::string detail;
    SymbolKind kind = SymbolKind::Function;
    bool deprecated = false;
    Range range;
    Range selection_range;
    std::vector<DocumentSymbol> children;
};

struct SymbolInformation {
    std::string name;
    SymbolKind kind = SymbolKind::Function;
    bool deprecated = false;
    Location location;
    std::string container_name;
};

// ═══════════════════════════════════════════════════════════════════
//  CODE ACTIONS
// ═══════════════════════════════════════════════════════════════════

enum class CodeActionKind {
    Empty,
    QuickFix,
    Refactor,
    RefactorExtract,
    RefactorInline,
    RefactorRewrite,
    Source,
    SourceOrganizeImports,
    SourceFixAll
};

struct WorkspaceEdit {
    std::map<std::string, std::vector<TextEdit>> changes;
};

struct CodeAction {
    std::string title;
    CodeActionKind kind = CodeActionKind::QuickFix;
    std::vector<Diagnostic> diagnostics;
    bool is_preferred = false;
    std::optional<WorkspaceEdit> edit;
    std::string command;
};

// ═══════════════════════════════════════════════════════════════════
//  SIGNATURE HELP
// ═══════════════════════════════════════════════════════════════════

struct ParameterInformation {
    std::string label;
    std::string documentation;
};

struct SignatureInformation {
    std::string label;
    std::string documentation;
    std::vector<ParameterInformation> parameters;
    std::optional<int> active_parameter;
};

struct SignatureHelp {
    std::vector<SignatureInformation> signatures;
    std::optional<int> active_signature;
    std::optional<int> active_parameter;
};

// ═══════════════════════════════════════════════════════════════════
//  FORMATTING
// ═══════════════════════════════════════════════════════════════════

struct FormattingOptions {
    int tab_size = 4;
    bool insert_spaces = true;
    bool trim_trailing_whitespace = true;
    bool insert_final_newline = true;
    bool trim_final_newlines = true;
};

// ═══════════════════════════════════════════════════════════════════
//  CAPABILITIES
// ═══════════════════════════════════════════════════════════════════

struct ServerCapabilities {
    bool hover_provider = true;
    bool completion_provider = true;
    bool signature_help_provider = true;
    bool definition_provider = true;
    bool type_definition_provider = true;
    bool implementation_provider = true;
    bool references_provider = true;
    bool document_highlight_provider = true;
    bool document_symbol_provider = true;
    bool workspace_symbol_provider = true;
    bool code_action_provider = true;
    bool code_lens_provider = false;
    bool document_formatting_provider = true;
    bool document_range_formatting_provider = true;
    bool rename_provider = true;
    bool folding_range_provider = true;
    bool semantic_tokens_provider = false;
    std::vector<std::string> trigger_characters;
    std::vector<std::string> signature_trigger_characters;
};

// ═══════════════════════════════════════════════════════════════════
//  LANGUAGE PROVIDER INTERFACE
// ═══════════════════════════════════════════════════════════════════

class LanguageProvider {
public:
    virtual ~LanguageProvider() = default;

    virtual std::string language_id() const = 0;
    virtual std::vector<std::string> file_extensions() const = 0;

    // Core features
    virtual std::vector<Diagnostic> get_diagnostics(const std::string& uri,
                                                    const std::string& content) = 0;

    virtual std::optional<Hover> get_hover(const std::string& uri,
                                           const std::string& content,
                                           Position position) = 0;

    virtual CompletionList get_completions(const std::string& uri,
                                           const std::string& content,
                                           Position position) = 0;

    virtual std::vector<Location> get_definition(const std::string& uri,
                                                  const std::string& content,
                                                  Position position) = 0;

    virtual std::vector<Location> get_references(const std::string& uri,
                                                  const std::string& content,
                                                  Position position,
                                                  bool include_declaration) = 0;

    virtual std::vector<DocumentSymbol> get_document_symbols(const std::string& uri,
                                                              const std::string& content) = 0;

    virtual std::optional<SignatureHelp> get_signature_help(const std::string& uri,
                                                            const std::string& content,
                                                            Position position) = 0;

    virtual std::vector<TextEdit> format_document(const std::string& uri,
                                                   const std::string& content,
                                                   const FormattingOptions& options) = 0;

    virtual std::vector<CodeAction> get_code_actions(const std::string& uri,
                                                      const std::string& content,
                                                      Range range,
                                                      const std::vector<Diagnostic>& diagnostics) = 0;

    virtual std::optional<WorkspaceEdit> rename(const std::string& uri,
                                                const std::string& content,
                                                Position position,
                                                const std::string& new_name) = 0;
};

// ═══════════════════════════════════════════════════════════════════
//  LSP SERVER
// ═══════════════════════════════════════════════════════════════════

struct LspMessage {
    std::string method;
    int id = -1;  // -1 for notifications
    std::string params;  // JSON string
    std::string result;  // JSON string (for responses)
    std::string error;
};

class LspServer {
public:
    LspServer();

    // Register language providers
    void register_provider(std::shared_ptr<LanguageProvider> provider);

    // Start server (stdio mode)
    void run();

    // Process a single message
    LspMessage process_message(const LspMessage& request);

    // Get capabilities
    ServerCapabilities get_capabilities() const;

    // Document management
    void open_document(const TextDocumentItem& doc);
    void close_document(const std::string& uri);
    void update_document(const std::string& uri, int version, const std::string& content);
    std::optional<std::string> get_document(const std::string& uri) const;

    // Set workspace root
    void set_workspace_root(const std::string& path);

    // Shutdown
    void shutdown();
    bool is_running() const { return running_; }

private:
    std::map<std::string, std::shared_ptr<LanguageProvider>> providers_;
    std::map<std::string, TextDocumentItem> documents_;
    std::string workspace_root_;
    bool initialized_ = false;
    bool running_ = true;
    int request_id_ = 0;

    // Message handlers
    LspMessage handle_initialize(const LspMessage& msg);
    LspMessage handle_initialized(const LspMessage& msg);
    LspMessage handle_shutdown(const LspMessage& msg);
    LspMessage handle_text_document_did_open(const LspMessage& msg);
    LspMessage handle_text_document_did_change(const LspMessage& msg);
    LspMessage handle_text_document_did_close(const LspMessage& msg);
    LspMessage handle_text_document_completion(const LspMessage& msg);
    LspMessage handle_text_document_hover(const LspMessage& msg);
    LspMessage handle_text_document_definition(const LspMessage& msg);
    LspMessage handle_text_document_references(const LspMessage& msg);
    LspMessage handle_text_document_document_symbol(const LspMessage& msg);
    LspMessage handle_text_document_formatting(const LspMessage& msg);
    LspMessage handle_text_document_code_action(const LspMessage& msg);
    LspMessage handle_text_document_rename(const LspMessage& msg);
    LspMessage handle_text_document_signature_help(const LspMessage& msg);

    // Helpers
    std::shared_ptr<LanguageProvider> get_provider_for_uri(const std::string& uri) const;
    std::string uri_to_language_id(const std::string& uri) const;
    std::string read_message();
    void write_message(const std::string& content);
    void send_diagnostics(const std::string& uri);
};

// ═══════════════════════════════════════════════════════════════════
//  BUILT-IN LANGUAGE PROVIDERS
// ═══════════════════════════════════════════════════════════════════

// Basic provider with common functionality
class BasicLanguageProvider : public LanguageProvider {
public:
    std::vector<Diagnostic> get_diagnostics(const std::string& uri,
                                            const std::string& content) override;

    std::optional<Hover> get_hover(const std::string& uri,
                                   const std::string& content,
                                   Position position) override;

    CompletionList get_completions(const std::string& uri,
                                   const std::string& content,
                                   Position position) override;

    std::vector<Location> get_definition(const std::string& uri,
                                          const std::string& content,
                                          Position position) override;

    std::vector<Location> get_references(const std::string& uri,
                                          const std::string& content,
                                          Position position,
                                          bool include_declaration) override;

    std::vector<DocumentSymbol> get_document_symbols(const std::string& uri,
                                                      const std::string& content) override;

    std::optional<SignatureHelp> get_signature_help(const std::string& uri,
                                                    const std::string& content,
                                                    Position position) override;

    std::vector<TextEdit> format_document(const std::string& uri,
                                           const std::string& content,
                                           const FormattingOptions& options) override;

    std::vector<CodeAction> get_code_actions(const std::string& uri,
                                              const std::string& content,
                                              Range range,
                                              const std::vector<Diagnostic>& diagnostics) override;

    std::optional<WorkspaceEdit> rename(const std::string& uri,
                                        const std::string& content,
                                        Position position,
                                        const std::string& new_name) override;

protected:
    // Helper to get word at position
    std::string get_word_at_position(const std::string& content, Position pos) const;
    // Helper to find all occurrences
    std::vector<Range> find_all_occurrences(const std::string& content, const std::string& word) const;
};

// C++ provider
class CppLanguageProvider : public BasicLanguageProvider {
public:
    std::string language_id() const override { return "cpp"; }
    std::vector<std::string> file_extensions() const override {
        return {".cpp", ".cxx", ".cc", ".hpp", ".hxx", ".h"};
    }

    CompletionList get_completions(const std::string& uri,
                                   const std::string& content,
                                   Position position) override;

    std::vector<DocumentSymbol> get_document_symbols(const std::string& uri,
                                                      const std::string& content) override;
};

// Python provider
class PythonLanguageProvider : public BasicLanguageProvider {
public:
    std::string language_id() const override { return "python"; }
    std::vector<std::string> file_extensions() const override {
        return {".py", ".pyw", ".pyx"};
    }

    CompletionList get_completions(const std::string& uri,
                                   const std::string& content,
                                   Position position) override;

    std::vector<DocumentSymbol> get_document_symbols(const std::string& uri,
                                                      const std::string& content) override;
};

// JavaScript/TypeScript provider
class JsLanguageProvider : public BasicLanguageProvider {
public:
    std::string language_id() const override { return "javascript"; }
    std::vector<std::string> file_extensions() const override {
        return {".js", ".mjs", ".jsx", ".ts", ".tsx"};
    }

    CompletionList get_completions(const std::string& uri,
                                   const std::string& content,
                                   Position position) override;

    std::vector<DocumentSymbol> get_document_symbols(const std::string& uri,
                                                      const std::string& content) override;
};

// ═══════════════════════════════════════════════════════════════════
//  JSON-RPC UTILITIES
// ═══════════════════════════════════════════════════════════════════

namespace jsonrpc {
    std::string encode_message(const std::string& content);
    std::string decode_message(const std::string& raw);
    std::string create_response(int id, const std::string& result);
    std::string create_error(int id, int code, const std::string& message);
    std::string create_notification(const std::string& method, const std::string& params);
}

} // namespace rael
