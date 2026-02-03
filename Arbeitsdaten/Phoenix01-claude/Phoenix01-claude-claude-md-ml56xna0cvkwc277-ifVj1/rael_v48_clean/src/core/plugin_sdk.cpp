// RAEL V49 - Plugin SDK Implementation (#36, #37)
#include "rael/plugin_sdk.h"
#include <algorithm>
#include <sstream>
#include <fstream>
#include <filesystem>
#include <thread>
#include <iostream>
#include <sys/stat.h>
#include <dlfcn.h>

namespace {
// Helper function for C++17 compatibility (ends_with is C++20)
bool str_ends_with(const std::string& str, const std::string& suffix) {
    if (suffix.size() > str.size()) return false;
    return str.compare(str.size() - suffix.size(), suffix.size(), suffix) == 0;
}
}

namespace fs = std::filesystem;

namespace rael {

// ═══════════════════════════════════════════════════════════════════════════
//  FILE WATCHER
// ═══════════════════════════════════════════════════════════════════════════

FileWatcher::FileWatcher() {}

FileWatcher::~FileWatcher() {
    stop();
}

void FileWatcher::watch(const std::string& path, bool recursive) {
    std::lock_guard<std::mutex> lock(mutex_);
    watched_paths_.push_back(path);
    (void)recursive;

    // Record initial file times
    if (fs::exists(path)) {
        if (fs::is_directory(path)) {
            for (const auto& entry : fs::recursive_directory_iterator(path)) {
                if (entry.is_regular_file()) {
                    auto time = fs::last_write_time(entry);
                    file_times_[entry.path().string()] =
                        std::chrono::system_clock::now();  // Simplified
                }
            }
        } else {
            file_times_[path] = std::chrono::system_clock::now();
        }
    }
}

void FileWatcher::unwatch(const std::string& path) {
    std::lock_guard<std::mutex> lock(mutex_);
    watched_paths_.erase(
        std::remove(watched_paths_.begin(), watched_paths_.end(), path),
        watched_paths_.end());
}

void FileWatcher::on_change(Callback cb) {
    std::lock_guard<std::mutex> lock(mutex_);
    callbacks_.push_back(cb);
}

void FileWatcher::start() {
    if (running_) return;
    running_ = true;
    watch_thread_ = std::thread(&FileWatcher::watch_loop, this);
}

void FileWatcher::stop() {
    running_ = false;
    if (watch_thread_.joinable()) {
        watch_thread_.join();
    }
}

bool FileWatcher::is_running() const {
    return running_;
}

void FileWatcher::watch_loop() {
    while (running_) {
        std::this_thread::sleep_for(std::chrono::seconds(1));

        std::lock_guard<std::mutex> lock(mutex_);
        for (const auto& path : watched_paths_) {
            if (!fs::exists(path)) continue;

            if (fs::is_directory(path)) {
                for (const auto& entry : fs::recursive_directory_iterator(path)) {
                    if (!entry.is_regular_file()) continue;

                    std::string file_path = entry.path().string();
                    auto current_time = std::chrono::system_clock::now();

                    auto it = file_times_.find(file_path);
                    if (it == file_times_.end()) {
                        // New file
                        file_times_[file_path] = current_time;
                        FileChangeEvent event{FileChangeEvent::Type::CREATED, file_path, current_time};
                        for (const auto& cb : callbacks_) {
                            cb(event);
                        }
                    }
                }
            }
        }
    }
}

// ═══════════════════════════════════════════════════════════════════════════
//  HOT RELOADER
// ═══════════════════════════════════════════════════════════════════════════

HotReloader::HotReloader()
    : watcher_(std::make_unique<FileWatcher>()) {}

HotReloader::~HotReloader() {
    disable();
}

void HotReloader::set_plugin_directory(const std::string& dir) {
    plugin_dir_ = dir;
}

void HotReloader::set_config_directory(const std::string& dir) {
    config_dir_ = dir;
}

void HotReloader::enable() {
    if (enabled_) return;
    enabled_ = true;

    if (!plugin_dir_.empty()) {
        watcher_->watch(plugin_dir_);
    }
    if (!config_dir_.empty()) {
        watcher_->watch(config_dir_);
    }

    watcher_->on_change([this](const FileChangeEvent& event) {
        if (str_ends_with(event.path, ".so") || str_ends_with(event.path, ".dll")) {
            // Extract plugin ID from path
            std::string plugin_id = fs::path(event.path).stem().string();
            reload_plugin(plugin_id);
        } else if (str_ends_with(event.path, ".json") || str_ends_with(event.path, ".yaml")) {
            reload_config(event.path);
        }
    });

    watcher_->start();
}

void HotReloader::disable() {
    enabled_ = false;
    watcher_->stop();
}

bool HotReloader::is_enabled() const {
    return enabled_;
}

bool HotReloader::reload_plugin(const std::string& plugin_id) {
    std::lock_guard<std::mutex> lock(mutex_);

    bool success = true;  // Would actually reload

    for (const auto& cb : callbacks_) {
        cb(plugin_id, success);
    }

    return success;
}

bool HotReloader::reload_config(const std::string& config_path) {
    std::lock_guard<std::mutex> lock(mutex_);
    (void)config_path;
    return true;
}

void HotReloader::reload_all() {
    // Reload all plugins
}

void HotReloader::on_reload(ReloadCallback cb) {
    std::lock_guard<std::mutex> lock(mutex_);
    callbacks_.push_back(cb);
}

void HotReloader::save_state(const std::string& plugin_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    // Would save plugin state
    saved_states_[plugin_id] = {};
}

void HotReloader::restore_state(const std::string& plugin_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    // Would restore plugin state
    auto it = saved_states_.find(plugin_id);
    if (it != saved_states_.end()) {
        // Restore
    }
}

// ═══════════════════════════════════════════════════════════════════════════
//  PLUGIN MANAGER
// ═══════════════════════════════════════════════════════════════════════════

PluginManager::PluginManager(const PluginConfig& config)
    : config_(config)
    , services_(std::make_unique<ServiceRegistry>())
    , hot_reloader_(std::make_unique<HotReloader>()) {

    hot_reloader_->set_plugin_directory(config.plugin_directory);

    if (config.hot_reload) {
        hot_reloader_->enable();
    }
}

PluginManager::~PluginManager() {
    unload_all();
}

PluginManager& PluginManager::instance() {
    static PluginManager inst;
    return inst;
}

bool PluginManager::load(const std::string& path) {
    std::lock_guard<std::mutex> lock(mutex_);

    // Load shared library
    void* handle = dlopen(path.c_str(), RTLD_NOW | RTLD_LOCAL);
    if (!handle) {
        return false;
    }

    // Get factory function
    using CreateFunc = IPlugin* (*)();
    auto create = reinterpret_cast<CreateFunc>(dlsym(handle, "create_plugin"));
    if (!create) {
        dlclose(handle);
        return false;
    }

    // Create plugin instance
    IPlugin* plugin = create();
    if (!plugin) {
        dlclose(handle);
        return false;
    }

    // Initialize
    if (!plugin->initialize()) {
        delete plugin;
        dlclose(handle);
        return false;
    }

    LoadedPlugin loaded;
    loaded.handle = handle;
    loaded.instance.reset(plugin);
    loaded.enabled = true;

    std::string id = plugin->info().id;
    plugins_[id] = std::move(loaded);

    on_load_.call(id, true);
    return true;
}

bool PluginManager::unload(const std::string& plugin_id) {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = plugins_.find(plugin_id);
    if (it == plugins_.end()) return false;

    on_unload_.call(plugin_id);

    it->second.instance->shutdown();

    dlclose(it->second.handle);
    plugins_.erase(it);

    return true;
}

bool PluginManager::reload(const std::string& plugin_id) {
    std::string path;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = plugins_.find(plugin_id);
        if (it == plugins_.end()) return false;
        // Would need to store the original path
        path = config_.plugin_directory + "/" + plugin_id + ".so";
    }

    unload(plugin_id);
    return load(path);
}

void PluginManager::load_all() {
    if (!fs::exists(config_.plugin_directory)) return;

    for (const auto& entry : fs::directory_iterator(config_.plugin_directory)) {
        if (entry.path().extension() == ".so" ||
            entry.path().extension() == ".dll") {
            std::string id = entry.path().stem().string();

            // Check if disabled
            bool disabled = std::find(config_.disabled_plugins.begin(),
                                      config_.disabled_plugins.end(), id)
                           != config_.disabled_plugins.end();

            if (!disabled) {
                load(entry.path().string());
            }
        }
    }
}

void PluginManager::unload_all() {
    std::vector<std::string> ids;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        for (const auto& [id, _] : plugins_) {
            ids.push_back(id);
        }
    }

    for (const auto& id : ids) {
        unload(id);
    }
}

IPlugin* PluginManager::get_plugin(const std::string& id) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = plugins_.find(id);
    if (it != plugins_.end()) {
        return it->second.instance.get();
    }
    return nullptr;
}

std::vector<PluginInfo> PluginManager::list_plugins() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<PluginInfo> result;
    for (const auto& [_, plugin] : plugins_) {
        result.push_back(plugin.instance->info());
    }
    return result;
}

bool PluginManager::is_loaded(const std::string& id) const {
    std::lock_guard<std::mutex> lock(mutex_);
    return plugins_.find(id) != plugins_.end();
}

bool PluginManager::is_enabled(const std::string& id) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = plugins_.find(id);
    if (it != plugins_.end()) {
        return it->second.enabled;
    }
    return false;
}

bool PluginManager::enable(const std::string& id) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = plugins_.find(id);
    if (it == plugins_.end()) return false;

    if (!it->second.enabled) {
        it->second.enabled = true;
        it->second.instance->on_enable();
        on_enable_.call(id);
    }
    return true;
}

bool PluginManager::disable(const std::string& id) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = plugins_.find(id);
    if (it == plugins_.end()) return false;

    if (it->second.enabled) {
        it->second.enabled = false;
        it->second.instance->on_disable();
        on_disable_.call(id);
    }
    return true;
}

void PluginManager::set_plugin_config(const std::string& id,
                                       const std::map<std::string, std::string>& config) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = plugins_.find(id);
    if (it != plugins_.end()) {
        it->second.config = config;
        it->second.instance->on_config_change(config);
    }
}

std::map<std::string, std::string> PluginManager::get_plugin_config(const std::string& id) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = plugins_.find(id);
    if (it != plugins_.end()) {
        return it->second.config;
    }
    return {};
}

std::vector<std::string> PluginManager::resolve_dependencies(const std::string& id) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = plugins_.find(id);
    if (it != plugins_.end()) {
        return it->second.instance->info().dependencies;
    }
    return {};
}

std::vector<std::string> PluginManager::get_dependents(const std::string& id) {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<std::string> dependents;

    for (const auto& [plugin_id, plugin] : plugins_) {
        auto deps = plugin.instance->info().dependencies;
        if (std::find(deps.begin(), deps.end(), id) != deps.end()) {
            dependents.push_back(plugin_id);
        }
    }

    return dependents;
}

ServiceRegistry& PluginManager::services() { return *services_; }
HotReloader& PluginManager::hot_reloader() { return *hot_reloader_; }

Hook<const std::string&, bool>& PluginManager::on_load() { return on_load_; }
Hook<const std::string&>& PluginManager::on_unload() { return on_unload_; }
Hook<const std::string&>& PluginManager::on_enable() { return on_enable_; }
Hook<const std::string&>& PluginManager::on_disable() { return on_disable_; }

// ═══════════════════════════════════════════════════════════════════════════
//  PLUGIN CONTEXT
// ═══════════════════════════════════════════════════════════════════════════

PluginContext::PluginContext(const std::string& plugin_id, PluginManager& manager)
    : plugin_id_(plugin_id), manager_(manager) {}

void PluginContext::log_info(const std::string& message) {
    std::cout << "[" << plugin_id_ << "] INFO: " << message << std::endl;
}

void PluginContext::log_warn(const std::string& message) {
    std::cout << "[" << plugin_id_ << "] WARN: " << message << std::endl;
}

void PluginContext::log_error(const std::string& message) {
    std::cerr << "[" << plugin_id_ << "] ERROR: " << message << std::endl;
}

std::map<std::string, std::string> PluginContext::get_config() const {
    return manager_.get_plugin_config(plugin_id_);
}

std::string PluginContext::get_config(const std::string& key,
                                       const std::string& default_val) const {
    auto config = get_config();
    auto it = config.find(key);
    if (it != config.end()) return it->second;
    return default_val;
}

std::string PluginContext::get_data_directory() const {
    return "~/.rael/plugins/" + plugin_id_ + "/data";
}

std::string PluginContext::get_config_directory() const {
    return "~/.rael/plugins/" + plugin_id_ + "/config";
}

PluginManager& plugins() {
    return PluginManager::instance();
}

} // namespace rael
