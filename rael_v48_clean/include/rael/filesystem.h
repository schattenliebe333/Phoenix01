#pragma once
// ═══════════════════════════════════════════════════════════════════════════
// RAEL FILESYSTEM API (Verbesserung #11)
// ═══════════════════════════════════════════════════════════════════════════
// Grundlage für alle Dateioperationen: lesen, schreiben, navigieren
// ═══════════════════════════════════════════════════════════════════════════

#include <string>
#include <vector>
#include <functional>
#include <chrono>
#include <cstdint>
#include <optional>

namespace rael {

// ═══════════════════════════════════════════════════════════════════════════
// FILE INFO - Metadaten einer Datei/eines Verzeichnisses
// ═══════════════════════════════════════════════════════════════════════════

enum class FileType : uint8_t {
    REGULAR    = 0,   // Normale Datei
    DIRECTORY  = 1,   // Verzeichnis
    SYMLINK    = 2,   // Symbolischer Link
    OTHER      = 3    // Andere (Pipe, Socket, etc.)
};

struct FileInfo {
    std::string path;
    std::string name;
    FileType type = FileType::REGULAR;
    uint64_t size = 0;
    std::chrono::system_clock::time_point modified;
    std::chrono::system_clock::time_point created;
    bool readable = false;
    bool writable = false;
    bool executable = false;
    bool hidden = false;

    bool is_file() const { return type == FileType::REGULAR; }
    bool is_directory() const { return type == FileType::DIRECTORY; }
    bool is_symlink() const { return type == FileType::SYMLINK; }
};

// ═══════════════════════════════════════════════════════════════════════════
// READ RESULT - Ergebnis einer Leseoperation
// ═══════════════════════════════════════════════════════════════════════════

struct ReadResult {
    bool success = false;
    std::string content;
    std::string error;
    uint64_t bytes_read = 0;

    operator bool() const { return success; }
};

// ═══════════════════════════════════════════════════════════════════════════
// WRITE RESULT - Ergebnis einer Schreiboperation
// ═══════════════════════════════════════════════════════════════════════════

struct WriteResult {
    bool success = false;
    std::string error;
    uint64_t bytes_written = 0;
    std::string backup_path;  // Falls Backup erstellt wurde

    operator bool() const { return success; }
};

// ═══════════════════════════════════════════════════════════════════════════
// FILESYSTEM CLASS - Hauptklasse für Dateioperationen
// ═══════════════════════════════════════════════════════════════════════════

class FileSystem {
public:
    FileSystem();

    // ─────────────────────────────────────────────────────────────────────────
    // Lesen
    // ─────────────────────────────────────────────────────────────────────────

    // Datei komplett lesen
    ReadResult read_file(const std::string& path);

    // Datei zeilenweise lesen
    ReadResult read_lines(const std::string& path, size_t start = 0, size_t count = 0);

    // Binärdatei lesen
    ReadResult read_binary(const std::string& path);

    // ─────────────────────────────────────────────────────────────────────────
    // Schreiben
    // ─────────────────────────────────────────────────────────────────────────

    // Datei schreiben (überschreiben)
    WriteResult write_file(const std::string& path, const std::string& content);

    // An Datei anhängen
    WriteResult append_file(const std::string& path, const std::string& content);

    // Binärdatei schreiben
    WriteResult write_binary(const std::string& path, const std::vector<uint8_t>& data);

    // Atomares Schreiben (erst temp, dann rename)
    WriteResult write_atomic(const std::string& path, const std::string& content);

    // ─────────────────────────────────────────────────────────────────────────
    // Verzeichnisoperationen
    // ─────────────────────────────────────────────────────────────────────────

    // Verzeichnisinhalt auflisten
    std::vector<FileInfo> list_dir(const std::string& path);

    // Rekursiv auflisten
    std::vector<FileInfo> list_recursive(const std::string& path,
                                          const std::string& pattern = "*",
                                          size_t max_depth = 10);

    // Verzeichnis erstellen
    bool create_dir(const std::string& path);

    // Verzeichnis rekursiv erstellen
    bool create_dirs(const std::string& path);

    // ─────────────────────────────────────────────────────────────────────────
    // Prüfungen
    // ─────────────────────────────────────────────────────────────────────────

    // Existiert Pfad?
    bool exists(const std::string& path);

    // Ist Datei?
    bool is_file(const std::string& path);

    // Ist Verzeichnis?
    bool is_directory(const std::string& path);

    // Dateiinfo abrufen
    std::optional<FileInfo> stat(const std::string& path);

    // ─────────────────────────────────────────────────────────────────────────
    // Manipulation
    // ─────────────────────────────────────────────────────────────────────────

    // Kopieren
    bool copy(const std::string& src, const std::string& dst);

    // Verschieben/Umbenennen
    bool move(const std::string& src, const std::string& dst);

    // Löschen
    bool remove(const std::string& path);

    // Rekursiv löschen
    bool remove_recursive(const std::string& path);

    // ─────────────────────────────────────────────────────────────────────────
    // Pfadoperationen
    // ─────────────────────────────────────────────────────────────────────────

    // Absoluter Pfad
    std::string absolute(const std::string& path);

    // Kanonischer Pfad (aufgelöste Symlinks)
    std::string canonical(const std::string& path);

    // Pfade zusammenfügen
    std::string join(const std::string& base, const std::string& path);

    // Dateiname extrahieren
    std::string basename(const std::string& path);

    // Verzeichnis extrahieren
    std::string dirname(const std::string& path);

    // Erweiterung extrahieren
    std::string extension(const std::string& path);

    // Erweiterung ändern
    std::string with_extension(const std::string& path, const std::string& ext);

    // Aktuelles Arbeitsverzeichnis
    std::string cwd();

    // Arbeitsverzeichnis ändern
    bool chdir(const std::string& path);

    // ─────────────────────────────────────────────────────────────────────────
    // Pattern Matching
    // ─────────────────────────────────────────────────────────────────────────

    // Glob-Pattern matching (*.cpp, src/**/*.h)
    std::vector<std::string> glob(const std::string& pattern);

    // Dateisuche nach Name
    std::vector<std::string> find_files(const std::string& dir,
                                         const std::string& name_pattern,
                                         size_t max_depth = 10);

    // Dateisuche nach Inhalt
    std::vector<std::string> grep_files(const std::string& dir,
                                         const std::string& content_pattern,
                                         const std::string& file_pattern = "*");

    // ─────────────────────────────────────────────────────────────────────────
    // Konfiguration
    // ─────────────────────────────────────────────────────────────────────────

    // Backup vor Überschreiben aktivieren
    void set_backup_on_write(bool enabled) { backup_on_write_ = enabled; }

    // Maximale Dateigröße zum Lesen
    void set_max_read_size(uint64_t bytes) { max_read_size_ = bytes; }

    // Sandbox-Modus (nur bestimmte Verzeichnisse erlaubt)
    void set_sandbox_roots(const std::vector<std::string>& roots);
    bool is_path_allowed(const std::string& path) const;

private:
    bool match_glob(const std::string& pattern, const std::string& text);
    std::string normalize_path(const std::string& path);

    bool backup_on_write_ = false;
    uint64_t max_read_size_ = 100 * 1024 * 1024;  // 100 MB default
    std::vector<std::string> sandbox_roots_;
    bool sandbox_enabled_ = false;
};

// Global instance
extern FileSystem gFileSystem;

} // namespace rael
