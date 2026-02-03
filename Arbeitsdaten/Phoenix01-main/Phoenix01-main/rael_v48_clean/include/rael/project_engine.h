// RAEL V48 - Project Understanding Engine (#15)
// Understands project structures, manifests, and dependencies
#pragma once

#include <string>
#include <vector>
#include <map>
#include <optional>
#include <set>
#include <variant>
#include <functional>

namespace rael {

// ═══════════════════════════════════════════════════════════════════
//  PROJECT TYPES
// ═══════════════════════════════════════════════════════════════════

enum class ProjectType {
    UNKNOWN,
    // Native
    CPP_CMAKE,
    CPP_MAKEFILE,
    CPP_MESON,
    CPP_BAZEL,
    C_CMAKE,
    C_MAKEFILE,
    // Rust
    RUST_CARGO,
    // JavaScript/TypeScript
    JS_NPM,
    JS_YARN,
    JS_PNPM,
    TS_NPM,
    // Python
    PYTHON_PIP,
    PYTHON_POETRY,
    PYTHON_PIPENV,
    PYTHON_SETUPTOOLS,
    // Go
    GO_MOD,
    // Java/Kotlin
    JAVA_MAVEN,
    JAVA_GRADLE,
    KOTLIN_GRADLE,
    // C#/.NET
    CSHARP_DOTNET,
    // Ruby
    RUBY_BUNDLER,
    // PHP
    PHP_COMPOSER
};

struct Dependency {
    std::string name;
    std::string version;          // Version constraint
    std::string resolved_version; // Actual resolved version
    bool is_dev = false;          // Dev dependency
    bool is_optional = false;
    std::string source;           // registry, git, path, etc.
    std::map<std::string, std::string> extras;
};

struct Script {
    std::string name;
    std::string command;
    std::string description;
};

struct Author {
    std::string name;
    std::string email;
    std::string url;
};

// ═══════════════════════════════════════════════════════════════════
//  PROJECT MANIFEST
// ═══════════════════════════════════════════════════════════════════

struct ProjectManifest {
    // Basic info
    std::string name;
    std::string version;
    std::string description;
    std::string license;
    std::vector<Author> authors;
    std::string homepage;
    std::string repository;
    std::vector<std::string> keywords;

    // Project type
    ProjectType type = ProjectType::UNKNOWN;
    std::string manifest_path;

    // Dependencies
    std::vector<Dependency> dependencies;
    std::vector<Dependency> dev_dependencies;
    std::vector<Dependency> peer_dependencies;  // JS
    std::vector<Dependency> optional_dependencies;

    // Build configuration
    std::string entry_point;        // main.cpp, index.js, main.py, etc.
    std::string output_dir;         // build/, dist/, target/, etc.
    std::vector<std::string> src_dirs;
    std::vector<std::string> test_dirs;
    std::vector<std::string> include_dirs;

    // Scripts/Tasks
    std::vector<Script> scripts;

    // Language-specific
    std::map<std::string, std::string> extra_fields;

    // For monorepos
    bool is_workspace = false;
    std::vector<std::string> workspace_members;
    std::string workspace_root;
};

// ═══════════════════════════════════════════════════════════════════
//  PROJECT STRUCTURE
// ═══════════════════════════════════════════════════════════════════

struct SourceFile {
    std::string path;
    std::string language;
    size_t lines = 0;
    size_t size_bytes = 0;
    bool is_test = false;
    bool is_generated = false;
};

struct Directory {
    std::string path;
    std::string purpose;  // src, test, docs, config, etc.
    size_t file_count = 0;
    size_t total_lines = 0;
};

struct ProjectStructure {
    std::string root_path;
    std::vector<SourceFile> source_files;
    std::vector<Directory> directories;
    std::map<std::string, size_t> files_by_language;
    std::map<std::string, size_t> lines_by_language;
    size_t total_files = 0;
    size_t total_lines = 0;
    std::vector<std::string> ignored_patterns;
};

// ═══════════════════════════════════════════════════════════════════
//  PROJECT ENGINE
// ═══════════════════════════════════════════════════════════════════

class ProjectEngine {
public:
    explicit ProjectEngine(const std::string& start_path = ".");

    // Detection
    std::string find_project_root() const;
    ProjectType detect_project_type() const;
    std::vector<std::string> find_manifest_files() const;

    // Parsing
    ProjectManifest parse_manifest() const;
    ProjectManifest parse_manifest(const std::string& path) const;

    // Structure analysis
    ProjectStructure analyze_structure() const;
    std::vector<SourceFile> find_source_files(const std::string& pattern = "*") const;
    std::vector<std::string> get_source_directories() const;
    std::vector<std::string> get_test_directories() const;

    // Dependency analysis
    std::vector<Dependency> get_all_dependencies() const;
    std::optional<Dependency> find_dependency(const std::string& name) const;
    bool has_dependency(const std::string& name) const;

    // Entry points
    std::string find_main_entry() const;
    std::vector<std::string> find_test_files() const;

    // Workspace/Monorepo
    bool is_monorepo() const;
    std::vector<std::string> get_workspace_packages() const;
    std::optional<ProjectManifest> get_package_manifest(const std::string& package_name) const;

    // Configuration files
    std::map<std::string, std::string> find_config_files() const;
    bool has_config_file(const std::string& name) const;

    // Language detection
    std::string detect_primary_language() const;
    std::vector<std::string> detect_languages() const;

    // Getters
    std::string root_path() const { return root_path_; }
    ProjectType project_type() const { return type_; }
    const ProjectManifest& manifest() const { return manifest_; }

private:
    std::string start_path_;
    mutable std::string root_path_;
    mutable ProjectType type_ = ProjectType::UNKNOWN;
    mutable ProjectManifest manifest_;
    mutable bool analyzed_ = false;

    void ensure_analyzed() const;

    // Parsers for different manifest types
    ProjectManifest parse_package_json(const std::string& path) const;
    ProjectManifest parse_cargo_toml(const std::string& path) const;
    ProjectManifest parse_cmake_lists(const std::string& path) const;
    ProjectManifest parse_pyproject_toml(const std::string& path) const;
    ProjectManifest parse_setup_py(const std::string& path) const;
    ProjectManifest parse_go_mod(const std::string& path) const;
    ProjectManifest parse_pom_xml(const std::string& path) const;
    ProjectManifest parse_build_gradle(const std::string& path) const;
    ProjectManifest parse_gemfile(const std::string& path) const;
    ProjectManifest parse_composer_json(const std::string& path) const;
    ProjectManifest parse_csproj(const std::string& path) const;
    ProjectManifest parse_makefile(const std::string& path) const;

    // Helpers
    std::string read_file(const std::string& path) const;
    bool file_exists(const std::string& path) const;
    std::vector<std::string> list_directory(const std::string& path, bool recursive = false) const;
    std::string get_language_for_extension(const std::string& ext) const;
};

// ═══════════════════════════════════════════════════════════════════
//  DEPENDENCY GRAPH
// ═══════════════════════════════════════════════════════════════════

struct DependencyNode {
    std::string name;
    std::string version;
    std::vector<std::string> dependencies;
    int depth = 0;
    bool is_circular = false;
};

class DependencyGraph {
public:
    void add_dependency(const std::string& from, const std::string& to);
    std::vector<DependencyNode> get_tree() const;
    std::vector<std::string> get_all_transitive(const std::string& name) const;
    bool has_circular_dependency() const;
    std::vector<std::vector<std::string>> find_cycles() const;
    std::vector<std::string> topological_sort() const;
    int max_depth() const;

private:
    std::map<std::string, std::set<std::string>> adj_;
    std::set<std::string> nodes_;
};

// ═══════════════════════════════════════════════════════════════════
//  PROJECT TEMPLATES
// ═══════════════════════════════════════════════════════════════════

struct ProjectTemplate {
    std::string name;
    ProjectType type;
    std::map<std::string, std::string> files;  // path -> content
    std::vector<std::string> directories;
    std::vector<std::string> post_create_commands;
};

class ProjectTemplates {
public:
    static ProjectTemplate get_template(ProjectType type);
    static std::vector<ProjectType> available_templates();
    static bool create_project(const std::string& path, ProjectType type, const std::string& name);

private:
    static ProjectTemplate cpp_cmake_template(const std::string& name);
    static ProjectTemplate rust_cargo_template(const std::string& name);
    static ProjectTemplate js_npm_template(const std::string& name);
    static ProjectTemplate python_pip_template(const std::string& name);
    static ProjectTemplate go_mod_template(const std::string& name);
};

// ═══════════════════════════════════════════════════════════════════
//  UTILITIES
// ═══════════════════════════════════════════════════════════════════

// Convert project type to string
std::string project_type_to_string(ProjectType type);
ProjectType string_to_project_type(const std::string& str);

// Get common paths for project type
std::string get_default_src_dir(ProjectType type);
std::string get_default_test_dir(ProjectType type);
std::string get_default_build_dir(ProjectType type);

} // namespace rael
