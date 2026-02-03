// RAEL V49 - Plugin SDK (#36) & Hot Reload (#37)
// Extensible plugin architecture
#pragma once

#include <string>
#include <vector>
#include <map>
#include <memory>
#include <functional>
#include <optional>
#include <mutex>
#include <chrono>
#include <atomic>
#include <thread>

namespace rael {

// ═══════════════════════════════════════════════════════════════════════════
//  PLUGIN INTERFACE
// ═══════════════════════════════════════════════════════════════════════════

struct PluginInfo {
    std::string id;
    std::string name;
    std::string version;
    std::string author;
    std::string description;
    std::vector<std::string> dependencies;
    std::map<std::string, std::string> metadata;
};

class IPlugin {
public:
    virtual ~IPlugin() = default;

    virtual PluginInfo info() const = 0;
    virtual bool initialize() = 0;
    virtual void shutdown() = 0;
    virtual bool is_initialized() const = 0;

    // Optional lifecycle
    virtual void on_enable() {}
    virtual void on_disable() {}
    virtual void on_config_change(const std::map<std::string, std::string>& config) {
        (void)config;
    }
};

// Plugin factory macro
#define RAEL_PLUGIN(ClassName) \
    extern "C" rael::IPlugin* create_plugin() { return new ClassName(); } \
    extern "C" void destroy_plugin(rael::IPlugin* p) { delete p; }

// ═══════════════════════════════════════════════════════════════════════════
//  EXTENSION POINTS
// ═══════════════════════════════════════════════════════════════════════════

template<typename T>
class ExtensionPoint {
public:
    using ExtensionFactory = std::function<std::unique_ptr<T>()>;

    void register_extension(const std::string& name, ExtensionFactory factory) {
        std::lock_guard<std::mutex> lock(mutex_);
        factories_[name] = factory;
    }

    void unregister_extension(const std::string& name) {
        std::lock_guard<std::mutex> lock(mutex_);
        factories_.erase(name);
    }

    std::unique_ptr<T> create(const std::string& name) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = factories_.find(name);
        if (it != factories_.end()) {
            return it->second();
        }
        return nullptr;
    }

    std::vector<std::string> list() const {
        std::lock_guard<std::mutex> lock(mutex_);
        std::vector<std::string> names;
        for (const auto& [name, _] : factories_) {
            names.push_back(name);
        }
        return names;
    }

private:
    std::map<std::string, ExtensionFactory> factories_;
    mutable std::mutex mutex_;
};

// ═══════════════════════════════════════════════════════════════════════════
//  HOOK SYSTEM
// ═══════════════════════════════════════════════════════════════════════════

enum class HookPriority {
    LOWEST = 0,
    LOW = 25,
    NORMAL = 50,
    HIGH = 75,
    HIGHEST = 100,
    MONITOR = 1000  // Read-only, always last
};

template<typename... Args>
class Hook {
public:
    using Handler = std::function<bool(Args...)>;

    struct Registration {
        std::string id;
        Handler handler;
        HookPriority priority;
        std::string plugin_id;
    };

    std::string add(Handler handler, HookPriority priority = HookPriority::NORMAL,
                    const std::string& plugin_id = "") {
        std::lock_guard<std::mutex> lock(mutex_);
        Registration reg;
        reg.id = "hook_" + std::to_string(next_id_++);
        reg.handler = handler;
        reg.priority = priority;
        reg.plugin_id = plugin_id;
        handlers_.push_back(reg);

        // Sort by priority (higher first)
        std::sort(handlers_.begin(), handlers_.end(),
                  [](const Registration& a, const Registration& b) {
                      return static_cast<int>(a.priority) > static_cast<int>(b.priority);
                  });

        return reg.id;
    }

    void remove(const std::string& id) {
        std::lock_guard<std::mutex> lock(mutex_);
        handlers_.erase(
            std::remove_if(handlers_.begin(), handlers_.end(),
                          [&id](const Registration& r) { return r.id == id; }),
            handlers_.end());
    }

    void remove_by_plugin(const std::string& plugin_id) {
        std::lock_guard<std::mutex> lock(mutex_);
        handlers_.erase(
            std::remove_if(handlers_.begin(), handlers_.end(),
                          [&plugin_id](const Registration& r) {
                              return r.plugin_id == plugin_id;
                          }),
            handlers_.end());
    }

    bool call(Args... args) {
        std::lock_guard<std::mutex> lock(mutex_);
        for (const auto& reg : handlers_) {
            if (!reg.handler(args...)) {
                return false;  // Handler cancelled
            }
        }
        return true;
    }

private:
    std::vector<Registration> handlers_;
    uint64_t next_id_ = 1;
    mutable std::mutex mutex_;
};

// ═══════════════════════════════════════════════════════════════════════════
//  SERVICE REGISTRY
// ═══════════════════════════════════════════════════════════════════════════

class ServiceRegistry {
public:
    template<typename T>
    void register_service(const std::string& name, std::shared_ptr<T> service) {
        std::lock_guard<std::mutex> lock(mutex_);
        services_[name] = service;
    }

    template<typename T>
    std::shared_ptr<T> get_service(const std::string& name) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = services_.find(name);
        if (it != services_.end()) {
            return std::static_pointer_cast<T>(it->second);
        }
        return nullptr;
    }

    void unregister_service(const std::string& name) {
        std::lock_guard<std::mutex> lock(mutex_);
        services_.erase(name);
    }

    std::vector<std::string> list_services() const {
        std::lock_guard<std::mutex> lock(mutex_);
        std::vector<std::string> names;
        for (const auto& [name, _] : services_) {
            names.push_back(name);
        }
        return names;
    }

private:
    std::map<std::string, std::shared_ptr<void>> services_;
    mutable std::mutex mutex_;
};

// ═══════════════════════════════════════════════════════════════════════════
//  HOT RELOAD (#37)
// ═══════════════════════════════════════════════════════════════════════════

struct FileChangeEvent {
    enum class Type { CREATED, MODIFIED, DELETED };
    Type type;
    std::string path;
    std::chrono::system_clock::time_point timestamp;
};

class FileWatcher {
public:
    FileWatcher();
    ~FileWatcher();

    void watch(const std::string& path, bool recursive = true);
    void unwatch(const std::string& path);

    using Callback = std::function<void(const FileChangeEvent&)>;
    void on_change(Callback cb);

    void start();
    void stop();
    bool is_running() const;

private:
    void watch_loop();

    std::vector<std::string> watched_paths_;
    std::vector<Callback> callbacks_;
    std::map<std::string, std::chrono::system_clock::time_point> file_times_;
    std::atomic<bool> running_{false};
    std::thread watch_thread_;
    mutable std::mutex mutex_;
};

class HotReloader {
public:
    HotReloader();
    ~HotReloader();

    // Configure
    void set_plugin_directory(const std::string& dir);
    void set_config_directory(const std::string& dir);

    // Enable/Disable
    void enable();
    void disable();
    bool is_enabled() const;

    // Manual reload
    bool reload_plugin(const std::string& plugin_id);
    bool reload_config(const std::string& config_path);
    void reload_all();

    // Callbacks
    using ReloadCallback = std::function<void(const std::string&, bool)>;
    void on_reload(ReloadCallback cb);

    // State preservation
    void save_state(const std::string& plugin_id);
    void restore_state(const std::string& plugin_id);

private:
    std::unique_ptr<FileWatcher> watcher_;
    std::string plugin_dir_;
    std::string config_dir_;
    std::vector<ReloadCallback> callbacks_;
    std::map<std::string, std::map<std::string, std::string>> saved_states_;
    std::atomic<bool> enabled_{false};
    mutable std::mutex mutex_;
};

// ═══════════════════════════════════════════════════════════════════════════
//  PLUGIN MANAGER
// ═══════════════════════════════════════════════════════════════════════════

struct PluginConfig {
    std::string plugin_directory = "./plugins";
    bool auto_load = true;
    bool hot_reload = true;
    std::vector<std::string> disabled_plugins;
};

class PluginManager {
public:
    explicit PluginManager(const PluginConfig& config = {});
    ~PluginManager();

    static PluginManager& instance();

    // Plugin lifecycle
    bool load(const std::string& path);
    bool unload(const std::string& plugin_id);
    bool reload(const std::string& plugin_id);
    void load_all();
    void unload_all();

    // Plugin access
    IPlugin* get_plugin(const std::string& id);
    std::vector<PluginInfo> list_plugins() const;
    bool is_loaded(const std::string& id) const;
    bool is_enabled(const std::string& id) const;

    // Enable/Disable
    bool enable(const std::string& id);
    bool disable(const std::string& id);

    // Configuration
    void set_plugin_config(const std::string& id, const std::map<std::string, std::string>& config);
    std::map<std::string, std::string> get_plugin_config(const std::string& id) const;

    // Dependencies
    std::vector<std::string> resolve_dependencies(const std::string& id);
    std::vector<std::string> get_dependents(const std::string& id);

    // Services
    ServiceRegistry& services();

    // Hot reload
    HotReloader& hot_reloader();

    // Events
    Hook<const std::string&, bool>& on_load();
    Hook<const std::string&>& on_unload();
    Hook<const std::string&>& on_enable();
    Hook<const std::string&>& on_disable();

private:
    struct LoadedPlugin {
        void* handle;
        std::unique_ptr<IPlugin> instance;
        bool enabled;
        std::map<std::string, std::string> config;
    };

    PluginConfig config_;
    std::map<std::string, LoadedPlugin> plugins_;
    std::unique_ptr<ServiceRegistry> services_;
    std::unique_ptr<HotReloader> hot_reloader_;

    Hook<const std::string&, bool> on_load_;
    Hook<const std::string&> on_unload_;
    Hook<const std::string&> on_enable_;
    Hook<const std::string&> on_disable_;

    mutable std::mutex mutex_;
};

// ═══════════════════════════════════════════════════════════════════════════
//  PLUGIN SDK HELPERS
// ═══════════════════════════════════════════════════════════════════════════

class PluginContext {
public:
    PluginContext(const std::string& plugin_id, PluginManager& manager);

    // Logging
    void log_info(const std::string& message);
    void log_warn(const std::string& message);
    void log_error(const std::string& message);

    // Services
    template<typename T>
    void provide_service(const std::string& name, std::shared_ptr<T> service) {
        manager_.services().register_service(name, service);
    }

    template<typename T>
    std::shared_ptr<T> require_service(const std::string& name) {
        return manager_.services().get_service<T>(name);
    }

    // Configuration
    std::map<std::string, std::string> get_config() const;
    std::string get_config(const std::string& key, const std::string& default_val = "") const;

    // Storage
    std::string get_data_directory() const;
    std::string get_config_directory() const;

private:
    std::string plugin_id_;
    PluginManager& manager_;
};

PluginManager& plugins();

} // namespace rael
