#pragma once
// ═══════════════════════════════════════════════════════════════════════════
// RAEL VOICEPACK INSTALLER (Verbesserung #7)
// ═══════════════════════════════════════════════════════════════════════════
// Natural-Voice offline nachinstallieren, aber supply-chain sicher
// (Quarantäne, Signatur, SHA256)
// ═══════════════════════════════════════════════════════════════════════════

#include <string>
#include <vector>
#include <unordered_map>
#include <functional>
#include <chrono>
#include <mutex>
#include <cstdint>

namespace rael {

// ═══════════════════════════════════════════════════════════════════════════
// VOICEPACK METADATA
// ═══════════════════════════════════════════════════════════════════════════

struct VoicePackInfo {
    std::string id;                // Eindeutige ID
    std::string name;              // Anzeigename
    std::string version;           // Version (semver)
    std::string language;          // Sprache (z.B. "de-DE", "en-US")
    std::string description;
    std::string author;
    std::string license;

    // Technische Details
    std::string format;            // z.B. "sapi", "espeak", "neural"
    uint64_t size_bytes = 0;
    std::string sample_rate;       // z.B. "22050Hz"

    // Sicherheit
    std::string sha256_hash;       // SHA256 des Pakets
    std::string signature;         // Digitale Signatur
    std::string signer;            // Wer hat signiert
    bool verified = false;         // Signatur verifiziert?

    // Status
    bool installed = false;
    bool quarantined = false;
    std::string install_path;
    std::chrono::system_clock::time_point install_time;
};

// ═══════════════════════════════════════════════════════════════════════════
// MANIFEST - Beschreibt ein VoicePack
// ═══════════════════════════════════════════════════════════════════════════

struct VoicePackManifest {
    std::string manifest_version = "1.0";
    VoicePackInfo info;

    // Dateien im Paket
    struct FileEntry {
        std::string path;
        std::string sha256;
        uint64_t size = 0;
        bool executable = false;
    };
    std::vector<FileEntry> files;

    // Abhängigkeiten
    std::vector<std::string> dependencies;

    // Validierung
    bool is_valid() const {
        return !info.id.empty() &&
               !info.name.empty() &&
               !info.sha256_hash.empty() &&
               !files.empty();
    }
};

// ═══════════════════════════════════════════════════════════════════════════
// INSTALL RESULT
// ═══════════════════════════════════════════════════════════════════════════

enum class InstallStatus : uint8_t {
    SUCCESS            = 0,
    INVALID_MANIFEST   = 1,
    HASH_MISMATCH      = 2,
    SIGNATURE_INVALID  = 3,
    QUARANTINE_FAILED  = 4,
    EXTRACTION_FAILED  = 5,
    DEPENDENCY_MISSING = 6,
    ALREADY_INSTALLED  = 7,
    PERMISSION_DENIED  = 8,
    UNKNOWN_ERROR      = 99
};

inline const char* install_status_name(InstallStatus s) {
    switch (s) {
        case InstallStatus::SUCCESS:            return "SUCCESS";
        case InstallStatus::INVALID_MANIFEST:   return "INVALID_MANIFEST";
        case InstallStatus::HASH_MISMATCH:      return "HASH_MISMATCH";
        case InstallStatus::SIGNATURE_INVALID:  return "SIGNATURE_INVALID";
        case InstallStatus::QUARANTINE_FAILED:  return "QUARANTINE_FAILED";
        case InstallStatus::EXTRACTION_FAILED:  return "EXTRACTION_FAILED";
        case InstallStatus::DEPENDENCY_MISSING: return "DEPENDENCY_MISSING";
        case InstallStatus::ALREADY_INSTALLED:  return "ALREADY_INSTALLED";
        case InstallStatus::PERMISSION_DENIED:  return "PERMISSION_DENIED";
        default:                                return "UNKNOWN_ERROR";
    }
}

struct InstallResult {
    InstallStatus status = InstallStatus::UNKNOWN_ERROR;
    std::string message;
    std::string install_path;
    VoicePackInfo pack_info;

    bool success() const { return status == InstallStatus::SUCCESS; }
};

// ═══════════════════════════════════════════════════════════════════════════
// QUARANTINE - Isolierte Umgebung für unbekannte Pakete
// ═══════════════════════════════════════════════════════════════════════════

class Quarantine {
public:
    explicit Quarantine(const std::string& quarantine_dir = "");

    // Paket in Quarantäne verschieben
    bool quarantine_package(const std::string& source_path, std::string& quarantine_path);

    // Paket aus Quarantäne freigeben
    bool release_package(const std::string& quarantine_path, const std::string& target_dir);

    // Paket aus Quarantäne löschen
    bool delete_from_quarantine(const std::string& quarantine_path);

    // Liste aller Pakete in Quarantäne
    std::vector<std::string> list_quarantined() const;

    // Quarantäne-Verzeichnis
    const std::string& directory() const { return quarantine_dir_; }

private:
    std::string quarantine_dir_;
};

// ═══════════════════════════════════════════════════════════════════════════
// SIGNATURE VERIFIER
// ═══════════════════════════════════════════════════════════════════════════

class SignatureVerifier {
public:
    // Verifiziere Signatur
    bool verify(const std::string& data, const std::string& signature,
                const std::string& signer_id);

    // Berechne SHA256
    static std::string compute_sha256(const std::string& data);
    static std::string compute_sha256_file(const std::string& file_path);

    // Bekannte Signierer registrieren
    void register_signer(const std::string& signer_id, const std::string& public_key);

    // Prüfe ob Signierer bekannt
    bool is_known_signer(const std::string& signer_id) const;

private:
    std::unordered_map<std::string, std::string> known_signers_;
};

// ═══════════════════════════════════════════════════════════════════════════
// VOICEPACK INSTALLER
// ═══════════════════════════════════════════════════════════════════════════

class VoicePackInstaller {
public:
    VoicePackInstaller();

    // ─────────────────────────────────────────────────────────────────────────
    // Installation
    // ─────────────────────────────────────────────────────────────────────────

    // Installiere VoicePack aus Datei
    InstallResult install(const std::string& pack_path);

    // Installiere mit Quarantäne-Umgehung (nur für bekannte Signierer)
    InstallResult install_trusted(const std::string& pack_path);

    // Deinstalliere VoicePack
    bool uninstall(const std::string& pack_id);

    // ─────────────────────────────────────────────────────────────────────────
    // Verwaltung
    // ─────────────────────────────────────────────────────────────────────────

    // Liste installierter Packs
    std::vector<VoicePackInfo> list_installed() const;

    // Pack-Info abrufen
    VoicePackInfo get_pack_info(const std::string& pack_id) const;

    // Prüfe ob Pack installiert
    bool is_installed(const std::string& pack_id) const;

    // ─────────────────────────────────────────────────────────────────────────
    // Sicherheit
    // ─────────────────────────────────────────────────────────────────────────

    // Verifiziere installiertes Pack
    bool verify_installed(const std::string& pack_id);

    // Registriere vertrauenswürdigen Signierer
    void trust_signer(const std::string& signer_id, const std::string& public_key);

    // Quarantäne-Zugriff
    Quarantine& quarantine() { return quarantine_; }
    const Quarantine& quarantine() const { return quarantine_; }

    // ─────────────────────────────────────────────────────────────────────────
    // Konfiguration
    // ─────────────────────────────────────────────────────────────────────────

    // Setze Installations-Verzeichnis
    void set_install_dir(const std::string& dir) { install_dir_ = dir; }

    // Setze Quarantäne-Verzeichnis
    void set_quarantine_dir(const std::string& dir);

    // Erlaube/Verbiete unsignierte Pakete
    void set_allow_unsigned(bool allow) { allow_unsigned_ = allow; }

    // Setze Callback für Bestätigungen
    using ConfirmCallback = std::function<bool(const VoicePackInfo&, const std::string&)>;
    void set_confirm_callback(ConfirmCallback cb) { confirm_callback_ = cb; }

private:
    // Interner Installationsablauf
    InstallResult do_install(const std::string& pack_path, bool skip_quarantine);

    // Manifest parsen
    VoicePackManifest parse_manifest(const std::string& manifest_content);

    // Hash verifizieren
    bool verify_file_hashes(const VoicePackManifest& manifest, const std::string& extract_dir);

    std::string install_dir_;
    Quarantine quarantine_;
    SignatureVerifier verifier_;

    std::unordered_map<std::string, VoicePackInfo> installed_packs_;

    bool allow_unsigned_ = false;
    ConfirmCallback confirm_callback_;

    mutable std::mutex mutex_;
};

// Global instance
extern VoicePackInstaller gVoicePackInstaller;

} // namespace rael
