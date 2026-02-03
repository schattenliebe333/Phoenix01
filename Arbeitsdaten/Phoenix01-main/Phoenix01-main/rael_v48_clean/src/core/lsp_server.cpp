// RAEL V48 - LSP/IDE Integration (#20)
// Implementation of Language Server Protocol

#include "rael/lsp_server.h"
#include <sstream>
#include <iostream>
#include <algorithm>
#include <regex>
#include <cctype>
#include <set>

namespace rael {

// ═══════════════════════════════════════════════════════════════════
//  JSON-RPC UTILITIES
// ═══════════════════════════════════════════════════════════════════

namespace jsonrpc {

std::string encode_message(const std::string& content) {
    std::ostringstream oss;
    oss << "Content-Length: " << content.size() << "\r\n\r\n" << content;
    return oss.str();
}

std::string decode_message(const std::string& raw) {
    // Find the end of headers
    size_t pos = raw.find("\r\n\r\n");
    if (pos == std::string::npos) {
        return raw;
    }
    return raw.substr(pos + 4);
}

std::string create_response(int id, const std::string& result) {
    std::ostringstream oss;
    oss << "{\"jsonrpc\":\"2.0\",\"id\":" << id << ",\"result\":" << result << "}";
    return oss.str();
}

std::string create_error(int id, int code, const std::string& message) {
    std::ostringstream oss;
    oss << "{\"jsonrpc\":\"2.0\",\"id\":" << id
        << ",\"error\":{\"code\":" << code << ",\"message\":\"" << message << "\"}}";
    return oss.str();
}

std::string create_notification(const std::string& method, const std::string& params) {
    std::ostringstream oss;
    oss << "{\"jsonrpc\":\"2.0\",\"method\":\"" << method << "\",\"params\":" << params << "}";
    return oss.str();
}

} // namespace jsonrpc

// ═══════════════════════════════════════════════════════════════════
//  BASIC LANGUAGE PROVIDER
// ═══════════════════════════════════════════════════════════════════

std::string BasicLanguageProvider::get_word_at_position(const std::string& content, Position pos) const {
    std::istringstream iss(content);
    std::string line;
    int current_line = 0;

    while (std::getline(iss, line)) {
        if (current_line == pos.line) {
            int start = pos.character;
            int end = pos.character;

            // Find word boundaries
            while (start > 0 && (std::isalnum(line[start - 1]) || line[start - 1] == '_')) {
                --start;
            }
            while (end < (int)line.length() && (std::isalnum(line[end]) || line[end] == '_')) {
                ++end;
            }

            if (start < end) {
                return line.substr(start, end - start);
            }
            return "";
        }
        ++current_line;
    }
    return "";
}

std::vector<Range> BasicLanguageProvider::find_all_occurrences(const std::string& content,
                                                               const std::string& word) const {
    std::vector<Range> occurrences;
    if (word.empty()) return occurrences;

    std::regex word_re("\\b" + word + "\\b");
    std::istringstream iss(content);
    std::string line;
    int line_num = 0;

    while (std::getline(iss, line)) {
        auto begin = std::sregex_iterator(line.begin(), line.end(), word_re);
        auto end = std::sregex_iterator();

        for (auto it = begin; it != end; ++it) {
            Range r;
            r.start.line = line_num;
            r.start.character = static_cast<int>(it->position());
            r.end.line = line_num;
            r.end.character = r.start.character + static_cast<int>(word.length());
            occurrences.push_back(r);
        }
        ++line_num;
    }

    return occurrences;
}

std::vector<Diagnostic> BasicLanguageProvider::get_diagnostics(const std::string& uri,
                                                               const std::string& content) {
    // Base implementation returns empty
    return {};
}

std::optional<Hover> BasicLanguageProvider::get_hover(const std::string& uri,
                                                      const std::string& content,
                                                      Position position) {
    std::string word = get_word_at_position(content, position);
    if (word.empty()) return std::nullopt;

    Hover h;
    h.contents.kind = "plaintext";
    h.contents.value = word;
    return h;
}

CompletionList BasicLanguageProvider::get_completions(const std::string& uri,
                                                      const std::string& content,
                                                      Position position) {
    CompletionList list;
    list.is_incomplete = false;

    // Extract current word prefix
    std::string prefix = get_word_at_position(content, position);

    // Find all identifiers in document for basic completion
    std::regex id_re(R"(\b([a-zA-Z_]\w*)\b)");
    std::set<std::string> identifiers;

    auto begin = std::sregex_iterator(content.begin(), content.end(), id_re);
    auto end = std::sregex_iterator();

    for (auto it = begin; it != end; ++it) {
        std::string id = (*it)[1].str();
        if (prefix.empty() || id.find(prefix) == 0) {
            identifiers.insert(id);
        }
    }

    for (const auto& id : identifiers) {
        if (id != prefix) {
            CompletionItem item;
            item.label = id;
            item.kind = CompletionItemKind::Variable;
            list.items.push_back(item);
        }
    }

    return list;
}

std::vector<Location> BasicLanguageProvider::get_definition(const std::string& uri,
                                                             const std::string& content,
                                                             Position position) {
    // Basic: find first occurrence
    std::string word = get_word_at_position(content, position);
    auto occurrences = find_all_occurrences(content, word);

    std::vector<Location> locations;
    if (!occurrences.empty()) {
        Location loc;
        loc.uri = uri;
        loc.range = occurrences[0];  // First occurrence as definition
        locations.push_back(loc);
    }
    return locations;
}

std::vector<Location> BasicLanguageProvider::get_references(const std::string& uri,
                                                             const std::string& content,
                                                             Position position,
                                                             bool include_declaration) {
    std::string word = get_word_at_position(content, position);
    auto occurrences = find_all_occurrences(content, word);

    std::vector<Location> locations;
    for (const auto& range : occurrences) {
        Location loc;
        loc.uri = uri;
        loc.range = range;
        locations.push_back(loc);
    }
    return locations;
}

std::vector<DocumentSymbol> BasicLanguageProvider::get_document_symbols(const std::string& uri,
                                                                         const std::string& content) {
    // Base implementation - override in specific providers
    return {};
}

std::optional<SignatureHelp> BasicLanguageProvider::get_signature_help(const std::string& uri,
                                                                       const std::string& content,
                                                                       Position position) {
    return std::nullopt;
}

std::vector<TextEdit> BasicLanguageProvider::format_document(const std::string& uri,
                                                              const std::string& content,
                                                              const FormattingOptions& options) {
    std::vector<TextEdit> edits;

    std::istringstream iss(content);
    std::ostringstream oss;
    std::string line;
    int line_num = 0;

    while (std::getline(iss, line)) {
        // Remove trailing whitespace
        size_t end = line.find_last_not_of(" \t\r");
        if (end != std::string::npos) {
            std::string trimmed = line.substr(0, end + 1);
            if (trimmed != line) {
                TextEdit edit;
                edit.range.start.line = line_num;
                edit.range.start.character = static_cast<int>(end + 1);
                edit.range.end.line = line_num;
                edit.range.end.character = static_cast<int>(line.length());
                edit.new_text = "";
                edits.push_back(edit);
            }
        }
        ++line_num;
    }

    return edits;
}

std::vector<CodeAction> BasicLanguageProvider::get_code_actions(const std::string& uri,
                                                                 const std::string& content,
                                                                 Range range,
                                                                 const std::vector<Diagnostic>& diagnostics) {
    return {};
}

std::optional<WorkspaceEdit> BasicLanguageProvider::rename(const std::string& uri,
                                                           const std::string& content,
                                                           Position position,
                                                           const std::string& new_name) {
    std::string old_name = get_word_at_position(content, position);
    if (old_name.empty()) return std::nullopt;

    auto occurrences = find_all_occurrences(content, old_name);
    if (occurrences.empty()) return std::nullopt;

    WorkspaceEdit edit;
    std::vector<TextEdit> text_edits;

    for (const auto& range : occurrences) {
        TextEdit te;
        te.range = range;
        te.new_text = new_name;
        text_edits.push_back(te);
    }

    edit.changes[uri] = text_edits;
    return edit;
}

// ═══════════════════════════════════════════════════════════════════
//  C++ LANGUAGE PROVIDER
// ═══════════════════════════════════════════════════════════════════

CompletionList CppLanguageProvider::get_completions(const std::string& uri,
                                                    const std::string& content,
                                                    Position position) {
    CompletionList list = BasicLanguageProvider::get_completions(uri, content, position);

    // Add C++ keywords
    static const std::vector<std::string> keywords = {
        "auto", "bool", "break", "case", "catch", "char", "class", "const",
        "continue", "default", "delete", "do", "double", "else", "enum",
        "explicit", "extern", "false", "float", "for", "friend", "goto",
        "if", "inline", "int", "long", "mutable", "namespace", "new",
        "noexcept", "nullptr", "operator", "override", "private", "protected",
        "public", "return", "short", "signed", "sizeof", "static", "struct",
        "switch", "template", "this", "throw", "true", "try", "typedef",
        "typename", "union", "unsigned", "using", "virtual", "void",
        "volatile", "while"
    };

    std::string prefix = get_word_at_position(content, position);

    for (const auto& kw : keywords) {
        if (prefix.empty() || kw.find(prefix) == 0) {
            CompletionItem item;
            item.label = kw;
            item.kind = CompletionItemKind::Keyword;
            list.items.push_back(item);
        }
    }

    // Add common STL types
    static const std::vector<std::pair<std::string, std::string>> stl_types = {
        {"std::string", "string type"},
        {"std::vector", "dynamic array"},
        {"std::map", "key-value map"},
        {"std::set", "unique sorted set"},
        {"std::unordered_map", "hash map"},
        {"std::unique_ptr", "unique ownership pointer"},
        {"std::shared_ptr", "shared ownership pointer"},
        {"std::optional", "optional value"},
        {"std::variant", "type-safe union"},
        {"std::function", "function wrapper"}
    };

    for (const auto& [name, detail] : stl_types) {
        if (prefix.empty() || name.find(prefix) != std::string::npos) {
            CompletionItem item;
            item.label = name;
            item.detail = detail;
            item.kind = CompletionItemKind::Class;
            list.items.push_back(item);
        }
    }

    return list;
}

std::vector<DocumentSymbol> CppLanguageProvider::get_document_symbols(const std::string& uri,
                                                                       const std::string& content) {
    std::vector<DocumentSymbol> symbols;

    // Find classes/structs
    std::regex class_re(R"((class|struct)\s+(\w+))");
    std::istringstream iss(content);
    std::string line;
    int line_num = 0;

    while (std::getline(iss, line)) {
        std::smatch match;
        if (std::regex_search(line, match, class_re)) {
            DocumentSymbol sym;
            sym.name = match[2].str();
            sym.kind = (match[1].str() == "class") ? SymbolKind::Class : SymbolKind::Struct;
            sym.range.start.line = line_num;
            sym.range.start.character = 0;
            sym.range.end.line = line_num;
            sym.range.end.character = static_cast<int>(line.length());
            sym.selection_range = sym.range;
            symbols.push_back(sym);
        }

        // Find functions
        std::regex func_re(R"((\w+)\s+(\w+)\s*\([^)]*\)\s*(?:const)?\s*(?:override)?\s*\{?)");
        if (std::regex_search(line, match, func_re)) {
            DocumentSymbol sym;
            sym.name = match[2].str();
            sym.detail = match[1].str();  // Return type
            sym.kind = SymbolKind::Function;
            sym.range.start.line = line_num;
            sym.range.start.character = 0;
            sym.range.end.line = line_num;
            sym.range.end.character = static_cast<int>(line.length());
            sym.selection_range = sym.range;
            symbols.push_back(sym);
        }

        ++line_num;
    }

    return symbols;
}

// ═══════════════════════════════════════════════════════════════════
//  PYTHON LANGUAGE PROVIDER
// ═══════════════════════════════════════════════════════════════════

CompletionList PythonLanguageProvider::get_completions(const std::string& uri,
                                                       const std::string& content,
                                                       Position position) {
    CompletionList list = BasicLanguageProvider::get_completions(uri, content, position);

    // Add Python keywords
    static const std::vector<std::string> keywords = {
        "False", "None", "True", "and", "as", "assert", "async", "await",
        "break", "class", "continue", "def", "del", "elif", "else", "except",
        "finally", "for", "from", "global", "if", "import", "in", "is",
        "lambda", "nonlocal", "not", "or", "pass", "raise", "return",
        "try", "while", "with", "yield"
    };

    std::string prefix = get_word_at_position(content, position);

    for (const auto& kw : keywords) {
        if (prefix.empty() || kw.find(prefix) == 0) {
            CompletionItem item;
            item.label = kw;
            item.kind = CompletionItemKind::Keyword;
            list.items.push_back(item);
        }
    }

    // Add built-in functions
    static const std::vector<std::string> builtins = {
        "print", "len", "range", "str", "int", "float", "list", "dict",
        "set", "tuple", "bool", "type", "isinstance", "hasattr", "getattr",
        "setattr", "open", "input", "format", "sum", "min", "max", "abs",
        "sorted", "reversed", "enumerate", "zip", "map", "filter"
    };

    for (const auto& fn : builtins) {
        if (prefix.empty() || fn.find(prefix) == 0) {
            CompletionItem item;
            item.label = fn;
            item.kind = CompletionItemKind::Function;
            item.detail = "built-in";
            list.items.push_back(item);
        }
    }

    return list;
}

std::vector<DocumentSymbol> PythonLanguageProvider::get_document_symbols(const std::string& uri,
                                                                          const std::string& content) {
    std::vector<DocumentSymbol> symbols;

    std::regex class_re(R"(^class\s+(\w+))");
    std::regex func_re(R"(^(?:async\s+)?def\s+(\w+))");

    std::istringstream iss(content);
    std::string line;
    int line_num = 0;

    while (std::getline(iss, line)) {
        std::smatch match;
        if (std::regex_search(line, match, class_re)) {
            DocumentSymbol sym;
            sym.name = match[1].str();
            sym.kind = SymbolKind::Class;
            sym.range.start.line = line_num;
            sym.range.start.character = 0;
            sym.range.end.line = line_num;
            sym.range.end.character = static_cast<int>(line.length());
            sym.selection_range = sym.range;
            symbols.push_back(sym);
        } else if (std::regex_search(line, match, func_re)) {
            DocumentSymbol sym;
            sym.name = match[1].str();
            sym.kind = SymbolKind::Function;
            sym.range.start.line = line_num;
            sym.range.start.character = 0;
            sym.range.end.line = line_num;
            sym.range.end.character = static_cast<int>(line.length());
            sym.selection_range = sym.range;
            symbols.push_back(sym);
        }

        ++line_num;
    }

    return symbols;
}

// ═══════════════════════════════════════════════════════════════════
//  JAVASCRIPT LANGUAGE PROVIDER
// ═══════════════════════════════════════════════════════════════════

CompletionList JsLanguageProvider::get_completions(const std::string& uri,
                                                   const std::string& content,
                                                   Position position) {
    CompletionList list = BasicLanguageProvider::get_completions(uri, content, position);

    // Add JavaScript keywords
    static const std::vector<std::string> keywords = {
        "async", "await", "break", "case", "catch", "class", "const",
        "continue", "debugger", "default", "delete", "do", "else", "export",
        "extends", "false", "finally", "for", "function", "if", "import",
        "in", "instanceof", "let", "new", "null", "return", "static",
        "super", "switch", "this", "throw", "true", "try", "typeof",
        "undefined", "var", "void", "while", "with", "yield"
    };

    std::string prefix = get_word_at_position(content, position);

    for (const auto& kw : keywords) {
        if (prefix.empty() || kw.find(prefix) == 0) {
            CompletionItem item;
            item.label = kw;
            item.kind = CompletionItemKind::Keyword;
            list.items.push_back(item);
        }
    }

    // Add common globals
    static const std::vector<std::string> globals = {
        "console", "document", "window", "Array", "Object", "String",
        "Number", "Boolean", "Function", "Promise", "Map", "Set",
        "JSON", "Math", "Date", "RegExp", "Error", "setTimeout",
        "setInterval", "fetch", "require", "module", "exports"
    };

    for (const auto& g : globals) {
        if (prefix.empty() || g.find(prefix) == 0) {
            CompletionItem item;
            item.label = g;
            item.kind = CompletionItemKind::Variable;
            item.detail = "global";
            list.items.push_back(item);
        }
    }

    return list;
}

std::vector<DocumentSymbol> JsLanguageProvider::get_document_symbols(const std::string& uri,
                                                                      const std::string& content) {
    std::vector<DocumentSymbol> symbols;

    std::regex class_re(R"(class\s+(\w+))");
    std::regex func_re(R"((?:function\s+(\w+)|(?:const|let|var)\s+(\w+)\s*=\s*(?:async\s*)?\([^)]*\)\s*=>))");
    std::regex method_re(R"((\w+)\s*\([^)]*\)\s*\{)");

    std::istringstream iss(content);
    std::string line;
    int line_num = 0;

    while (std::getline(iss, line)) {
        std::smatch match;
        if (std::regex_search(line, match, class_re)) {
            DocumentSymbol sym;
            sym.name = match[1].str();
            sym.kind = SymbolKind::Class;
            sym.range.start.line = line_num;
            sym.range.start.character = 0;
            sym.range.end.line = line_num;
            sym.range.end.character = static_cast<int>(line.length());
            sym.selection_range = sym.range;
            symbols.push_back(sym);
        } else if (std::regex_search(line, match, func_re)) {
            DocumentSymbol sym;
            sym.name = match[1].matched ? match[1].str() : match[2].str();
            sym.kind = SymbolKind::Function;
            sym.range.start.line = line_num;
            sym.range.start.character = 0;
            sym.range.end.line = line_num;
            sym.range.end.character = static_cast<int>(line.length());
            sym.selection_range = sym.range;
            symbols.push_back(sym);
        }

        ++line_num;
    }

    return symbols;
}

// ═══════════════════════════════════════════════════════════════════
//  LSP SERVER
// ═══════════════════════════════════════════════════════════════════

LspServer::LspServer() {
    // Register default providers
    register_provider(std::make_shared<CppLanguageProvider>());
    register_provider(std::make_shared<PythonLanguageProvider>());
    register_provider(std::make_shared<JsLanguageProvider>());
}

void LspServer::register_provider(std::shared_ptr<LanguageProvider> provider) {
    providers_[provider->language_id()] = provider;

    // Also register by extension
    for (const auto& ext : provider->file_extensions()) {
        providers_[ext] = provider;
    }
}

ServerCapabilities LspServer::get_capabilities() const {
    ServerCapabilities caps;
    caps.trigger_characters = {".", ":", "<", ">", "/"};
    caps.signature_trigger_characters = {"(", ","};
    return caps;
}

void LspServer::open_document(const TextDocumentItem& doc) {
    documents_[doc.uri] = doc;
    send_diagnostics(doc.uri);
}

void LspServer::close_document(const std::string& uri) {
    documents_.erase(uri);
}

void LspServer::update_document(const std::string& uri, int version, const std::string& content) {
    auto it = documents_.find(uri);
    if (it != documents_.end()) {
        it->second.version = version;
        it->second.text = content;
        send_diagnostics(uri);
    }
}

std::optional<std::string> LspServer::get_document(const std::string& uri) const {
    auto it = documents_.find(uri);
    if (it != documents_.end()) {
        return it->second.text;
    }
    return std::nullopt;
}

void LspServer::set_workspace_root(const std::string& path) {
    workspace_root_ = path;
}

void LspServer::shutdown() {
    running_ = false;
}

std::shared_ptr<LanguageProvider> LspServer::get_provider_for_uri(const std::string& uri) const {
    // Find by extension
    size_t dot = uri.rfind('.');
    if (dot != std::string::npos) {
        std::string ext = uri.substr(dot);
        auto it = providers_.find(ext);
        if (it != providers_.end()) {
            return it->second;
        }
    }

    // Find by language id from document
    auto doc_it = documents_.find(uri);
    if (doc_it != documents_.end()) {
        auto it = providers_.find(doc_it->second.language_id);
        if (it != providers_.end()) {
            return it->second;
        }
    }

    return nullptr;
}

std::string LspServer::uri_to_language_id(const std::string& uri) const {
    static const std::map<std::string, std::string> ext_to_lang = {
        {".cpp", "cpp"}, {".cxx", "cpp"}, {".cc", "cpp"}, {".hpp", "cpp"},
        {".c", "c"}, {".h", "c"},
        {".py", "python"}, {".pyw", "python"},
        {".js", "javascript"}, {".mjs", "javascript"}, {".jsx", "javascript"},
        {".ts", "typescript"}, {".tsx", "typescript"},
        {".rs", "rust"},
        {".go", "go"},
        {".java", "java"},
        {".rb", "ruby"},
        {".php", "php"}
    };

    size_t dot = uri.rfind('.');
    if (dot != std::string::npos) {
        std::string ext = uri.substr(dot);
        auto it = ext_to_lang.find(ext);
        if (it != ext_to_lang.end()) {
            return it->second;
        }
    }

    return "plaintext";
}

void LspServer::send_diagnostics(const std::string& uri) {
    auto provider = get_provider_for_uri(uri);
    auto content = get_document(uri);

    if (!provider || !content) return;

    auto diagnostics = provider->get_diagnostics(uri, *content);

    // Build diagnostics JSON
    std::ostringstream params;
    params << "{\"uri\":\"" << uri << "\",\"diagnostics\":[";

    for (size_t i = 0; i < diagnostics.size(); ++i) {
        const auto& d = diagnostics[i];
        if (i > 0) params << ",";
        params << "{\"range\":{\"start\":{\"line\":" << d.range.start.line
               << ",\"character\":" << d.range.start.character
               << "},\"end\":{\"line\":" << d.range.end.line
               << ",\"character\":" << d.range.end.character
               << "}},\"severity\":" << static_cast<int>(d.severity)
               << ",\"message\":\"" << d.message << "\"}";
    }
    params << "]}";

    std::string notification = jsonrpc::create_notification("textDocument/publishDiagnostics",
                                                            params.str());
    write_message(notification);
}

std::string LspServer::read_message() {
    // Read Content-Length header
    std::string header;
    while (true) {
        char c;
        if (!std::cin.get(c)) return "";
        header += c;
        if (header.size() >= 4 &&
            header.substr(header.size() - 4) == "\r\n\r\n") {
            break;
        }
    }

    // Parse content length
    std::regex cl_re(R"(Content-Length:\s*(\d+))");
    std::smatch match;
    if (!std::regex_search(header, match, cl_re)) {
        return "";
    }

    int length = std::stoi(match[1].str());

    // Read content
    std::string content(length, '\0');
    std::cin.read(&content[0], length);

    return content;
}

void LspServer::write_message(const std::string& content) {
    std::cout << "Content-Length: " << content.size() << "\r\n\r\n" << content;
    std::cout.flush();
}

void LspServer::run() {
    while (running_) {
        std::string msg = read_message();
        if (msg.empty()) break;

        LspMessage request;
        request.params = msg;

        // Simple JSON parsing for method and id
        std::regex method_re(R"RE("method"\s*:\s*"([^"]+)")RE");
        std::regex id_re(R"("id"\s*:\s*(\d+))");

        std::smatch match;
        if (std::regex_search(msg, match, method_re)) {
            request.method = match[1].str();
        }
        if (std::regex_search(msg, match, id_re)) {
            request.id = std::stoi(match[1].str());
        }

        LspMessage response = process_message(request);

        if (!response.result.empty() || !response.error.empty()) {
            write_message(response.result.empty() ? response.error : response.result);
        }
    }
}

LspMessage LspServer::process_message(const LspMessage& request) {
    LspMessage response;
    response.id = request.id;

    if (request.method == "initialize") {
        return handle_initialize(request);
    } else if (request.method == "initialized") {
        return handle_initialized(request);
    } else if (request.method == "shutdown") {
        return handle_shutdown(request);
    } else if (request.method == "exit") {
        shutdown();
        return response;
    } else if (request.method == "textDocument/didOpen") {
        return handle_text_document_did_open(request);
    } else if (request.method == "textDocument/didChange") {
        return handle_text_document_did_change(request);
    } else if (request.method == "textDocument/didClose") {
        return handle_text_document_did_close(request);
    } else if (request.method == "textDocument/completion") {
        return handle_text_document_completion(request);
    } else if (request.method == "textDocument/hover") {
        return handle_text_document_hover(request);
    } else if (request.method == "textDocument/definition") {
        return handle_text_document_definition(request);
    } else if (request.method == "textDocument/references") {
        return handle_text_document_references(request);
    } else if (request.method == "textDocument/documentSymbol") {
        return handle_text_document_document_symbol(request);
    } else if (request.method == "textDocument/formatting") {
        return handle_text_document_formatting(request);
    } else if (request.method == "textDocument/codeAction") {
        return handle_text_document_code_action(request);
    } else if (request.method == "textDocument/rename") {
        return handle_text_document_rename(request);
    } else if (request.method == "textDocument/signatureHelp") {
        return handle_text_document_signature_help(request);
    }

    return response;
}

LspMessage LspServer::handle_initialize(const LspMessage& msg) {
    LspMessage response;
    response.id = msg.id;

    // Build capabilities response
    std::ostringstream result;
    result << "{\"capabilities\":{";
    result << "\"hoverProvider\":true,";
    result << "\"completionProvider\":{\"triggerCharacters\":[\".\",\"::\",\"<\"]},";
    result << "\"signatureHelpProvider\":{\"triggerCharacters\":[\"(\",\",\"]},";
    result << "\"definitionProvider\":true,";
    result << "\"referencesProvider\":true,";
    result << "\"documentSymbolProvider\":true,";
    result << "\"documentFormattingProvider\":true,";
    result << "\"renameProvider\":true,";
    result << "\"codeActionProvider\":true";
    result << "}}";

    response.result = jsonrpc::create_response(msg.id, result.str());
    initialized_ = true;
    return response;
}

LspMessage LspServer::handle_initialized(const LspMessage& msg) {
    LspMessage response;
    // No response needed for notification
    return response;
}

LspMessage LspServer::handle_shutdown(const LspMessage& msg) {
    LspMessage response;
    response.id = msg.id;
    response.result = jsonrpc::create_response(msg.id, "null");
    return response;
}

LspMessage LspServer::handle_text_document_did_open(const LspMessage& msg) {
    // Extract document info from params
    std::regex uri_re(R"RE("uri"\s*:\s*"([^"]+)")RE");
    std::regex lang_re(R"RE("languageId"\s*:\s*"([^"]+)")RE");
    std::regex version_re(R"("version"\s*:\s*(\d+))");
    std::regex text_re(R"RE("text"\s*:\s*"((?:[^"\\]|\\.)*)")RE");

    std::smatch match;
    TextDocumentItem doc;

    if (std::regex_search(msg.params, match, uri_re)) {
        doc.uri = match[1].str();
    }
    if (std::regex_search(msg.params, match, lang_re)) {
        doc.language_id = match[1].str();
    }
    if (std::regex_search(msg.params, match, version_re)) {
        doc.version = std::stoi(match[1].str());
    }
    if (std::regex_search(msg.params, match, text_re)) {
        doc.text = match[1].str();
        // Unescape common sequences
        std::string& t = doc.text;
        size_t pos = 0;
        while ((pos = t.find("\\n", pos)) != std::string::npos) {
            t.replace(pos, 2, "\n");
            pos++;
        }
        pos = 0;
        while ((pos = t.find("\\t", pos)) != std::string::npos) {
            t.replace(pos, 2, "\t");
            pos++;
        }
    }

    open_document(doc);
    return LspMessage{};
}

LspMessage LspServer::handle_text_document_did_change(const LspMessage& msg) {
    std::regex uri_re(R"RE("uri"\s*:\s*"([^"]+)")RE");
    std::regex version_re(R"("version"\s*:\s*(\d+))");
    std::regex text_re(R"RE("text"\s*:\s*"((?:[^"\\]|\\.)*)")RE");

    std::smatch match;
    std::string uri;
    int version = 0;
    std::string text;

    if (std::regex_search(msg.params, match, uri_re)) {
        uri = match[1].str();
    }
    if (std::regex_search(msg.params, match, version_re)) {
        version = std::stoi(match[1].str());
    }
    if (std::regex_search(msg.params, match, text_re)) {
        text = match[1].str();
    }

    update_document(uri, version, text);
    return LspMessage{};
}

LspMessage LspServer::handle_text_document_did_close(const LspMessage& msg) {
    std::regex uri_re(R"RE("uri"\s*:\s*"([^"]+)")RE");
    std::smatch match;
    if (std::regex_search(msg.params, match, uri_re)) {
        close_document(match[1].str());
    }
    return LspMessage{};
}

LspMessage LspServer::handle_text_document_completion(const LspMessage& msg) {
    LspMessage response;
    response.id = msg.id;

    std::regex uri_re(R"RE("uri"\s*:\s*"([^"]+)")RE");
    std::regex line_re(R"("line"\s*:\s*(\d+))");
    std::regex char_re(R"("character"\s*:\s*(\d+))");

    std::smatch match;
    std::string uri;
    Position pos;

    if (std::regex_search(msg.params, match, uri_re)) {
        uri = match[1].str();
    }
    if (std::regex_search(msg.params, match, line_re)) {
        pos.line = std::stoi(match[1].str());
    }
    if (std::regex_search(msg.params, match, char_re)) {
        pos.character = std::stoi(match[1].str());
    }

    auto provider = get_provider_for_uri(uri);
    auto content = get_document(uri);

    if (!provider || !content) {
        response.result = jsonrpc::create_response(msg.id, "null");
        return response;
    }

    auto completions = provider->get_completions(uri, *content, pos);

    // Build completion list JSON
    std::ostringstream result;
    result << "{\"isIncomplete\":" << (completions.is_incomplete ? "true" : "false")
           << ",\"items\":[";

    for (size_t i = 0; i < completions.items.size(); ++i) {
        const auto& item = completions.items[i];
        if (i > 0) result << ",";
        result << "{\"label\":\"" << item.label << "\"";
        result << ",\"kind\":" << static_cast<int>(item.kind);
        if (!item.detail.empty()) {
            result << ",\"detail\":\"" << item.detail << "\"";
        }
        result << "}";
    }
    result << "]}";

    response.result = jsonrpc::create_response(msg.id, result.str());
    return response;
}

LspMessage LspServer::handle_text_document_hover(const LspMessage& msg) {
    LspMessage response;
    response.id = msg.id;

    std::regex uri_re(R"RE("uri"\s*:\s*"([^"]+)")RE");
    std::regex line_re(R"("line"\s*:\s*(\d+))");
    std::regex char_re(R"("character"\s*:\s*(\d+))");

    std::smatch match;
    std::string uri;
    Position pos;

    if (std::regex_search(msg.params, match, uri_re)) uri = match[1].str();
    if (std::regex_search(msg.params, match, line_re)) pos.line = std::stoi(match[1].str());
    if (std::regex_search(msg.params, match, char_re)) pos.character = std::stoi(match[1].str());

    auto provider = get_provider_for_uri(uri);
    auto content = get_document(uri);

    if (!provider || !content) {
        response.result = jsonrpc::create_response(msg.id, "null");
        return response;
    }

    auto hover = provider->get_hover(uri, *content, pos);
    if (!hover) {
        response.result = jsonrpc::create_response(msg.id, "null");
        return response;
    }

    std::ostringstream result;
    result << "{\"contents\":{\"kind\":\"" << hover->contents.kind
           << "\",\"value\":\"" << hover->contents.value << "\"}}";

    response.result = jsonrpc::create_response(msg.id, result.str());
    return response;
}

LspMessage LspServer::handle_text_document_definition(const LspMessage& msg) {
    LspMessage response;
    response.id = msg.id;

    // Similar pattern extraction as hover
    std::regex uri_re(R"RE("uri"\s*:\s*"([^"]+)")RE");
    std::regex line_re(R"("line"\s*:\s*(\d+))");
    std::regex char_re(R"("character"\s*:\s*(\d+))");

    std::smatch match;
    std::string uri;
    Position pos;

    if (std::regex_search(msg.params, match, uri_re)) uri = match[1].str();
    if (std::regex_search(msg.params, match, line_re)) pos.line = std::stoi(match[1].str());
    if (std::regex_search(msg.params, match, char_re)) pos.character = std::stoi(match[1].str());

    auto provider = get_provider_for_uri(uri);
    auto content = get_document(uri);

    if (!provider || !content) {
        response.result = jsonrpc::create_response(msg.id, "[]");
        return response;
    }

    auto locations = provider->get_definition(uri, *content, pos);

    std::ostringstream result;
    result << "[";
    for (size_t i = 0; i < locations.size(); ++i) {
        const auto& loc = locations[i];
        if (i > 0) result << ",";
        result << "{\"uri\":\"" << loc.uri << "\",\"range\":{"
               << "\"start\":{\"line\":" << loc.range.start.line
               << ",\"character\":" << loc.range.start.character << "},"
               << "\"end\":{\"line\":" << loc.range.end.line
               << ",\"character\":" << loc.range.end.character << "}}}";
    }
    result << "]";

    response.result = jsonrpc::create_response(msg.id, result.str());
    return response;
}

LspMessage LspServer::handle_text_document_references(const LspMessage& msg) {
    // Similar to definition but calls get_references
    return handle_text_document_definition(msg);  // Simplified
}

LspMessage LspServer::handle_text_document_document_symbol(const LspMessage& msg) {
    LspMessage response;
    response.id = msg.id;

    std::regex uri_re(R"RE("uri"\s*:\s*"([^"]+)")RE");
    std::smatch match;
    std::string uri;

    if (std::regex_search(msg.params, match, uri_re)) uri = match[1].str();

    auto provider = get_provider_for_uri(uri);
    auto content = get_document(uri);

    if (!provider || !content) {
        response.result = jsonrpc::create_response(msg.id, "[]");
        return response;
    }

    auto symbols = provider->get_document_symbols(uri, *content);

    std::ostringstream result;
    result << "[";
    for (size_t i = 0; i < symbols.size(); ++i) {
        const auto& sym = symbols[i];
        if (i > 0) result << ",";
        result << "{\"name\":\"" << sym.name << "\""
               << ",\"kind\":" << static_cast<int>(sym.kind)
               << ",\"range\":{"
               << "\"start\":{\"line\":" << sym.range.start.line
               << ",\"character\":" << sym.range.start.character << "},"
               << "\"end\":{\"line\":" << sym.range.end.line
               << ",\"character\":" << sym.range.end.character << "}}"
               << ",\"selectionRange\":{"
               << "\"start\":{\"line\":" << sym.selection_range.start.line
               << ",\"character\":" << sym.selection_range.start.character << "},"
               << "\"end\":{\"line\":" << sym.selection_range.end.line
               << ",\"character\":" << sym.selection_range.end.character << "}}}";
    }
    result << "]";

    response.result = jsonrpc::create_response(msg.id, result.str());
    return response;
}

LspMessage LspServer::handle_text_document_formatting(const LspMessage& msg) {
    LspMessage response;
    response.id = msg.id;

    std::regex uri_re(R"RE("uri"\s*:\s*"([^"]+)")RE");
    std::smatch match;
    std::string uri;

    if (std::regex_search(msg.params, match, uri_re)) uri = match[1].str();

    auto provider = get_provider_for_uri(uri);
    auto content = get_document(uri);

    if (!provider || !content) {
        response.result = jsonrpc::create_response(msg.id, "[]");
        return response;
    }

    FormattingOptions opts;
    auto edits = provider->format_document(uri, *content, opts);

    std::ostringstream result;
    result << "[";
    for (size_t i = 0; i < edits.size(); ++i) {
        const auto& edit = edits[i];
        if (i > 0) result << ",";
        result << "{\"range\":{"
               << "\"start\":{\"line\":" << edit.range.start.line
               << ",\"character\":" << edit.range.start.character << "},"
               << "\"end\":{\"line\":" << edit.range.end.line
               << ",\"character\":" << edit.range.end.character << "}},"
               << "\"newText\":\"" << edit.new_text << "\"}";
    }
    result << "]";

    response.result = jsonrpc::create_response(msg.id, result.str());
    return response;
}

LspMessage LspServer::handle_text_document_code_action(const LspMessage& msg) {
    LspMessage response;
    response.id = msg.id;
    response.result = jsonrpc::create_response(msg.id, "[]");
    return response;
}

LspMessage LspServer::handle_text_document_rename(const LspMessage& msg) {
    LspMessage response;
    response.id = msg.id;
    response.result = jsonrpc::create_response(msg.id, "null");
    return response;
}

LspMessage LspServer::handle_text_document_signature_help(const LspMessage& msg) {
    LspMessage response;
    response.id = msg.id;
    response.result = jsonrpc::create_response(msg.id, "null");
    return response;
}

} // namespace rael
