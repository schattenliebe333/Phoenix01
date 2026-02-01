// RAEL V49 - Neural Memory Store Implementation
#include "rael/neural_memory.h"
#include <algorithm>
#include <cmath>
#include <random>
#include <sstream>
#include <fstream>
#include <queue>
#include <iomanip>

namespace rael {

// ═══════════════════════════════════════════════════════════════════════════
//  UTILITY FUNCTIONS
// ═══════════════════════════════════════════════════════════════════════════

static float cosine_similarity(const std::vector<float>& a, const std::vector<float>& b) {
    if (a.empty() || b.empty() || a.size() != b.size()) return 0.0f;

    float dot = 0.0f, norm_a = 0.0f, norm_b = 0.0f;
    for (size_t i = 0; i < a.size(); i++) {
        dot += a[i] * b[i];
        norm_a += a[i] * a[i];
        norm_b += b[i] * b[i];
    }

    if (norm_a > 0 && norm_b > 0) {
        return dot / (std::sqrt(norm_a) * std::sqrt(norm_b));
    }
    return 0.0f;
}

static std::vector<std::string> tokenize(const std::string& text) {
    std::vector<std::string> tokens;
    std::string current;

    for (char c : text) {
        if (std::isalnum(c)) {
            current += std::tolower(c);
        } else if (!current.empty()) {
            tokens.push_back(current);
            current.clear();
        }
    }
    if (!current.empty()) {
        tokens.push_back(current);
    }
    return tokens;
}

// REAL embedding using multiple techniques:
// 1. Character n-gram features (FastText-style)
// 2. Word position encoding (Transformer-style)
// 3. Semantic clustering via RST frequency harmonics
static std::vector<float> simple_embed(const std::string& text, int dim = 256) {
    std::vector<float> emb(dim, 0.0f);
    auto tokens = tokenize(text);
    if (tokens.empty()) return emb;

    // RST frequency constants for harmonic encoding
    const double PHI = 1.6180339887;
    const double G0 = 8.0 / 9.0;  // 0.888889

    // 1. Character trigram features (FastText-style)
    std::string padded = "<" + text + ">";
    for (size_t i = 0; i + 2 < padded.size(); i++) {
        // Extract trigram
        uint32_t trigram = (static_cast<uint32_t>(padded[i]) << 16) |
                           (static_cast<uint32_t>(padded[i+1]) << 8) |
                           static_cast<uint32_t>(padded[i+2]);

        // Hash trigram to embedding dimensions
        uint32_t h = trigram;
        h ^= h >> 16;
        h *= 0x85ebca6b;
        h ^= h >> 13;
        h *= 0xc2b2ae35;
        h ^= h >> 16;

        // Distribute across embedding using PHI-based spacing
        for (int j = 0; j < 4; j++) {
            int idx = (h + static_cast<uint32_t>(j * PHI * 1000)) % dim;
            float sign = (h & (1 << j)) ? 1.0f : -1.0f;
            emb[idx] += sign * G0 / std::sqrt(static_cast<float>(padded.size()));
        }
    }

    // 2. Word-level positional encoding (Transformer-style)
    for (size_t i = 0; i < tokens.size(); i++) {
        const std::string& token = tokens[i];
        double pos = static_cast<double>(i) / std::max(tokens.size(), size_t(1));

        // Word hash
        uint64_t word_hash = 0;
        for (char c : token) {
            word_hash = word_hash * 31 + static_cast<uint64_t>(c);
        }

        // Add positional encoding using sin/cos at different frequencies
        for (int d = 0; d < dim; d += 2) {
            double freq = 1.0 / std::pow(10000.0, static_cast<double>(d) / dim);

            // Combine word position with word identity
            double angle = pos * freq + (word_hash % 1000) * 0.001;

            if (d < dim) emb[d] += std::sin(angle) * 0.5f;
            if (d + 1 < dim) emb[d + 1] += std::cos(angle) * 0.5f;
        }

        // 3. Semantic frequency bands (RST harmonics)
        // Map words to frequency bands based on length and first char
        int freq_band = (token.length() + (token[0] % 7)) % 7;
        double base_freq = 1440.0;  // RST fundamental
        for (int k = 0; k < freq_band; k++) {
            base_freq /= PHI;  // Descend through frequency cascade
        }

        // Embed frequency signature
        int band_start = (freq_band * dim) / 7;
        int band_end = ((freq_band + 1) * dim) / 7;
        for (int d = band_start; d < band_end && d < dim; d++) {
            emb[d] += std::sin(base_freq * (d - band_start) * 0.001) * G0;
        }
    }

    // 4. L2 Normalize to unit sphere
    float norm = 0.0f;
    for (float v : emb) norm += v * v;
    norm = std::sqrt(norm);
    if (norm > 1e-8f) {
        for (float& v : emb) v /= norm;
    }

    return emb;
}

// ═══════════════════════════════════════════════════════════════════════════
//  ASSOCIATION GRAPH
// ═══════════════════════════════════════════════════════════════════════════

AssociationGraph::AssociationGraph() {}

void AssociationGraph::add_node(const std::string& id) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (adjacency_.find(id) == adjacency_.end()) {
        adjacency_[id] = {};
    }
}

void AssociationGraph::remove_node(const std::string& id) {
    std::lock_guard<std::mutex> lock(mutex_);
    adjacency_.erase(id);
    for (auto& [_, edges] : adjacency_) {
        edges.erase(id);
    }
}

bool AssociationGraph::has_node(const std::string& id) const {
    std::lock_guard<std::mutex> lock(mutex_);
    return adjacency_.find(id) != adjacency_.end();
}

std::vector<std::string> AssociationGraph::all_nodes() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<std::string> nodes;
    for (const auto& [id, _] : adjacency_) {
        nodes.push_back(id);
    }
    return nodes;
}

void AssociationGraph::add_edge(const std::string& from, const std::string& to, double weight) {
    std::lock_guard<std::mutex> lock(mutex_);
    adjacency_[from][to] = weight;
    adjacency_[to][from] = weight;  // Bidirectional
}

void AssociationGraph::remove_edge(const std::string& from, const std::string& to) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (adjacency_.count(from)) adjacency_[from].erase(to);
    if (adjacency_.count(to)) adjacency_[to].erase(from);
}

double AssociationGraph::get_edge_weight(const std::string& from, const std::string& to) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = adjacency_.find(from);
    if (it != adjacency_.end()) {
        auto eit = it->second.find(to);
        if (eit != it->second.end()) {
            return eit->second;
        }
    }
    return 0.0;
}

void AssociationGraph::strengthen_edge(const std::string& from, const std::string& to, double delta) {
    std::lock_guard<std::mutex> lock(mutex_);
    double current = 0.0;
    if (adjacency_.count(from) && adjacency_[from].count(to)) {
        current = adjacency_[from][to];
    }
    double new_weight = std::min(1.0, current + delta);
    adjacency_[from][to] = new_weight;
    adjacency_[to][from] = new_weight;
}

void AssociationGraph::weaken_edge(const std::string& from, const std::string& to, double delta) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (adjacency_.count(from) && adjacency_[from].count(to)) {
        double new_weight = std::max(0.0, adjacency_[from][to] - delta);
        if (new_weight <= 0) {
            adjacency_[from].erase(to);
            adjacency_[to].erase(from);
        } else {
            adjacency_[from][to] = new_weight;
            adjacency_[to][from] = new_weight;
        }
    }
}

std::vector<std::pair<std::string, double>> AssociationGraph::neighbors(const std::string& id) const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<std::pair<std::string, double>> result;
    auto it = adjacency_.find(id);
    if (it != adjacency_.end()) {
        for (const auto& [neighbor, weight] : it->second) {
            result.emplace_back(neighbor, weight);
        }
    }
    std::sort(result.begin(), result.end(),
        [](const auto& a, const auto& b) { return a.second > b.second; });
    return result;
}

std::vector<std::string> AssociationGraph::path(const std::string& from, const std::string& to) const {
    std::lock_guard<std::mutex> lock(mutex_);

    if (adjacency_.find(from) == adjacency_.end() ||
        adjacency_.find(to) == adjacency_.end()) {
        return {};
    }

    // BFS
    std::queue<std::string> queue;
    std::map<std::string, std::string> parent;
    std::set<std::string> visited;

    queue.push(from);
    visited.insert(from);

    while (!queue.empty()) {
        std::string current = queue.front();
        queue.pop();

        if (current == to) {
            std::vector<std::string> result;
            std::string node = to;
            while (node != from) {
                result.push_back(node);
                node = parent[node];
            }
            result.push_back(from);
            std::reverse(result.begin(), result.end());
            return result;
        }

        auto it = adjacency_.find(current);
        if (it != adjacency_.end()) {
            for (const auto& [neighbor, _] : it->second) {
                if (visited.find(neighbor) == visited.end()) {
                    visited.insert(neighbor);
                    parent[neighbor] = current;
                    queue.push(neighbor);
                }
            }
        }
    }

    return {};  // No path found
}

double AssociationGraph::distance(const std::string& from, const std::string& to) const {
    auto p = path(from, to);
    if (p.empty()) return std::numeric_limits<double>::infinity();
    return static_cast<double>(p.size() - 1);
}

std::vector<std::string> AssociationGraph::strongest_associations(const std::string& id, int k) const {
    auto neighs = neighbors(id);
    std::vector<std::string> result;
    for (size_t i = 0; i < neighs.size() && static_cast<int>(i) < k; i++) {
        result.push_back(neighs[i].first);
    }
    return result;
}

std::vector<std::string> AssociationGraph::spreading_activation(const std::string& start,
                                                                 double threshold,
                                                                 int max_depth) const {
    std::lock_guard<std::mutex> lock(mutex_);

    std::map<std::string, double> activation;
    std::set<std::string> result;

    activation[start] = 1.0;
    result.insert(start);

    for (int depth = 0; depth < max_depth; depth++) {
        std::map<std::string, double> new_activation;

        for (const auto& [node, act] : activation) {
            auto it = adjacency_.find(node);
            if (it != adjacency_.end()) {
                for (const auto& [neighbor, weight] : it->second) {
                    double spread = act * weight * 0.5;  // Decay factor
                    if (spread >= threshold) {
                        new_activation[neighbor] = std::max(new_activation[neighbor], spread);
                        result.insert(neighbor);
                    }
                }
            }
        }

        if (new_activation.empty()) break;
        activation = new_activation;
    }

    return std::vector<std::string>(result.begin(), result.end());
}

std::vector<std::vector<std::string>> AssociationGraph::find_clusters(int min_size) const {
    std::lock_guard<std::mutex> lock(mutex_);

    std::set<std::string> visited;
    std::vector<std::vector<std::string>> clusters;

    for (const auto& [node, _] : adjacency_) {
        if (visited.count(node)) continue;

        std::vector<std::string> cluster;
        std::queue<std::string> queue;
        queue.push(node);
        visited.insert(node);

        while (!queue.empty()) {
            std::string current = queue.front();
            queue.pop();
            cluster.push_back(current);

            auto it = adjacency_.find(current);
            if (it != adjacency_.end()) {
                for (const auto& [neighbor, _] : it->second) {
                    if (visited.find(neighbor) == visited.end()) {
                        visited.insert(neighbor);
                        queue.push(neighbor);
                    }
                }
            }
        }

        if (static_cast<int>(cluster.size()) >= min_size) {
            clusters.push_back(cluster);
        }
    }

    return clusters;
}

double AssociationGraph::clustering_coefficient(const std::string& id) const {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = adjacency_.find(id);
    if (it == adjacency_.end() || it->second.size() < 2) return 0.0;

    std::vector<std::string> neighs;
    for (const auto& [n, _] : it->second) {
        neighs.push_back(n);
    }

    int edges_between = 0;
    for (size_t i = 0; i < neighs.size(); i++) {
        for (size_t j = i + 1; j < neighs.size(); j++) {
            auto nit = adjacency_.find(neighs[i]);
            if (nit != adjacency_.end() && nit->second.count(neighs[j])) {
                edges_between++;
            }
        }
    }

    int max_edges = static_cast<int>(neighs.size() * (neighs.size() - 1) / 2);
    return max_edges > 0 ? static_cast<double>(edges_between) / max_edges : 0.0;
}

void AssociationGraph::prune_weak_edges(double min_weight) {
    std::lock_guard<std::mutex> lock(mutex_);

    for (auto& [_, edges] : adjacency_) {
        for (auto it = edges.begin(); it != edges.end(); ) {
            if (it->second < min_weight) {
                it = edges.erase(it);
            } else {
                ++it;
            }
        }
    }
}

void AssociationGraph::decay_all_edges(double factor) {
    std::lock_guard<std::mutex> lock(mutex_);

    for (auto& [_, edges] : adjacency_) {
        for (auto& [__, weight] : edges) {
            weight *= factor;
        }
    }
}

size_t AssociationGraph::node_count() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return adjacency_.size();
}

size_t AssociationGraph::edge_count() const {
    std::lock_guard<std::mutex> lock(mutex_);
    size_t count = 0;
    for (const auto& [_, edges] : adjacency_) {
        count += edges.size();
    }
    return count / 2;  // Undirected
}

// ═══════════════════════════════════════════════════════════════════════════
//  FORGETTING CURVE
// ═══════════════════════════════════════════════════════════════════════════

ForgettingCurve::ForgettingCurve(double initial_strength, double stability)
    : initial_strength_(initial_strength)
    , stability_(stability)
    , last_rehearsal_(std::chrono::system_clock::now()) {}

double ForgettingCurve::retention(std::chrono::seconds elapsed) const {
    return decay(initial_strength_, elapsed, stability_);
}

void ForgettingCurve::rehearse() {
    rehearsal_count_++;
    // Each rehearsal increases stability
    stability_ *= 1.5;
    last_rehearsal_ = std::chrono::system_clock::now();
}

std::chrono::seconds ForgettingCurve::next_review_interval() const {
    // Spaced repetition: interval grows with stability
    double days = stability_ * std::pow(2.0, rehearsal_count_);
    return std::chrono::seconds(static_cast<long>(days * 86400));
}

double ForgettingCurve::decay(double initial, std::chrono::seconds elapsed, double stability) {
    // Ebbinghaus forgetting curve: R = e^(-t/S)
    double t = elapsed.count() / 86400.0;  // Convert to days
    return initial * std::exp(-t / stability);
}

// ═══════════════════════════════════════════════════════════════════════════
//  MEMORY CONSOLIDATION
// ═══════════════════════════════════════════════════════════════════════════

MemoryConsolidator::MemoryConsolidator() {}

void MemoryConsolidator::consolidate(MemoryNode& node) {
    auto now = std::chrono::system_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(
        now - node.last_accessed);

    // Apply forgetting curve
    double stability = 1.0 + node.rehearsal_count * 0.5;
    node.strength = ForgettingCurve::decay(1.0, elapsed, stability);

    // Boost based on importance and emotion
    node.strength *= (0.5 + 0.5 * node.importance);
    node.strength *= (1.0 + 0.3 * std::abs(node.emotional_valence));
    node.strength = std::min(1.0, node.strength);

    // Update state
    if (node.strength < 0.1) {
        node.state = MemoryState::DECAYING;
    } else if (node.state == MemoryState::ACTIVE) {
        node.state = MemoryState::CONSOLIDATING;
    }

    node.last_consolidated = now;
}

void MemoryConsolidator::consolidate_batch(std::vector<MemoryNode*>& nodes) {
    for (auto* node : nodes) {
        if (node) consolidate(*node);
    }
}

void MemoryConsolidator::dream_cycle(std::vector<MemoryNode*>& memories,
                                     AssociationGraph& graph,
                                     std::chrono::seconds duration) {
    // Simulate memory reorganization during "sleep"
    static std::random_device rd;
    static std::mt19937 gen(rd());

    // Random replay and association strengthening
    int cycles = static_cast<int>(duration.count() / 10);

    for (int c = 0; c < cycles && !memories.empty(); c++) {
        // Select random memory
        std::uniform_int_distribution<> dis(0, static_cast<int>(memories.size()) - 1);
        auto* mem = memories[dis(gen)];
        if (!mem) continue;

        // Strengthen its associations based on similarity
        for (auto* other : memories) {
            if (other && other != mem) {
                float sim = cosine_similarity(mem->embedding, other->embedding);
                if (sim > 0.7) {
                    graph.strengthen_edge(mem->id, other->id, 0.05 * sim);
                }
            }
        }

        // Slight boost to important memories
        if (mem->importance > 0.5) {
            mem->strength = std::min(1.0, mem->strength + 0.01);
        }
    }

    // Prune weak associations
    graph.prune_weak_edges(0.05);
}

double MemoryConsolidator::calculate_importance(const MemoryNode& node) const {
    double imp = node.importance;

    // Factor in access frequency
    imp += 0.1 * std::log1p(node.access_count);

    // Factor in emotional significance
    imp += 0.2 * std::abs(node.emotional_valence);
    imp += 0.1 * node.emotional_arousal;

    // Factor in recency
    auto now = std::chrono::system_clock::now();
    auto age = std::chrono::duration_cast<std::chrono::hours>(now - node.created).count();
    imp += 0.1 / (1.0 + age / 24.0);  // Decay over days

    return std::min(1.0, std::max(0.0, imp));
}

bool MemoryConsolidator::should_keep(const MemoryNode& node) const {
    return node.strength > importance_threshold_ ||
           node.importance > 0.8 ||
           node.access_count > 10;
}

bool MemoryConsolidator::should_archive(const MemoryNode& node) const {
    return node.strength < 0.3 &&
           node.access_count < 3 &&
           node.importance < 0.5;
}

// ═══════════════════════════════════════════════════════════════════════════
//  MEMORY INDEX
// ═══════════════════════════════════════════════════════════════════════════

MemoryIndex::MemoryIndex() {}

void MemoryIndex::index(const MemoryNode& node) {
    std::lock_guard<std::mutex> lock(mutex_);

    // Word index
    auto words = tokenize(node.content);
    for (const auto& word : words) {
        word_index_[word].insert(node.id);
    }

    // Tag index
    for (const auto& tag : node.tags) {
        tag_index_[tag].insert(node.id);
    }

    // Type index
    type_index_[node.type].insert(node.id);

    // Time index
    time_index_.emplace(node.created, node.id);

    // Embedding
    if (!node.embedding.empty()) {
        embeddings_[node.id] = node.embedding;
    }
}

void MemoryIndex::reindex(const MemoryNode& node) {
    remove(node.id);
    index(node);
}

void MemoryIndex::remove(const std::string& id) {
    std::lock_guard<std::mutex> lock(mutex_);

    for (auto& [_, ids] : word_index_) {
        ids.erase(id);
    }
    for (auto& [_, ids] : tag_index_) {
        ids.erase(id);
    }
    for (auto& [_, ids] : type_index_) {
        ids.erase(id);
    }
    for (auto it = time_index_.begin(); it != time_index_.end(); ) {
        if (it->second == id) {
            it = time_index_.erase(it);
        } else {
            ++it;
        }
    }
    embeddings_.erase(id);
}

std::vector<std::string> MemoryIndex::search_text(const std::string& query, int limit) const {
    std::lock_guard<std::mutex> lock(mutex_);

    auto words = tokenize(query);
    std::map<std::string, int> scores;

    for (const auto& word : words) {
        auto it = word_index_.find(word);
        if (it != word_index_.end()) {
            for (const auto& id : it->second) {
                scores[id]++;
            }
        }
    }

    std::vector<std::pair<std::string, int>> ranked(scores.begin(), scores.end());
    std::sort(ranked.begin(), ranked.end(),
        [](const auto& a, const auto& b) { return a.second > b.second; });

    std::vector<std::string> result;
    for (size_t i = 0; i < ranked.size() && static_cast<int>(i) < limit; i++) {
        result.push_back(ranked[i].first);
    }
    return result;
}

std::vector<std::string> MemoryIndex::search_embedding(const std::vector<float>& embedding,
                                                        int limit) const {
    std::lock_guard<std::mutex> lock(mutex_);

    std::vector<std::pair<std::string, float>> similarities;

    for (const auto& [id, emb] : embeddings_) {
        float sim = cosine_similarity(embedding, emb);
        similarities.emplace_back(id, sim);
    }

    std::sort(similarities.begin(), similarities.end(),
        [](const auto& a, const auto& b) { return a.second > b.second; });

    std::vector<std::string> result;
    for (size_t i = 0; i < similarities.size() && static_cast<int>(i) < limit; i++) {
        result.push_back(similarities[i].first);
    }
    return result;
}

std::vector<std::string> MemoryIndex::search_tags(const std::vector<std::string>& tags) const {
    std::lock_guard<std::mutex> lock(mutex_);

    std::set<std::string> result;
    for (const auto& tag : tags) {
        auto it = tag_index_.find(tag);
        if (it != tag_index_.end()) {
            result.insert(it->second.begin(), it->second.end());
        }
    }
    return std::vector<std::string>(result.begin(), result.end());
}

std::vector<std::string> MemoryIndex::search_type(MemoryType type) const {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = type_index_.find(type);
    if (it != type_index_.end()) {
        return std::vector<std::string>(it->second.begin(), it->second.end());
    }
    return {};
}

std::vector<std::string> MemoryIndex::search_time_range(
    std::chrono::system_clock::time_point start,
    std::chrono::system_clock::time_point end) const {
    std::lock_guard<std::mutex> lock(mutex_);

    std::vector<std::string> result;
    auto it_start = time_index_.lower_bound(start);
    auto it_end = time_index_.upper_bound(end);

    for (auto it = it_start; it != it_end; ++it) {
        result.push_back(it->second);
    }
    return result;
}

std::vector<std::string> MemoryIndex::search(const SearchQuery& query) const {
    std::set<std::string> candidates;
    bool first = true;

    auto intersect = [&](const std::vector<std::string>& ids) {
        if (first) {
            candidates.insert(ids.begin(), ids.end());
            first = false;
        } else {
            std::set<std::string> new_set;
            for (const auto& id : ids) {
                if (candidates.count(id)) {
                    new_set.insert(id);
                }
            }
            candidates = new_set;
        }
    };

    if (!query.text.empty()) {
        intersect(search_text(query.text, 1000));
    }

    if (!query.embedding.empty()) {
        intersect(search_embedding(query.embedding, 1000));
    }

    if (!query.tags.empty()) {
        intersect(search_tags(query.tags));
    }

    if (query.type) {
        intersect(search_type(*query.type));
    }

    std::vector<std::string> result(candidates.begin(), candidates.end());
    if (static_cast<int>(result.size()) > query.limit) {
        result.resize(query.limit);
    }
    return result;
}

size_t MemoryIndex::size() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return embeddings_.size();
}

// ═══════════════════════════════════════════════════════════════════════════
//  NEURAL MEMORY STORE
// ═══════════════════════════════════════════════════════════════════════════

NeuralMemoryStore::NeuralMemoryStore()
    : associations_(std::make_unique<AssociationGraph>())
    , index_(std::make_unique<MemoryIndex>())
    , consolidator_(std::make_unique<MemoryConsolidator>())
    , embedding_fn_([](const std::string& text) { return simple_embed(text, 256); }) {}

NeuralMemoryStore::~NeuralMemoryStore() = default;

NeuralMemoryStore& NeuralMemoryStore::instance() {
    static NeuralMemoryStore inst;
    return inst;
}

std::string NeuralMemoryStore::generate_id() {
    std::ostringstream oss;
    oss << "mem_" << std::hex << std::setw(16) << std::setfill('0') << next_id_++;
    return oss.str();
}

void NeuralMemoryStore::generate_embedding(MemoryNode& node) {
    if (embedding_fn_) {
        node.embedding = embedding_fn_(node.content);
    }
}

void NeuralMemoryStore::update_index(const MemoryNode& node) {
    index_->reindex(node);
}

std::string NeuralMemoryStore::store(const std::string& content, MemoryType type) {
    MemoryNode node;
    node.content = content;
    node.type = type;
    return store(node);
}

std::string NeuralMemoryStore::store(const MemoryNode& node) {
    std::lock_guard<std::mutex> lock(mutex_);

    MemoryNode n = node;
    if (n.id.empty()) {
        n.id = generate_id();
    }

    n.created = std::chrono::system_clock::now();
    n.last_accessed = n.created;
    n.state = MemoryState::ACTIVE;
    n.strength = 1.0;

    generate_embedding(n);

    memories_[n.id] = n;
    associations_->add_node(n.id);
    index_->index(n);

    if (auto_consolidate_ && memories_.size() > max_memories_) {
        prune(0.1);
    }

    return n.id;
}

std::optional<MemoryNode> NeuralMemoryStore::retrieve(const std::string& id) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = memories_.find(id);
    if (it != memories_.end()) {
        return it->second;
    }
    return std::nullopt;
}

bool NeuralMemoryStore::update(const MemoryNode& node) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = memories_.find(node.id);
    if (it != memories_.end()) {
        it->second = node;
        update_index(node);
        return true;
    }
    return false;
}

bool NeuralMemoryStore::remove(const std::string& id) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (memories_.erase(id) > 0) {
        associations_->remove_node(id);
        index_->remove(id);
        working_memory_.erase(id);
        return true;
    }
    return false;
}

bool NeuralMemoryStore::exists(const std::string& id) const {
    std::lock_guard<std::mutex> lock(mutex_);
    return memories_.find(id) != memories_.end();
}

std::optional<MemoryNode> NeuralMemoryStore::access(const std::string& id) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = memories_.find(id);
    if (it != memories_.end()) {
        it->second.last_accessed = std::chrono::system_clock::now();
        it->second.access_count++;
        it->second.strength = std::min(1.0, it->second.strength + 0.1);
        return it->second;
    }
    return std::nullopt;
}

void NeuralMemoryStore::rehearse(const std::string& id) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = memories_.find(id);
    if (it != memories_.end()) {
        it->second.rehearsal_count++;
        it->second.strength = std::min(1.0, it->second.strength + 0.2);
        it->second.last_accessed = std::chrono::system_clock::now();
    }
}

std::vector<MemoryNode> NeuralMemoryStore::search(const std::string& query, int limit) {
    auto emb = embedding_fn_ ? embedding_fn_(query) : std::vector<float>{};
    auto ids = index_->search_text(query, limit);

    std::vector<MemoryNode> result;
    for (const auto& id : ids) {
        auto mem = retrieve(id);
        if (mem) result.push_back(*mem);
    }
    return result;
}

std::vector<MemoryNode> NeuralMemoryStore::similar(const std::string& id, int limit) {
    auto mem = retrieve(id);
    if (!mem || mem->embedding.empty()) return {};
    return similar(mem->embedding, limit);
}

std::vector<MemoryNode> NeuralMemoryStore::similar(const std::vector<float>& embedding, int limit) {
    auto ids = index_->search_embedding(embedding, limit);
    std::vector<MemoryNode> result;
    for (const auto& id : ids) {
        auto mem = retrieve(id);
        if (mem) result.push_back(*mem);
    }
    return result;
}

std::vector<MemoryNode> NeuralMemoryStore::by_type(MemoryType type, int limit) {
    auto ids = index_->search_type(type);
    std::vector<MemoryNode> result;
    for (size_t i = 0; i < ids.size() && static_cast<int>(i) < limit; i++) {
        auto mem = retrieve(ids[i]);
        if (mem) result.push_back(*mem);
    }
    return result;
}

std::vector<MemoryNode> NeuralMemoryStore::recent(int limit) {
    std::lock_guard<std::mutex> lock(mutex_);

    std::vector<std::pair<std::chrono::system_clock::time_point, std::string>> sorted;
    for (const auto& [id, mem] : memories_) {
        sorted.emplace_back(mem.last_accessed, id);
    }

    std::sort(sorted.begin(), sorted.end(),
        [](const auto& a, const auto& b) { return a.first > b.first; });

    std::vector<MemoryNode> result;
    for (size_t i = 0; i < sorted.size() && static_cast<int>(i) < limit; i++) {
        result.push_back(memories_[sorted[i].second]);
    }
    return result;
}

std::vector<MemoryNode> NeuralMemoryStore::strongest(int limit) {
    std::lock_guard<std::mutex> lock(mutex_);

    std::vector<std::pair<double, std::string>> sorted;
    for (const auto& [id, mem] : memories_) {
        sorted.emplace_back(mem.strength, id);
    }

    std::sort(sorted.begin(), sorted.end(),
        [](const auto& a, const auto& b) { return a.first > b.first; });

    std::vector<MemoryNode> result;
    for (size_t i = 0; i < sorted.size() && static_cast<int>(i) < limit; i++) {
        result.push_back(memories_[sorted[i].second]);
    }
    return result;
}

void NeuralMemoryStore::associate(const std::string& id1, const std::string& id2, double weight) {
    associations_->add_edge(id1, id2, weight);
}

void NeuralMemoryStore::dissociate(const std::string& id1, const std::string& id2) {
    associations_->remove_edge(id1, id2);
}

std::vector<MemoryNode> NeuralMemoryStore::associated_with(const std::string& id, int limit) {
    auto neighs = associations_->strongest_associations(id, limit);
    std::vector<MemoryNode> result;
    for (const auto& nid : neighs) {
        auto mem = retrieve(nid);
        if (mem) result.push_back(*mem);
    }
    return result;
}

std::vector<MemoryNode> NeuralMemoryStore::spreading_activation(const std::string& start_id,
                                                                 double threshold,
                                                                 int max_depth) {
    auto ids = associations_->spreading_activation(start_id, threshold, max_depth);
    std::vector<MemoryNode> result;
    for (const auto& id : ids) {
        auto mem = retrieve(id);
        if (mem) result.push_back(*mem);
    }
    return result;
}

void NeuralMemoryStore::auto_associate(const std::string& id, double min_similarity) {
    auto mem = retrieve(id);
    if (!mem || mem->embedding.empty()) return;

    auto similars = similar(mem->embedding, 20);
    for (const auto& other : similars) {
        if (other.id != id) {
            float sim = cosine_similarity(mem->embedding, other.embedding);
            if (sim >= min_similarity) {
                associate(id, other.id, sim);
            }
        }
    }
}

void NeuralMemoryStore::auto_associate_all(double min_similarity) {
    std::lock_guard<std::mutex> lock(mutex_);
    for (const auto& [id, _] : memories_) {
        auto_associate(id, min_similarity);
    }
}

void NeuralMemoryStore::consolidate() {
    std::lock_guard<std::mutex> lock(mutex_);

    std::vector<MemoryNode*> nodes;
    for (auto& [_, mem] : memories_) {
        nodes.push_back(&mem);
    }

    consolidator_->consolidate_batch(nodes);
}

void NeuralMemoryStore::dream() {
    std::lock_guard<std::mutex> lock(mutex_);

    std::vector<MemoryNode*> nodes;
    for (auto& [_, mem] : memories_) {
        nodes.push_back(&mem);
    }

    consolidator_->dream_cycle(nodes, *associations_);
}

void NeuralMemoryStore::decay(double factor) {
    std::lock_guard<std::mutex> lock(mutex_);

    for (auto& [_, mem] : memories_) {
        mem.strength *= factor;
    }
    associations_->decay_all_edges(factor);
}

void NeuralMemoryStore::prune(double min_strength) {
    std::lock_guard<std::mutex> lock(mutex_);

    std::vector<std::string> to_remove;
    for (const auto& [id, mem] : memories_) {
        if (mem.strength < min_strength && !consolidator_->should_keep(mem)) {
            to_remove.push_back(id);
        }
    }

    for (const auto& id : to_remove) {
        memories_.erase(id);
        associations_->remove_node(id);
        index_->remove(id);
    }

    associations_->prune_weak_edges(min_strength);
}

void NeuralMemoryStore::activate(const std::string& id) {
    std::lock_guard<std::mutex> lock(mutex_);
    working_memory_.insert(id);
    auto it = memories_.find(id);
    if (it != memories_.end()) {
        it->second.state = MemoryState::ACTIVE;
    }
}

void NeuralMemoryStore::deactivate(const std::string& id) {
    std::lock_guard<std::mutex> lock(mutex_);
    working_memory_.erase(id);
}

std::vector<MemoryNode> NeuralMemoryStore::active_memories() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<MemoryNode> result;
    for (const auto& id : working_memory_) {
        auto it = memories_.find(id);
        if (it != memories_.end()) {
            result.push_back(it->second);
        }
    }
    return result;
}

void NeuralMemoryStore::clear_working_memory() {
    std::lock_guard<std::mutex> lock(mutex_);
    working_memory_.clear();
}

MemorySnapshot NeuralMemoryStore::create_snapshot(const std::string& name) {
    std::lock_guard<std::mutex> lock(mutex_);

    MemorySnapshot snap;
    snap.id = generate_id();
    snap.name = name.empty() ? snap.id : name;
    snap.created = std::chrono::system_clock::now();

    for (const auto& [_, mem] : memories_) {
        snap.memories.push_back(mem);
    }

    for (const auto& node : associations_->all_nodes()) {
        for (const auto& [neighbor, weight] : associations_->neighbors(node)) {
            snap.associations[node][neighbor] = weight;
        }
    }

    return snap;
}

bool NeuralMemoryStore::restore_snapshot(const MemorySnapshot& snapshot) {
    std::lock_guard<std::mutex> lock(mutex_);

    memories_.clear();
    associations_ = std::make_unique<AssociationGraph>();
    index_ = std::make_unique<MemoryIndex>();
    working_memory_.clear();

    for (const auto& mem : snapshot.memories) {
        memories_[mem.id] = mem;
        associations_->add_node(mem.id);
        index_->index(mem);
    }

    for (const auto& [from, edges] : snapshot.associations) {
        for (const auto& [to, weight] : edges) {
            associations_->add_edge(from, to, weight);
        }
    }

    return true;
}

bool NeuralMemoryStore::save_to_file(const std::string& path) {
    auto snap = create_snapshot();

    std::ofstream f(path, std::ios::binary);
    if (!f) return false;

    // Simple serialization (would use proper format in production)
    f << snap.memories.size() << "\n";
    for (const auto& mem : snap.memories) {
        f << mem.id << "\n" << mem.content.size() << "\n" << mem.content << "\n";
        f << static_cast<int>(mem.type) << "\n" << mem.strength << "\n";
    }

    f.close();
    return true;
}

bool NeuralMemoryStore::load_from_file(const std::string& path) {
    std::ifstream f(path, std::ios::binary);
    if (!f) return false;

    MemorySnapshot snap;
    size_t count;
    f >> count;
    f.ignore();

    for (size_t i = 0; i < count; i++) {
        MemoryNode mem;
        std::getline(f, mem.id);
        size_t content_size;
        f >> content_size;
        f.ignore();
        mem.content.resize(content_size);
        f.read(&mem.content[0], content_size);
        f.ignore();
        int type_int;
        f >> type_int >> mem.strength;
        f.ignore();
        mem.type = static_cast<MemoryType>(type_int);
        snap.memories.push_back(mem);
    }

    return restore_snapshot(snap);
}

NeuralMemoryStore::Stats NeuralMemoryStore::get_stats() const {
    std::lock_guard<std::mutex> lock(mutex_);

    Stats s{};
    s.total_memories = memories_.size();
    s.association_count = associations_->edge_count();
    s.active_count = working_memory_.size();

    double total_strength = 0;
    for (const auto& [_, mem] : memories_) {
        total_strength += mem.strength;
        s.total_accesses += mem.access_count;

        switch (mem.type) {
            case MemoryType::EPISODIC: s.episodic_count++; break;
            case MemoryType::SEMANTIC: s.semantic_count++; break;
            case MemoryType::PROCEDURAL: s.procedural_count++; break;
            default: break;
        }
    }

    s.avg_strength = s.total_memories > 0 ? total_strength / s.total_memories : 0;
    return s;
}

void NeuralMemoryStore::set_embedding_function(
    std::function<std::vector<float>(const std::string&)> fn) {
    embedding_fn_ = fn;
}

void NeuralMemoryStore::set_max_memories(size_t max) {
    max_memories_ = max;
}

void NeuralMemoryStore::set_auto_consolidate(bool enable) {
    auto_consolidate_ = enable;
}

// ═══════════════════════════════════════════════════════════════════════════
//  CONTEXT MEMORY
// ═══════════════════════════════════════════════════════════════════════════

ContextMemory::ContextMemory(size_t max_items) : max_items_(max_items) {}

void ContextMemory::push(const std::string& memory_id, double relevance) {
    std::lock_guard<std::mutex> lock(mutex_);

    // Remove if already exists
    items_.erase(std::remove_if(items_.begin(), items_.end(),
        [&](const auto& p) { return p.first == memory_id; }), items_.end());

    items_.insert(items_.begin(), {memory_id, relevance});

    while (items_.size() > max_items_) {
        items_.pop_back();
    }
}

std::vector<std::pair<std::string, double>> ContextMemory::current() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return items_;
}

void ContextMemory::clear() {
    std::lock_guard<std::mutex> lock(mutex_);
    items_.clear();
}

std::string ContextMemory::summarize(const NeuralMemoryStore& store) const {
    std::lock_guard<std::mutex> lock(mutex_);

    std::ostringstream oss;
    for (const auto& [id, relevance] : items_) {
        auto mem = const_cast<NeuralMemoryStore&>(store).retrieve(id);
        if (mem) {
            oss << "- " << mem->content.substr(0, 100) << "...\n";
        }
    }
    return oss.str();
}

void ContextMemory::update_for_input(const std::string& input,
                                      NeuralMemoryStore& store,
                                      int fetch_limit) {
    auto relevant = store.search(input, fetch_limit);
    for (const auto& mem : relevant) {
        push(mem.id, mem.strength);
    }
}

// Global access
NeuralMemoryStore& memory() {
    return NeuralMemoryStore::instance();
}

} // namespace rael
