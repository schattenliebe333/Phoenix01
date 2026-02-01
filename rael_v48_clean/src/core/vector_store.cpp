// RAEL V49 - Vector Database Implementation
#include "rael/vector_store.h"
#include <algorithm>
#include <fstream>
#include <sstream>
#include <random>
#include <set>
#include <queue>
#include <cstring>
#include <iomanip>

namespace rael {

// ═══════════════════════════════════════════════════════════════════════════
//  DISTANCE CALCULATOR
// ═══════════════════════════════════════════════════════════════════════════

double DistanceCalculator::cosine(const Vector& a, const Vector& b) {
    if (a.size() != b.size() || a.empty()) return 0.0;

    double dot = 0, norm_a = 0, norm_b = 0;
    for (size_t i = 0; i < a.size(); i++) {
        dot += a[i] * b[i];
        norm_a += a[i] * a[i];
        norm_b += b[i] * b[i];
    }

    if (norm_a <= 0 || norm_b <= 0) return 0.0;
    return dot / (std::sqrt(norm_a) * std::sqrt(norm_b));
}

double DistanceCalculator::euclidean(const Vector& a, const Vector& b) {
    if (a.size() != b.size()) return std::numeric_limits<double>::max();

    double sum = 0;
    for (size_t i = 0; i < a.size(); i++) {
        double diff = a[i] - b[i];
        sum += diff * diff;
    }
    return std::sqrt(sum);
}

double DistanceCalculator::dot_product(const Vector& a, const Vector& b) {
    if (a.size() != b.size()) return 0.0;

    double dot = 0;
    for (size_t i = 0; i < a.size(); i++) {
        dot += a[i] * b[i];
    }
    return dot;
}

double DistanceCalculator::manhattan(const Vector& a, const Vector& b) {
    if (a.size() != b.size()) return std::numeric_limits<double>::max();

    double sum = 0;
    for (size_t i = 0; i < a.size(); i++) {
        sum += std::abs(a[i] - b[i]);
    }
    return sum;
}

double DistanceCalculator::calculate(const Vector& a, const Vector& b, DistanceMetric metric) {
    switch (metric) {
        case DistanceMetric::COSINE: return cosine(a, b);
        case DistanceMetric::EUCLIDEAN: return -euclidean(a, b);  // Negative for max-heap
        case DistanceMetric::DOT_PRODUCT: return dot_product(a, b);
        case DistanceMetric::MANHATTAN: return -manhattan(a, b);
        default: return cosine(a, b);
    }
}

Vector DistanceCalculator::normalize(const Vector& v) {
    Vector result = v;
    double norm = 0;
    for (float x : v) norm += x * x;
    norm = std::sqrt(norm);
    if (norm > 0) {
        for (float& x : result) x /= static_cast<float>(norm);
    }
    return result;
}

// ═══════════════════════════════════════════════════════════════════════════
//  SIMPLE EMBEDDER
// ═══════════════════════════════════════════════════════════════════════════

SimpleEmbedder::SimpleEmbedder(int dim) : dim_(dim) {}

Vector SimpleEmbedder::embed(const std::string& text) {
    Vector vec(dim_, 0.0f);

    // Simple hash-based embedding
    uint64_t hash = 0xcbf29ce484222325ULL;
    for (char c : text) {
        hash ^= static_cast<uint8_t>(c);
        hash *= 0x100000001b3ULL;
    }

    for (int i = 0; i < dim_; i++) {
        vec[i] = static_cast<float>(std::sin(hash * 0.0001 + i * 0.1));
        hash = (hash * 6364136223846793005ULL) + 1442695040888963407ULL;
    }

    return DistanceCalculator::normalize(vec);
}

std::vector<Vector> SimpleEmbedder::embed_batch(const std::vector<std::string>& texts) {
    std::vector<Vector> results;
    results.reserve(texts.size());
    for (const auto& text : texts) {
        results.push_back(embed(text));
    }
    return results;
}

// ═══════════════════════════════════════════════════════════════════════════
//  FLAT INDEX
// ═══════════════════════════════════════════════════════════════════════════

FlatIndex::FlatIndex(DistanceMetric metric) : metric_(metric) {}

void FlatIndex::add(const VectorId& id, const Vector& vec) {
    std::lock_guard<std::mutex> lock(mutex_);
    vectors_[id] = vec;
}

void FlatIndex::remove(const VectorId& id) {
    std::lock_guard<std::mutex> lock(mutex_);
    vectors_.erase(id);
}

bool FlatIndex::contains(const VectorId& id) const {
    std::lock_guard<std::mutex> lock(mutex_);
    return vectors_.find(id) != vectors_.end();
}

std::vector<std::pair<VectorId, double>> FlatIndex::search(
    const Vector& query, int k, double threshold) const {
    std::lock_guard<std::mutex> lock(mutex_);

    std::vector<std::pair<VectorId, double>> scores;
    for (const auto& [id, vec] : vectors_) {
        double score = DistanceCalculator::calculate(query, vec, metric_);
        if (score >= threshold) {
            scores.emplace_back(id, score);
        }
    }

    std::sort(scores.begin(), scores.end(),
              [](const auto& a, const auto& b) { return a.second > b.second; });

    if (static_cast<int>(scores.size()) > k) {
        scores.resize(k);
    }
    return scores;
}

size_t FlatIndex::size() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return vectors_.size();
}

void FlatIndex::clear() {
    std::lock_guard<std::mutex> lock(mutex_);
    vectors_.clear();
}

bool FlatIndex::save(const std::string& path) const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::ofstream f(path, std::ios::binary);
    if (!f) return false;

    size_t count = vectors_.size();
    f.write(reinterpret_cast<const char*>(&count), sizeof(count));

    for (const auto& [id, vec] : vectors_) {
        size_t id_len = id.size();
        f.write(reinterpret_cast<const char*>(&id_len), sizeof(id_len));
        f.write(id.data(), id_len);

        size_t vec_size = vec.size();
        f.write(reinterpret_cast<const char*>(&vec_size), sizeof(vec_size));
        f.write(reinterpret_cast<const char*>(vec.data()), vec_size * sizeof(float));
    }

    return true;
}

bool FlatIndex::load(const std::string& path) {
    std::lock_guard<std::mutex> lock(mutex_);
    std::ifstream f(path, std::ios::binary);
    if (!f) return false;

    vectors_.clear();

    size_t count;
    f.read(reinterpret_cast<char*>(&count), sizeof(count));

    for (size_t i = 0; i < count; i++) {
        size_t id_len;
        f.read(reinterpret_cast<char*>(&id_len), sizeof(id_len));
        std::string id(id_len, '\0');
        f.read(&id[0], id_len);

        size_t vec_size;
        f.read(reinterpret_cast<char*>(&vec_size), sizeof(vec_size));
        Vector vec(vec_size);
        f.read(reinterpret_cast<char*>(vec.data()), vec_size * sizeof(float));

        vectors_[id] = vec;
    }

    return true;
}

// ═══════════════════════════════════════════════════════════════════════════
//  HNSW INDEX
// ═══════════════════════════════════════════════════════════════════════════

HNSWIndex::HNSWIndex(int M, int ef_construction, DistanceMetric metric)
    : M_(M), ef_construction_(ef_construction), metric_(metric) {}

int HNSWIndex::select_level() {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_real_distribution<> dis(0.0, 1.0);

    double ml = 1.0 / std::log(M_);
    return static_cast<int>(-std::log(dis(gen)) * ml);
}

void HNSWIndex::add(const VectorId& id, const Vector& vec) {
    std::lock_guard<std::mutex> lock(mutex_);

    int level = select_level();
    Node node;
    node.id = id;
    node.vec = vec;
    node.neighbors.resize(level + 1);

    if (nodes_.empty()) {
        entry_point_ = id;
        max_level_ = level;
        nodes_[id] = node;
        return;
    }

    // Search from entry point down to level 0
    VectorId current = entry_point_;
    for (int l = max_level_; l > level; l--) {
        auto candidates = search_layer(vec, current, 1, l);
        if (!candidates.empty()) {
            current = candidates[0];
        }
    }

    // Insert at each level
    for (int l = std::min(level, max_level_); l >= 0; l--) {
        auto candidates = search_layer(vec, current, ef_construction_, l);

        // Select M best neighbors
        std::vector<std::pair<VectorId, double>> scored;
        for (const auto& c : candidates) {
            if (nodes_.count(c)) {
                double score = DistanceCalculator::calculate(vec, nodes_[c].vec, metric_);
                scored.emplace_back(c, score);
            }
        }
        std::sort(scored.begin(), scored.end(),
                  [](const auto& a, const auto& b) { return a.second > b.second; });

        int m_max = (l == 0) ? M_ * 2 : M_;
        for (int i = 0; i < std::min(m_max, static_cast<int>(scored.size())); i++) {
            node.neighbors[l].push_back(scored[i].first);
            // Add bidirectional edge
            if (nodes_[scored[i].first].neighbors.size() > static_cast<size_t>(l)) {
                nodes_[scored[i].first].neighbors[l].push_back(id);
            }
        }

        if (!candidates.empty()) {
            current = candidates[0];
        }
    }

    nodes_[id] = node;

    if (level > max_level_) {
        max_level_ = level;
        entry_point_ = id;
    }
}

void HNSWIndex::remove(const VectorId& id) {
    std::lock_guard<std::mutex> lock(mutex_);

    if (!nodes_.count(id)) return;

    // Remove from all neighbors
    for (auto& [_, node] : nodes_) {
        for (auto& level_neighbors : node.neighbors) {
            level_neighbors.erase(
                std::remove(level_neighbors.begin(), level_neighbors.end(), id),
                level_neighbors.end());
        }
    }

    nodes_.erase(id);

    if (entry_point_ == id && !nodes_.empty()) {
        entry_point_ = nodes_.begin()->first;
    }
}

bool HNSWIndex::contains(const VectorId& id) const {
    std::lock_guard<std::mutex> lock(mutex_);
    return nodes_.find(id) != nodes_.end();
}

std::vector<VectorId> HNSWIndex::search_layer(const Vector& query, const VectorId& entry,
                                               int ef, int level) const {
    std::set<VectorId> visited;
    std::priority_queue<std::pair<double, VectorId>> candidates;
    std::priority_queue<std::pair<double, VectorId>,
                       std::vector<std::pair<double, VectorId>>,
                       std::greater<>> result;

    auto it = nodes_.find(entry);
    if (it == nodes_.end()) return {};

    double entry_dist = DistanceCalculator::calculate(query, it->second.vec, metric_);
    candidates.emplace(entry_dist, entry);
    result.emplace(entry_dist, entry);
    visited.insert(entry);

    while (!candidates.empty()) {
        auto [dist, current] = candidates.top();
        candidates.pop();

        if (!result.empty() && dist < result.top().first) {
            break;
        }

        auto node_it = nodes_.find(current);
        if (node_it == nodes_.end()) continue;
        if (static_cast<int>(node_it->second.neighbors.size()) <= level) continue;

        for (const auto& neighbor : node_it->second.neighbors[level]) {
            if (visited.count(neighbor)) continue;
            visited.insert(neighbor);

            auto neighbor_it = nodes_.find(neighbor);
            if (neighbor_it == nodes_.end()) continue;

            double d = DistanceCalculator::calculate(query, neighbor_it->second.vec, metric_);

            if (static_cast<int>(result.size()) < ef || d > result.top().first) {
                candidates.emplace(d, neighbor);
                result.emplace(d, neighbor);
                if (static_cast<int>(result.size()) > ef) {
                    result.pop();
                }
            }
        }
    }

    std::vector<VectorId> output;
    while (!result.empty()) {
        output.push_back(result.top().second);
        result.pop();
    }
    std::reverse(output.begin(), output.end());
    return output;
}

std::vector<std::pair<VectorId, double>> HNSWIndex::search(
    const Vector& query, int k, double threshold) const {
    std::lock_guard<std::mutex> lock(mutex_);

    if (nodes_.empty()) return {};

    VectorId current = entry_point_;
    for (int l = max_level_; l > 0; l--) {
        auto candidates = search_layer(query, current, 1, l);
        if (!candidates.empty()) {
            current = candidates[0];
        }
    }

    auto candidates = search_layer(query, current, ef_search_, 0);

    std::vector<std::pair<VectorId, double>> results;
    for (const auto& id : candidates) {
        auto it = nodes_.find(id);
        if (it != nodes_.end()) {
            double score = DistanceCalculator::calculate(query, it->second.vec, metric_);
            if (score >= threshold) {
                results.emplace_back(id, score);
            }
        }
    }

    std::sort(results.begin(), results.end(),
              [](const auto& a, const auto& b) { return a.second > b.second; });

    if (static_cast<int>(results.size()) > k) {
        results.resize(k);
    }
    return results;
}

size_t HNSWIndex::size() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return nodes_.size();
}

void HNSWIndex::clear() {
    std::lock_guard<std::mutex> lock(mutex_);
    nodes_.clear();
    entry_point_.clear();
    max_level_ = 0;
}

bool HNSWIndex::save(const std::string& path) const {
    // Simplified save
    std::lock_guard<std::mutex> lock(mutex_);
    std::ofstream f(path, std::ios::binary);
    if (!f) return false;

    size_t count = nodes_.size();
    f.write(reinterpret_cast<const char*>(&count), sizeof(count));
    f.write(reinterpret_cast<const char*>(&max_level_), sizeof(max_level_));

    size_t ep_len = entry_point_.size();
    f.write(reinterpret_cast<const char*>(&ep_len), sizeof(ep_len));
    f.write(entry_point_.data(), ep_len);

    for (const auto& [id, node] : nodes_) {
        size_t id_len = id.size();
        f.write(reinterpret_cast<const char*>(&id_len), sizeof(id_len));
        f.write(id.data(), id_len);

        size_t vec_size = node.vec.size();
        f.write(reinterpret_cast<const char*>(&vec_size), sizeof(vec_size));
        f.write(reinterpret_cast<const char*>(node.vec.data()), vec_size * sizeof(float));
    }

    return true;
}

bool HNSWIndex::load(const std::string& path) {
    std::lock_guard<std::mutex> lock(mutex_);
    std::ifstream f(path, std::ios::binary);
    if (!f) return false;

    nodes_.clear();

    size_t count;
    f.read(reinterpret_cast<char*>(&count), sizeof(count));
    f.read(reinterpret_cast<char*>(&max_level_), sizeof(max_level_));

    size_t ep_len;
    f.read(reinterpret_cast<char*>(&ep_len), sizeof(ep_len));
    entry_point_.resize(ep_len);
    f.read(&entry_point_[0], ep_len);

    for (size_t i = 0; i < count; i++) {
        size_t id_len;
        f.read(reinterpret_cast<char*>(&id_len), sizeof(id_len));
        std::string id(id_len, '\0');
        f.read(&id[0], id_len);

        Node node;
        node.id = id;

        size_t vec_size;
        f.read(reinterpret_cast<char*>(&vec_size), sizeof(vec_size));
        node.vec.resize(vec_size);
        f.read(reinterpret_cast<char*>(node.vec.data()), vec_size * sizeof(float));

        nodes_[id] = node;
    }

    return true;
}

// ═══════════════════════════════════════════════════════════════════════════
//  LSH INDEX
// ═══════════════════════════════════════════════════════════════════════════

LSHIndex::LSHIndex(int num_tables, int num_hashes, int dim)
    : num_tables_(num_tables), num_hashes_(num_hashes), dim_(dim) {
    // Initialize random projections
    std::random_device rd;
    std::mt19937 gen(rd());
    std::normal_distribution<float> dis(0.0f, 1.0f);

    hash_functions_.resize(num_tables_);
    tables_.resize(num_tables_);

    for (int t = 0; t < num_tables_; t++) {
        hash_functions_[t].resize(num_hashes_);
        for (int h = 0; h < num_hashes_; h++) {
            hash_functions_[t][h].resize(dim_);
            for (int d = 0; d < dim_; d++) {
                hash_functions_[t][h][d] = dis(gen);
            }
        }
    }
}

uint64_t LSHIndex::hash_vector(const Vector& vec, int table_idx) const {
    uint64_t hash = 0;
    for (int h = 0; h < num_hashes_; h++) {
        float dot = 0;
        const auto& proj = hash_functions_[table_idx][h];
        for (size_t d = 0; d < vec.size() && d < proj.size(); d++) {
            dot += vec[d] * proj[d];
        }
        if (dot > 0) {
            hash |= (1ULL << h);
        }
    }
    return hash;
}

void LSHIndex::add(const VectorId& id, const Vector& vec) {
    std::lock_guard<std::mutex> lock(mutex_);

    vectors_[id] = vec;

    for (int t = 0; t < num_tables_; t++) {
        uint64_t h = hash_vector(vec, t);
        tables_[t][h].push_back(id);
    }
}

void LSHIndex::remove(const VectorId& id) {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = vectors_.find(id);
    if (it == vectors_.end()) return;

    for (int t = 0; t < num_tables_; t++) {
        uint64_t h = hash_vector(it->second, t);
        auto& bucket = tables_[t][h];
        bucket.erase(std::remove(bucket.begin(), bucket.end(), id), bucket.end());
    }

    vectors_.erase(id);
}

bool LSHIndex::contains(const VectorId& id) const {
    std::lock_guard<std::mutex> lock(mutex_);
    return vectors_.find(id) != vectors_.end();
}

std::vector<std::pair<VectorId, double>> LSHIndex::search(
    const Vector& query, int k, double threshold) const {
    std::lock_guard<std::mutex> lock(mutex_);

    std::set<VectorId> candidates;

    for (int t = 0; t < num_tables_; t++) {
        uint64_t h = hash_vector(query, t);
        auto it = tables_[t].find(h);
        if (it != tables_[t].end()) {
            for (const auto& id : it->second) {
                candidates.insert(id);
            }
        }
    }

    std::vector<std::pair<VectorId, double>> results;
    for (const auto& id : candidates) {
        auto it = vectors_.find(id);
        if (it != vectors_.end()) {
            double score = DistanceCalculator::cosine(query, it->second);
            if (score >= threshold) {
                results.emplace_back(id, score);
            }
        }
    }

    std::sort(results.begin(), results.end(),
              [](const auto& a, const auto& b) { return a.second > b.second; });

    if (static_cast<int>(results.size()) > k) {
        results.resize(k);
    }
    return results;
}

size_t LSHIndex::size() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return vectors_.size();
}

void LSHIndex::clear() {
    std::lock_guard<std::mutex> lock(mutex_);
    vectors_.clear();
    for (auto& table : tables_) {
        table.clear();
    }
}

bool LSHIndex::save(const std::string& path) const {
    // Would implement proper serialization
    return true;
}

bool LSHIndex::load(const std::string& path) {
    return true;
}

// ═══════════════════════════════════════════════════════════════════════════
//  METADATA FILTER
// ═══════════════════════════════════════════════════════════════════════════

bool MetadataFilter::matches(const std::map<std::string, std::string>& metadata) const {
    auto it = metadata.find(field);
    if (it == metadata.end()) return false;

    switch (op) {
        case Op::EQ: return it->second == value;
        case Op::NE: return it->second != value;
        case Op::GT: return it->second > value;
        case Op::LT: return it->second < value;
        case Op::GTE: return it->second >= value;
        case Op::LTE: return it->second <= value;
        case Op::IN:
            return std::find(values.begin(), values.end(), it->second) != values.end();
        case Op::CONTAINS:
            return it->second.find(value) != std::string::npos;
        default: return false;
    }
}

FilterBuilder& FilterBuilder::eq(const std::string& field, const std::string& value) {
    MetadataFilter f;
    f.field = field;
    f.op = MetadataFilter::Op::EQ;
    f.value = value;
    filters_.push_back(f);
    return *this;
}

FilterBuilder& FilterBuilder::ne(const std::string& field, const std::string& value) {
    MetadataFilter f;
    f.field = field;
    f.op = MetadataFilter::Op::NE;
    f.value = value;
    filters_.push_back(f);
    return *this;
}

FilterBuilder& FilterBuilder::gt(const std::string& field, const std::string& value) {
    MetadataFilter f;
    f.field = field;
    f.op = MetadataFilter::Op::GT;
    f.value = value;
    filters_.push_back(f);
    return *this;
}

FilterBuilder& FilterBuilder::lt(const std::string& field, const std::string& value) {
    MetadataFilter f;
    f.field = field;
    f.op = MetadataFilter::Op::LT;
    f.value = value;
    filters_.push_back(f);
    return *this;
}

FilterBuilder& FilterBuilder::in(const std::string& field, const std::vector<std::string>& values) {
    MetadataFilter f;
    f.field = field;
    f.op = MetadataFilter::Op::IN;
    f.values = values;
    filters_.push_back(f);
    return *this;
}

FilterBuilder& FilterBuilder::contains(const std::string& field, const std::string& value) {
    MetadataFilter f;
    f.field = field;
    f.op = MetadataFilter::Op::CONTAINS;
    f.value = value;
    filters_.push_back(f);
    return *this;
}

std::vector<MetadataFilter> FilterBuilder::build() const {
    return filters_;
}

// ═══════════════════════════════════════════════════════════════════════════
//  VECTOR COLLECTION
// ═══════════════════════════════════════════════════════════════════════════

VectorCollection::VectorCollection(const CollectionConfig& config) : config_(config) {
    if (config.index_type == "flat") {
        index_ = std::make_unique<FlatIndex>(config.metric);
    } else if (config.index_type == "lsh") {
        index_ = std::make_unique<LSHIndex>(config.lsh_tables, config.lsh_hashes, config.dimension);
    } else {
        index_ = std::make_unique<HNSWIndex>(config.hnsw_m, config.hnsw_ef, config.metric);
    }

    embedder_ = std::make_shared<SimpleEmbedder>(config.dimension);
}

void VectorCollection::upsert(const VectorEntry& entry) {
    std::lock_guard<std::mutex> lock(mutex_);

    VectorEntry e = entry;

    // Generate embedding if not provided
    if (e.embedding.empty() && !e.content.empty() && embedder_) {
        e.embedding = embedder_->embed(e.content);
    }

    // Remove old entry if exists
    if (entries_.count(e.id)) {
        index_->remove(e.id);
    }

    entries_[e.id] = e;
    if (!e.embedding.empty()) {
        index_->add(e.id, e.embedding);
    }

    // Index keywords
    index_keywords(e.id, e.content);
}

void VectorCollection::upsert_batch(const std::vector<VectorEntry>& entries) {
    for (const auto& entry : entries) {
        upsert(entry);
    }
}

std::optional<VectorEntry> VectorCollection::get(const VectorId& id) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = entries_.find(id);
    if (it != entries_.end()) {
        return it->second;
    }
    return std::nullopt;
}

bool VectorCollection::remove(const VectorId& id) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (entries_.erase(id) > 0) {
        index_->remove(id);
        return true;
    }
    return false;
}

bool VectorCollection::exists(const VectorId& id) const {
    std::lock_guard<std::mutex> lock(mutex_);
    return entries_.find(id) != entries_.end();
}

std::vector<SearchResult> VectorCollection::search(
    const Vector& query, int k, const std::vector<MetadataFilter>& filters) const {
    std::lock_guard<std::mutex> lock(mutex_);

    auto candidates = index_->search(query, k * 2);  // Over-fetch for filtering

    std::vector<SearchResult> results;
    for (const auto& [id, score] : candidates) {
        auto it = entries_.find(id);
        if (it == entries_.end()) continue;

        // Apply filters
        bool pass = true;
        for (const auto& filter : filters) {
            if (!filter.matches(it->second.metadata)) {
                pass = false;
                break;
            }
        }

        if (pass) {
            SearchResult r;
            r.id = id;
            r.score = score;
            r.metadata = it->second.metadata;
            r.content = it->second.content;
            results.push_back(r);
        }

        if (static_cast<int>(results.size()) >= k) break;
    }

    return results;
}

std::vector<SearchResult> VectorCollection::search_text(
    const std::string& text, int k, const std::vector<MetadataFilter>& filters) {
    if (!embedder_) return {};
    Vector query = embedder_->embed(text);
    return search(query, k, filters);
}

void VectorCollection::index_keywords(const VectorId& id, const std::string& content) {
    std::istringstream iss(content);
    std::string word;
    while (iss >> word) {
        // Simple normalization
        std::transform(word.begin(), word.end(), word.begin(), ::tolower);
        word.erase(std::remove_if(word.begin(), word.end(),
                   [](char c) { return !std::isalnum(c); }), word.end());
        if (!word.empty()) {
            keyword_index_[word].insert(id);
        }
    }
}

std::vector<SearchResult> VectorCollection::hybrid_search(
    const std::string& text, int k, double alpha) {

    std::lock_guard<std::mutex> lock(mutex_);

    // Vector search
    std::map<VectorId, double> scores;

    if (embedder_) {
        Vector query = embedder_->embed(text);
        auto vec_results = index_->search(query, k * 2);
        for (const auto& [id, score] : vec_results) {
            scores[id] = alpha * score;
        }
    }

    // Keyword search
    std::istringstream iss(text);
    std::string word;
    std::map<VectorId, int> keyword_hits;

    while (iss >> word) {
        std::transform(word.begin(), word.end(), word.begin(), ::tolower);
        auto it = keyword_index_.find(word);
        if (it != keyword_index_.end()) {
            for (const auto& id : it->second) {
                keyword_hits[id]++;
            }
        }
    }

    int max_hits = 1;
    for (const auto& [_, hits] : keyword_hits) {
        max_hits = std::max(max_hits, hits);
    }

    for (const auto& [id, hits] : keyword_hits) {
        scores[id] += (1.0 - alpha) * (static_cast<double>(hits) / max_hits);
    }

    // Sort by score
    std::vector<std::pair<VectorId, double>> sorted(scores.begin(), scores.end());
    std::sort(sorted.begin(), sorted.end(),
              [](const auto& a, const auto& b) { return a.second > b.second; });

    std::vector<SearchResult> results;
    for (const auto& [id, score] : sorted) {
        auto it = entries_.find(id);
        if (it != entries_.end()) {
            SearchResult r;
            r.id = id;
            r.score = score;
            r.metadata = it->second.metadata;
            r.content = it->second.content;
            results.push_back(r);
        }
        if (static_cast<int>(results.size()) >= k) break;
    }

    return results;
}

size_t VectorCollection::count() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return entries_.size();
}

void VectorCollection::set_embedder(std::shared_ptr<EmbeddingProvider> embedder) {
    embedder_ = embedder;
}

bool VectorCollection::save(const std::string& dir) const {
    return index_->save(dir + "/" + config_.name + ".idx");
}

bool VectorCollection::load(const std::string& dir) {
    return index_->load(dir + "/" + config_.name + ".idx");
}

// ═══════════════════════════════════════════════════════════════════════════
//  VECTOR DATABASE
// ═══════════════════════════════════════════════════════════════════════════

VectorDatabase::VectorDatabase() {
    embedder_ = std::make_shared<SimpleEmbedder>();
}

VectorDatabase::~VectorDatabase() = default;

VectorDatabase& VectorDatabase::instance() {
    static VectorDatabase inst;
    return inst;
}

bool VectorDatabase::create_collection(const CollectionConfig& config) {
    std::lock_guard<std::mutex> lock(mutex_);

    if (collections_.count(config.name)) return false;

    auto coll = std::make_shared<VectorCollection>(config);
    coll->set_embedder(embedder_);
    collections_[config.name] = coll;

    if (default_collection_name_.empty()) {
        default_collection_name_ = config.name;
    }

    return true;
}

bool VectorDatabase::delete_collection(const std::string& name) {
    std::lock_guard<std::mutex> lock(mutex_);
    return collections_.erase(name) > 0;
}

std::shared_ptr<VectorCollection> VectorDatabase::get_collection(const std::string& name) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = collections_.find(name);
    return it != collections_.end() ? it->second : nullptr;
}

std::vector<std::string> VectorDatabase::list_collections() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<std::string> names;
    for (const auto& [name, _] : collections_) {
        names.push_back(name);
    }
    return names;
}

bool VectorDatabase::collection_exists(const std::string& name) const {
    std::lock_guard<std::mutex> lock(mutex_);
    return collections_.find(name) != collections_.end();
}

void VectorDatabase::set_default_collection(const std::string& name) {
    default_collection_name_ = name;
}

std::shared_ptr<VectorCollection> VectorDatabase::default_collection() {
    return get_collection(default_collection_name_);
}

void VectorDatabase::upsert(const VectorEntry& entry) {
    auto coll = default_collection();
    if (coll) coll->upsert(entry);
}

std::optional<VectorEntry> VectorDatabase::get(const VectorId& id) {
    auto coll = default_collection();
    return coll ? coll->get(id) : std::nullopt;
}

bool VectorDatabase::remove(const VectorId& id) {
    auto coll = default_collection();
    return coll ? coll->remove(id) : false;
}

std::vector<SearchResult> VectorDatabase::search(const Vector& query, int k) {
    auto coll = default_collection();
    return coll ? coll->search(query, k) : std::vector<SearchResult>{};
}

std::vector<SearchResult> VectorDatabase::search_text(const std::string& text, int k) {
    auto coll = default_collection();
    return coll ? coll->search_text(text, k) : std::vector<SearchResult>{};
}

void VectorDatabase::set_embedder(std::shared_ptr<EmbeddingProvider> embedder) {
    std::lock_guard<std::mutex> lock(mutex_);
    embedder_ = embedder;
    for (auto& [_, coll] : collections_) {
        coll->set_embedder(embedder);
    }
}

std::shared_ptr<EmbeddingProvider> VectorDatabase::get_embedder() const {
    return embedder_;
}

Vector VectorDatabase::embed(const std::string& text) {
    return embedder_ ? embedder_->embed(text) : Vector{};
}

void VectorDatabase::set_storage_path(const std::string& path) {
    storage_path_ = path;
}

bool VectorDatabase::save_all() const {
    std::lock_guard<std::mutex> lock(mutex_);
    for (const auto& [_, coll] : collections_) {
        coll->save(storage_path_);
    }
    return true;
}

bool VectorDatabase::load_all() {
    std::lock_guard<std::mutex> lock(mutex_);
    for (auto& [_, coll] : collections_) {
        coll->load(storage_path_);
    }
    return true;
}

VectorDatabase::Stats VectorDatabase::get_stats() const {
    std::lock_guard<std::mutex> lock(mutex_);
    Stats s{};
    s.collection_count = collections_.size();
    for (const auto& [_, coll] : collections_) {
        s.total_vectors += coll->count();
    }
    return s;
}

// ═══════════════════════════════════════════════════════════════════════════
//  DOCUMENT CHUNKER
// ═══════════════════════════════════════════════════════════════════════════

DocumentChunker::DocumentChunker(int chunk_size, int overlap)
    : chunk_size_(chunk_size), overlap_(overlap) {}

std::vector<Chunk> DocumentChunker::chunk(const std::string& document,
                                          const std::string& doc_id,
                                          const std::map<std::string, std::string>& metadata) {
    std::vector<Chunk> chunks;

    int start = 0;
    int chunk_num = 0;

    while (start < static_cast<int>(document.size())) {
        int end = std::min(start + chunk_size_, static_cast<int>(document.size()));

        // Try to break at word boundary
        if (end < static_cast<int>(document.size())) {
            int last_space = document.rfind(' ', end);
            if (last_space > start) {
                end = last_space;
            }
        }

        Chunk c;
        c.id = doc_id.empty() ? std::to_string(chunk_num) : doc_id + "_" + std::to_string(chunk_num);
        c.text = document.substr(start, end - start);
        c.start_char = start;
        c.end_char = end;
        c.metadata = metadata;
        c.metadata["chunk_num"] = std::to_string(chunk_num);
        c.metadata["doc_id"] = doc_id;

        chunks.push_back(c);

        start = end - overlap_;
        if (start < 0) start = 0;
        chunk_num++;

        if (end >= static_cast<int>(document.size())) break;
    }

    return chunks;
}

std::vector<Chunk> DocumentChunker::chunk_by_sentences(const std::string& document) {
    std::vector<Chunk> chunks;
    // Simple sentence splitting
    std::vector<std::string> sentences;
    std::string current;

    for (char c : document) {
        current += c;
        if (c == '.' || c == '!' || c == '?') {
            sentences.push_back(current);
            current.clear();
        }
    }
    if (!current.empty()) sentences.push_back(current);

    // Group sentences into chunks
    std::string chunk_text;
    int chunk_num = 0;
    int start = 0;

    for (const auto& sentence : sentences) {
        if (static_cast<int>(chunk_text.size() + sentence.size()) > chunk_size_ && !chunk_text.empty()) {
            Chunk c;
            c.id = std::to_string(chunk_num++);
            c.text = chunk_text;
            c.start_char = start;
            chunks.push_back(c);
            start += chunk_text.size();
            chunk_text.clear();
        }
        chunk_text += sentence;
    }

    if (!chunk_text.empty()) {
        Chunk c;
        c.id = std::to_string(chunk_num);
        c.text = chunk_text;
        c.start_char = start;
        chunks.push_back(c);
    }

    return chunks;
}

std::vector<Chunk> DocumentChunker::chunk_by_paragraphs(const std::string& document) {
    std::vector<Chunk> chunks;
    std::istringstream iss(document);
    std::string line;
    std::string paragraph;
    int chunk_num = 0;
    int start = 0;

    while (std::getline(iss, line)) {
        if (line.empty()) {
            if (!paragraph.empty()) {
                Chunk c;
                c.id = std::to_string(chunk_num++);
                c.text = paragraph;
                c.start_char = start;
                chunks.push_back(c);
                start += paragraph.size() + 1;
                paragraph.clear();
            }
        } else {
            if (!paragraph.empty()) paragraph += "\n";
            paragraph += line;
        }
    }

    if (!paragraph.empty()) {
        Chunk c;
        c.id = std::to_string(chunk_num);
        c.text = paragraph;
        c.start_char = start;
        chunks.push_back(c);
    }

    return chunks;
}

std::vector<Chunk> DocumentChunker::chunk_by_tokens(const std::string& document, int max_tokens) {
    // Simple word-based approximation (1 token ≈ 4 chars)
    int approx_chars = max_tokens * 4;
    chunk_size_ = approx_chars;
    return chunk(document);
}

// ═══════════════════════════════════════════════════════════════════════════
//  RAG HELPER
// ═══════════════════════════════════════════════════════════════════════════

RAGHelper::RAGHelper(VectorDatabase& db, const std::string& collection)
    : db_(db), collection_(collection) {

    if (!collection_.empty() && !db_.collection_exists(collection_)) {
        CollectionConfig config;
        config.name = collection_;
        db_.create_collection(config);
    }
}

void RAGHelper::index_document(const std::string& doc_id, const std::string& content,
                              const std::map<std::string, std::string>& metadata) {
    auto chunks = chunker_.chunk(content, doc_id, metadata);

    auto coll = db_.get_collection(collection_);
    if (!coll) return;

    for (const auto& chunk : chunks) {
        VectorEntry entry;
        entry.id = chunk.id;
        entry.content = chunk.text;
        entry.metadata = chunk.metadata;
        coll->upsert(entry);
    }
}

std::string RAGHelper::retrieve_context(const std::string& query, int k) {
    auto chunks = retrieve_chunks(query, k);
    std::ostringstream oss;
    for (const auto& chunk : chunks) {
        oss << chunk.text << "\n\n";
    }
    return oss.str();
}

std::vector<Chunk> RAGHelper::retrieve_chunks(const std::string& query, int k) {
    auto coll = db_.get_collection(collection_);
    if (!coll) return {};

    auto results = coll->search_text(query, k);

    std::vector<Chunk> chunks;
    for (const auto& r : results) {
        Chunk c;
        c.id = r.id;
        c.text = r.content;
        c.metadata = r.metadata;
        chunks.push_back(c);
    }
    return chunks;
}

std::string RAGHelper::build_prompt(const std::string& query, const std::string& system_prompt,
                                   int context_chunks) {
    std::string context = retrieve_context(query, context_chunks);

    std::ostringstream oss;
    if (!system_prompt.empty()) {
        oss << system_prompt << "\n\n";
    }
    oss << "Context:\n" << context << "\n\n";
    oss << "Question: " << query << "\n\nAnswer:";

    return oss.str();
}

void RAGHelper::set_chunk_size(int size) {
    chunker_.set_chunk_size(size);
}

void RAGHelper::set_overlap(int overlap) {
    chunker_.set_overlap(overlap);
}

// Global access
VectorDatabase& vectordb() {
    return VectorDatabase::instance();
}

} // namespace rael
