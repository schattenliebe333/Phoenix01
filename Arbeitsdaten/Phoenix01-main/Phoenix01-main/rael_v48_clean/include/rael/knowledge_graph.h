// RAEL V49 - Knowledge Graph (#26)
// Graph-based knowledge representation and reasoning
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
#include <chrono>
#include <variant>

namespace rael {

// ═══════════════════════════════════════════════════════════════════════════
//  NODE TYPES
// ═══════════════════════════════════════════════════════════════════════════

enum class NodeType {
    ENTITY,         // Real-world object/concept
    CONCEPT,        // Abstract idea
    EVENT,          // Something that happened
    PROPERTY,       // Attribute value
    LITERAL,        // Raw value (string, number)
    RULE,           // Inference rule
    QUERY,          // Saved query
    CONTEXT         // Contextual information
};

using PropertyValue = std::variant<
    std::string,
    int64_t,
    double,
    bool,
    std::vector<std::string>
>;

struct Node {
    std::string id;
    std::string label;
    NodeType type = NodeType::ENTITY;
    std::map<std::string, PropertyValue> properties;
    std::vector<float> embedding;  // For semantic search
    std::chrono::system_clock::time_point created;
    std::chrono::system_clock::time_point modified;
    double confidence = 1.0;
    std::string source;  // Where this knowledge came from

    bool has_property(const std::string& key) const;
    std::optional<PropertyValue> get_property(const std::string& key) const;
    void set_property(const std::string& key, const PropertyValue& value);
};

// ═══════════════════════════════════════════════════════════════════════════
//  EDGE TYPES
// ═══════════════════════════════════════════════════════════════════════════

enum class EdgeType {
    // Hierarchical
    IS_A,           // Subclass/instance
    PART_OF,        // Composition
    HAS_PART,       // Inverse of PART_OF

    // Semantic
    RELATED_TO,     // General relation
    SIMILAR_TO,     // Semantic similarity
    OPPOSITE_OF,    // Antonym
    SYNONYM_OF,     // Same meaning

    // Causal
    CAUSES,         // Causation
    CAUSED_BY,      // Inverse
    ENABLES,        // Precondition
    PREVENTS,       // Inhibition

    // Temporal
    BEFORE,         // Temporal order
    AFTER,          // Inverse
    DURING,         // Temporal overlap

    // Spatial
    LOCATED_IN,     // Location
    NEAR,           // Proximity
    CONTAINS,       // Spatial containment

    // Attributes
    HAS_PROPERTY,   // Property link
    HAS_VALUE,      // Value link

    // Provenance
    DERIVED_FROM,   // Source
    INFERRED_BY,    // Inference rule

    // Custom
    CUSTOM          // User-defined
};

struct Edge {
    std::string id;
    std::string from_id;
    std::string to_id;
    EdgeType type = EdgeType::RELATED_TO;
    std::string custom_label;  // For CUSTOM type
    std::map<std::string, PropertyValue> properties;
    double weight = 1.0;
    double confidence = 1.0;
    std::chrono::system_clock::time_point created;
    bool bidirectional = false;
    std::string source;
};

// ═══════════════════════════════════════════════════════════════════════════
//  TRIPLE PATTERN
// ═══════════════════════════════════════════════════════════════════════════

struct Triple {
    std::string subject;
    std::string predicate;
    std::string object;
    double confidence = 1.0;

    static Triple from_edge(const Node& from, const Edge& edge, const Node& to);
    std::string to_string() const;
};

// ═══════════════════════════════════════════════════════════════════════════
//  QUERY LANGUAGE
// ═══════════════════════════════════════════════════════════════════════════

struct QueryPattern {
    std::optional<std::string> subject;
    std::optional<EdgeType> predicate;
    std::optional<std::string> object;
    std::map<std::string, PropertyValue> filters;
    bool invert = false;
};

struct PathQuery {
    std::string start_node;
    std::string end_node;
    std::vector<EdgeType> allowed_edges;
    int max_depth = 5;
    bool shortest = true;
    bool all_paths = false;
};

struct GraphQuery {
    std::vector<QueryPattern> patterns;
    std::vector<std::string> select_vars;
    std::optional<int> limit;
    std::optional<int> offset;
    std::string order_by;
    bool ascending = true;
    bool distinct = false;
};

struct QueryResult {
    std::vector<std::map<std::string, std::string>> bindings;
    std::vector<Node> nodes;
    std::vector<Edge> edges;
    std::vector<std::vector<std::string>> paths;
    double execution_time_ms;
    size_t total_matches;
};

// ═══════════════════════════════════════════════════════════════════════════
//  INFERENCE ENGINE
// ═══════════════════════════════════════════════════════════════════════════

struct InferenceRule {
    std::string id;
    std::string name;
    std::vector<QueryPattern> antecedent;  // IF conditions
    QueryPattern consequent;               // THEN result
    double confidence_factor = 0.9;
    bool enabled = true;
    int priority = 0;
};

class InferenceEngine {
public:
    InferenceEngine();

    // Rule management
    void add_rule(const InferenceRule& rule);
    void remove_rule(const std::string& id);
    void enable_rule(const std::string& id, bool enabled);
    std::vector<InferenceRule> list_rules() const;

    // Built-in rules
    void add_transitivity_rule(EdgeType edge_type);
    void add_symmetry_rule(EdgeType edge_type);
    void add_inverse_rule(EdgeType type1, EdgeType type2);
    void add_inheritance_rule();

    // Inference
    std::vector<Triple> infer(const std::vector<Node>& nodes,
                              const std::vector<Edge>& edges);
    std::vector<Triple> forward_chain(const std::vector<Node>& nodes,
                                      const std::vector<Edge>& edges,
                                      int max_iterations = 10);
    std::vector<std::string> explain_inference(const Triple& inferred);

    // Stats
    size_t rule_count() const;
    size_t inferences_made() const;

private:
    bool matches_pattern(const Node& node, const Edge& edge,
                         const QueryPattern& pattern) const;

    std::vector<InferenceRule> rules_;
    size_t inference_count_ = 0;
    mutable std::mutex mutex_;
};

// ═══════════════════════════════════════════════════════════════════════════
//  ONTOLOGY
// ═══════════════════════════════════════════════════════════════════════════

struct OntologyClass {
    std::string uri;
    std::string label;
    std::vector<std::string> parent_classes;
    std::vector<std::string> properties;
    std::string comment;
};

struct OntologyProperty {
    std::string uri;
    std::string label;
    std::string domain;  // Subject type
    std::string range;   // Object type
    bool functional = false;     // Max one value
    bool inverse_functional = false;
    std::optional<std::string> inverse_of;
    std::string comment;
};

class Ontology {
public:
    Ontology(const std::string& namespace_uri = "http://rael.ai/ontology#");

    // Class management
    void add_class(const OntologyClass& cls);
    void remove_class(const std::string& uri);
    std::optional<OntologyClass> get_class(const std::string& uri) const;
    std::vector<OntologyClass> list_classes() const;
    std::vector<std::string> get_subclasses(const std::string& uri) const;
    std::vector<std::string> get_superclasses(const std::string& uri) const;
    bool is_subclass_of(const std::string& child, const std::string& parent) const;

    // Property management
    void add_property(const OntologyProperty& prop);
    void remove_property(const std::string& uri);
    std::optional<OntologyProperty> get_property(const std::string& uri) const;
    std::vector<OntologyProperty> list_properties() const;
    std::vector<std::string> get_properties_for_class(const std::string& class_uri) const;

    // Validation
    struct ValidationError {
        std::string node_id;
        std::string message;
        std::string property;
    };
    std::vector<ValidationError> validate(const std::vector<Node>& nodes,
                                           const std::vector<Edge>& edges) const;

    // Import/Export
    std::string export_owl() const;
    std::string export_rdfs() const;
    bool import_owl(const std::string& owl);

    std::string namespace_uri() const;

private:
    std::string namespace_;
    std::map<std::string, OntologyClass> classes_;
    std::map<std::string, OntologyProperty> properties_;
    mutable std::mutex mutex_;
};

// ═══════════════════════════════════════════════════════════════════════════
//  GRAPH ALGORITHMS
// ═══════════════════════════════════════════════════════════════════════════

class GraphAlgorithms {
public:
    // Path finding
    static std::vector<std::string> shortest_path(
        const std::string& from,
        const std::string& to,
        const std::map<std::string, std::vector<std::pair<std::string, double>>>& graph);

    static std::vector<std::vector<std::string>> all_paths(
        const std::string& from,
        const std::string& to,
        const std::map<std::string, std::vector<std::pair<std::string, double>>>& graph,
        int max_depth = 5);

    // Centrality
    static std::map<std::string, double> pagerank(
        const std::map<std::string, std::vector<std::string>>& graph,
        double damping = 0.85,
        int iterations = 100);

    static std::map<std::string, double> betweenness_centrality(
        const std::map<std::string, std::vector<std::string>>& graph);

    static std::map<std::string, double> closeness_centrality(
        const std::map<std::string, std::vector<std::string>>& graph);

    // Community detection
    static std::vector<std::set<std::string>> louvain_communities(
        const std::map<std::string, std::vector<std::pair<std::string, double>>>& graph);

    static std::vector<std::set<std::string>> label_propagation(
        const std::map<std::string, std::vector<std::string>>& graph);

    // Similarity
    static double jaccard_similarity(
        const std::set<std::string>& neighbors1,
        const std::set<std::string>& neighbors2);

    static double adamic_adar(
        const std::string& node1,
        const std::string& node2,
        const std::map<std::string, std::set<std::string>>& graph);

    // Clustering
    static double clustering_coefficient(
        const std::string& node,
        const std::map<std::string, std::set<std::string>>& graph);

    static double global_clustering_coefficient(
        const std::map<std::string, std::set<std::string>>& graph);

    // Connectivity
    static std::vector<std::set<std::string>> connected_components(
        const std::map<std::string, std::vector<std::string>>& graph);

    static bool is_connected(
        const std::string& from,
        const std::string& to,
        const std::map<std::string, std::vector<std::string>>& graph);
};

// ═══════════════════════════════════════════════════════════════════════════
//  SEMANTIC LAYER
// ═══════════════════════════════════════════════════════════════════════════

class SemanticLayer {
public:
    SemanticLayer();

    // Entity resolution
    std::vector<std::string> find_similar_entities(
        const std::string& label,
        const std::vector<Node>& nodes,
        double threshold = 0.7);

    std::optional<std::string> resolve_entity(
        const std::string& mention,
        const std::vector<Node>& nodes);

    std::vector<std::pair<std::string, std::string>> find_duplicates(
        const std::vector<Node>& nodes,
        double threshold = 0.9);

    // Relation extraction
    struct ExtractedRelation {
        std::string subject;
        std::string predicate;
        std::string object;
        double confidence;
        std::string source_text;
    };

    std::vector<ExtractedRelation> extract_relations(const std::string& text);

    // Semantic search
    std::vector<std::pair<std::string, double>> semantic_search(
        const std::string& query,
        const std::vector<Node>& nodes,
        int top_k = 10);

    // Embedding
    std::vector<float> compute_embedding(const std::string& text);
    std::vector<float> compute_node_embedding(const Node& node);

    // Question answering
    std::string answer_question(
        const std::string& question,
        const std::vector<Node>& context_nodes,
        const std::vector<Edge>& context_edges);

private:
    double string_similarity(const std::string& a, const std::string& b) const;
};

// ═══════════════════════════════════════════════════════════════════════════
//  KNOWLEDGE GRAPH
// ═══════════════════════════════════════════════════════════════════════════

struct KnowledgeGraphConfig {
    std::string name = "default";
    std::string storage_path;
    bool enable_inference = true;
    bool enable_versioning = true;
    bool enable_provenance = true;
    size_t cache_size = 10000;
    int inference_depth = 3;
};

class KnowledgeGraph {
public:
    KnowledgeGraph(const KnowledgeGraphConfig& config = {});
    ~KnowledgeGraph();

    // Singleton
    static KnowledgeGraph& instance();

    // Node operations
    std::string add_node(const Node& node);
    std::string add_node(const std::string& label, NodeType type = NodeType::ENTITY);
    bool update_node(const Node& node);
    bool remove_node(const std::string& id);
    std::optional<Node> get_node(const std::string& id) const;
    std::vector<Node> get_nodes_by_type(NodeType type) const;
    std::vector<Node> get_nodes_by_label(const std::string& label) const;
    std::vector<Node> search_nodes(const std::string& query, int limit = 100) const;

    // Edge operations
    std::string add_edge(const Edge& edge);
    std::string add_edge(const std::string& from, EdgeType type, const std::string& to);
    std::string add_triple(const std::string& subject, const std::string& predicate,
                          const std::string& object);
    bool update_edge(const Edge& edge);
    bool remove_edge(const std::string& id);
    std::optional<Edge> get_edge(const std::string& id) const;
    std::vector<Edge> get_edges_from(const std::string& node_id) const;
    std::vector<Edge> get_edges_to(const std::string& node_id) const;
    std::vector<Edge> get_edges_between(const std::string& from, const std::string& to) const;
    std::vector<Edge> get_edges_by_type(EdgeType type) const;

    // Query
    QueryResult query(const GraphQuery& q) const;
    QueryResult query_pattern(const QueryPattern& pattern) const;
    std::vector<std::vector<std::string>> find_paths(const PathQuery& pq) const;
    std::vector<Triple> get_triples(const std::string& subject = "",
                                     const std::string& predicate = "",
                                     const std::string& object = "") const;

    // Traversal
    std::vector<Node> get_neighbors(const std::string& node_id,
                                     std::optional<EdgeType> edge_type = std::nullopt) const;
    std::vector<Node> traverse_bfs(const std::string& start,
                                    int max_depth = 3) const;
    std::vector<Node> traverse_dfs(const std::string& start,
                                    int max_depth = 3) const;

    // Subgraph
    struct Subgraph {
        std::vector<Node> nodes;
        std::vector<Edge> edges;
    };
    Subgraph get_subgraph(const std::string& center, int radius = 2) const;
    Subgraph get_induced_subgraph(const std::vector<std::string>& node_ids) const;

    // Inference
    void run_inference();
    std::vector<Triple> get_inferred_triples() const;
    std::vector<std::string> explain(const std::string& triple_id) const;
    InferenceEngine& inference_engine();

    // Ontology
    Ontology& ontology();
    bool validate() const;

    // Semantic operations
    SemanticLayer& semantic();
    std::vector<Node> semantic_search(const std::string& query, int top_k = 10);

    // Analytics
    std::map<std::string, double> compute_pagerank() const;
    std::vector<std::set<std::string>> detect_communities() const;
    std::map<std::string, double> compute_centrality() const;

    // Import/Export
    bool import_rdf(const std::string& rdf, const std::string& format = "turtle");
    std::string export_rdf(const std::string& format = "turtle") const;
    bool import_json(const std::string& json);
    std::string export_json() const;
    bool import_cypher(const std::string& cypher);
    std::string export_cypher() const;

    // Persistence
    bool save(const std::string& path = "");
    bool load(const std::string& path = "");

    // Versioning
    std::string create_snapshot(const std::string& name = "");
    bool restore_snapshot(const std::string& snapshot_id);
    std::vector<std::pair<std::string, std::string>> list_snapshots() const;

    // Stats
    struct Stats {
        size_t node_count;
        size_t edge_count;
        size_t triple_count;
        size_t inferred_count;
        std::map<NodeType, size_t> nodes_by_type;
        std::map<EdgeType, size_t> edges_by_type;
        double avg_out_degree;
        double clustering_coefficient;
    };
    Stats get_stats() const;

    // Merge
    void merge(const KnowledgeGraph& other, bool resolve_conflicts = true);

    // Clear
    void clear();

private:
    std::string generate_id();
    void index_node(const Node& node);
    void unindex_node(const std::string& id);
    void build_adjacency();

    KnowledgeGraphConfig config_;
    std::map<std::string, Node> nodes_;
    std::map<std::string, Edge> edges_;
    std::vector<Triple> inferred_triples_;

    // Indices
    std::map<std::string, std::set<std::string>> label_index_;
    std::map<NodeType, std::set<std::string>> type_index_;
    std::map<std::string, std::set<std::string>> adj_out_;  // node -> outgoing edges
    std::map<std::string, std::set<std::string>> adj_in_;   // node -> incoming edges

    std::unique_ptr<InferenceEngine> inference_;
    std::unique_ptr<Ontology> ontology_;
    std::unique_ptr<SemanticLayer> semantic_;

    // Versioning
    struct Snapshot {
        std::string id;
        std::string name;
        std::chrono::system_clock::time_point timestamp;
        std::map<std::string, Node> nodes;
        std::map<std::string, Edge> edges;
    };
    std::vector<Snapshot> snapshots_;

    std::atomic<uint64_t> next_id_{1};
    mutable std::mutex mutex_;
};

// ═══════════════════════════════════════════════════════════════════════════
//  SPARQL-LIKE QUERY BUILDER
// ═══════════════════════════════════════════════════════════════════════════

class QueryBuilder {
public:
    QueryBuilder();

    // Pattern building
    QueryBuilder& match(const std::string& subject,
                       const std::string& predicate,
                       const std::string& object);
    QueryBuilder& where(const std::string& var,
                       const std::string& property,
                       const PropertyValue& value);
    QueryBuilder& filter(const std::string& expression);

    // Modifiers
    QueryBuilder& select(const std::vector<std::string>& vars);
    QueryBuilder& select_all();
    QueryBuilder& distinct();
    QueryBuilder& limit(int n);
    QueryBuilder& offset(int n);
    QueryBuilder& order_by(const std::string& var, bool ascending = true);

    // Path patterns
    QueryBuilder& path(const std::string& from, const std::string& to);
    QueryBuilder& via(EdgeType type);
    QueryBuilder& max_depth(int depth);

    // Execute
    GraphQuery build() const;
    QueryResult execute(KnowledgeGraph& kg) const;

    // String representation
    std::string to_sparql() const;
    std::string to_cypher() const;

private:
    GraphQuery query_;
    std::vector<std::string> path_via_;
};

// ═══════════════════════════════════════════════════════════════════════════
//  HELPER FUNCTIONS
// ═══════════════════════════════════════════════════════════════════════════

// Edge type to string
std::string edge_type_to_string(EdgeType type);
EdgeType string_to_edge_type(const std::string& str);

// Node type to string
std::string node_type_to_string(NodeType type);
NodeType string_to_node_type(const std::string& str);

// Property value helpers
std::string property_to_string(const PropertyValue& value);

// ═══════════════════════════════════════════════════════════════════════════
//  GLOBAL ACCESS
// ═══════════════════════════════════════════════════════════════════════════

KnowledgeGraph& kg();

} // namespace rael
