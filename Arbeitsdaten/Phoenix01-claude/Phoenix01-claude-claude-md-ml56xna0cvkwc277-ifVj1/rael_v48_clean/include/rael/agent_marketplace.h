// RAEL V49 - Agent Marketplace (#28)
// Discover, share, and deploy agents
#pragma once

#include <string>
#include <vector>
#include <map>
#include <memory>
#include <functional>
#include <optional>
#include <mutex>
#include <chrono>

namespace rael {

// ═══════════════════════════════════════════════════════════════════════════
//  AGENT METADATA
// ═══════════════════════════════════════════════════════════════════════════

struct AgentCapability {
    std::string name;
    std::string description;
    std::vector<std::string> required_inputs;
    std::vector<std::string> outputs;
};

struct AgentVersion {
    int major = 1;
    int minor = 0;
    int patch = 0;
    std::string prerelease;

    std::string to_string() const;
    bool operator<(const AgentVersion& other) const;
    bool operator==(const AgentVersion& other) const;
    static AgentVersion parse(const std::string& str);
};

struct AgentMetadata {
    std::string id;
    std::string name;
    std::string description;
    std::string author;
    std::string license;
    AgentVersion version;
    std::vector<std::string> tags;
    std::vector<AgentCapability> capabilities;
    std::map<std::string, std::string> dependencies;
    std::string repository_url;
    std::string documentation_url;
    std::chrono::system_clock::time_point created;
    std::chrono::system_clock::time_point updated;
    size_t download_count = 0;
    double rating = 0;
    int rating_count = 0;
};

// ═══════════════════════════════════════════════════════════════════════════
//  AGENT PACKAGE
// ═══════════════════════════════════════════════════════════════════════════

struct AgentPackage {
    AgentMetadata metadata;
    std::vector<uint8_t> binary;
    std::string manifest_json;
    std::string signature;
    std::string checksum;
    bool verified = false;
};

class PackageBuilder {
public:
    PackageBuilder& set_metadata(const AgentMetadata& meta);
    PackageBuilder& add_file(const std::string& path, const std::vector<uint8_t>& content);
    PackageBuilder& add_dependency(const std::string& name, const std::string& version);
    PackageBuilder& sign(const std::string& private_key);
    AgentPackage build();

private:
    AgentMetadata metadata_;
    std::map<std::string, std::vector<uint8_t>> files_;
    std::string signature_;
};

// ═══════════════════════════════════════════════════════════════════════════
//  REGISTRY
// ═══════════════════════════════════════════════════════════════════════════

struct RegistryConfig {
    std::string url = "https://registry.rael.ai";
    std::string api_key;
    std::string cache_dir = "~/.rael/cache";
    std::chrono::seconds timeout{30};
    bool verify_signatures = true;
};

class AgentRegistry {
public:
    explicit AgentRegistry(const RegistryConfig& config = {});

    // Search and browse
    std::vector<AgentMetadata> search(const std::string& query, int limit = 20);
    std::vector<AgentMetadata> list_by_tag(const std::string& tag);
    std::vector<AgentMetadata> list_popular(int limit = 10);
    std::vector<AgentMetadata> list_recent(int limit = 10);
    std::optional<AgentMetadata> get_metadata(const std::string& agent_id);
    std::vector<AgentVersion> get_versions(const std::string& agent_id);

    // Download
    std::optional<AgentPackage> download(const std::string& agent_id,
                                          const std::string& version = "latest");
    bool verify_package(const AgentPackage& pkg);

    // Publish
    bool publish(const AgentPackage& pkg);
    bool unpublish(const std::string& agent_id, const std::string& version);

    // Ratings
    bool rate(const std::string& agent_id, int stars, const std::string& review = "");
    std::vector<std::pair<int, std::string>> get_reviews(const std::string& agent_id);

    // Authentication
    bool authenticate(const std::string& api_key);
    bool is_authenticated() const;

private:
    RegistryConfig config_;
    bool authenticated_ = false;
    mutable std::mutex mutex_;
};

// ═══════════════════════════════════════════════════════════════════════════
//  LOCAL REPOSITORY
// ═══════════════════════════════════════════════════════════════════════════

class LocalRepository {
public:
    explicit LocalRepository(const std::string& path = "~/.rael/agents");

    // Install/Uninstall
    bool install(const AgentPackage& pkg);
    bool uninstall(const std::string& agent_id);
    bool update(const std::string& agent_id, const AgentPackage& pkg);

    // Query
    std::vector<AgentMetadata> list_installed();
    std::optional<AgentMetadata> get_installed(const std::string& agent_id);
    bool is_installed(const std::string& agent_id);
    std::string get_install_path(const std::string& agent_id);

    // Dependencies
    std::vector<std::string> resolve_dependencies(const AgentMetadata& meta);
    bool check_dependencies(const AgentMetadata& meta);

    // Cleanup
    void cleanup_cache();
    size_t cache_size() const;

private:
    std::string repo_path_;
    std::map<std::string, AgentMetadata> installed_;
    mutable std::mutex mutex_;
};

// ═══════════════════════════════════════════════════════════════════════════
//  AGENT LOADER
// ═══════════════════════════════════════════════════════════════════════════

class AgentLoader {
public:
    AgentLoader(LocalRepository& repo);

    // Load agents
    bool load(const std::string& agent_id);
    bool unload(const std::string& agent_id);
    bool reload(const std::string& agent_id);

    // Query loaded
    std::vector<std::string> list_loaded() const;
    bool is_loaded(const std::string& agent_id) const;

    // Lifecycle hooks
    using LoadCallback = std::function<void(const std::string&, bool)>;
    void on_load(LoadCallback cb);
    void on_unload(LoadCallback cb);

private:
    LocalRepository& repo_;
    std::map<std::string, void*> loaded_handles_;
    std::vector<LoadCallback> load_callbacks_;
    std::vector<LoadCallback> unload_callbacks_;
    mutable std::mutex mutex_;
};

// ═══════════════════════════════════════════════════════════════════════════
//  MARKETPLACE
// ═══════════════════════════════════════════════════════════════════════════

struct MarketplaceConfig {
    RegistryConfig registry;
    std::string local_repo_path = "~/.rael/agents";
    bool auto_update = false;
    std::chrono::hours update_interval{24};
};

class AgentMarketplace {
public:
    explicit AgentMarketplace(const MarketplaceConfig& config = {});
    ~AgentMarketplace();

    static AgentMarketplace& instance();

    // High-level operations
    bool install(const std::string& agent_id, const std::string& version = "latest");
    bool uninstall(const std::string& agent_id);
    bool update(const std::string& agent_id);
    bool update_all();

    // Search
    std::vector<AgentMetadata> search(const std::string& query);
    std::vector<AgentMetadata> browse(const std::string& category = "");

    // Info
    std::optional<AgentMetadata> info(const std::string& agent_id);
    std::vector<AgentMetadata> list_installed();
    std::vector<std::pair<std::string, AgentVersion>> check_updates();

    // Components
    AgentRegistry& registry();
    LocalRepository& local_repo();
    AgentLoader& loader();

    // Events
    using ProgressCallback = std::function<void(const std::string&, double)>;
    void on_progress(ProgressCallback cb);

private:
    MarketplaceConfig config_;
    std::unique_ptr<AgentRegistry> registry_;
    std::unique_ptr<LocalRepository> local_repo_;
    std::unique_ptr<AgentLoader> loader_;
    std::vector<ProgressCallback> progress_callbacks_;
    mutable std::mutex mutex_;
};

AgentMarketplace& marketplace();

} // namespace rael
