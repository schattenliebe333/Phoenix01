#include "rael/filesystem.h"
#include "rael/events.h"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <regex>
#include <cstring>

#ifdef _WIN32
#include <windows.h>
#include <direct.h>
#define getcwd _getcwd
#define chdir _chdir
#else
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <unistd.h>
#include <fnmatch.h>
#endif

namespace rael {

// Global instance
FileSystem gFileSystem;

// ═══════════════════════════════════════════════════════════════════════════
// CONSTRUCTOR
// ═══════════════════════════════════════════════════════════════════════════

FileSystem::FileSystem() {}

// ═══════════════════════════════════════════════════════════════════════════
// HELPER FUNCTIONS
// ═══════════════════════════════════════════════════════════════════════════

std::string FileSystem::normalize_path(const std::string& path) {
    if (path.empty()) return ".";
    std::string result = path;
    // Normalize slashes
#ifdef _WIN32
    std::replace(result.begin(), result.end(), '/', '\\');
#else
    std::replace(result.begin(), result.end(), '\\', '/');
#endif
    return result;
}

bool FileSystem::match_glob(const std::string& pattern, const std::string& text) {
#ifdef _WIN32
    // Simple glob implementation for Windows
    std::string regex_pattern;
    for (char c : pattern) {
        switch (c) {
            case '*': regex_pattern += ".*"; break;
            case '?': regex_pattern += "."; break;
            case '.': regex_pattern += "\\."; break;
            default: regex_pattern += c;
        }
    }
    try {
        std::regex re(regex_pattern, std::regex::icase);
        return std::regex_match(text, re);
    } catch (...) {
        return false;
    }
#else
    return fnmatch(pattern.c_str(), text.c_str(), FNM_PATHNAME) == 0;
#endif
}

// ═══════════════════════════════════════════════════════════════════════════
// LESEN
// ═══════════════════════════════════════════════════════════════════════════

ReadResult FileSystem::read_file(const std::string& path) {
    ReadResult result;
    std::string norm_path = normalize_path(path);

    if (sandbox_enabled_ && !is_path_allowed(norm_path)) {
        result.error = "Path not allowed in sandbox mode";
        return result;
    }

    std::ifstream file(norm_path, std::ios::binary | std::ios::ate);
    if (!file) {
        result.error = "Could not open file: " + norm_path;
        return result;
    }

    auto size = file.tellg();
    if (size < 0) {
        result.error = "Could not determine file size";
        return result;
    }

    if (static_cast<uint64_t>(size) > max_read_size_) {
        result.error = "File too large (max " + std::to_string(max_read_size_) + " bytes)";
        return result;
    }

    file.seekg(0);
    result.content.resize(static_cast<size_t>(size));
    file.read(&result.content[0], size);
    result.bytes_read = static_cast<uint64_t>(file.gcount());
    result.success = true;

    EventBus::push("FS_READ", "path=" + norm_path + "|bytes=" + std::to_string(result.bytes_read));
    return result;
}

ReadResult FileSystem::read_lines(const std::string& path, size_t start, size_t count) {
    ReadResult result = read_file(path);
    if (!result.success) return result;

    std::istringstream iss(result.content);
    std::string line;
    std::ostringstream oss;
    size_t line_num = 0;
    size_t lines_output = 0;

    while (std::getline(iss, line)) {
        if (line_num >= start) {
            if (count == 0 || lines_output < count) {
                oss << line << "\n";
                lines_output++;
            } else {
                break;
            }
        }
        line_num++;
    }

    result.content = oss.str();
    return result;
}

ReadResult FileSystem::read_binary(const std::string& path) {
    return read_file(path);
}

// ═══════════════════════════════════════════════════════════════════════════
// SCHREIBEN
// ═══════════════════════════════════════════════════════════════════════════

WriteResult FileSystem::write_file(const std::string& path, const std::string& content) {
    WriteResult result;
    std::string norm_path = normalize_path(path);

    if (sandbox_enabled_ && !is_path_allowed(norm_path)) {
        result.error = "Path not allowed in sandbox mode";
        return result;
    }

    // Backup erstellen falls aktiviert
    if (backup_on_write_ && exists(norm_path)) {
        result.backup_path = norm_path + ".bak";
        copy(norm_path, result.backup_path);
    }

    std::ofstream file(norm_path, std::ios::binary | std::ios::trunc);
    if (!file) {
        result.error = "Could not open file for writing: " + norm_path;
        return result;
    }

    file.write(content.data(), content.size());
    if (!file) {
        result.error = "Write failed";
        return result;
    }

    result.bytes_written = content.size();
    result.success = true;

    EventBus::push("FS_WRITE", "path=" + norm_path + "|bytes=" + std::to_string(result.bytes_written));
    return result;
}

WriteResult FileSystem::append_file(const std::string& path, const std::string& content) {
    WriteResult result;
    std::string norm_path = normalize_path(path);

    if (sandbox_enabled_ && !is_path_allowed(norm_path)) {
        result.error = "Path not allowed in sandbox mode";
        return result;
    }

    std::ofstream file(norm_path, std::ios::binary | std::ios::app);
    if (!file) {
        result.error = "Could not open file for appending: " + norm_path;
        return result;
    }

    file.write(content.data(), content.size());
    result.bytes_written = content.size();
    result.success = true;

    return result;
}

WriteResult FileSystem::write_binary(const std::string& path, const std::vector<uint8_t>& data) {
    return write_file(path, std::string(data.begin(), data.end()));
}

WriteResult FileSystem::write_atomic(const std::string& path, const std::string& content) {
    std::string temp_path = path + ".tmp." + std::to_string(std::chrono::system_clock::now().time_since_epoch().count());
    WriteResult result = write_file(temp_path, content);

    if (!result.success) {
        remove(temp_path);
        return result;
    }

    // Rename temp to target
    if (!move(temp_path, path)) {
        result.success = false;
        result.error = "Failed to rename temp file to target";
        remove(temp_path);
    }

    return result;
}

// ═══════════════════════════════════════════════════════════════════════════
// VERZEICHNISOPERATIONEN
// ═══════════════════════════════════════════════════════════════════════════

std::vector<FileInfo> FileSystem::list_dir(const std::string& path) {
    std::vector<FileInfo> result;
    std::string norm_path = normalize_path(path);

#ifdef _WIN32
    WIN32_FIND_DATAA ffd;
    std::string search_path = norm_path + "\\*";
    HANDLE hFind = FindFirstFileA(search_path.c_str(), &ffd);

    if (hFind != INVALID_HANDLE_VALUE) {
        do {
            std::string name = ffd.cFileName;
            if (name == "." || name == "..") continue;

            FileInfo info;
            info.name = name;
            info.path = join(norm_path, name);
            info.type = (ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) ?
                        FileType::DIRECTORY : FileType::REGULAR;
            info.size = (static_cast<uint64_t>(ffd.nFileSizeHigh) << 32) | ffd.nFileSizeLow;
            info.hidden = (ffd.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN) != 0;
            info.readable = true;
            info.writable = !(ffd.dwFileAttributes & FILE_ATTRIBUTE_READONLY);

            result.push_back(info);
        } while (FindNextFileA(hFind, &ffd));
        FindClose(hFind);
    }
#else
    DIR* dir = opendir(norm_path.c_str());
    if (dir) {
        struct dirent* entry;
        while ((entry = readdir(dir)) != nullptr) {
            std::string name = entry->d_name;
            if (name == "." || name == "..") continue;

            FileInfo info;
            info.name = name;
            info.path = join(norm_path, name);
            info.hidden = (name[0] == '.');

            struct stat st;
            if (::stat(info.path.c_str(), &st) == 0) {
                if (S_ISDIR(st.st_mode)) info.type = FileType::DIRECTORY;
                else if (S_ISLNK(st.st_mode)) info.type = FileType::SYMLINK;
                else if (S_ISREG(st.st_mode)) info.type = FileType::REGULAR;
                else info.type = FileType::OTHER;

                info.size = st.st_size;
                info.readable = (st.st_mode & S_IRUSR) != 0;
                info.writable = (st.st_mode & S_IWUSR) != 0;
                info.executable = (st.st_mode & S_IXUSR) != 0;
            }

            result.push_back(info);
        }
        closedir(dir);
    }
#endif

    // Sortieren: Verzeichnisse zuerst, dann alphabetisch
    std::sort(result.begin(), result.end(), [](const FileInfo& a, const FileInfo& b) {
        if (a.type != b.type) return a.type < b.type;
        return a.name < b.name;
    });

    return result;
}

std::vector<FileInfo> FileSystem::list_recursive(const std::string& path,
                                                  const std::string& pattern,
                                                  size_t max_depth) {
    std::vector<FileInfo> result;
    if (max_depth == 0) return result;

    auto entries = list_dir(path);
    for (const auto& entry : entries) {
        if (match_glob(pattern, entry.name) || pattern == "*") {
            result.push_back(entry);
        }
        if (entry.is_directory()) {
            auto sub = list_recursive(entry.path, pattern, max_depth - 1);
            result.insert(result.end(), sub.begin(), sub.end());
        }
    }

    return result;
}

bool FileSystem::create_dir(const std::string& path) {
    std::string norm_path = normalize_path(path);
#ifdef _WIN32
    return CreateDirectoryA(norm_path.c_str(), nullptr) || GetLastError() == ERROR_ALREADY_EXISTS;
#else
    return mkdir(norm_path.c_str(), 0755) == 0 || errno == EEXIST;
#endif
}

bool FileSystem::create_dirs(const std::string& path) {
    std::string norm_path = normalize_path(path);
    std::string current;

    for (size_t i = 0; i < norm_path.size(); ++i) {
        current += norm_path[i];
#ifdef _WIN32
        if (norm_path[i] == '\\' || i == norm_path.size() - 1) {
#else
        if (norm_path[i] == '/' || i == norm_path.size() - 1) {
#endif
            if (!current.empty() && !exists(current)) {
                if (!create_dir(current)) return false;
            }
        }
    }
    return true;
}

// ═══════════════════════════════════════════════════════════════════════════
// PRÜFUNGEN
// ═══════════════════════════════════════════════════════════════════════════

bool FileSystem::exists(const std::string& path) {
    std::string norm_path = normalize_path(path);
#ifdef _WIN32
    DWORD attr = GetFileAttributesA(norm_path.c_str());
    return attr != INVALID_FILE_ATTRIBUTES;
#else
    struct stat st;
    return ::stat(norm_path.c_str(), &st) == 0;
#endif
}

bool FileSystem::is_file(const std::string& path) {
    auto info = stat(path);
    return info && info->is_file();
}

bool FileSystem::is_directory(const std::string& path) {
    auto info = stat(path);
    return info && info->is_directory();
}

std::optional<FileInfo> FileSystem::stat(const std::string& path) {
    std::string norm_path = normalize_path(path);
    FileInfo info;
    info.path = norm_path;
    info.name = basename(norm_path);

#ifdef _WIN32
    WIN32_FILE_ATTRIBUTE_DATA data;
    if (!GetFileAttributesExA(norm_path.c_str(), GetFileExInfoStandard, &data)) {
        return std::nullopt;
    }
    info.type = (data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) ?
                FileType::DIRECTORY : FileType::REGULAR;
    info.size = (static_cast<uint64_t>(data.nFileSizeHigh) << 32) | data.nFileSizeLow;
    info.hidden = (data.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN) != 0;
    info.readable = true;
    info.writable = !(data.dwFileAttributes & FILE_ATTRIBUTE_READONLY);
#else
    struct ::stat st;
    if (::stat(norm_path.c_str(), &st) != 0) {
        return std::nullopt;
    }
    if (S_ISDIR(st.st_mode)) info.type = FileType::DIRECTORY;
    else if (S_ISLNK(st.st_mode)) info.type = FileType::SYMLINK;
    else if (S_ISREG(st.st_mode)) info.type = FileType::REGULAR;
    else info.type = FileType::OTHER;

    info.size = st.st_size;
    info.readable = (st.st_mode & S_IRUSR) != 0;
    info.writable = (st.st_mode & S_IWUSR) != 0;
    info.executable = (st.st_mode & S_IXUSR) != 0;
    info.hidden = (info.name[0] == '.');
#endif

    return info;
}

// ═══════════════════════════════════════════════════════════════════════════
// MANIPULATION
// ═══════════════════════════════════════════════════════════════════════════

bool FileSystem::copy(const std::string& src, const std::string& dst) {
    auto content = read_file(src);
    if (!content.success) return false;
    auto result = write_file(dst, content.content);
    return result.success;
}

bool FileSystem::move(const std::string& src, const std::string& dst) {
    std::string norm_src = normalize_path(src);
    std::string norm_dst = normalize_path(dst);
    return std::rename(norm_src.c_str(), norm_dst.c_str()) == 0;
}

bool FileSystem::remove(const std::string& path) {
    std::string norm_path = normalize_path(path);
    return std::remove(norm_path.c_str()) == 0;
}

bool FileSystem::remove_recursive(const std::string& path) {
    if (!exists(path)) return true;

    if (is_directory(path)) {
        auto entries = list_dir(path);
        for (const auto& entry : entries) {
            if (!remove_recursive(entry.path)) return false;
        }
#ifdef _WIN32
        return RemoveDirectoryA(path.c_str()) != 0;
#else
        return rmdir(path.c_str()) == 0;
#endif
    }
    return remove(path);
}

// ═══════════════════════════════════════════════════════════════════════════
// PFADOPERATIONEN
// ═══════════════════════════════════════════════════════════════════════════

std::string FileSystem::absolute(const std::string& path) {
    std::string norm_path = normalize_path(path);
#ifdef _WIN32
    char buffer[MAX_PATH];
    if (GetFullPathNameA(norm_path.c_str(), MAX_PATH, buffer, nullptr)) {
        return std::string(buffer);
    }
#else
    char buffer[PATH_MAX];
    if (realpath(norm_path.c_str(), buffer)) {
        return std::string(buffer);
    }
#endif
    return norm_path;
}

std::string FileSystem::canonical(const std::string& path) {
    return absolute(path);
}

std::string FileSystem::join(const std::string& base, const std::string& path) {
    if (base.empty()) return path;
    if (path.empty()) return base;

#ifdef _WIN32
    char sep = '\\';
#else
    char sep = '/';
#endif

    std::string result = base;
    if (result.back() != sep && result.back() != '/' && result.back() != '\\') {
        result += sep;
    }
    return result + path;
}

std::string FileSystem::basename(const std::string& path) {
    size_t pos = path.find_last_of("/\\");
    if (pos == std::string::npos) return path;
    return path.substr(pos + 1);
}

std::string FileSystem::dirname(const std::string& path) {
    size_t pos = path.find_last_of("/\\");
    if (pos == std::string::npos) return ".";
    if (pos == 0) return "/";
    return path.substr(0, pos);
}

std::string FileSystem::extension(const std::string& path) {
    std::string name = basename(path);
    size_t pos = name.rfind('.');
    if (pos == std::string::npos || pos == 0) return "";
    return name.substr(pos);
}

std::string FileSystem::with_extension(const std::string& path, const std::string& ext) {
    std::string current_ext = extension(path);
    if (current_ext.empty()) {
        return path + (ext[0] == '.' ? ext : "." + ext);
    }
    return path.substr(0, path.size() - current_ext.size()) +
           (ext[0] == '.' ? ext : "." + ext);
}

std::string FileSystem::cwd() {
    char buffer[4096];
    if (getcwd(buffer, sizeof(buffer))) {
        return std::string(buffer);
    }
    return ".";
}

bool FileSystem::chdir(const std::string& path) {
    return ::chdir(normalize_path(path).c_str()) == 0;
}

// ═══════════════════════════════════════════════════════════════════════════
// PATTERN MATCHING
// ═══════════════════════════════════════════════════════════════════════════

std::vector<std::string> FileSystem::glob(const std::string& pattern) {
    std::vector<std::string> result;
    std::string dir = dirname(pattern);
    std::string file_pattern = basename(pattern);

    if (dir == ".") dir = cwd();

    auto entries = list_dir(dir);
    for (const auto& entry : entries) {
        if (match_glob(file_pattern, entry.name)) {
            result.push_back(entry.path);
        }
    }
    return result;
}

std::vector<std::string> FileSystem::find_files(const std::string& dir,
                                                  const std::string& name_pattern,
                                                  size_t max_depth) {
    std::vector<std::string> result;
    auto entries = list_recursive(dir, "*", max_depth);

    for (const auto& entry : entries) {
        if (entry.is_file() && match_glob(name_pattern, entry.name)) {
            result.push_back(entry.path);
        }
    }
    return result;
}

std::vector<std::string> FileSystem::grep_files(const std::string& dir,
                                                  const std::string& content_pattern,
                                                  const std::string& file_pattern) {
    std::vector<std::string> result;
    auto files = find_files(dir, file_pattern, 10);

    std::regex re(content_pattern);
    for (const auto& file : files) {
        auto content = read_file(file);
        if (content.success && std::regex_search(content.content, re)) {
            result.push_back(file);
        }
    }
    return result;
}

// ═══════════════════════════════════════════════════════════════════════════
// SANDBOX
// ═══════════════════════════════════════════════════════════════════════════

void FileSystem::set_sandbox_roots(const std::vector<std::string>& roots) {
    sandbox_roots_.clear();
    for (const auto& root : roots) {
        sandbox_roots_.push_back(absolute(root));
    }
    sandbox_enabled_ = !sandbox_roots_.empty();
}

bool FileSystem::is_path_allowed(const std::string& path) const {
    if (!sandbox_enabled_) return true;

    std::string abs_path = const_cast<FileSystem*>(this)->absolute(path);
    for (const auto& root : sandbox_roots_) {
        if (abs_path.find(root) == 0) return true;
    }
    return false;
}

} // namespace rael
