#include "rael/parser.h"
#include "rael/filesystem.h"
#include "rael/events.h"
#include <cctype>
#include <algorithm>
#include <sstream>

namespace rael {

// ═══════════════════════════════════════════════════════════════════════════
// LEXER IMPLEMENTATION
// ═══════════════════════════════════════════════════════════════════════════

Lexer::Lexer(Language lang) : language_(lang) {
    set_language(lang);
}

void Lexer::set_language(Language lang) {
    language_ = lang;
    keywords_.clear();

    // Keywords je nach Sprache
    switch (lang) {
        case Language::CPP:
        case Language::C:
            keywords_ = {"auto", "break", "case", "catch", "class", "const", "constexpr",
                        "continue", "default", "delete", "do", "else", "enum", "explicit",
                        "extern", "false", "for", "friend", "goto", "if", "inline",
                        "namespace", "new", "noexcept", "nullptr", "operator", "override",
                        "private", "protected", "public", "return", "sizeof", "static",
                        "static_cast", "struct", "switch", "template", "this", "throw",
                        "true", "try", "typedef", "typename", "union", "using", "virtual",
                        "void", "volatile", "while", "int", "char", "float", "double",
                        "long", "short", "unsigned", "signed", "bool", "wchar_t"};
            break;
        case Language::PYTHON:
            keywords_ = {"False", "None", "True", "and", "as", "assert", "async", "await",
                        "break", "class", "continue", "def", "del", "elif", "else", "except",
                        "finally", "for", "from", "global", "if", "import", "in", "is",
                        "lambda", "nonlocal", "not", "or", "pass", "raise", "return", "try",
                        "while", "with", "yield"};
            break;
        case Language::JAVASCRIPT:
        case Language::TYPESCRIPT:
            keywords_ = {"break", "case", "catch", "class", "const", "continue", "debugger",
                        "default", "delete", "do", "else", "enum", "export", "extends",
                        "false", "finally", "for", "function", "if", "import", "in",
                        "instanceof", "let", "new", "null", "return", "super", "switch",
                        "this", "throw", "true", "try", "typeof", "undefined", "var",
                        "void", "while", "with", "yield", "async", "await"};
            break;
        case Language::JAVA:
            keywords_ = {"abstract", "assert", "boolean", "break", "byte", "case", "catch",
                        "char", "class", "const", "continue", "default", "do", "double",
                        "else", "enum", "extends", "final", "finally", "float", "for",
                        "goto", "if", "implements", "import", "instanceof", "int",
                        "interface", "long", "native", "new", "package", "private",
                        "protected", "public", "return", "short", "static", "strictfp",
                        "super", "switch", "synchronized", "this", "throw", "throws",
                        "transient", "try", "void", "volatile", "while"};
            break;
        case Language::RUST:
            keywords_ = {"as", "break", "const", "continue", "crate", "else", "enum",
                        "extern", "false", "fn", "for", "if", "impl", "in", "let", "loop",
                        "match", "mod", "move", "mut", "pub", "ref", "return", "self",
                        "Self", "static", "struct", "super", "trait", "true", "type",
                        "unsafe", "use", "where", "while", "async", "await", "dyn"};
            break;
        case Language::GO:
            keywords_ = {"break", "case", "chan", "const", "continue", "default", "defer",
                        "else", "fallthrough", "for", "func", "go", "goto", "if", "import",
                        "interface", "map", "package", "range", "return", "select", "struct",
                        "switch", "type", "var"};
            break;
        default:
            break;
    }
}

bool Lexer::is_keyword(const std::string& word) const {
    return std::find(keywords_.begin(), keywords_.end(), word) != keywords_.end();
}

std::vector<Token> Lexer::tokenize(const std::string& source) {
    source_ = source;
    start_ = 0;
    current_ = 0;
    line_ = 1;
    column_ = 1;

    std::vector<Token> tokens;

    while (!at_end()) {
        start_ = current_;
        Token tok = next_token();

        if (skip_whitespace_ && tok.type == TokenType::WHITESPACE) continue;
        if (skip_comments_ && tok.type == TokenType::COMMENT) continue;

        tokens.push_back(tok);
    }

    tokens.push_back(make_token(TokenType::END_OF_FILE, ""));
    return tokens;
}

Token Lexer::next_token() {
    if (at_end()) return make_token(TokenType::END_OF_FILE);

    char c = advance();

    // Whitespace
    if (std::isspace(c)) {
        while (!at_end() && std::isspace(peek()) && peek() != '\n') advance();
        if (c == '\n') {
            line_++;
            column_ = 1;
            return make_token(TokenType::NEWLINE);
        }
        return make_token(TokenType::WHITESPACE);
    }

    // Comments
    if (c == '/') {
        if (peek() == '/') {
            return read_comment();
        }
        if (peek() == '*') {
            return read_comment();
        }
        return make_token(TokenType::SLASH);
    }

    // Python comments
    if (c == '#' && language_ == Language::PYTHON) {
        while (!at_end() && peek() != '\n') advance();
        return make_token(TokenType::COMMENT);
    }

    // Preprocessor
    if (c == '#' && (language_ == Language::CPP || language_ == Language::C)) {
        return read_preprocessor();
    }

    // Strings
    if (c == '"' || c == '\'') {
        return read_string(c);
    }

    // Numbers
    if (std::isdigit(c)) {
        return read_number();
    }

    // Identifiers and keywords
    if (std::isalpha(c) || c == '_') {
        return read_identifier();
    }

    // Operators and punctuation
    switch (c) {
        case '(': return make_token(TokenType::LPAREN);
        case ')': return make_token(TokenType::RPAREN);
        case '{': return make_token(TokenType::LBRACE);
        case '}': return make_token(TokenType::RBRACE);
        case '[': return make_token(TokenType::LBRACKET);
        case ']': return make_token(TokenType::RBRACKET);
        case ';': return make_token(TokenType::SEMICOLON);
        case ',': return make_token(TokenType::COMMA);
        case '.': return make_token(TokenType::DOT);
        case '+': return make_token(TokenType::PLUS);
        case '-':
            if (peek() == '>') { advance(); return make_token(TokenType::ARROW); }
            return make_token(TokenType::MINUS);
        case '*': return make_token(TokenType::STAR);
        case '%': return make_token(TokenType::PERCENT);
        case '=':
            if (peek() == '=') { advance(); return make_token(TokenType::EQUALS); }
            return make_token(TokenType::ASSIGN);
        case '!':
            if (peek() == '=') { advance(); return make_token(TokenType::NOT_EQUALS); }
            return make_token(TokenType::NOT);
        case '<':
            if (peek() == '=') { advance(); return make_token(TokenType::LESS_EQ); }
            return make_token(TokenType::LESS);
        case '>':
            if (peek() == '=') { advance(); return make_token(TokenType::GREATER_EQ); }
            return make_token(TokenType::GREATER);
        case '&':
            if (peek() == '&') { advance(); return make_token(TokenType::AND); }
            return make_token(TokenType::AMPERSAND);
        case '|':
            if (peek() == '|') { advance(); return make_token(TokenType::OR); }
            return make_token(TokenType::PIPE);
        case '^': return make_token(TokenType::CARET);
        case '~': return make_token(TokenType::TILDE);
        case '?': return make_token(TokenType::QUESTION);
        case ':':
            if (peek() == ':') { advance(); return make_token(TokenType::DOUBLE_COLON); }
            return make_token(TokenType::COLON);
    }

    return make_token(TokenType::UNKNOWN);
}

char Lexer::peek(int offset) const {
    if (current_ + offset >= source_.size()) return '\0';
    return source_[current_ + offset];
}

char Lexer::advance() {
    column_++;
    return source_[current_++];
}

bool Lexer::match(char expected) {
    if (at_end() || source_[current_] != expected) return false;
    current_++;
    column_++;
    return true;
}

bool Lexer::at_end() const {
    return current_ >= source_.size();
}

Token Lexer::make_token(TokenType type) {
    Token tok;
    tok.type = type;
    tok.value = source_.substr(start_, current_ - start_);
    tok.line = line_;
    tok.column = column_ - static_cast<int>(current_ - start_);
    tok.offset = static_cast<int>(start_);
    tok.length = static_cast<int>(current_ - start_);
    return tok;
}

Token Lexer::make_token(TokenType type, const std::string& value) {
    Token tok = make_token(type);
    tok.value = value;
    return tok;
}

Token Lexer::read_identifier() {
    while (!at_end() && (std::isalnum(peek()) || peek() == '_')) {
        advance();
    }
    Token tok = make_token(TokenType::IDENTIFIER);
    if (is_keyword(tok.value)) {
        tok.type = TokenType::KEYWORD;
    }
    return tok;
}

Token Lexer::read_number() {
    while (!at_end() && std::isdigit(peek())) advance();

    // Decimal
    if (peek() == '.' && std::isdigit(peek(1))) {
        advance();
        while (!at_end() && std::isdigit(peek())) advance();
    }

    // Exponent
    if (peek() == 'e' || peek() == 'E') {
        advance();
        if (peek() == '+' || peek() == '-') advance();
        while (!at_end() && std::isdigit(peek())) advance();
    }

    // Suffix (f, L, u, etc.)
    while (!at_end() && std::isalpha(peek())) advance();

    return make_token(TokenType::NUMBER);
}

Token Lexer::read_string(char quote) {
    bool escaped = false;
    while (!at_end()) {
        char c = peek();
        if (c == '\n') {
            line_++;
            column_ = 0;
        }
        if (!escaped && c == quote) {
            advance();
            break;
        }
        escaped = (c == '\\' && !escaped);
        advance();
    }
    return make_token(TokenType::STRING);
}

Token Lexer::read_comment() {
    char c = source_[current_ - 1];
    if (c == '/' && peek() == '/') {
        // Single-line comment
        while (!at_end() && peek() != '\n') advance();
    } else if (c == '/' && peek() == '*') {
        // Multi-line comment
        advance(); // consume *
        while (!at_end()) {
            if (peek() == '*' && peek(1) == '/') {
                advance();
                advance();
                break;
            }
            if (peek() == '\n') {
                line_++;
                column_ = 0;
            }
            advance();
        }
    }
    return make_token(TokenType::COMMENT);
}

Token Lexer::read_preprocessor() {
    while (!at_end() && peek() != '\n') {
        if (peek() == '\\' && peek(1) == '\n') {
            advance();
            advance();
            line_++;
            column_ = 1;
        } else {
            advance();
        }
    }
    return make_token(TokenType::PREPROCESSOR);
}

// ═══════════════════════════════════════════════════════════════════════════
// SIMPLE PARSER IMPLEMENTATION
// ═══════════════════════════════════════════════════════════════════════════

SimpleParser::SimpleParser(Language lang) : lexer_(lang), language_(lang) {}

void SimpleParser::set_language(Language lang) {
    language_ = lang;
    lexer_.set_language(lang);
}

std::shared_ptr<ASTNode> SimpleParser::parse(const std::string& source) {
    tokens_ = lexer_.tokenize(source);
    token_index_ = 0;
    symbols_.clear();

    return parse_program();
}

std::shared_ptr<ASTNode> SimpleParser::parse_file(const std::string& path) {
    auto content = gFileSystem.read_file(path);
    if (!content.success) return nullptr;

    // Sprache aus Dateiendung erkennen
    Language lang = detect_language(path);
    if (lang != Language::UNKNOWN) {
        set_language(lang);
    }

    return parse(content.content);
}

Token SimpleParser::current_token() {
    if (token_index_ >= tokens_.size()) return Token{TokenType::END_OF_FILE, "", 0, 0, 0, 0};
    return tokens_[token_index_];
}

Token SimpleParser::advance_token() {
    Token tok = current_token();
    if (token_index_ < tokens_.size()) token_index_++;
    return tok;
}

bool SimpleParser::match_token(TokenType type) {
    if (check_token(type)) {
        advance_token();
        return true;
    }
    return false;
}

bool SimpleParser::check_token(TokenType type) {
    return current_token().type == type;
}

std::shared_ptr<ASTNode> SimpleParser::parse_program() {
    auto program = std::make_shared<ASTNode>();
    program->type = ASTNodeType::PROGRAM;

    while (!check_token(TokenType::END_OF_FILE)) {
        // Skip whitespace and newlines
        while (match_token(TokenType::WHITESPACE) || match_token(TokenType::NEWLINE)) {}

        if (check_token(TokenType::END_OF_FILE)) break;

        Token tok = current_token();

        // Preprocessor
        if (tok.type == TokenType::PREPROCESSOR) {
            auto node = std::make_shared<ASTNode>();
            node->type = ASTNodeType::IMPORT;
            node->value = tok.value;
            node->line_start = tok.line;
            program->children.push_back(node);
            advance_token();
            continue;
        }

        // Comments
        if (tok.type == TokenType::COMMENT) {
            auto node = std::make_shared<ASTNode>();
            node->type = ASTNodeType::COMMENT;
            node->value = tok.value;
            node->line_start = tok.line;
            program->children.push_back(node);
            advance_token();
            continue;
        }

        // Keywords für Funktionen/Klassen/etc.
        if (tok.is_keyword()) {
            if (tok.value == "class" || tok.value == "struct") {
                auto node = parse_class();
                if (node) program->children.push_back(node);
                continue;
            }
            if (tok.value == "def" || tok.value == "fn" || tok.value == "func" ||
                tok.value == "function") {
                auto node = parse_function();
                if (node) program->children.push_back(node);
                continue;
            }
            if (tok.value == "namespace") {
                advance_token();
                if (current_token().is_identifier()) {
                    auto node = std::make_shared<ASTNode>();
                    node->type = ASTNodeType::NAMESPACE;
                    node->name = current_token().value;
                    node->line_start = tok.line;
                    program->children.push_back(node);
                }
                advance_token();
                continue;
            }
            if (tok.value == "import" || tok.value == "from" || tok.value == "use" ||
                tok.value == "include" || tok.value == "require") {
                auto node = std::make_shared<ASTNode>();
                node->type = ASTNodeType::IMPORT;
                node->line_start = tok.line;

                while (!check_token(TokenType::NEWLINE) && !check_token(TokenType::SEMICOLON) &&
                       !check_token(TokenType::END_OF_FILE)) {
                    node->value += current_token().value + " ";
                    advance_token();
                }
                program->children.push_back(node);
                continue;
            }
        }

        // Mögliche Funktion (C++/C Stil: return_type name(...))
        if (tok.is_identifier()) {
            // Schau voraus ob es eine Funktion ist
            size_t save = token_index_;
            std::string potential_type;

            // Sammle potentiellen Rückgabetyp
            while (current_token().is_identifier() || current_token().type == TokenType::STAR ||
                   current_token().type == TokenType::AMPERSAND ||
                   current_token().type == TokenType::DOUBLE_COLON ||
                   current_token().type == TokenType::LESS ||
                   current_token().type == TokenType::GREATER) {
                potential_type += current_token().value;
                advance_token();
            }

            // Ist das nächste eine öffnende Klammer? Dann ist es eine Funktion
            if (check_token(TokenType::LPAREN)) {
                token_index_ = save;
                auto node = parse_function();
                if (node) program->children.push_back(node);
                continue;
            }

            // Sonst zurücksetzen und überspringen
            token_index_ = save;
        }

        // Alles andere überspringen
        advance_token();
    }

    return program;
}

std::shared_ptr<ASTNode> SimpleParser::parse_function() {
    auto node = std::make_shared<ASTNode>();
    node->type = ASTNodeType::FUNCTION;
    node->line_start = current_token().line;

    // Python: def name(...)
    if (current_token().value == "def") {
        advance_token();
        if (current_token().is_identifier()) {
            node->name = current_token().value;
            advance_token();
        }
    }
    // Rust: fn name(...)
    else if (current_token().value == "fn") {
        advance_token();
        if (current_token().is_identifier()) {
            node->name = current_token().value;
            advance_token();
        }
    }
    // JavaScript: function name(...)
    else if (current_token().value == "function") {
        advance_token();
        if (current_token().is_identifier()) {
            node->name = current_token().value;
            advance_token();
        }
    }
    // C/C++: return_type name(...)
    else {
        std::string return_type;
        while (current_token().is_identifier() || current_token().is_keyword() ||
               current_token().type == TokenType::STAR ||
               current_token().type == TokenType::AMPERSAND ||
               current_token().type == TokenType::DOUBLE_COLON) {

            // Wenn LPAREN folgt, ist das aktuelle Token der Funktionsname
            if (tokens_.size() > token_index_ + 1 &&
                tokens_[token_index_ + 1].type == TokenType::LPAREN) {
                node->name = current_token().value;
                advance_token();
                break;
            }
            return_type += current_token().value + " ";
            advance_token();
        }
        node->return_type = return_type;
    }

    // Parameter parsen
    if (match_token(TokenType::LPAREN)) {
        int depth = 1;
        std::string param_name, param_type;
        bool in_type = false;

        while (depth > 0 && !check_token(TokenType::END_OF_FILE)) {
            Token tok = current_token();
            if (tok.type == TokenType::LPAREN) depth++;
            else if (tok.type == TokenType::RPAREN) {
                depth--;
                if (depth == 0) {
                    if (!param_name.empty() || !param_type.empty()) {
                        node->parameters.push_back({param_name, param_type});
                    }
                    break;
                }
            } else if (tok.type == TokenType::COMMA && depth == 1) {
                if (!param_name.empty() || !param_type.empty()) {
                    node->parameters.push_back({param_name, param_type});
                }
                param_name.clear();
                param_type.clear();
                in_type = false;
            } else if (tok.type == TokenType::COLON && depth == 1) {
                // Python style: name: type
                in_type = true;
            } else if (tok.is_identifier() || tok.is_keyword()) {
                if (in_type || language_ != Language::PYTHON) {
                    if (param_name.empty()) param_name = tok.value;
                    else param_type += tok.value + " ";
                } else {
                    param_name = tok.value;
                }
            }
            advance_token();
        }
        match_token(TokenType::RPAREN);
    }

    // Symbol registrieren
    Symbol sym;
    sym.name = node->name;
    sym.type = node->return_type;
    sym.kind = ASTNodeType::FUNCTION;
    sym.line = node->line_start;
    symbols_.push_back(sym);

    // Body überspringen (Klammern zählen)
    if (match_token(TokenType::LBRACE)) {
        int depth = 1;
        while (depth > 0 && !check_token(TokenType::END_OF_FILE)) {
            if (current_token().type == TokenType::LBRACE) depth++;
            else if (current_token().type == TokenType::RBRACE) depth--;
            if (depth > 0) advance_token();
        }
        node->line_end = current_token().line;
        match_token(TokenType::RBRACE);
    } else if (match_token(TokenType::COLON)) {
        // Python: Einrückungsbasiert - suche nächste Funktion/Klasse auf gleicher Ebene
        node->line_end = current_token().line;
    }

    return node;
}

std::shared_ptr<ASTNode> SimpleParser::parse_class() {
    auto node = std::make_shared<ASTNode>();
    node->type = (current_token().value == "struct") ? ASTNodeType::STRUCT : ASTNodeType::CLASS;
    node->line_start = current_token().line;

    advance_token(); // skip class/struct

    if (current_token().is_identifier()) {
        node->name = current_token().value;
        advance_token();
    }

    // Symbol registrieren
    Symbol sym;
    sym.name = node->name;
    sym.kind = node->type;
    sym.line = node->line_start;
    symbols_.push_back(sym);

    // Inheritance überspringen
    while (!check_token(TokenType::LBRACE) && !check_token(TokenType::COLON) &&
           !check_token(TokenType::END_OF_FILE)) {
        advance_token();
    }

    // Body
    if (match_token(TokenType::LBRACE)) {
        int depth = 1;
        while (depth > 0 && !check_token(TokenType::END_OF_FILE)) {
            if (current_token().type == TokenType::LBRACE) depth++;
            else if (current_token().type == TokenType::RBRACE) depth--;
            if (depth > 0) advance_token();
        }
        node->line_end = current_token().line;
        match_token(TokenType::RBRACE);
    }

    return node;
}

std::shared_ptr<ASTNode> SimpleParser::parse_statement() {
    // Placeholder für erweiterte Statement-Analyse
    return nullptr;
}

std::vector<Symbol> SimpleParser::extract_symbols(const std::string& source) {
    parse(source);
    return symbols_;
}

std::optional<Symbol> SimpleParser::find_symbol(const std::string& name) {
    for (const auto& sym : symbols_) {
        if (sym.name == name) return sym;
    }
    return std::nullopt;
}

std::shared_ptr<ASTNode> SimpleParser::find_function(const std::string& name) {
    // Würde AST durchsuchen - hier vereinfacht
    return nullptr;
}

std::shared_ptr<ASTNode> SimpleParser::find_class(const std::string& name) {
    return nullptr;
}

// ═══════════════════════════════════════════════════════════════════════════
// CODE STRUCTURE ANALYZER
// ═══════════════════════════════════════════════════════════════════════════

CodeStructureAnalyzer::CodeStructureAnalyzer(Language lang) : parser_(lang), language_(lang) {}

void CodeStructureAnalyzer::analyze(const std::string& source) {
    functions_.clear();
    classes_.clear();
    variables_.clear();
    imports_.clear();

    // Zeilen zählen
    line_count_ = 1;
    comment_lines_ = 0;
    for (char c : source) {
        if (c == '\n') line_count_++;
    }

    // Parse
    auto ast = parser_.parse(source);
    if (!ast) return;

    // Symbole extrahieren
    auto symbols = parser_.extract_symbols(source);
    for (const auto& sym : symbols) {
        if (sym.kind == ASTNodeType::FUNCTION) {
            functions_.push_back(sym);
        } else if (sym.kind == ASTNodeType::CLASS || sym.kind == ASTNodeType::STRUCT) {
            classes_.push_back(sym);
        } else if (sym.kind == ASTNodeType::VARIABLE) {
            variables_.push_back(sym);
        }
    }

    // Imports aus AST extrahieren
    for (const auto& child : ast->children) {
        if (child->type == ASTNodeType::IMPORT) {
            imports_.push_back(child->value);
        }
        if (child->type == ASTNodeType::COMMENT) {
            comment_lines_++;
        }
    }

    EventBus::push("CODE_ANALYZED", "functions=" + std::to_string(functions_.size()) +
                   "|classes=" + std::to_string(classes_.size()) +
                   "|lines=" + std::to_string(line_count_));
}

void CodeStructureAnalyzer::analyze_file(const std::string& path) {
    auto content = gFileSystem.read_file(path);
    if (!content.success) return;

    Language lang = detect_language(path);
    if (lang != Language::UNKNOWN) {
        parser_.set_language(lang);
        language_ = lang;
    }

    analyze(content.content);
}

double CodeStructureAnalyzer::avg_function_length() const {
    if (functions_.empty()) return 0.0;
    // Vereinfacht: Gesamtzeilen / Anzahl Funktionen
    return static_cast<double>(line_count_ - comment_lines_) / functions_.size();
}

// ═══════════════════════════════════════════════════════════════════════════
// LANGUAGE REGISTRY
// ═══════════════════════════════════════════════════════════════════════════

LanguageRegistry& LanguageRegistry::instance() {
    static LanguageRegistry inst;
    return inst;
}

LanguageRegistry::LanguageRegistry() {
    // Keywords werden in Lexer::set_language() gesetzt
}

std::unique_ptr<Lexer> LanguageRegistry::create_lexer(Language lang) {
    return std::make_unique<Lexer>(lang);
}

std::unique_ptr<SimpleParser> LanguageRegistry::create_parser(Language lang) {
    return std::make_unique<SimpleParser>(lang);
}

std::vector<std::string> LanguageRegistry::keywords(Language lang) const {
    Lexer lexer(lang);
    // Keywords sind private, daher hier manuell zurückgeben
    switch (lang) {
        case Language::CPP:
            return {"class", "struct", "namespace", "void", "int", "return", "if", "else", "for", "while"};
        case Language::PYTHON:
            return {"def", "class", "if", "else", "for", "while", "import", "from", "return"};
        default:
            return {};
    }
}

std::string LanguageRegistry::line_comment(Language lang) const {
    switch (lang) {
        case Language::CPP:
        case Language::C:
        case Language::JAVA:
        case Language::JAVASCRIPT:
        case Language::TYPESCRIPT:
        case Language::RUST:
        case Language::GO:
            return "//";
        case Language::PYTHON:
        case Language::SHELL:
            return "#";
        default:
            return "";
    }
}

std::pair<std::string, std::string> LanguageRegistry::block_comment(Language lang) const {
    switch (lang) {
        case Language::CPP:
        case Language::C:
        case Language::JAVA:
        case Language::JAVASCRIPT:
        case Language::TYPESCRIPT:
        case Language::RUST:
        case Language::GO:
            return {"/*", "*/"};
        case Language::PYTHON:
            return {"\"\"\"", "\"\"\""};
        default:
            return {"", ""};
    }
}

} // namespace rael
