// RAEL V49 - Agent Marketplace Implementation (#28)
#include "rael/agent_marketplace.h"
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <random>

namespace rael {

// ═══════════════════════════════════════════════════════════════════════════
//  AGENT VERSION
// ═══════════════════════════════════════════════════════════════════════════

std::string AgentVersion::to_string() const {
    std::ostringstream oss;
    oss << major << "." << minor << "." << patch;
    if (!prerelease.empty()) oss << "-" << prerelease;
    return oss.str();
}

bool AgentVersion::operator<(const AgentVersion& other) const {
    if (major != other.major) return major < other.major;
    if (minor != other.minor) return minor < other.minor;
    if (patch != other.patch) return patch < other.patch;
    return prerelease < other.prerelease;
}

bool AgentVersion::operator==(const AgentVersion& other) const {
    return major == other.major && minor == other.minor &&
           patch == other.patch && prerelease == other.prerelease;
}

AgentVersion AgentVersion::parse(const std::string& str) {
    AgentVersion v;
    std::istringstream iss(str);
    char dot;
    iss >> v.major >> dot >> v.minor >> dot >> v.patch;
    if (iss.peek() == '-') {
        iss >> dot;
        std::getline(iss, v.prerelease);
    }
    return v;
}

// ═══════════════════════════════════════════════════════════════════════════
//  PACKAGE BUILDER
// ═══════════════════════════════════════════════════════════════════════════

PackageBuilder& PackageBuilder::set_metadata(const AgentMetadata& meta) {
    metadata_ = meta;
    return *this;
}

PackageBuilder& PackageBuilder::add_file(const std::string& path,
                                          const std::vector<uint8_t>& content) {
    files_[path] = content;
    return *this;
}

PackageBuilder& PackageBuilder::add_dependency(const std::string& name,
                                                const std::string& version) {
    metadata_.dependencies[name] = version;
    return *this;
}

PackageBuilder& PackageBuilder::sign(const std::string& private_key) {
    (void)private_key;
    signature_ = "sig_" + metadata_.id;
    return *this;
}

AgentPackage PackageBuilder::build() {
    AgentPackage pkg;
    pkg.metadata = metadata_;
    pkg.signature = signature_;

    // Combine files into binary
    for (const auto& [path, content] : files_) {
        pkg.binary.insert(pkg.binary.end(), content.begin(), content.end());
    }

    // Compute checksum
    std::hash<std::string> hasher;
    pkg.checksum = std::to_string(hasher(std::string(pkg.binary.begin(), pkg.binary.end())));

    return pkg;
}

// ═══════════════════════════════════════════════════════════════════════════
//  AGENT REGISTRY
// ═══════════════════════════════════════════════════════════════════════════

AgentRegistry::AgentRegistry(const RegistryConfig& config) : config_(config) {}

std::vector<AgentMetadata> AgentRegistry::search(const std::string& query, int limit) {
    std::lock_guard<std::mutex> lock(mutex_);
    (void)query;
    (void)limit;
    // Simulated search results
    return {};
}

std::vector<AgentMetadata> AgentRegistry::list_by_tag(const std::string& tag) {
    std::lock_guard<std::mutex> lock(mutex_);
    (void)tag;
    return {};
}

std::vector<AgentMetadata> AgentRegistry::list_popular(int limit) {
    std::lock_guard<std::mutex> lock(mutex_);
    (void)limit;
    return {};
}

std::vector<AgentMetadata> AgentRegistry::list_recent(int limit) {
    std::lock_guard<std::mutex> lock(mutex_);
    (void)limit;
    return {};
}

std::optional<AgentMetadata> AgentRegistry::get_metadata(const std::string& agent_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    (void)agent_id;
    return std::nullopt;
}

std::vector<AgentVersion> AgentRegistry::get_versions(const std::string& agent_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    (void)agent_id;
    return {};
}

std::optional<AgentPackage> AgentRegistry::download(const std::string& agent_id,
                                                     const std::string& version) {
    std::lock_guard<std::mutex> lock(mutex_);
    (void)agent_id;
    (void)version;
    return std::nullopt;
}

bool AgentRegistry::verify_package(const AgentPackage& pkg) {
    return !pkg.signature.empty() && !pkg.checksum.empty();
}

bool AgentRegistry::publish(const AgentPackage& pkg) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!authenticated_) return false;
    (void)pkg;
    return true;
}

bool AgentRegistry::unpublish(const std::string& agent_id, const std::string& version) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!authenticated_) return false;
    (void)agent_id;
    (void)version;
    return true;
}

bool AgentRegistry::rate(const std::string& agent_id, int stars, const std::string& review) {
    std::lock_guard<std::mutex> lock(mutex_);
    (void)agent_id;
    (void)stars;
    (void)review;
    return true;
}

std::vector<std::pair<int, std::string>> AgentRegistry::get_reviews(const std::string& agent_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    (void)agent_id;
    return {};
}

bool AgentRegistry::authenticate(const std::string& api_key) {
    std::lock_guard<std::mutex> lock(mutex_);
    authenticated_ = !api_key.empty();
    return authenticated_;
}

bool AgentRegistry::is_authenticated() const {
    return authenticated_;
}

// ═══════════════════════════════════════════════════════════════════════════
//  LOCAL REPOSITORY
// ═══════════════════════════════════════════════════════════════════════════

LocalRepository::LocalRepository(const std::string& path) : repo_path_(path) {}

bool LocalRepository::install(const AgentPackage& pkg) {
    std::lock_guard<std::mutex> lock(mutex_);
    installed_[pkg.metadata.id] = pkg.metadata;
    return true;
}

bool LocalRepository::uninstall(const std::string& agent_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    return installed_.erase(agent_id) > 0;
}

bool LocalRepository::update(const std::string& agent_id, const AgentPackage& pkg) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (installed_.find(agent_id) == installed_.end()) return false;
    installed_[agent_id] = pkg.metadata;
    return true;
}

std::vector<AgentMetadata> LocalRepository::list_installed() {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<AgentMetadata> result;
    for (const auto& [_, meta] : installed_) {
        result.push_back(meta);
    }
    return result;
}

std::optional<AgentMetadata> LocalRepository::get_installed(const std::string& agent_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = installed_.find(agent_id);
    if (it != installed_.end()) return it->second;
    return std::nullopt;
}

bool LocalRepository::is_installed(const std::string& agent_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    return installed_.find(agent_id) != installed_.end();
}

std::string LocalRepository::get_install_path(const std::string& agent_id) {
    return repo_path_ + "/" + agent_id;
}

std::vector<std::string> LocalRepository::resolve_dependencies(const AgentMetadata& meta) {
    std::vector<std::string> deps;
    for (const auto& [name, _] : meta.dependencies) {
        deps.push_back(name);
    }
    return deps;
}

bool LocalRepository::check_dependencies(const AgentMetadata& meta) {
    for (const auto& [name, _] : meta.dependencies) {
        if (!is_installed(name)) return false;
    }
    return true;
}

void LocalRepository::cleanup_cache() {}

size_t LocalRepository::cache_size() const {
    return 0;
}

// ═══════════════════════════════════════════════════════════════════════════
//  AGENT LOADER
// ═══════════════════════════════════════════════════════════════════════════

AgentLoader::AgentLoader(LocalRepository& repo) : repo_(repo) {}

bool AgentLoader::load(const std::string& agent_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (loaded_handles_.find(agent_id) != loaded_handles_.end()) return true;

    // Simulate loading
    loaded_handles_[agent_id] = nullptr;

    for (const auto& cb : load_callbacks_) {
        cb(agent_id, true);
    }
    return true;
}

bool AgentLoader::unload(const std::string& agent_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (loaded_handles_.erase(agent_id) > 0) {
        for (const auto& cb : unload_callbacks_) {
            cb(agent_id, true);
        }
        return true;
    }
    return false;
}

bool AgentLoader::reload(const std::string& agent_id) {
    unload(agent_id);
    return load(agent_id);
}

std::vector<std::string> AgentLoader::list_loaded() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<std::string> result;
    for (const auto& [id, _] : loaded_handles_) {
        result.push_back(id);
    }
    return result;
}

bool AgentLoader::is_loaded(const std::string& agent_id) const {
    std::lock_guard<std::mutex> lock(mutex_);
    return loaded_handles_.find(agent_id) != loaded_handles_.end();
}

void AgentLoader::on_load(LoadCallback cb) {
    std::lock_guard<std::mutex> lock(mutex_);
    load_callbacks_.push_back(cb);
}

void AgentLoader::on_unload(LoadCallback cb) {
    std::lock_guard<std::mutex> lock(mutex_);
    unload_callbacks_.push_back(cb);
}

// ═══════════════════════════════════════════════════════════════════════════
//  AGENT MARKETPLACE
// ═══════════════════════════════════════════════════════════════════════════

AgentMarketplace::AgentMarketplace(const MarketplaceConfig& config)
    : config_(config)
    , registry_(std::make_unique<AgentRegistry>(config.registry))
    , local_repo_(std::make_unique<LocalRepository>(config.local_repo_path))
    , loader_(std::make_unique<AgentLoader>(*local_repo_)) {}

AgentMarketplace::~AgentMarketplace() = default;

AgentMarketplace& AgentMarketplace::instance() {
    static AgentMarketplace inst;
    return inst;
}

bool AgentMarketplace::install(const std::string& agent_id, const std::string& version) {
    auto pkg = registry_->download(agent_id, version);
    if (!pkg) return false;

    if (config_.registry.verify_signatures && !registry_->verify_package(*pkg)) {
        return false;
    }

    // Resolve and install dependencies
    auto deps = local_repo_->resolve_dependencies(pkg->metadata);
    for (const auto& dep : deps) {
        if (!local_repo_->is_installed(dep)) {
            install(dep);
        }
    }

    return local_repo_->install(*pkg);
}

bool AgentMarketplace::uninstall(const std::string& agent_id) {
    loader_->unload(agent_id);
    return local_repo_->uninstall(agent_id);
}

bool AgentMarketplace::update(const std::string& agent_id) {
    auto installed = local_repo_->get_installed(agent_id);
    if (!installed) return false;

    auto versions = registry_->get_versions(agent_id);
    if (versions.empty()) return false;

    auto latest = versions.back();
    if (!(installed->version < latest)) return false;

    return install(agent_id, latest.to_string());
}

bool AgentMarketplace::update_all() {
    bool all_success = true;
    for (const auto& meta : local_repo_->list_installed()) {
        if (!update(meta.id)) {
            all_success = false;
        }
    }
    return all_success;
}

std::vector<AgentMetadata> AgentMarketplace::search(const std::string& query) {
    return registry_->search(query);
}

std::vector<AgentMetadata> AgentMarketplace::browse(const std::string& category) {
    if (category.empty()) {
        return registry_->list_popular();
    }
    return registry_->list_by_tag(category);
}

std::optional<AgentMetadata> AgentMarketplace::info(const std::string& agent_id) {
    auto installed = local_repo_->get_installed(agent_id);
    if (installed) return installed;
    return registry_->get_metadata(agent_id);
}

std::vector<AgentMetadata> AgentMarketplace::list_installed() {
    return local_repo_->list_installed();
}

std::vector<std::pair<std::string, AgentVersion>> AgentMarketplace::check_updates() {
    std::vector<std::pair<std::string, AgentVersion>> updates;
    for (const auto& meta : local_repo_->list_installed()) {
        auto versions = registry_->get_versions(meta.id);
        if (!versions.empty() && meta.version < versions.back()) {
            updates.push_back({meta.id, versions.back()});
        }
    }
    return updates;
}

AgentRegistry& AgentMarketplace::registry() { return *registry_; }
LocalRepository& AgentMarketplace::local_repo() { return *local_repo_; }
AgentLoader& AgentMarketplace::loader() { return *loader_; }

void AgentMarketplace::on_progress(ProgressCallback cb) {
    std::lock_guard<std::mutex> lock(mutex_);
    progress_callbacks_.push_back(cb);
}

AgentMarketplace& marketplace() {
    return AgentMarketplace::instance();
}

} // namespace rael
