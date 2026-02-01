#pragma once
// ═══════════════════════════════════════════════════════════════════════════
// RAEL LANGUAGE PARSER + LEXER (Verbesserung #12)
// ═══════════════════════════════════════════════════════════════════════════
// Tokenizer und SimpleAST für mehrere Programmiersprachen
// ═══════════════════════════════════════════════════════════════════════════

#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <functional>
#include <cstdint>
#include <optional>

namespace rael {

// ═══════════════════════════════════════════════════════════════════════════
// TOKEN TYPES
// ═══════════════════════════════════════════════════════════════════════════

enum class TokenType : uint8_t {
    // Literals
    IDENTIFIER = 1,
    NUMBER = 2,
    STRING = 3,
    CHAR = 4,

    // Keywords
    KEYWORD = 10,

    // Operators
    OPERATOR = 20,
    ASSIGN = 21,
    PLUS = 22,
    MINUS = 23,
    STAR = 24,
    SLASH = 25,
    PERCENT = 26,
    EQUALS = 27,
    NOT_EQUALS = 28,
    LESS = 29,
    GREATER = 30,
    LESS_EQ = 31,
    GREATER_EQ = 32,
    AND = 33,
    OR = 34,
    NOT = 35,
    AMPERSAND = 36,
    PIPE = 37,
    CARET = 38,
    TILDE = 39,
    ARROW = 40,
    DOT = 41,
    COMMA = 42,
    COLON = 43,
    SEMICOLON = 44,
    QUESTION = 45,
    DOUBLE_COLON = 46,

    // Brackets
    LPAREN = 50,
    RPAREN = 51,
    LBRACE = 52,
    RBRACE = 53,
    LBRACKET = 54,
    RBRACKET = 55,

    // Special
    COMMENT = 60,
    WHITESPACE = 61,
    NEWLINE = 62,
    PREPROCESSOR = 63,

    // End
    END_OF_FILE = 99,
    UNKNOWN = 0
};

inline const char* token_type_name(TokenType t) {
    switch (t) {
        case TokenType::IDENTIFIER: return "IDENTIFIER";
        case TokenType::NUMBER: return "NUMBER";
        case TokenType::STRING: return "STRING";
        case TokenType::KEYWORD: return "KEYWORD";
        case TokenType::OPERATOR: return "OPERATOR";
        case TokenType::COMMENT: return "COMMENT";
        case TokenType::WHITESPACE: return "WHITESPACE";
        case TokenType::END_OF_FILE: return "EOF";
        default: return "OTHER";
    }
}

// ═══════════════════════════════════════════════════════════════════════════
// TOKEN - Ein einzelnes Token
// ═══════════════════════════════════════════════════════════════════════════

struct Token {
    TokenType type = TokenType::UNKNOWN;
    std::string value;
    int line = 0;
    int column = 0;
    int offset = 0;
    int length = 0;

    bool is(TokenType t) const { return type == t; }
    bool is_identifier() const { return type == TokenType::IDENTIFIER; }
    bool is_keyword() const { return type == TokenType::KEYWORD; }
    bool is_operator() const { return type >= TokenType::OPERATOR && type <= TokenType::DOUBLE_COLON; }
    bool is_literal() const { return type >= TokenType::IDENTIFIER && type <= TokenType::CHAR; }
};

// ═══════════════════════════════════════════════════════════════════════════
// LANGUAGE - Unterstützte Programmiersprachen
// ═══════════════════════════════════════════════════════════════════════════

enum class Language : uint8_t {
    UNKNOWN = 0,
    CPP = 1,
    C = 2,
    PYTHON = 3,
    JAVASCRIPT = 4,
    TYPESCRIPT = 5,
    JAVA = 6,
    RUST = 7,
    GO = 8,
    JSON = 9,
    YAML = 10,
    MARKDOWN = 11,
    SHELL = 12
};

inline Language detect_language(const std::string& filename) {
    size_t dot = filename.rfind('.');
    if (dot == std::string::npos) return Language::UNKNOWN;

    std::string ext = filename.substr(dot + 1);
    if (ext == "cpp" || ext == "cc" || ext == "cxx" || ext == "hpp" || ext == "h") return Language::CPP;
    if (ext == "c") return Language::C;
    if (ext == "py" || ext == "pyw") return Language::PYTHON;
    if (ext == "js" || ext == "mjs") return Language::JAVASCRIPT;
    if (ext == "ts" || ext == "tsx") return Language::TYPESCRIPT;
    if (ext == "java") return Language::JAVA;
    if (ext == "rs") return Language::RUST;
    if (ext == "go") return Language::GO;
    if (ext == "json") return Language::JSON;
    if (ext == "yaml" || ext == "yml") return Language::YAML;
    if (ext == "md" || ext == "markdown") return Language::MARKDOWN;
    if (ext == "sh" || ext == "bash" || ext == "zsh") return Language::SHELL;
    return Language::UNKNOWN;
}

inline const char* language_name(Language l) {
    switch (l) {
        case Language::CPP: return "C++";
        case Language::C: return "C";
        case Language::PYTHON: return "Python";
        case Language::JAVASCRIPT: return "JavaScript";
        case Language::TYPESCRIPT: return "TypeScript";
        case Language::JAVA: return "Java";
        case Language::RUST: return "Rust";
        case Language::GO: return "Go";
        case Language::JSON: return "JSON";
        case Language::YAML: return "YAML";
        case Language::MARKDOWN: return "Markdown";
        case Language::SHELL: return "Shell";
        default: return "Unknown";
    }
}

// ═══════════════════════════════════════════════════════════════════════════
// LEXER - Tokenisiert Quellcode
// ═══════════════════════════════════════════════════════════════════════════

class Lexer {
public:
    explicit Lexer(Language lang = Language::CPP);

    // Tokenisiere Quellcode
    std::vector<Token> tokenize(const std::string& source);

    // Konfiguration
    void set_skip_whitespace(bool skip) { skip_whitespace_ = skip; }
    void set_skip_comments(bool skip) { skip_comments_ = skip; }

    // Sprache ändern
    void set_language(Language lang);

private:
    Token next_token();
    char peek(int offset = 0) const;
    char advance();
    bool match(char expected);
    bool at_end() const;

    Token make_token(TokenType type);
    Token make_token(TokenType type, const std::string& value);

    void skip_whitespace_chars();
    Token read_identifier();
    Token read_number();
    Token read_string(char quote);
    Token read_comment();
    Token read_preprocessor();

    bool is_keyword(const std::string& word) const;

    std::string source_;
    size_t start_ = 0;
    size_t current_ = 0;
    int line_ = 1;
    int column_ = 1;

    Language language_ = Language::CPP;
    bool skip_whitespace_ = true;
    bool skip_comments_ = false;

    std::vector<std::string> keywords_;
};

// ═══════════════════════════════════════════════════════════════════════════
// AST NODE TYPES
// ═══════════════════════════════════════════════════════════════════════════

enum class ASTNodeType : uint8_t {
    PROGRAM = 1,
    FUNCTION = 2,
    CLASS = 3,
    STRUCT = 4,
    ENUM = 5,
    NAMESPACE = 6,
    VARIABLE = 7,
    PARAMETER = 8,
    BLOCK = 9,
    STATEMENT = 10,
    EXPRESSION = 11,
    IF = 12,
    FOR = 13,
    WHILE = 14,
    RETURN = 15,
    CALL = 16,
    IMPORT = 17,
    COMMENT = 18,
    UNKNOWN = 0
};

// ═══════════════════════════════════════════════════════════════════════════
// AST NODE - Knoten im abstrakten Syntaxbaum
// ═══════════════════════════════════════════════════════════════════════════

struct ASTNode {
    ASTNodeType type = ASTNodeType::UNKNOWN;
    std::string name;
    std::string value;

    // Position im Quellcode
    int line_start = 0;
    int line_end = 0;
    int column_start = 0;
    int column_end = 0;

    // Für Funktionen/Klassen
    std::string return_type;
    std::string visibility;  // public, private, protected
    std::vector<std::string> modifiers;  // static, virtual, const, etc.
    std::vector<std::pair<std::string, std::string>> parameters;  // name, type

    // Kindknoten
    std::vector<std::shared_ptr<ASTNode>> children;

    // Parent (weak reference)
    std::weak_ptr<ASTNode> parent;

    // Hilfsmethoden
    bool is_function() const { return type == ASTNodeType::FUNCTION; }
    bool is_class() const { return type == ASTNodeType::CLASS || type == ASTNodeType::STRUCT; }
    bool is_variable() const { return type == ASTNodeType::VARIABLE; }
};

// ═══════════════════════════════════════════════════════════════════════════
// SYMBOL - Ein Symbol (Funktion, Variable, Klasse, etc.)
// ═══════════════════════════════════════════════════════════════════════════

struct Symbol {
    std::string name;
    std::string type;           // Typ (z.B. "int", "std::string", "MyClass")
    std::string scope;          // Vollständiger Scope (z.B. "namespace::class::")
    ASTNodeType kind = ASTNodeType::UNKNOWN;
    int line = 0;
    int column = 0;
    std::string file;

    std::string full_name() const {
        return scope.empty() ? name : scope + name;
    }
};

// ═══════════════════════════════════════════════════════════════════════════
// SIMPLE PARSER - Einfacher Parser für Struktur-Extraktion
// ═══════════════════════════════════════════════════════════════════════════

class SimpleParser {
public:
    explicit SimpleParser(Language lang = Language::CPP);

    // Parse Quellcode in AST
    std::shared_ptr<ASTNode> parse(const std::string& source);

    // Parse Datei
    std::shared_ptr<ASTNode> parse_file(const std::string& path);

    // Extrahiere nur Top-Level-Symbole (schneller)
    std::vector<Symbol> extract_symbols(const std::string& source);

    // Finde Symbol by Name
    std::optional<Symbol> find_symbol(const std::string& name);

    // Finde Funktion by Name
    std::shared_ptr<ASTNode> find_function(const std::string& name);

    // Finde Klasse by Name
    std::shared_ptr<ASTNode> find_class(const std::string& name);

    // Sprache
    void set_language(Language lang);

private:
    // Parser-Methoden
    std::shared_ptr<ASTNode> parse_program();
    std::shared_ptr<ASTNode> parse_function();
    std::shared_ptr<ASTNode> parse_class();
    std::shared_ptr<ASTNode> parse_statement();

    Token current_token();
    Token advance_token();
    bool match_token(TokenType type);
    bool check_token(TokenType type);

    Lexer lexer_;
    std::vector<Token> tokens_;
    size_t token_index_ = 0;
    Language language_ = Language::CPP;
    std::vector<Symbol> symbols_;
};

// ═══════════════════════════════════════════════════════════════════════════
// CODE ANALYZER - Analysiert Code für verschiedene Zwecke
// ═══════════════════════════════════════════════════════════════════════════

class CodeStructureAnalyzer {
public:
    explicit CodeStructureAnalyzer(Language lang = Language::CPP);

    // Analysiere Quellcode
    void analyze(const std::string& source);

    // Analysiere Datei
    void analyze_file(const std::string& path);

    // Ergebnisse
    std::vector<Symbol> functions() const { return functions_; }
    std::vector<Symbol> classes() const { return classes_; }
    std::vector<Symbol> variables() const { return variables_; }
    std::vector<std::string> imports() const { return imports_; }

    // Code-Metriken
    int line_count() const { return line_count_; }
    int function_count() const { return static_cast<int>(functions_.size()); }
    int class_count() const { return static_cast<int>(classes_.size()); }
    int comment_line_count() const { return comment_lines_; }

    // Durchschnittliche Funktionslänge
    double avg_function_length() const;

private:
    SimpleParser parser_;
    Language language_;

    std::vector<Symbol> functions_;
    std::vector<Symbol> classes_;
    std::vector<Symbol> variables_;
    std::vector<std::string> imports_;

    int line_count_ = 0;
    int comment_lines_ = 0;
};

// ═══════════════════════════════════════════════════════════════════════════
// LANGUAGE REGISTRY - Verwaltet Sprach-Konfigurationen
// ═══════════════════════════════════════════════════════════════════════════

class LanguageRegistry {
public:
    static LanguageRegistry& instance();

    // Lexer für Sprache erstellen
    std::unique_ptr<Lexer> create_lexer(Language lang);

    // Parser für Sprache erstellen
    std::unique_ptr<SimpleParser> create_parser(Language lang);

    // Keywords für Sprache
    std::vector<std::string> keywords(Language lang) const;

    // Kommentar-Syntax
    std::string line_comment(Language lang) const;
    std::pair<std::string, std::string> block_comment(Language lang) const;

private:
    LanguageRegistry();
    std::unordered_map<Language, std::vector<std::string>> keywords_;
};

} // namespace rael
