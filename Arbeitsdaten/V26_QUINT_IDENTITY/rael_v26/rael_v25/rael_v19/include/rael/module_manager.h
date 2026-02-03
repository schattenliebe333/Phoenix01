#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>

#include "rael/module_api.h"
#include "rael/hotswap.h"

namespace rael {

struct LoadedModule {
    std::string path;
    void* handle = nullptr;              // dlopen/LoadLibrary
    const RaelModuleApi* api = nullptr;  // module API
    bool active = false;
};

class ModuleManager {
public:
    ModuleManager();
    ~ModuleManager();

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

    HotSwapManager hotswap;

    // platform ops
    void* open_lib(const std::string& path, std::string& err);
    void  close_lib(void* h);
    void* get_sym(void* h, const char* name, std::string& err);

    static void host_log(const char* msg);
    static int  host_ethics_allows(const char* intention, const char** out_reason);
    static const char* host_now_iso8601();
    static void host_telemetry_tick();
};

}
