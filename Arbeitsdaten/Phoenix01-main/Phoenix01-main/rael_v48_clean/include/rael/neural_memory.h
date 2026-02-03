// RAEL V49 - Neural Memory Store (#24)
// Long-term memory with associations and forgetting curve
#pragma once

#include <string>
#include <vector>
#include <map>
#include <set>
#include <memory>
#include <functional>
#include <chrono>
#include <optional>
#include <mutex>

namespace rael {

// ═══════════════════════════════════════════════════════════════════════════
//  MEMORY TYPES
// ═══════════════════════════════════════════════════════════════════════════

enum class MemoryType {
    EPISODIC,       // Events, experiences
    SEMANTIC,       // Facts, concepts
    PROCEDURAL,     // How to do things
    EMOTIONAL,      // Feelings associated with memories
    WORKING         // Short-term, active processing
};

enum class MemoryState {
    ACTIVE,         // Currently being processed
    CONSOLIDATING,  // Being moved to long-term
    STORED,         // In long-term storage
    DECAYING,       // Losing strength
    ARCHIVED        // Rarely accessed but preserved
};

// ═══════════════════════════════════════════════════════════════════════════
//  MEMORY NODE
// ═══════════════════════════════════════════════════════════════════════════

struct MemoryNode {
    std::string id;
    std::string content;
    MemoryType type = MemoryType::SEMANTIC;
    MemoryState state = MemoryState::ACTIVE;

    // Embedding for similarity search
    std::vector<float> embedding;

    // Strength and decay
    double strength = 1.0;          // 0.0 to 1.0
    double importance = 0.5;        // 0.0 to 1.0
    double emotional_valence = 0.0; // -1.0 to 1.0 (negative to positive)
    double emotional_arousal = 0.0; // 0.0 to 1.0 (calm to exciting)

    // Timing
    std::chrono::system_clock::time_point created;
    std::chrono::system_clock::time_point last_accessed;
    std::chrono::system_clock::time_point last_consolidated;
    int access_count = 0;
    int rehearsal_count = 0;

    // Associations
    std::map<std::string, double> associations;  // node_id -> weight

    // Metadata
    std::map<std::string, std::string> metadata;
    std::vector<std::string> tags;
    std::string source;
    std::string context;
};

// ═══════════════════════════════════════════════════════════════════════════
//  ASSOCIATION GRAPH
// ═══════════════════════════════════════════════════════════════════════════

class AssociationGraph {
public:
    AssociationGraph();

    // Node operations
    void add_node(const std::string& id);
    void remove_node(const std::string& id);
    bool has_node(const std::string& id) const;
    std::vector<std::string> all_nodes() const;

    // Edge operations
    void add_edge(const std::string& from, const std::string& to, double weight = 1.0);
    void remove_edge(const std::string& from, const std::string& to);
    double get_edge_weight(const std::string& from, const std::string& to) const;
    void strengthen_edge(const std::string& from, const std::string& to, double delta = 0.1);
    void weaken_edge(const std::string& from, const std::string& to, double delta = 0.1);

    // Traversal
    std::vector<std::pair<std::string, double>> neighbors(const std::string& id) const;
    std::vector<std::string> path(const std::string& from, const std::string& to) const;
    double distance(const std::string& from, const std::string& to) const;

    // Analysis
    std::vector<std::string> strongest_associations(const std::string& id, int k = 10) const;
    std::vector<std::string> spreading_activation(const std::string& start,
                                                   double threshold = 0.1,
                                                   int max_depth = 3) const;
    std::vector<std::vector<std::string>> find_clusters(int min_size = 3) const;
    double clustering_coefficient(const std::string& id) const;

    // Maintenance
    void prune_weak_edges(double min_weight = 0.01);
    void decay_all_edges(double factor = 0.99);

    size_t node_count() const;
    size_t edge_count() const;

private:
    std::map<std::string, std::map<std::string, double>> adjacency_;
    mutable std::mutex mutex_;
};

// ═══════════════════════════════════════════════════════════════════════════
//  FORGETTING CURVE (Ebbinghaus)
// ═══════════════════════════════════════════════════════════════════════════

class ForgettingCurve {
public:
    ForgettingCurve(double initial_strength = 1.0, double stability = 1.0);

    // Calculate retention after time
    double retention(std::chrono::seconds elapsed) const;

    // Update after rehearsal
    void rehearse();

    // Parameters
    double stability() const { return stability_; }
    void set_stability(double s) { stability_ = s; }

    // Optimal review timing (spaced repetition)
    std::chrono::seconds next_review_interval() const;

    // Calculate strength decay
    static double decay(double initial, std::chrono::seconds elapsed, double stability);

private:
    double initial_strength_;
    double stability_;
    int rehearsal_count_ = 0;
    std::chrono::system_clock::time_point last_rehearsal_;
};

// ═══════════════════════════════════════════════════════════════════════════
//  MEMORY CONSOLIDATION
// ═══════════════════════════════════════════════════════════════════════════

class MemoryConsolidator {
public:
    MemoryConsolidator();

    // Consolidation process
    void consolidate(MemoryNode& node);
    void consolidate_batch(std::vector<MemoryNode*>& nodes);

    // Dream simulation (memory reorganization)
    void dream_cycle(std::vector<MemoryNode*>& memories,
                     AssociationGraph& graph,
                     std::chrono::seconds duration = std::chrono::minutes(10));

    // Importance scoring
    double calculate_importance(const MemoryNode& node) const;

    // Determine if memory should be kept
    bool should_keep(const MemoryNode& node) const;
    bool should_archive(const MemoryNode& node) const;

    // Configuration
    void set_importance_threshold(double t) { importance_threshold_ = t; }
    void set_decay_rate(double r) { decay_rate_ = r; }

private:
    double importance_threshold_ = 0.1;
    double decay_rate_ = 0.01;
};

// ═══════════════════════════════════════════════════════════════════════════
//  MEMORY INDEX
// ═══════════════════════════════════════════════════════════════════════════

class MemoryIndex {
public:
    MemoryIndex();

    // Indexing
    void index(const MemoryNode& node);
    void reindex(const MemoryNode& node);
    void remove(const std::string& id);

    // Search
    std::vector<std::string> search_text(const std::string& query, int limit = 10) const;
    std::vector<std::string> search_embedding(const std::vector<float>& embedding,
                                               int limit = 10) const;
    std::vector<std::string> search_tags(const std::vector<std::string>& tags) const;
    std::vector<std::string> search_type(MemoryType type) const;
    std::vector<std::string> search_time_range(
        std::chrono::system_clock::time_point start,
        std::chrono::system_clock::time_point end) const;

    // Combined search
    struct SearchQuery {
        std::string text;
        std::vector<float> embedding;
        std::vector<std::string> tags;
        std::optional<MemoryType> type;
        std::optional<std::chrono::system_clock::time_point> after;
        std::optional<std::chrono::system_clock::time_point> before;
        double min_strength = 0.0;
        int limit = 10;
    };
    std::vector<std::string> search(const SearchQuery& query) const;

    size_t size() const;

private:
    // Inverted index for text search
    std::map<std::string, std::set<std::string>> word_index_;

    // Tag index
    std::map<std::string, std::set<std::string>> tag_index_;

    // Type index
    std::map<MemoryType, std::set<std::string>> type_index_;

    // Time-sorted index
    std::multimap<std::chrono::system_clock::time_point, std::string> time_index_;

    // Embedding storage for similarity search
    std::map<std::string, std::vector<float>> embeddings_;

    mutable std::mutex mutex_;
};

// ═══════════════════════════════════════════════════════════════════════════
//  MEMORY SNAPSHOT
// ═══════════════════════════════════════════════════════════════════════════

struct MemorySnapshot {
    std::string id;
    std::string name;
    std::chrono::system_clock::time_point created;
    std::vector<MemoryNode> memories;
    std::map<std::string, std::map<std::string, double>> associations;
    std::map<std::string, std::string> metadata;
};

// ═══════════════════════════════════════════════════════════════════════════
//  NEURAL MEMORY STORE (Main Interface)
// ═══════════════════════════════════════════════════════════════════════════

class NeuralMemoryStore {
public:
    NeuralMemoryStore();
    ~NeuralMemoryStore();

    // Singleton
    static NeuralMemoryStore& instance();

    // Basic CRUD
    std::string store(const std::string& content, MemoryType type = MemoryType::SEMANTIC);
    std::string store(const MemoryNode& node);
    std::optional<MemoryNode> retrieve(const std::string& id);
    bool update(const MemoryNode& node);
    bool remove(const std::string& id);
    bool exists(const std::string& id) const;

    // Access (updates strength and timing)
    std::optional<MemoryNode> access(const std::string& id);
    void rehearse(const std::string& id);

    // Search
    std::vector<MemoryNode> search(const std::string& query, int limit = 10);
    std::vector<MemoryNode> similar(const std::string& id, int limit = 10);
    std::vector<MemoryNode> similar(const std::vector<float>& embedding, int limit = 10);
    std::vector<MemoryNode> by_type(MemoryType type, int limit = 100);
    std::vector<MemoryNode> recent(int limit = 10);
    std::vector<MemoryNode> strongest(int limit = 10);

    // Associations
    void associate(const std::string& id1, const std::string& id2, double weight = 1.0);
    void dissociate(const std::string& id1, const std::string& id2);
    std::vector<MemoryNode> associated_with(const std::string& id, int limit = 10);
    std::vector<MemoryNode> spreading_activation(const std::string& start_id,
                                                  double threshold = 0.1,
                                                  int max_depth = 3);

    // Auto-association (based on content similarity)
    void auto_associate(const std::string& id, double min_similarity = 0.7);
    void auto_associate_all(double min_similarity = 0.7);

    // Consolidation
    void consolidate();
    void dream();
    void decay(double factor = 0.99);
    void prune(double min_strength = 0.01);

    // Working memory
    void activate(const std::string& id);
    void deactivate(const std::string& id);
    std::vector<MemoryNode> active_memories() const;
    void clear_working_memory();

    // Snapshots
    MemorySnapshot create_snapshot(const std::string& name = "");
    bool restore_snapshot(const MemorySnapshot& snapshot);
    bool save_to_file(const std::string& path);
    bool load_from_file(const std::string& path);

    // Stats
    struct Stats {
        size_t total_memories;
        size_t episodic_count;
        size_t semantic_count;
        size_t procedural_count;
        size_t active_count;
        size_t association_count;
        double avg_strength;
        size_t total_accesses;
    };
    Stats get_stats() const;

    // Configuration
    void set_embedding_function(std::function<std::vector<float>(const std::string&)> fn);
    void set_max_memories(size_t max);
    void set_auto_consolidate(bool enable);

private:
    void generate_embedding(MemoryNode& node);
    std::string generate_id();
    void update_index(const MemoryNode& node);

    std::map<std::string, MemoryNode> memories_;
    std::set<std::string> working_memory_;
    std::unique_ptr<AssociationGraph> associations_;
    std::unique_ptr<MemoryIndex> index_;
    std::unique_ptr<MemoryConsolidator> consolidator_;

    std::function<std::vector<float>(const std::string&)> embedding_fn_;
    size_t max_memories_ = 100000;
    bool auto_consolidate_ = true;

    mutable std::mutex mutex_;
    uint64_t next_id_ = 1;
};

// ═══════════════════════════════════════════════════════════════════════════
//  CONTEXT MEMORY
// ═══════════════════════════════════════════════════════════════════════════

class ContextMemory {
public:
    ContextMemory(size_t max_items = 20);

    // Add to context
    void push(const std::string& memory_id, double relevance = 1.0);

    // Get current context
    std::vector<std::pair<std::string, double>> current() const;

    // Clear context
    void clear();

    // Summarize context
    std::string summarize(const NeuralMemoryStore& store) const;

    // Auto-update based on new input
    void update_for_input(const std::string& input,
                          NeuralMemoryStore& store,
                          int fetch_limit = 5);

private:
    std::vector<std::pair<std::string, double>> items_;
    size_t max_items_;
    mutable std::mutex mutex_;
};

// ═══════════════════════════════════════════════════════════════════════════
//  GLOBAL ACCESS
// ═══════════════════════════════════════════════════════════════════════════

NeuralMemoryStore& memory();

} // namespace rael
