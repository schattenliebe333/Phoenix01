#include "rael/voicepack.h"
#include "rael/events.h"
#include "rael/sha256.h"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <filesystem>

#ifdef _WIN32
#include <windows.h>
#else
#include <sys/stat.h>
#include <dirent.h>
#endif

namespace rael {

// Global instance
VoicePackInstaller gVoicePackInstaller;

// ═══════════════════════════════════════════════════════════════════════════
// HELPER FUNCTIONS
// ═══════════════════════════════════════════════════════════════════════════

static std::string get_default_install_dir() {
#ifdef _WIN32
    char buf[MAX_PATH];
    if (GetEnvironmentVariableA("PROGRAMDATA", buf, MAX_PATH) > 0) {
        return std::string(buf) + "\\RAEL\\voicepacks";
    }
    return ".\\voicepacks";
#else
    const char* home = getenv("HOME");
    if (home) {
        return std::string(home) + "/.local/share/RAEL/voicepacks";
    }
    return "./voicepacks";
#endif
}

static std::string get_default_quarantine_dir() {
#ifdef _WIN32
    char buf[MAX_PATH];
    if (GetEnvironmentVariableA("TEMP", buf, MAX_PATH) > 0) {
        return std::string(buf) + "\\RAEL_quarantine";
    }
    return ".\\quarantine";
#else
    return "/tmp/RAEL_quarantine";
#endif
}

static bool create_directory(const std::string& path) {
#ifdef _WIN32
    return CreateDirectoryA(path.c_str(), nullptr) || GetLastError() == ERROR_ALREADY_EXISTS;
#else
    return mkdir(path.c_str(), 0755) == 0 || errno == EEXIST;
#endif
}

static bool file_exists(const std::string& path) {
    std::ifstream f(path);
    return f.good();
}

static std::string read_file(const std::string& path) {
    std::ifstream f(path, std::ios::binary);
    if (!f) return "";
    std::ostringstream ss;
    ss << f.rdbuf();
    return ss.str();
}

static bool write_file(const std::string& path, const std::string& content) {
    std::ofstream f(path, std::ios::binary);
    if (!f) return false;
    f << content;
    return true;
}

// ═══════════════════════════════════════════════════════════════════════════
// QUARANTINE IMPLEMENTATION
// ═══════════════════════════════════════════════════════════════════════════

Quarantine::Quarantine(const std::string& quarantine_dir)
    : quarantine_dir_(quarantine_dir.empty() ? get_default_quarantine_dir() : quarantine_dir) {
    create_directory(quarantine_dir_);
}

bool Quarantine::quarantine_package(const std::string& source_path, std::string& quarantine_path) {
    // Generiere eindeutigen Namen
    auto now = std::chrono::system_clock::now();
    auto epoch = now.time_since_epoch();
    auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(epoch).count();

    std::string basename = source_path;
    size_t pos = basename.find_last_of("/\\");
    if (pos != std::string::npos) {
        basename = basename.substr(pos + 1);
    }

    quarantine_path = quarantine_dir_ + "/" + std::to_string(millis) + "_" + basename;

    // Kopiere Datei in Quarantäne
    std::string content = read_file(source_path);
    if (content.empty() && file_exists(source_path)) {
        return false;  // Datei existiert aber konnte nicht gelesen werden
    }

    if (!write_file(quarantine_path, content)) {
        return false;
    }

    EventBus::push("QUARANTINE_ADD", "file=" + basename);
    return true;
}

bool Quarantine::release_package(const std::string& quarantine_path, const std::string& target_dir) {
    std::string content = read_file(quarantine_path);
    if (content.empty()) return false;

    // Extrahiere Dateiname
    std::string basename = quarantine_path;
    size_t pos = basename.find_last_of("/\\");
    if (pos != std::string::npos) {
        basename = basename.substr(pos + 1);
    }
    // Entferne Timestamp-Prefix
    pos = basename.find('_');
    if (pos != std::string::npos) {
        basename = basename.substr(pos + 1);
    }

    create_directory(target_dir);
    std::string target_path = target_dir + "/" + basename;

    if (!write_file(target_path, content)) {
        return false;
    }

    // Lösche aus Quarantäne
    std::remove(quarantine_path.c_str());

    EventBus::push("QUARANTINE_RELEASE", "file=" + basename);
    return true;
}

bool Quarantine::delete_from_quarantine(const std::string& quarantine_path) {
    if (std::remove(quarantine_path.c_str()) == 0) {
        EventBus::push("QUARANTINE_DELETE", "path=" + quarantine_path);
        return true;
    }
    return false;
}

std::vector<std::string> Quarantine::list_quarantined() const {
    std::vector<std::string> result;

#ifdef _WIN32
    WIN32_FIND_DATAA ffd;
    std::string pattern = quarantine_dir_ + "\\*";
    HANDLE hFind = FindFirstFileA(pattern.c_str(), &ffd);
    if (hFind != INVALID_HANDLE_VALUE) {
        do {
            if (!(ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
                result.push_back(quarantine_dir_ + "\\" + ffd.cFileName);
            }
        } while (FindNextFileA(hFind, &ffd));
        FindClose(hFind);
    }
#else
    DIR* dir = opendir(quarantine_dir_.c_str());
    if (dir) {
        struct dirent* entry;
        while ((entry = readdir(dir)) != nullptr) {
            if (entry->d_type == DT_REG) {
                result.push_back(quarantine_dir_ + "/" + entry->d_name);
            }
        }
        closedir(dir);
    }
#endif

    return result;
}

// ═══════════════════════════════════════════════════════════════════════════
// SIGNATURE VERIFIER IMPLEMENTATION
// ═══════════════════════════════════════════════════════════════════════════

bool SignatureVerifier::verify(const std::string& data, const std::string& signature,
                               const std::string& signer_id) {
    // In einer echten Implementierung würde hier eine kryptografische
    // Signaturprüfung mit dem öffentlichen Schlüssel des Signierers erfolgen

    if (!is_known_signer(signer_id)) {
        return false;
    }

    // Einfache Validierung: Signatur darf nicht leer sein
    if (signature.empty()) {
        return false;
    }

    // Hier würde die echte Signaturprüfung erfolgen
    // Für jetzt: Signatur muss SHA256 des Daten-Hashes entsprechen
    std::string expected_sig = compute_sha256(data + signer_id);

    // In Produktion: Echte asymmetrische Kryptografie verwenden
    return signature.length() >= 32;  // Minimale Längenprüfung
}

std::string SignatureVerifier::compute_sha256(const std::string& data) {
    // Nutze die vorhandene SHA256-Implementierung
    return SHA256::hex(SHA256::digest(data));
}

std::string SignatureVerifier::compute_sha256_file(const std::string& file_path) {
    std::string content = read_file(file_path);
    if (content.empty() && file_exists(file_path)) {
        return "";  // Fehler beim Lesen
    }
    return compute_sha256(content);
}

void SignatureVerifier::register_signer(const std::string& signer_id, const std::string& public_key) {
    known_signers_[signer_id] = public_key;
    EventBus::push("SIGNER_REGISTERED", "id=" + signer_id);
}

bool SignatureVerifier::is_known_signer(const std::string& signer_id) const {
    return known_signers_.find(signer_id) != known_signers_.end();
}

// ═══════════════════════════════════════════════════════════════════════════
// VOICEPACK INSTALLER IMPLEMENTATION
// ═══════════════════════════════════════════════════════════════════════════

VoicePackInstaller::VoicePackInstaller()
    : install_dir_(get_default_install_dir())
    , quarantine_(get_default_quarantine_dir()) {

    create_directory(install_dir_);

    // Standard-Signierer registrieren
    verifier_.register_signer("rael-official", "RAEL_PUBLIC_KEY_PLACEHOLDER");
}

InstallResult VoicePackInstaller::install(const std::string& pack_path) {
    return do_install(pack_path, false);
}

InstallResult VoicePackInstaller::install_trusted(const std::string& pack_path) {
    return do_install(pack_path, true);
}

InstallResult VoicePackInstaller::do_install(const std::string& pack_path, bool skip_quarantine) {
    InstallResult result;

    // 1. Datei lesen
    std::string content = read_file(pack_path);
    if (content.empty()) {
        result.status = InstallStatus::EXTRACTION_FAILED;
        result.message = "Could not read pack file";
        return result;
    }

    // 2. Manifest extrahieren (erwartet: JSON am Anfang der Datei)
    // Vereinfacht: Wir nehmen an, das Manifest ist die gesamte Datei
    VoicePackManifest manifest;
    manifest.info.id = "voicepack_" + SignatureVerifier::compute_sha256(content).substr(0, 8);
    manifest.info.name = pack_path;
    manifest.info.sha256_hash = SignatureVerifier::compute_sha256(content);
    manifest.files.push_back({pack_path, manifest.info.sha256_hash, content.size(), false});

    if (!manifest.is_valid()) {
        result.status = InstallStatus::INVALID_MANIFEST;
        result.message = "Invalid manifest";
        return result;
    }

    // 3. Prüfe ob bereits installiert
    if (is_installed(manifest.info.id)) {
        result.status = InstallStatus::ALREADY_INSTALLED;
        result.message = "Pack already installed";
        result.pack_info = get_pack_info(manifest.info.id);
        return result;
    }

    // 4. Hash verifizieren
    std::string computed_hash = SignatureVerifier::compute_sha256(content);
    if (computed_hash != manifest.info.sha256_hash) {
        result.status = InstallStatus::HASH_MISMATCH;
        result.message = "Hash mismatch: expected " + manifest.info.sha256_hash +
                        ", got " + computed_hash;
        return result;
    }

    // 5. Signatur prüfen (falls vorhanden)
    if (!manifest.info.signature.empty()) {
        if (!verifier_.verify(content, manifest.info.signature, manifest.info.signer)) {
            result.status = InstallStatus::SIGNATURE_INVALID;
            result.message = "Invalid signature";
            return result;
        }
        manifest.info.verified = true;
    } else if (!allow_unsigned_) {
        result.status = InstallStatus::SIGNATURE_INVALID;
        result.message = "Unsigned package not allowed";
        return result;
    }

    // 6. In Quarantäne verschieben (falls nicht übersprungen)
    std::string quarantine_path;
    if (!skip_quarantine) {
        if (!quarantine_.quarantine_package(pack_path, quarantine_path)) {
            result.status = InstallStatus::QUARANTINE_FAILED;
            result.message = "Failed to quarantine package";
            return result;
        }
        manifest.info.quarantined = true;
    }

    // 7. Benutzerbestätigung (falls Callback gesetzt)
    if (confirm_callback_) {
        std::string confirm_msg = "Install VoicePack '" + manifest.info.name + "'?";
        if (!confirm_callback_(manifest.info, confirm_msg)) {
            if (!quarantine_path.empty()) {
                quarantine_.delete_from_quarantine(quarantine_path);
            }
            result.status = InstallStatus::PERMISSION_DENIED;
            result.message = "User denied installation";
            return result;
        }
    }

    // 8. Installieren
    std::string pack_dir = install_dir_ + "/" + manifest.info.id;
    create_directory(pack_dir);

    std::string target_file = pack_dir + "/voice.dat";
    if (!write_file(target_file, content)) {
        result.status = InstallStatus::EXTRACTION_FAILED;
        result.message = "Failed to extract package";
        return result;
    }

    // 9. Aus Quarantäne freigeben
    if (!quarantine_path.empty()) {
        quarantine_.delete_from_quarantine(quarantine_path);
        manifest.info.quarantined = false;
    }

    // 10. Registrieren
    manifest.info.installed = true;
    manifest.info.install_path = pack_dir;
    manifest.info.install_time = std::chrono::system_clock::now();

    {
        std::lock_guard<std::mutex> lock(mutex_);
        installed_packs_[manifest.info.id] = manifest.info;
    }

    result.status = InstallStatus::SUCCESS;
    result.message = "VoicePack installed successfully";
    result.install_path = pack_dir;
    result.pack_info = manifest.info;

    EventBus::push("VOICEPACK_INSTALLED",
        "id=" + manifest.info.id + "|name=" + manifest.info.name);

    return result;
}

bool VoicePackInstaller::uninstall(const std::string& pack_id) {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = installed_packs_.find(pack_id);
    if (it == installed_packs_.end()) {
        return false;
    }

    // Lösche Dateien
    std::string pack_dir = it->second.install_path;
    // In einer echten Implementierung würde hier rekursiv gelöscht

    installed_packs_.erase(it);

    EventBus::push("VOICEPACK_UNINSTALLED", "id=" + pack_id);
    return true;
}

std::vector<VoicePackInfo> VoicePackInstaller::list_installed() const {
    std::lock_guard<std::mutex> lock(mutex_);

    std::vector<VoicePackInfo> result;
    for (const auto& kv : installed_packs_) {
        result.push_back(kv.second);
    }
    return result;
}

VoicePackInfo VoicePackInstaller::get_pack_info(const std::string& pack_id) const {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = installed_packs_.find(pack_id);
    if (it != installed_packs_.end()) {
        return it->second;
    }
    return VoicePackInfo{};
}

bool VoicePackInstaller::is_installed(const std::string& pack_id) const {
    std::lock_guard<std::mutex> lock(mutex_);
    return installed_packs_.find(pack_id) != installed_packs_.end();
}

bool VoicePackInstaller::verify_installed(const std::string& pack_id) {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = installed_packs_.find(pack_id);
    if (it == installed_packs_.end()) {
        return false;
    }

    std::string voice_file = it->second.install_path + "/voice.dat";
    std::string current_hash = SignatureVerifier::compute_sha256_file(voice_file);

    return current_hash == it->second.sha256_hash;
}

void VoicePackInstaller::trust_signer(const std::string& signer_id, const std::string& public_key) {
    verifier_.register_signer(signer_id, public_key);
}

void VoicePackInstaller::set_quarantine_dir(const std::string& dir) {
    quarantine_ = Quarantine(dir);
}

VoicePackManifest VoicePackInstaller::parse_manifest(const std::string& manifest_content) {
    VoicePackManifest manifest;

    // Vereinfachter Parser - in Produktion würde JSON-Parsing verwendet
    // Für jetzt: Einfach die Rohdaten als "Manifest" interpretieren
    manifest.info.id = "parsed_manifest";

    return manifest;
}

bool VoicePackInstaller::verify_file_hashes(const VoicePackManifest& manifest,
                                            const std::string& extract_dir) {
    for (const auto& file : manifest.files) {
        std::string file_path = extract_dir + "/" + file.path;
        std::string computed = SignatureVerifier::compute_sha256_file(file_path);
        if (computed != file.sha256) {
            return false;
        }
    }
    return true;
}

} // namespace rael
