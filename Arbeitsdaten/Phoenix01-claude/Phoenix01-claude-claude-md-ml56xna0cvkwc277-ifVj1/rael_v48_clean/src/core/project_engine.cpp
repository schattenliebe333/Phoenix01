// RAEL V48 - Project Understanding Engine (#15)
// Implementation of project structure analysis

#include "rael/project_engine.h"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <regex>
#include <filesystem>
#include <stack>

namespace fs = std::filesystem;

namespace rael {

// C++17 helper for ends_with (available in C++20)
static bool str_ends_with(const std::string& str, const std::string& suffix) {
    if (suffix.size() > str.size()) return false;
    return str.compare(str.size() - suffix.size(), suffix.size(), suffix) == 0;
}

// ═══════════════════════════════════════════════════════════════════
//  UTILITIES
// ═══════════════════════════════════════════════════════════════════

std::string project_type_to_string(ProjectType type) {
    switch (type) {
        case ProjectType::CPP_CMAKE: return "C++ (CMake)";
        case ProjectType::CPP_MAKEFILE: return "C++ (Makefile)";
        case ProjectType::CPP_MESON: return "C++ (Meson)";
        case ProjectType::CPP_BAZEL: return "C++ (Bazel)";
        case ProjectType::C_CMAKE: return "C (CMake)";
        case ProjectType::C_MAKEFILE: return "C (Makefile)";
        case ProjectType::RUST_CARGO: return "Rust (Cargo)";
        case ProjectType::JS_NPM: return "JavaScript (npm)";
        case ProjectType::JS_YARN: return "JavaScript (Yarn)";
        case ProjectType::JS_PNPM: return "JavaScript (pnpm)";
        case ProjectType::TS_NPM: return "TypeScript (npm)";
        case ProjectType::PYTHON_PIP: return "Python (pip)";
        case ProjectType::PYTHON_POETRY: return "Python (Poetry)";
        case ProjectType::PYTHON_PIPENV: return "Python (Pipenv)";
        case ProjectType::PYTHON_SETUPTOOLS: return "Python (setuptools)";
        case ProjectType::GO_MOD: return "Go (modules)";
        case ProjectType::JAVA_MAVEN: return "Java (Maven)";
        case ProjectType::JAVA_GRADLE: return "Java (Gradle)";
        case ProjectType::KOTLIN_GRADLE: return "Kotlin (Gradle)";
        case ProjectType::CSHARP_DOTNET: return "C# (.NET)";
        case ProjectType::RUBY_BUNDLER: return "Ruby (Bundler)";
        case ProjectType::PHP_COMPOSER: return "PHP (Composer)";
        default: return "Unknown";
    }
}

std::string get_default_src_dir(ProjectType type) {
    switch (type) {
        case ProjectType::RUST_CARGO: return "src";
        case ProjectType::JS_NPM:
        case ProjectType::JS_YARN:
        case ProjectType::JS_PNPM:
        case ProjectType::TS_NPM: return "src";
        case ProjectType::JAVA_MAVEN: return "src/main/java";
        case ProjectType::JAVA_GRADLE:
        case ProjectType::KOTLIN_GRADLE: return "src/main";
        case ProjectType::GO_MOD: return ".";
        case ProjectType::PYTHON_PIP:
        case ProjectType::PYTHON_POETRY: return "src";
        default: return "src";
    }
}

std::string get_default_test_dir(ProjectType type) {
    switch (type) {
        case ProjectType::RUST_CARGO: return "tests";
        case ProjectType::JS_NPM:
        case ProjectType::JS_YARN:
        case ProjectType::TS_NPM: return "test";
        case ProjectType::JAVA_MAVEN: return "src/test/java";
        case ProjectType::JAVA_GRADLE:
        case ProjectType::KOTLIN_GRADLE: return "src/test";
        case ProjectType::GO_MOD: return ".";
        case ProjectType::PYTHON_PIP:
        case ProjectType::PYTHON_POETRY: return "tests";
        default: return "test";
    }
}

std::string get_default_build_dir(ProjectType type) {
    switch (type) {
        case ProjectType::CPP_CMAKE: return "build";
        case ProjectType::RUST_CARGO: return "target";
        case ProjectType::JS_NPM:
        case ProjectType::JS_YARN:
        case ProjectType::TS_NPM: return "dist";
        case ProjectType::JAVA_MAVEN: return "target";
        case ProjectType::JAVA_GRADLE:
        case ProjectType::KOTLIN_GRADLE: return "build";
        case ProjectType::GO_MOD: return "bin";
        case ProjectType::PYTHON_PIP: return "dist";
        case ProjectType::CSHARP_DOTNET: return "bin";
        default: return "build";
    }
}

// ═══════════════════════════════════════════════════════════════════
//  PROJECT ENGINE IMPLEMENTATION
// ═══════════════════════════════════════════════════════════════════

ProjectEngine::ProjectEngine(const std::string& start_path) : start_path_(start_path) {
    if (start_path_.empty()) {
        start_path_ = ".";
    }
}

std::string ProjectEngine::read_file(const std::string& path) const {
    std::ifstream ifs(path);
    if (!ifs) return "";
    std::stringstream ss;
    ss << ifs.rdbuf();
    return ss.str();
}

bool ProjectEngine::file_exists(const std::string& path) const {
    return fs::exists(path);
}

std::vector<std::string> ProjectEngine::list_directory(const std::string& path, bool recursive) const {
    std::vector<std::string> result;
    try {
        if (recursive) {
            for (const auto& entry : fs::recursive_directory_iterator(path)) {
                result.push_back(entry.path().string());
            }
        } else {
            for (const auto& entry : fs::directory_iterator(path)) {
                result.push_back(entry.path().string());
            }
        }
    } catch (...) {}
    return result;
}

std::string ProjectEngine::find_project_root() const {
    if (!root_path_.empty()) return root_path_;

    fs::path current = fs::absolute(start_path_);

    // Walk up directory tree looking for project markers
    const std::vector<std::string> markers = {
        "package.json", "Cargo.toml", "CMakeLists.txt", "Makefile",
        "pyproject.toml", "setup.py", "go.mod", "pom.xml",
        "build.gradle", "build.gradle.kts", "Gemfile", "composer.json",
        ".git", "meson.build", "BUILD", "WORKSPACE"
    };

    while (!current.empty() && current != current.root_path()) {
        for (const auto& marker : markers) {
            if (fs::exists(current / marker)) {
                root_path_ = current.string();
                return root_path_;
            }
        }
        current = current.parent_path();
    }

    root_path_ = fs::absolute(start_path_).string();
    return root_path_;
}

ProjectType ProjectEngine::detect_project_type() const {
    if (type_ != ProjectType::UNKNOWN) return type_;

    std::string root = find_project_root();

    // Check for specific manifest files
    if (file_exists(root + "/Cargo.toml")) {
        type_ = ProjectType::RUST_CARGO;
    } else if (file_exists(root + "/package.json")) {
        // Check for TypeScript
        if (file_exists(root + "/tsconfig.json")) {
            type_ = ProjectType::TS_NPM;
        } else if (file_exists(root + "/yarn.lock")) {
            type_ = ProjectType::JS_YARN;
        } else if (file_exists(root + "/pnpm-lock.yaml")) {
            type_ = ProjectType::JS_PNPM;
        } else {
            type_ = ProjectType::JS_NPM;
        }
    } else if (file_exists(root + "/CMakeLists.txt")) {
        // Check if C or C++
        auto files = list_directory(root, true);
        bool has_cpp = false;
        for (const auto& f : files) {
            if (str_ends_with(f, ".cpp") || str_ends_with(f, ".cxx") || str_ends_with(f, ".cc")) {
                has_cpp = true;
                break;
            }
        }
        type_ = has_cpp ? ProjectType::CPP_CMAKE : ProjectType::C_CMAKE;
    } else if (file_exists(root + "/meson.build")) {
        type_ = ProjectType::CPP_MESON;
    } else if (file_exists(root + "/Makefile") || file_exists(root + "/makefile")) {
        type_ = ProjectType::CPP_MAKEFILE;
    } else if (file_exists(root + "/BUILD") || file_exists(root + "/WORKSPACE")) {
        type_ = ProjectType::CPP_BAZEL;
    } else if (file_exists(root + "/pyproject.toml")) {
        std::string content = read_file(root + "/pyproject.toml");
        if (content.find("[tool.poetry]") != std::string::npos) {
            type_ = ProjectType::PYTHON_POETRY;
        } else {
            type_ = ProjectType::PYTHON_PIP;
        }
    } else if (file_exists(root + "/setup.py")) {
        type_ = ProjectType::PYTHON_SETUPTOOLS;
    } else if (file_exists(root + "/Pipfile")) {
        type_ = ProjectType::PYTHON_PIPENV;
    } else if (file_exists(root + "/go.mod")) {
        type_ = ProjectType::GO_MOD;
    } else if (file_exists(root + "/pom.xml")) {
        type_ = ProjectType::JAVA_MAVEN;
    } else if (file_exists(root + "/build.gradle") || file_exists(root + "/build.gradle.kts")) {
        // Check for Kotlin
        auto files = list_directory(root, true);
        bool has_kotlin = false;
        for (const auto& f : files) {
            if (str_ends_with(f, ".kt") || str_ends_with(f, ".kts")) {
                has_kotlin = true;
                break;
            }
        }
        type_ = has_kotlin ? ProjectType::KOTLIN_GRADLE : ProjectType::JAVA_GRADLE;
    } else if (file_exists(root + "/Gemfile")) {
        type_ = ProjectType::RUBY_BUNDLER;
    } else if (file_exists(root + "/composer.json")) {
        type_ = ProjectType::PHP_COMPOSER;
    } else {
        // Look for .csproj files
        for (const auto& entry : fs::directory_iterator(root)) {
            if (entry.path().extension() == ".csproj" || entry.path().extension() == ".sln") {
                type_ = ProjectType::CSHARP_DOTNET;
                break;
            }
        }
    }

    return type_;
}

std::vector<std::string> ProjectEngine::find_manifest_files() const {
    std::vector<std::string> manifests;
    std::string root = find_project_root();

    const std::vector<std::string> names = {
        "package.json", "Cargo.toml", "CMakeLists.txt", "Makefile",
        "pyproject.toml", "setup.py", "setup.cfg", "go.mod", "go.sum",
        "pom.xml", "build.gradle", "build.gradle.kts", "settings.gradle",
        "Gemfile", "composer.json", "meson.build", "BUILD", "WORKSPACE"
    };

    for (const auto& name : names) {
        std::string path = root + "/" + name;
        if (file_exists(path)) {
            manifests.push_back(path);
        }
    }

    // Also look for .csproj files
    for (const auto& entry : fs::directory_iterator(root)) {
        if (entry.path().extension() == ".csproj") {
            manifests.push_back(entry.path().string());
        }
    }

    return manifests;
}

void ProjectEngine::ensure_analyzed() const {
    if (analyzed_) return;

    find_project_root();
    detect_project_type();
    manifest_ = parse_manifest();
    analyzed_ = true;
}

ProjectManifest ProjectEngine::parse_manifest() const {
    std::string root = find_project_root();
    ProjectType type = detect_project_type();

    switch (type) {
        case ProjectType::JS_NPM:
        case ProjectType::JS_YARN:
        case ProjectType::JS_PNPM:
        case ProjectType::TS_NPM:
            return parse_package_json(root + "/package.json");
        case ProjectType::RUST_CARGO:
            return parse_cargo_toml(root + "/Cargo.toml");
        case ProjectType::CPP_CMAKE:
        case ProjectType::C_CMAKE:
            return parse_cmake_lists(root + "/CMakeLists.txt");
        case ProjectType::PYTHON_PIP:
        case ProjectType::PYTHON_POETRY:
            return parse_pyproject_toml(root + "/pyproject.toml");
        case ProjectType::PYTHON_SETUPTOOLS:
            return parse_setup_py(root + "/setup.py");
        case ProjectType::GO_MOD:
            return parse_go_mod(root + "/go.mod");
        case ProjectType::JAVA_MAVEN:
            return parse_pom_xml(root + "/pom.xml");
        case ProjectType::JAVA_GRADLE:
        case ProjectType::KOTLIN_GRADLE:
            return parse_build_gradle(root + "/build.gradle");
        case ProjectType::RUBY_BUNDLER:
            return parse_gemfile(root + "/Gemfile");
        case ProjectType::PHP_COMPOSER:
            return parse_composer_json(root + "/composer.json");
        case ProjectType::CPP_MAKEFILE:
        case ProjectType::C_MAKEFILE:
            return parse_makefile(root + "/Makefile");
        default:
            return ProjectManifest{};
    }
}

ProjectManifest ProjectEngine::parse_manifest(const std::string& path) const {
    std::string filename = fs::path(path).filename().string();

    if (filename == "package.json") return parse_package_json(path);
    if (filename == "Cargo.toml") return parse_cargo_toml(path);
    if (filename == "CMakeLists.txt") return parse_cmake_lists(path);
    if (filename == "pyproject.toml") return parse_pyproject_toml(path);
    if (filename == "setup.py") return parse_setup_py(path);
    if (filename == "go.mod") return parse_go_mod(path);
    if (filename == "pom.xml") return parse_pom_xml(path);
    if (filename == "build.gradle" || filename == "build.gradle.kts") return parse_build_gradle(path);
    if (filename == "Gemfile") return parse_gemfile(path);
    if (filename == "composer.json") return parse_composer_json(path);
    if (filename == "Makefile" || filename == "makefile") return parse_makefile(path);
    if (str_ends_with(filename, ".csproj")) return parse_csproj(path);

    return ProjectManifest{};
}

// Simple JSON value extraction (no external lib required)
static std::string json_get_string(const std::string& json, const std::string& key) {
    std::string pattern = "\"" + key + "\"\\s*:\\s*\"([^\"]+)\"";
    std::regex re(pattern);
    std::smatch match;
    if (std::regex_search(json, match, re)) {
        return match[1].str();
    }
    return "";
}

static std::vector<std::string> json_get_array(const std::string& json, const std::string& key) {
    std::vector<std::string> result;
    std::string pattern = "\"" + key + "\"\\s*:\\s*\\[([^\\]]+)\\]";
    std::regex re(pattern);
    std::smatch match;
    if (std::regex_search(json, match, re)) {
        std::string arr = match[1].str();
        std::regex item_re("\"([^\"]+)\"");
        auto begin = std::sregex_iterator(arr.begin(), arr.end(), item_re);
        auto end = std::sregex_iterator();
        for (auto it = begin; it != end; ++it) {
            result.push_back((*it)[1].str());
        }
    }
    return result;
}

static std::map<std::string, std::string> json_get_object(const std::string& json, const std::string& key) {
    std::map<std::string, std::string> result;
    // Find the object start
    std::string pattern = "\"" + key + "\"\\s*:\\s*\\{";
    std::regex re(pattern);
    std::smatch match;
    if (std::regex_search(json, match, re)) {
        size_t start = match.position() + match.length();
        int depth = 1;
        size_t end = start;
        for (; end < json.size() && depth > 0; ++end) {
            if (json[end] == '{') ++depth;
            else if (json[end] == '}') --depth;
        }
        std::string obj = json.substr(start, end - start - 1);

        // Parse key-value pairs
        std::regex kv_re("\"([^\"]+)\"\\s*:\\s*\"([^\"]*)\"");
        auto begin = std::sregex_iterator(obj.begin(), obj.end(), kv_re);
        auto it_end = std::sregex_iterator();
        for (auto it = begin; it != it_end; ++it) {
            result[(*it)[1].str()] = (*it)[2].str();
        }
    }
    return result;
}

ProjectManifest ProjectEngine::parse_package_json(const std::string& path) const {
    ProjectManifest m;
    std::string content = read_file(path);
    if (content.empty()) return m;

    m.manifest_path = path;
    m.type = file_exists(fs::path(path).parent_path().string() + "/tsconfig.json")
             ? ProjectType::TS_NPM : ProjectType::JS_NPM;

    m.name = json_get_string(content, "name");
    m.version = json_get_string(content, "version");
    m.description = json_get_string(content, "description");
    m.license = json_get_string(content, "license");
    m.entry_point = json_get_string(content, "main");
    m.homepage = json_get_string(content, "homepage");
    m.repository = json_get_string(content, "repository");
    m.keywords = json_get_array(content, "keywords");

    // Check for workspaces
    auto workspaces = json_get_array(content, "workspaces");
    if (!workspaces.empty()) {
        m.is_workspace = true;
        m.workspace_members = workspaces;
    }

    // Dependencies
    auto deps = json_get_object(content, "dependencies");
    for (const auto& [name, version] : deps) {
        Dependency d;
        d.name = name;
        d.version = version;
        d.is_dev = false;
        m.dependencies.push_back(d);
    }

    auto dev_deps = json_get_object(content, "devDependencies");
    for (const auto& [name, version] : dev_deps) {
        Dependency d;
        d.name = name;
        d.version = version;
        d.is_dev = true;
        m.dev_dependencies.push_back(d);
    }

    auto peer_deps = json_get_object(content, "peerDependencies");
    for (const auto& [name, version] : peer_deps) {
        Dependency d;
        d.name = name;
        d.version = version;
        m.peer_dependencies.push_back(d);
    }

    // Scripts
    auto scripts = json_get_object(content, "scripts");
    for (const auto& [name, cmd] : scripts) {
        Script s;
        s.name = name;
        s.command = cmd;
        m.scripts.push_back(s);
    }

    m.src_dirs = {"src"};
    m.test_dirs = {"test", "tests", "__tests__"};
    m.output_dir = "dist";

    return m;
}

ProjectManifest ProjectEngine::parse_cargo_toml(const std::string& path) const {
    ProjectManifest m;
    std::string content = read_file(path);
    if (content.empty()) return m;

    m.manifest_path = path;
    m.type = ProjectType::RUST_CARGO;

    // Simple TOML parsing
    std::regex name_re(R"RE(name\s*=\s*"([^"]+)")RE");
    std::regex version_re(R"RE(version\s*=\s*"([^"]+)")RE");
    std::regex desc_re(R"RE(description\s*=\s*"([^"]+)")RE");
    std::regex license_re(R"RE(license\s*=\s*"([^"]+)")RE");

    std::smatch match;
    if (std::regex_search(content, match, name_re)) m.name = match[1].str();
    if (std::regex_search(content, match, version_re)) m.version = match[1].str();
    if (std::regex_search(content, match, desc_re)) m.description = match[1].str();
    if (std::regex_search(content, match, license_re)) m.license = match[1].str();

    // Check for workspace
    if (content.find("[workspace]") != std::string::npos) {
        m.is_workspace = true;
        std::regex members_re(R"(members\s*=\s*\[([^\]]+)\])");
        if (std::regex_search(content, match, members_re)) {
            std::string members = match[1].str();
            std::regex item_re(R"RE("([^"]+)")RE");
            auto begin = std::sregex_iterator(members.begin(), members.end(), item_re);
            auto end = std::sregex_iterator();
            for (auto it = begin; it != end; ++it) {
                m.workspace_members.push_back((*it)[1].str());
            }
        }
    }

    // Parse dependencies
    std::regex dep_re(R"RE((\w[\w-]*)\s*=\s*(?:"([^"]+)"|\{[^}]*version\s*=\s*"([^"]+)"))RE");
    size_t deps_pos = content.find("[dependencies]");
    size_t dev_deps_pos = content.find("[dev-dependencies]");

    auto parse_deps = [&](size_t start, size_t end, bool is_dev) {
        std::string section = content.substr(start, end - start);
        auto begin = std::sregex_iterator(section.begin(), section.end(), dep_re);
        auto it_end = std::sregex_iterator();
        for (auto it = begin; it != it_end; ++it) {
            Dependency d;
            d.name = (*it)[1].str();
            d.version = (*it)[2].matched ? (*it)[2].str() : (*it)[3].str();
            d.is_dev = is_dev;
            if (is_dev) m.dev_dependencies.push_back(d);
            else m.dependencies.push_back(d);
        }
    };

    if (deps_pos != std::string::npos) {
        size_t end = dev_deps_pos != std::string::npos ? dev_deps_pos : content.size();
        parse_deps(deps_pos, end, false);
    }
    if (dev_deps_pos != std::string::npos) {
        parse_deps(dev_deps_pos, content.size(), true);
    }

    m.src_dirs = {"src"};
    m.test_dirs = {"tests"};
    m.output_dir = "target";
    m.entry_point = "src/main.rs";

    return m;
}

ProjectManifest ProjectEngine::parse_cmake_lists(const std::string& path) const {
    ProjectManifest m;
    std::string content = read_file(path);
    if (content.empty()) return m;

    m.manifest_path = path;
    m.type = ProjectType::CPP_CMAKE;

    std::regex project_re(R"(project\s*\(\s*(\w+)(?:\s+VERSION\s+([\d.]+))?)", std::regex::icase);
    std::smatch match;
    if (std::regex_search(content, match, project_re)) {
        m.name = match[1].str();
        if (match[2].matched) m.version = match[2].str();
    }

    m.src_dirs = {"src"};
    m.include_dirs = {"include"};
    m.test_dirs = {"test", "tests"};
    m.output_dir = "build";

    return m;
}

ProjectManifest ProjectEngine::parse_pyproject_toml(const std::string& path) const {
    ProjectManifest m;
    std::string content = read_file(path);
    if (content.empty()) return m;

    m.manifest_path = path;
    m.type = content.find("[tool.poetry]") != std::string::npos
             ? ProjectType::PYTHON_POETRY : ProjectType::PYTHON_PIP;

    std::regex name_re(R"RE(name\s*=\s*"([^"]+)")RE");
    std::regex version_re(R"RE(version\s*=\s*"([^"]+)")RE");
    std::regex desc_re(R"RE(description\s*=\s*"([^"]+)")RE");

    std::smatch match;
    if (std::regex_search(content, match, name_re)) m.name = match[1].str();
    if (std::regex_search(content, match, version_re)) m.version = match[1].str();
    if (std::regex_search(content, match, desc_re)) m.description = match[1].str();

    m.src_dirs = {"src", m.name};
    m.test_dirs = {"tests"};
    m.output_dir = "dist";

    return m;
}

ProjectManifest ProjectEngine::parse_setup_py(const std::string& path) const {
    ProjectManifest m;
    std::string content = read_file(path);
    if (content.empty()) return m;

    m.manifest_path = path;
    m.type = ProjectType::PYTHON_SETUPTOOLS;

    std::regex name_re(R"(name\s*=\s*['\"]([^'\"]+)['\"])");
    std::regex version_re(R"(version\s*=\s*['\"]([^'\"]+)['\"])");

    std::smatch match;
    if (std::regex_search(content, match, name_re)) m.name = match[1].str();
    if (std::regex_search(content, match, version_re)) m.version = match[1].str();

    m.src_dirs = {"src", m.name};
    m.test_dirs = {"tests"};
    m.output_dir = "dist";

    return m;
}

ProjectManifest ProjectEngine::parse_go_mod(const std::string& path) const {
    ProjectManifest m;
    std::string content = read_file(path);
    if (content.empty()) return m;

    m.manifest_path = path;
    m.type = ProjectType::GO_MOD;

    std::regex module_re(R"(module\s+(\S+))");
    std::regex go_re(R"(go\s+([\d.]+))");
    std::regex require_re(R"((\S+)\s+(v[\d.]+(?:-[\w.]+)?))");

    std::smatch match;
    if (std::regex_search(content, match, module_re)) m.name = match[1].str();
    if (std::regex_search(content, match, go_re)) m.version = match[1].str();

    // Parse requirements
    size_t req_start = content.find("require (");
    size_t req_end = content.find(")", req_start);
    if (req_start != std::string::npos && req_end != std::string::npos) {
        std::string req_block = content.substr(req_start, req_end - req_start);
        auto begin = std::sregex_iterator(req_block.begin(), req_block.end(), require_re);
        auto end = std::sregex_iterator();
        for (auto it = begin; it != end; ++it) {
            Dependency d;
            d.name = (*it)[1].str();
            d.version = (*it)[2].str();
            m.dependencies.push_back(d);
        }
    }

    m.src_dirs = {".", "cmd", "pkg", "internal"};
    m.test_dirs = {"."};
    m.output_dir = "bin";
    m.entry_point = "main.go";

    return m;
}

ProjectManifest ProjectEngine::parse_pom_xml(const std::string& path) const {
    ProjectManifest m;
    std::string content = read_file(path);
    if (content.empty()) return m;

    m.manifest_path = path;
    m.type = ProjectType::JAVA_MAVEN;

    // Simple XML parsing
    auto get_tag = [&](const std::string& tag) -> std::string {
        std::regex re("<" + tag + ">([^<]+)</" + tag + ">");
        std::smatch match;
        if (std::regex_search(content, match, re)) {
            return match[1].str();
        }
        return "";
    };

    m.name = get_tag("artifactId");
    m.version = get_tag("version");
    m.description = get_tag("description");

    m.src_dirs = {"src/main/java"};
    m.test_dirs = {"src/test/java"};
    m.output_dir = "target";

    return m;
}

ProjectManifest ProjectEngine::parse_build_gradle(const std::string& path) const {
    ProjectManifest m;
    std::string content = read_file(path);
    if (content.empty()) return m;

    m.manifest_path = path;
    m.type = content.find(".kt") != std::string::npos
             ? ProjectType::KOTLIN_GRADLE : ProjectType::JAVA_GRADLE;

    std::regex version_re(R"(version\s*=?\s*['\"]([^'\"]+)['\"])");
    std::smatch match;
    if (std::regex_search(content, match, version_re)) m.version = match[1].str();

    m.src_dirs = {"src/main"};
    m.test_dirs = {"src/test"};
    m.output_dir = "build";

    return m;
}

ProjectManifest ProjectEngine::parse_gemfile(const std::string& path) const {
    ProjectManifest m;
    std::string content = read_file(path);
    if (content.empty()) return m;

    m.manifest_path = path;
    m.type = ProjectType::RUBY_BUNDLER;

    // Parse gems
    std::regex gem_re(R"(gem\s+['\"]([^'\"]+)['\"](?:\s*,\s*['\"]([^'\"]+)['\"])?)");
    auto begin = std::sregex_iterator(content.begin(), content.end(), gem_re);
    auto end = std::sregex_iterator();
    for (auto it = begin; it != end; ++it) {
        Dependency d;
        d.name = (*it)[1].str();
        if ((*it)[2].matched) d.version = (*it)[2].str();
        m.dependencies.push_back(d);
    }

    m.src_dirs = {"lib"};
    m.test_dirs = {"spec", "test"};

    return m;
}

ProjectManifest ProjectEngine::parse_composer_json(const std::string& path) const {
    ProjectManifest m;
    std::string content = read_file(path);
    if (content.empty()) return m;

    m.manifest_path = path;
    m.type = ProjectType::PHP_COMPOSER;

    m.name = json_get_string(content, "name");
    m.version = json_get_string(content, "version");
    m.description = json_get_string(content, "description");
    m.license = json_get_string(content, "license");

    auto deps = json_get_object(content, "require");
    for (const auto& [name, version] : deps) {
        if (name.find("php") != 0 && name.find("ext-") != 0) {
            Dependency d;
            d.name = name;
            d.version = version;
            m.dependencies.push_back(d);
        }
    }

    auto dev_deps = json_get_object(content, "require-dev");
    for (const auto& [name, version] : dev_deps) {
        Dependency d;
        d.name = name;
        d.version = version;
        d.is_dev = true;
        m.dev_dependencies.push_back(d);
    }

    m.src_dirs = {"src"};
    m.test_dirs = {"tests"};

    return m;
}

ProjectManifest ProjectEngine::parse_csproj(const std::string& path) const {
    ProjectManifest m;
    std::string content = read_file(path);
    if (content.empty()) return m;

    m.manifest_path = path;
    m.type = ProjectType::CSHARP_DOTNET;

    auto get_tag = [&](const std::string& tag) -> std::string {
        std::regex re("<" + tag + ">([^<]+)</" + tag + ">");
        std::smatch match;
        if (std::regex_search(content, match, re)) {
            return match[1].str();
        }
        return "";
    };

    m.name = fs::path(path).stem().string();
    m.version = get_tag("Version");
    m.description = get_tag("Description");

    // Parse PackageReferences
    std::regex pkg_re(R"(<PackageReference\s+Include=\"([^\"]+)\"\s+Version=\"([^\"]+)\")");
    auto begin = std::sregex_iterator(content.begin(), content.end(), pkg_re);
    auto end = std::sregex_iterator();
    for (auto it = begin; it != end; ++it) {
        Dependency d;
        d.name = (*it)[1].str();
        d.version = (*it)[2].str();
        m.dependencies.push_back(d);
    }

    m.output_dir = "bin";

    return m;
}

ProjectManifest ProjectEngine::parse_makefile(const std::string& path) const {
    ProjectManifest m;
    std::string content = read_file(path);
    if (content.empty()) return m;

    m.manifest_path = path;
    m.type = ProjectType::CPP_MAKEFILE;

    // Try to extract project name from common patterns
    std::regex name_re(R"((?:PROJECT|TARGET|NAME)\s*[:=]\s*(\w+))");
    std::smatch match;
    if (std::regex_search(content, match, name_re)) {
        m.name = match[1].str();
    } else {
        m.name = fs::path(path).parent_path().filename().string();
    }

    m.src_dirs = {"src", "."};
    m.include_dirs = {"include"};
    m.test_dirs = {"test", "tests"};

    return m;
}

std::string ProjectEngine::get_language_for_extension(const std::string& ext) const {
    static const std::map<std::string, std::string> ext_map = {
        {".cpp", "C++"}, {".cxx", "C++"}, {".cc", "C++"}, {".hpp", "C++"}, {".hxx", "C++"},
        {".c", "C"}, {".h", "C"},
        {".rs", "Rust"},
        {".js", "JavaScript"}, {".mjs", "JavaScript"}, {".jsx", "JavaScript"},
        {".ts", "TypeScript"}, {".tsx", "TypeScript"},
        {".py", "Python"}, {".pyw", "Python"}, {".pyx", "Python"},
        {".go", "Go"},
        {".java", "Java"},
        {".kt", "Kotlin"}, {".kts", "Kotlin"},
        {".cs", "C#"},
        {".rb", "Ruby"},
        {".php", "PHP"},
        {".swift", "Swift"},
        {".scala", "Scala"},
        {".lua", "Lua"},
        {".sh", "Shell"}, {".bash", "Shell"},
        {".ps1", "PowerShell"},
        {".sql", "SQL"},
        {".html", "HTML"}, {".htm", "HTML"},
        {".css", "CSS"}, {".scss", "SCSS"}, {".sass", "Sass"}, {".less", "Less"},
        {".json", "JSON"}, {".yaml", "YAML"}, {".yml", "YAML"}, {".toml", "TOML"}, {".xml", "XML"},
        {".md", "Markdown"}, {".rst", "reStructuredText"}
    };

    auto it = ext_map.find(ext);
    return it != ext_map.end() ? it->second : "";
}

ProjectStructure ProjectEngine::analyze_structure() const {
    ProjectStructure ps;
    ps.root_path = find_project_root();

    std::set<std::string> ignored = {
        ".git", "node_modules", "target", "build", "dist", "__pycache__",
        ".venv", "venv", ".idea", ".vscode", "vendor", "bin", "obj"
    };

    try {
        for (const auto& entry : fs::recursive_directory_iterator(ps.root_path)) {
            std::string path = entry.path().string();

            // Check if in ignored directory
            bool skip = false;
            for (const auto& ig : ignored) {
                if (path.find("/" + ig + "/") != std::string::npos ||
                    path.find("\\" + ig + "\\") != std::string::npos) {
                    skip = true;
                    break;
                }
            }
            if (skip) continue;

            if (entry.is_regular_file()) {
                SourceFile sf;
                sf.path = path;
                sf.size_bytes = entry.file_size();

                std::string ext = entry.path().extension().string();
                sf.language = get_language_for_extension(ext);

                // Count lines
                std::ifstream ifs(path);
                sf.lines = std::count(std::istreambuf_iterator<char>(ifs),
                                      std::istreambuf_iterator<char>(), '\n');

                // Check if test file
                std::string filename = entry.path().filename().string();
                sf.is_test = filename.find("test") != std::string::npos ||
                             filename.find("spec") != std::string::npos ||
                             path.find("/test") != std::string::npos ||
                             path.find("/tests") != std::string::npos;

                ps.source_files.push_back(sf);
                ps.total_files++;
                ps.total_lines += sf.lines;

                if (!sf.language.empty()) {
                    ps.files_by_language[sf.language]++;
                    ps.lines_by_language[sf.language] += sf.lines;
                }
            }
        }
    } catch (...) {}

    return ps;
}

std::string ProjectEngine::detect_primary_language() const {
    auto structure = analyze_structure();

    std::string primary;
    size_t max_lines = 0;

    for (const auto& [lang, lines] : structure.lines_by_language) {
        if (lines > max_lines) {
            max_lines = lines;
            primary = lang;
        }
    }

    return primary;
}

std::vector<std::string> ProjectEngine::detect_languages() const {
    auto structure = analyze_structure();

    std::vector<std::string> langs;
    for (const auto& [lang, count] : structure.files_by_language) {
        langs.push_back(lang);
    }

    return langs;
}

std::vector<Dependency> ProjectEngine::get_all_dependencies() const {
    ensure_analyzed();
    std::vector<Dependency> all;
    all.insert(all.end(), manifest_.dependencies.begin(), manifest_.dependencies.end());
    all.insert(all.end(), manifest_.dev_dependencies.begin(), manifest_.dev_dependencies.end());
    return all;
}

std::optional<Dependency> ProjectEngine::find_dependency(const std::string& name) const {
    ensure_analyzed();
    for (const auto& d : manifest_.dependencies) {
        if (d.name == name) return d;
    }
    for (const auto& d : manifest_.dev_dependencies) {
        if (d.name == name) return d;
    }
    return std::nullopt;
}

bool ProjectEngine::has_dependency(const std::string& name) const {
    return find_dependency(name).has_value();
}

bool ProjectEngine::is_monorepo() const {
    ensure_analyzed();
    return manifest_.is_workspace;
}

std::vector<std::string> ProjectEngine::get_workspace_packages() const {
    ensure_analyzed();
    return manifest_.workspace_members;
}

// ═══════════════════════════════════════════════════════════════════
//  DEPENDENCY GRAPH IMPLEMENTATION
// ═══════════════════════════════════════════════════════════════════

void DependencyGraph::add_dependency(const std::string& from, const std::string& to) {
    nodes_.insert(from);
    nodes_.insert(to);
    adj_[from].insert(to);
}

std::vector<std::string> DependencyGraph::get_all_transitive(const std::string& name) const {
    std::vector<std::string> result;
    std::set<std::string> visited;
    std::stack<std::string> stack;

    stack.push(name);

    while (!stack.empty()) {
        std::string current = stack.top();
        stack.pop();

        if (visited.count(current)) continue;
        visited.insert(current);

        if (current != name) {
            result.push_back(current);
        }

        auto it = adj_.find(current);
        if (it != adj_.end()) {
            for (const auto& dep : it->second) {
                stack.push(dep);
            }
        }
    }

    return result;
}

bool DependencyGraph::has_circular_dependency() const {
    return !find_cycles().empty();
}

std::vector<std::vector<std::string>> DependencyGraph::find_cycles() const {
    std::vector<std::vector<std::string>> cycles;
    std::set<std::string> visited;
    std::set<std::string> rec_stack;
    std::vector<std::string> path;

    std::function<bool(const std::string&)> dfs = [&](const std::string& node) -> bool {
        visited.insert(node);
        rec_stack.insert(node);
        path.push_back(node);

        auto it = adj_.find(node);
        if (it != adj_.end()) {
            for (const auto& neighbor : it->second) {
                if (!visited.count(neighbor)) {
                    if (dfs(neighbor)) return true;
                } else if (rec_stack.count(neighbor)) {
                    // Found cycle
                    std::vector<std::string> cycle;
                    bool found = false;
                    for (const auto& n : path) {
                        if (n == neighbor) found = true;
                        if (found) cycle.push_back(n);
                    }
                    cycle.push_back(neighbor);
                    cycles.push_back(cycle);
                }
            }
        }

        path.pop_back();
        rec_stack.erase(node);
        return false;
    };

    for (const auto& node : nodes_) {
        if (!visited.count(node)) {
            dfs(node);
        }
    }

    return cycles;
}

std::vector<std::string> DependencyGraph::topological_sort() const {
    std::vector<std::string> result;
    std::set<std::string> visited;
    std::stack<std::string> stack;

    std::function<void(const std::string&)> dfs = [&](const std::string& node) {
        visited.insert(node);
        auto it = adj_.find(node);
        if (it != adj_.end()) {
            for (const auto& neighbor : it->second) {
                if (!visited.count(neighbor)) {
                    dfs(neighbor);
                }
            }
        }
        stack.push(node);
    };

    for (const auto& node : nodes_) {
        if (!visited.count(node)) {
            dfs(node);
        }
    }

    while (!stack.empty()) {
        result.push_back(stack.top());
        stack.pop();
    }

    return result;
}

} // namespace rael
