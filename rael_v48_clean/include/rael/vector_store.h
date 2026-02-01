// RAEL V49 - Vector Database Integration (#25)
// Semantic search with embeddings
#pragma once

#include <string>
#include <vector>
#include <map>
#include <set>
#include <memory>
#include <functional>
#include <optional>
#include <mutex>
#include <atomic>
#include <cmath>

namespace rael {

// ═══════════════════════════════════════════════════════════════════════════
//  VECTOR TYPES
// ═══════════════════════════════════════════════════════════════════════════

using Vector = std::vector<float>;
using VectorId = std::string;

struct VectorEntry {
    VectorId id;
    Vector embedding;
    std::map<std::string, std::string> metadata;
    std::string content;           // Original text
    std::string namespace_id;      // For partitioning
    double score = 0.0;            // Similarity score (filled during search)
};

struct SearchResult {
    VectorId id;
    double score;
    std::map<std::string, std::string> metadata;
    std::string content;
};

// ═══════════════════════════════════════════════════════════════════════════
//  DISTANCE METRICS
// ═══════════════════════════════════════════════════════════════════════════

enum class DistanceMetric {
    COSINE,
    EUCLIDEAN,
    DOT_PRODUCT,
    MANHATTAN
};

class DistanceCalculator {
public:
    static double cosine(const Vector& a, const Vector& b);
    static double euclidean(const Vector& a, const Vector& b);
    static double dot_product(const Vector& a, const Vector& b);
    static double manhattan(const Vector& a, const Vector& b);

    static double calculate(const Vector& a, const Vector& b, DistanceMetric metric);

    // Normalize vector for cosine similarity
    static Vector normalize(const Vector& v);
};

// ═══════════════════════════════════════════════════════════════════════════
//  EMBEDDING PROVIDER
// ═══════════════════════════════════════════════════════════════════════════

class EmbeddingProvider {
public:
    virtual ~EmbeddingProvider() = default;

    virtual Vector embed(const std::string& text) = 0;
    virtual std::vector<Vector> embed_batch(const std::vector<std::string>& texts) = 0;
    virtual int dimension() const = 0;
    virtual std::string model_name() const = 0;
};

// Simple hash-based embedder for testing
class SimpleEmbedder : public EmbeddingProvider {
public:
    SimpleEmbedder(int dim = 384);

    Vector embed(const std::string& text) override;
    std::vector<Vector> embed_batch(const std::vector<std::string>& texts) override;
    int dimension() const override { return dim_; }
    std::string model_name() const override { return "simple-hash"; }

private:
    int dim_;
};

// ═══════════════════════════════════════════════════════════════════════════
//  INDEX STRUCTURES
// ═══════════════════════════════════════════════════════════════════════════

// Abstract index interface
class VectorIndex {
public:
    virtual ~VectorIndex() = default;

    virtual void add(const VectorId& id, const Vector& vec) = 0;
    virtual void remove(const VectorId& id) = 0;
    virtual bool contains(const VectorId& id) const = 0;
    virtual std::vector<std::pair<VectorId, double>> search(
        const Vector& query, int k, double threshold = 0.0) const = 0;

    virtual size_t size() const = 0;
    virtual void clear() = 0;

    // Persistence
    virtual bool save(const std::string& path) const = 0;
    virtual bool load(const std::string& path) = 0;
};

// Flat (brute-force) index - exact but slow
class FlatIndex : public VectorIndex {
public:
    FlatIndex(DistanceMetric metric = DistanceMetric::COSINE);

    void add(const VectorId& id, const Vector& vec) override;
    void remove(const VectorId& id) override;
    bool contains(const VectorId& id) const override;
    std::vector<std::pair<VectorId, double>> search(
        const Vector& query, int k, double threshold = 0.0) const override;

    size_t size() const override;
    void clear() override;
    bool save(const std::string& path) const override;
    bool load(const std::string& path) override;

private:
    DistanceMetric metric_;
    std::map<VectorId, Vector> vectors_;
    mutable std::mutex mutex_;
};

// HNSW-like index - approximate but fast
class HNSWIndex : public VectorIndex {
public:
    HNSWIndex(int M = 16, int ef_construction = 200, DistanceMetric metric = DistanceMetric::COSINE);

    void add(const VectorId& id, const Vector& vec) override;
    void remove(const VectorId& id) override;
    bool contains(const VectorId& id) const override;
    std::vector<std::pair<VectorId, double>> search(
        const Vector& query, int k, double threshold = 0.0) const override;

    size_t size() const override;
    void clear() override;
    bool save(const std::string& path) const override;
    bool load(const std::string& path) override;

    // HNSW-specific
    void set_ef_search(int ef) { ef_search_ = ef; }

private:
    struct Node {
        VectorId id;
        Vector vec;
        std::vector<std::vector<VectorId>> neighbors;  // Per level
    };

    int select_level();
    void insert_node(const VectorId& id, const Vector& vec);
    std::vector<VectorId> search_layer(const Vector& query, const VectorId& entry,
                                       int ef, int level) const;

    int M_;
    int ef_construction_;
    int ef_search_ = 50;
    int max_level_ = 0;
    DistanceMetric metric_;
    std::map<VectorId, Node> nodes_;
    VectorId entry_point_;
    mutable std::mutex mutex_;
};

// LSH (Locality-Sensitive Hashing) index
class LSHIndex : public VectorIndex {
public:
    LSHIndex(int num_tables = 10, int num_hashes = 8, int dim = 384);

    void add(const VectorId& id, const Vector& vec) override;
    void remove(const VectorId& id) override;
    bool contains(const VectorId& id) const override;
    std::vector<std::pair<VectorId, double>> search(
        const Vector& query, int k, double threshold = 0.0) const override;

    size_t size() const override;
    void clear() override;
    bool save(const std::string& path) const override;
    bool load(const std::string& path) override;

private:
    uint64_t hash_vector(const Vector& vec, int table_idx) const;

    int num_tables_;
    int num_hashes_;
    int dim_;
    std::vector<std::vector<Vector>> hash_functions_;  // Random projections
    std::vector<std::map<uint64_t, std::vector<VectorId>>> tables_;
    std::map<VectorId, Vector> vectors_;
    mutable std::mutex mutex_;
};

// ═══════════════════════════════════════════════════════════════════════════
//  METADATA FILTER
// ═══════════════════════════════════════════════════════════════════════════

struct MetadataFilter {
    enum class Op { EQ, NE, GT, LT, GTE, LTE, IN, CONTAINS };

    std::string field;
    Op op;
    std::string value;
    std::vector<std::string> values;  // For IN operation

    bool matches(const std::map<std::string, std::string>& metadata) const;
};

class FilterBuilder {
public:
    FilterBuilder& eq(const std::string& field, const std::string& value);
    FilterBuilder& ne(const std::string& field, const std::string& value);
    FilterBuilder& gt(const std::string& field, const std::string& value);
    FilterBuilder& lt(const std::string& field, const std::string& value);
    FilterBuilder& in(const std::string& field, const std::vector<std::string>& values);
    FilterBuilder& contains(const std::string& field, const std::string& value);

    std::vector<MetadataFilter> build() const;

private:
    std::vector<MetadataFilter> filters_;
};

// ═══════════════════════════════════════════════════════════════════════════
//  VECTOR COLLECTION
// ═══════════════════════════════════════════════════════════════════════════

struct CollectionConfig {
    std::string name;
    int dimension = 384;
    DistanceMetric metric = DistanceMetric::COSINE;
    std::string index_type = "hnsw";  // flat, hnsw, lsh
    int hnsw_m = 16;
    int hnsw_ef = 200;
    int lsh_tables = 10;
    int lsh_hashes = 8;
};

class VectorCollection {
public:
    VectorCollection(const CollectionConfig& config);

    // CRUD operations
    void upsert(const VectorEntry& entry);
    void upsert_batch(const std::vector<VectorEntry>& entries);
    std::optional<VectorEntry> get(const VectorId& id) const;
    bool remove(const VectorId& id);
    bool exists(const VectorId& id) const;

    // Search
    std::vector<SearchResult> search(const Vector& query, int k = 10,
                                     const std::vector<MetadataFilter>& filters = {}) const;
    std::vector<SearchResult> search_text(const std::string& text, int k = 10,
                                          const std::vector<MetadataFilter>& filters = {});

    // Hybrid search (keyword + vector)
    std::vector<SearchResult> hybrid_search(const std::string& text, int k = 10,
                                            double alpha = 0.5);  // alpha: weight for vector

    // Info
    std::string name() const { return config_.name; }
    size_t count() const;
    CollectionConfig config() const { return config_; }

    // Embedding
    void set_embedder(std::shared_ptr<EmbeddingProvider> embedder);

    // Persistence
    bool save(const std::string& dir) const;
    bool load(const std::string& dir);

private:
    CollectionConfig config_;
    std::unique_ptr<VectorIndex> index_;
    std::map<VectorId, VectorEntry> entries_;
    std::shared_ptr<EmbeddingProvider> embedder_;

    // Keyword index for hybrid search
    std::map<std::string, std::set<VectorId>> keyword_index_;
    void index_keywords(const VectorId& id, const std::string& content);

    mutable std::mutex mutex_;
};

// ═══════════════════════════════════════════════════════════════════════════
//  VECTOR DATABASE
// ═══════════════════════════════════════════════════════════════════════════

class VectorDatabase {
public:
    VectorDatabase();
    ~VectorDatabase();

    // Singleton
    static VectorDatabase& instance();

    // Collection management
    bool create_collection(const CollectionConfig& config);
    bool delete_collection(const std::string& name);
    std::shared_ptr<VectorCollection> get_collection(const std::string& name);
    std::vector<std::string> list_collections() const;
    bool collection_exists(const std::string& name) const;

    // Default collection operations
    void set_default_collection(const std::string& name);
    std::shared_ptr<VectorCollection> default_collection();

    // Convenience methods (use default collection)
    void upsert(const VectorEntry& entry);
    std::optional<VectorEntry> get(const VectorId& id);
    bool remove(const VectorId& id);
    std::vector<SearchResult> search(const Vector& query, int k = 10);
    std::vector<SearchResult> search_text(const std::string& text, int k = 10);

    // Embedding provider
    void set_embedder(std::shared_ptr<EmbeddingProvider> embedder);
    std::shared_ptr<EmbeddingProvider> get_embedder() const;
    Vector embed(const std::string& text);

    // Persistence
    void set_storage_path(const std::string& path);
    bool save_all() const;
    bool load_all();

    // Stats
    struct Stats {
        size_t collection_count;
        size_t total_vectors;
        size_t total_storage_bytes;
    };
    Stats get_stats() const;

private:
    std::map<std::string, std::shared_ptr<VectorCollection>> collections_;
    std::string default_collection_name_;
    std::shared_ptr<EmbeddingProvider> embedder_;
    std::string storage_path_;
    mutable std::mutex mutex_;
};

// ═══════════════════════════════════════════════════════════════════════════
//  DOCUMENT CHUNKER
// ═══════════════════════════════════════════════════════════════════════════

struct Chunk {
    std::string id;
    std::string text;
    int start_char;
    int end_char;
    std::map<std::string, std::string> metadata;
};

class DocumentChunker {
public:
    DocumentChunker(int chunk_size = 512, int overlap = 50);

    std::vector<Chunk> chunk(const std::string& document,
                             const std::string& doc_id = "",
                             const std::map<std::string, std::string>& metadata = {});

    // Different chunking strategies
    std::vector<Chunk> chunk_by_sentences(const std::string& document);
    std::vector<Chunk> chunk_by_paragraphs(const std::string& document);
    std::vector<Chunk> chunk_by_tokens(const std::string& document, int max_tokens);

    void set_chunk_size(int size) { chunk_size_ = size; }
    void set_overlap(int overlap) { overlap_ = overlap; }

private:
    int chunk_size_;
    int overlap_;
};

// ═══════════════════════════════════════════════════════════════════════════
//  RAG (Retrieval-Augmented Generation) HELPER
// ═══════════════════════════════════════════════════════════════════════════

class RAGHelper {
public:
    RAGHelper(VectorDatabase& db, const std::string& collection = "");

    // Index document
    void index_document(const std::string& doc_id, const std::string& content,
                       const std::map<std::string, std::string>& metadata = {});

    // Retrieve relevant context
    std::string retrieve_context(const std::string& query, int k = 5);
    std::vector<Chunk> retrieve_chunks(const std::string& query, int k = 5);

    // Build prompt with context
    std::string build_prompt(const std::string& query, const std::string& system_prompt = "",
                            int context_chunks = 5);

    // Chunking settings
    void set_chunk_size(int size);
    void set_overlap(int overlap);

private:
    VectorDatabase& db_;
    std::string collection_;
    DocumentChunker chunker_;
};

// ═══════════════════════════════════════════════════════════════════════════
//  GLOBAL ACCESS
// ═══════════════════════════════════════════════════════════════════════════

VectorDatabase& vectordb();

} // namespace rael
