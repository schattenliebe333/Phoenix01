#include "rael/module_manager.h"
#include "rael/ethics.h"
#include "rael/util.h"
#include "rael/telemetry.h"
#include "rael/metrics.h"
#include "rael/events.h"
#include <iostream>

#ifdef _WIN32
  #include <windows.h>
#else
  #include <dlfcn.h>
#endif

namespace rael {

ModuleManager::ModuleManager(){
    host.log = &ModuleManager::host_log;
    host.ethics_allows = &ModuleManager::host_ethics_allows;
    host.now_iso8601 = &ModuleManager::host_now_iso8601;
    host.telemetry_tick = &ModuleManager::host_telemetry_tick;
}

ModuleManager::~ModuleManager(){
    // Best-effort unload all
    std::vector<std::string> names = list_names();
    for(const auto& n: names){
        std::string err;
        deactivate(n, err);
        unload(n, err);
    }
}

void ModuleManager::host_log(const char* msg){
    if(msg) std::cerr << "[module] " << msg << "\n";
}

int ModuleManager::host_ethics_allows(const char* intention, const char** out_reason){
    static std::string reason;
    std::string r;
    bool ok = EthicsCore::allows(intention ? std::string(intention) : std::string(), r);
    reason = r;
    if(out_reason) *out_reason = reason.empty() ? nullptr : reason.c_str();
    return ok ? 1 : 0;
}

const char* ModuleManager::host_now_iso8601(){
    static std::string s;
    s = now_iso8601();
    return s.c_str();
}

void ModuleManager::host_telemetry_tick(){
    telemetry_tick();
}

void* ModuleManager::open_lib(const std::string& path, std::string& err){
#ifdef _WIN32
    HMODULE h = LoadLibraryA(path.c_str());
    if(!h){
        err = "LoadLibrary failed";
        return nullptr;
    }
    return (void*)h;
#else
    void* h = dlopen(path.c_str(), RTLD_NOW);
    if(!h){
        err = dlerror();
        return nullptr;
    }
    return h;
#endif
}

void ModuleManager::close_lib(void* h){
    if(!h) return;
#ifdef _WIN32
    FreeLibrary((HMODULE)h);
#else
    dlclose(h);
#endif
}

void* ModuleManager::get_sym(void* h, const char* name, std::string& err){
#ifdef _WIN32
    FARPROC p = GetProcAddress((HMODULE)h, name);
    if(!p){
        err = "GetProcAddress failed";
        return nullptr;
    }
    return (void*)p;
#else
    dlerror();
    void* p = dlsym(h, name);
    const char* e = dlerror();
    if(e){
        err = e;
        return nullptr;
    }
    return p;
#endif
}

bool ModuleManager::load(const std::string& path, std::string& err){
    void* h = open_lib(path, err);
    if(!h) return false;

    auto sym = (const RaelModuleApi*(*)()) get_sym(h, "rael_module_get_api", err);
    if(!sym){
        close_lib(h);
        return false;
    }

    const RaelModuleApi* api = sym();
    if(!api || !api->info.name || !api->info.version){
        err = "Invalid module API";
        close_lib(h);
        return false;
    }
    if(api->info.abi_version != RAEL_MODULE_ABI_VERSION){
        err = "ABI mismatch";
        close_lib(h);
        return false;
    }

    std::string name = api->info.name;
    if(mods.find(name) != mods.end()){
        err = "Module already loaded: " + name;
        close_lib(h);
        return false;
    }

    // init
    if(api->init){
        int ok = api->init(&host);
        if(!ok){
            err = "Module init failed: " + name;
            close_lib(h);
            return false;
        }
    }

    auto m = std::make_unique<LoadedModule>();
    m->path = path;
    m->handle = h;
    m->api = api;
    m->active = false;
    mods[name] = std::move(m);
    metrics_mark_module_load();
    EventBus::push("MODULE_LOAD", name + " @ " + path);
    return true;
}

bool ModuleManager::activate(const std::string& name, std::string& err){
    auto it = mods.find(name);
    if(it == mods.end()){ err="Not loaded"; return false; }
    auto& m = *it->second;
    if(m.active) return true;
    if(m.api->activate){
        if(!m.api->activate()){
            err="Activate failed";
            return false;
        }
    }
    m.active = true;
    metrics_mark_module_activation();
    EventBus::push("MODULE_ACTIVATE", name);
    return true;
}

bool ModuleManager::deactivate(const std::string& name, std::string& err){
    auto it = mods.find(name);
    if(it == mods.end()){ err="Not loaded"; return false; }
    auto& m = *it->second;
    if(!m.active) return true;
    if(m.api->deactivate){
        if(!m.api->deactivate()){
            err="Deactivate failed";
            return false;
        }
    }
    m.active = false;
    return true;
}

bool ModuleManager::unload(const std::string& name, std::string& err){
    auto it = mods.find(name);
    if(it == mods.end()){ err="Not loaded"; return false; }
    // Deactivate BEFORE removing from map (Bug fix)
    if(it->second->active){
        std::string e2;
        deactivate(name, e2);
    }
    auto m = std::move(it->second);
    mods.erase(it);

    if(m->api && m->api->shutdown){
        m->api->shutdown();
    }
    close_lib(m->handle);
    metrics_mark_module_unload();
    EventBus::push("MODULE_UNLOAD", name);
    return true;
}

std::vector<std::string> ModuleManager::list_names() const{
    std::vector<std::string> out;
    out.reserve(mods.size());
    for(const auto& kv: mods) out.push_back(kv.first);
    return out;
}

const LoadedModule* ModuleManager::get(const std::string& name) const{
    auto it = mods.find(name);
    if(it==mods.end()) return nullptr;
    return it->second.get();
}

bool ModuleManager::is_protected(const std::string& name){
    return name=="IchBinKern" || name=="EthikKern" || name=="MathCore";
}

bool ModuleManager::hotswap_semantic(const std::string& name, std::string& err){
    if(is_protected(name)){
        err = "PROTECTED_CORE";
        return false;
    }

    // name=="" disables module semantic.
    if(name.empty()){
        hotswap.set_active_semantic("");
        metrics_mark_hotswap();
        EventBus::push("HOTSWAP", "(disabled)");
        return true;
    }

    auto it = mods.find(name);
    if(it==mods.end()){
        err = "hotswap_semantic: module not loaded: " + name;
        return false;
    }
    const auto& m = *it->second;
    if(!m.active){
        err = "hotswap_semantic: module not active: " + name;
        return false;
    }
    if(!m.api || m.api->info.kind != RAEL_MOD_SEMANTIC || !m.api->process_text){
        err = "hotswap_semantic: module is not a semantic processor: " + name;
        return false;
    }

    // Validation hook: if module provides validate, call it.
    if(m.api->validate){
        const char* reason = nullptr;
        if(!m.api->validate(&reason)){
            err = std::string("hotswap_semantic: validate failed: ") + (reason ? reason : "unknown");
            return false;
        }
    }

    hotswap.set_active_semantic(name);
    metrics_mark_hotswap();
    EventBus::push("HOTSWAP", name);
    return true;
}

std::string ModuleManager::process_text_chain(const std::string& input){
    // Phase 2: dispatch to the currently active semantic module.
    const std::string active = hotswap.active_semantic();
    if(active.empty()) return input;

    auto it = mods.find(active);
    if(it==mods.end()) return input;
    const auto& m = *it->second;
    if(!m.active || !m.api || m.api->info.kind != RAEL_MOD_SEMANTIC || !m.api->process_text) return input;

    const char* out = m.api->process_text(input.c_str());
    if(out) return std::string(out);
    return input;
}

std::vector<std::string> ModuleManager::get_extra_formulas() const{
    std::vector<std::string> out;
    for(const auto& kv: mods){
        const auto& m = *kv.second;
        if(!m.active || !m.api) continue;
        if(m.api->info.kind != RAEL_MOD_MATH) continue;
        if(!m.api->get_formulas) continue;
        const char** lines = nullptr;
        size_t n = m.api->get_formulas(&lines);
        for(size_t i=0;i<n;i++){
            if(lines && lines[i]) out.push_back(lines[i]);
        }
    }
    return out;
}

} // namespace rael
