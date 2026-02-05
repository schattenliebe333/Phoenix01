#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <memory>

#include "rael/module_api.h"
#include "rael/hotswap.h"

namespace rael {

// ============================================================================
// SECURITY: Module Trust Configuration
// ============================================================================
struct ModuleTrustEntry {
    std::string name;
    std::string filename;
    std::string sha256_hash;  // Lowercase hex
};

struct ModuleTrustConfig {
    std::string modules_dir;                          // Canonical base directory
    std::vector<ModuleTrustEntry> trusted_modules;    // Manifest entries
    bool require_hash_validation = true;              // SECURITY: Default ON
    bool allow_unsigned_in_dev = false;               // SECURITY: Default OFF
};

struct LoadedModule {
    std::string path;
    std::string verified_hash;                        // SECURITY: Hash at load time
    void* handle = nullptr;                           // dlopen/LoadLibrary
    const RaelModuleApi* api = nullptr;               // module API
    bool active = false;
};

class ModuleManager {
public:
    ModuleManager();
    ~ModuleManager();

    // SECURITY: Configure trust chain before loading modules
    void set_trust_config(const ModuleTrustConfig& config);
    bool load_manifest(const std::string& manifest_path, std::string& err);
    const ModuleTrustConfig& get_trust_config() const { return trust_config_; }

    bool load(const std::string& path, std::string& err);
    bool activate(const std::string& name, std::string& err);
    bool deactivate(const std::string& name, std::string& err);
    bool unload(const std::string& name, std::string& err);

    std::vector<std::string> list_names() const;
    const LoadedModule* get(const std::string& name) const;

    // Phase 2: Hot-swap active semantic module by name (must be loaded+active).
    bool hotswap_semantic(const std::string& name, std::string& err);
    static bool is_protected(const std::string& name);

    // Dispatch: if semantic module active, let it transform input before host SemanticEngine.
    std::string process_text_chain(const std::string& input);
    std::string active_semantic_name() const { return hotswap.active_semantic(); }

    // Aggregate formulas from active math modules.
    std::vector<std::string> get_extra_formulas() const;

private:
    RaelHostApi host{};
    std::unordered_map<std::string, std::unique_ptr<LoadedModule>> mods;
    ModuleTrustConfig trust_config_;

    HotSwapManager hotswap;

    // platform ops
    void* open_lib(const std::string& path, std::string& err);
    void  close_lib(void* h);
    void* get_sym(void* h, const char* name, std::string& err);

    // SECURITY: Trust chain validation
    bool validate_path(const std::string& path, std::string& canonical_path, std::string& err);
    bool validate_hash(const std::string& path, const std::string& expected_hash, std::string& actual_hash, std::string& err);
    const ModuleTrustEntry* find_trusted_entry(const std::string& filename);
    std::string compute_file_sha256(const std::string& path);

    static void host_log(const char* msg);
    static int  host_ethics_allows(const char* intention, const char** out_reason);
    static const char* host_now_iso8601();
    static void host_telemetry_tick();
};

}
