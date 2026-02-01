// RAEL V49 - Knowledge Graph Implementation (#26)
#include "rael/knowledge_graph.h"
#include <algorithm>
#include <queue>
#include <stack>
#include <sstream>
#include <cmath>
#include <random>
#include <iomanip>

namespace rael {

// ═══════════════════════════════════════════════════════════════════════════
//  NODE IMPLEMENTATION
// ═══════════════════════════════════════════════════════════════════════════

bool Node::has_property(const std::string& key) const {
    return properties.find(key) != properties.end();
}

std::optional<PropertyValue> Node::get_property(const std::string& key) const {
    auto it = properties.find(key);
    if (it != properties.end()) return it->second;
    return std::nullopt;
}

void Node::set_property(const std::string& key, const PropertyValue& value) {
    properties[key] = value;
}

// ═══════════════════════════════════════════════════════════════════════════
//  TRIPLE IMPLEMENTATION
// ═══════════════════════════════════════════════════════════════════════════

Triple Triple::from_edge(const Node& from, const Edge& edge, const Node& to) {
    Triple t;
    t.subject = from.label;
    t.predicate = edge.custom_label.empty() ?
        edge_type_to_string(edge.type) : edge.custom_label;
    t.object = to.label;
    t.confidence = edge.confidence;
    return t;
}

std::string Triple::to_string() const {
    std::ostringstream oss;
    oss << "(" << subject << ") --[" << predicate << "]--> (" << object << ")";
    if (confidence < 1.0) {
        oss << " [conf: " << std::fixed << std::setprecision(2) << confidence << "]";
    }
    return oss.str();
}

// ═══════════════════════════════════════════════════════════════════════════
//  INFERENCE ENGINE
// ═══════════════════════════════════════════════════════════════════════════

InferenceEngine::InferenceEngine() {
    // Add default rules
    add_transitivity_rule(EdgeType::IS_A);
    add_transitivity_rule(EdgeType::PART_OF);
    add_symmetry_rule(EdgeType::SIMILAR_TO);
    add_symmetry_rule(EdgeType::RELATED_TO);
    add_inverse_rule(EdgeType::CAUSES, EdgeType::CAUSED_BY);
    add_inverse_rule(EdgeType::BEFORE, EdgeType::AFTER);
    add_inverse_rule(EdgeType::PART_OF, EdgeType::HAS_PART);
}

void InferenceEngine::add_rule(const InferenceRule& rule) {
    std::lock_guard<std::mutex> lock(mutex_);
    rules_.push_back(rule);
}

void InferenceEngine::remove_rule(const std::string& id) {
    std::lock_guard<std::mutex> lock(mutex_);
    rules_.erase(
        std::remove_if(rules_.begin(), rules_.end(),
            [&id](const InferenceRule& r) { return r.id == id; }),
        rules_.end());
}

void InferenceEngine::enable_rule(const std::string& id, bool enabled) {
    std::lock_guard<std::mutex> lock(mutex_);
    for (auto& r : rules_) {
        if (r.id == id) {
            r.enabled = enabled;
            break;
        }
    }
}

std::vector<InferenceRule> InferenceEngine::list_rules() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return rules_;
}

void InferenceEngine::add_transitivity_rule(EdgeType edge_type) {
    InferenceRule rule;
    rule.id = "transitivity_" + edge_type_to_string(edge_type);
    rule.name = "Transitivity for " + edge_type_to_string(edge_type);
    // If A -> B and B -> C, then A -> C
    QueryPattern p1, p2, cons;
    p1.subject = "?a";
    p1.predicate = edge_type;
    p1.object = "?b";
    p2.subject = "?b";
    p2.predicate = edge_type;
    p2.object = "?c";
    cons.subject = "?a";
    cons.predicate = edge_type;
    cons.object = "?c";
    rule.antecedent = {p1, p2};
    rule.consequent = cons;
    rule.confidence_factor = 0.9;
    add_rule(rule);
}

void InferenceEngine::add_symmetry_rule(EdgeType edge_type) {
    InferenceRule rule;
    rule.id = "symmetry_" + edge_type_to_string(edge_type);
    rule.name = "Symmetry for " + edge_type_to_string(edge_type);
    // If A -> B then B -> A
    QueryPattern p1, cons;
    p1.subject = "?a";
    p1.predicate = edge_type;
    p1.object = "?b";
    cons.subject = "?b";
    cons.predicate = edge_type;
    cons.object = "?a";
    rule.antecedent = {p1};
    rule.consequent = cons;
    rule.confidence_factor = 1.0;
    add_rule(rule);
}

void InferenceEngine::add_inverse_rule(EdgeType type1, EdgeType type2) {
    InferenceRule rule;
    rule.id = "inverse_" + edge_type_to_string(type1) + "_" + edge_type_to_string(type2);
    rule.name = "Inverse: " + edge_type_to_string(type1) + " <-> " + edge_type_to_string(type2);
    QueryPattern p1, cons;
    p1.subject = "?a";
    p1.predicate = type1;
    p1.object = "?b";
    cons.subject = "?b";
    cons.predicate = type2;
    cons.object = "?a";
    rule.antecedent = {p1};
    rule.consequent = cons;
    rule.confidence_factor = 1.0;
    add_rule(rule);
}

void InferenceEngine::add_inheritance_rule() {
    InferenceRule rule;
    rule.id = "inheritance";
    rule.name = "Property inheritance through IS_A";
    // If A IS_A B and B HAS_PROPERTY P, then A HAS_PROPERTY P
    QueryPattern p1, p2, cons;
    p1.subject = "?a";
    p1.predicate = EdgeType::IS_A;
    p1.object = "?b";
    p2.subject = "?b";
    p2.predicate = EdgeType::HAS_PROPERTY;
    p2.object = "?p";
    cons.subject = "?a";
    cons.predicate = EdgeType::HAS_PROPERTY;
    cons.object = "?p";
    rule.antecedent = {p1, p2};
    rule.consequent = cons;
    rule.confidence_factor = 0.95;
    add_rule(rule);
}

std::vector<Triple> InferenceEngine::infer(const std::vector<Node>& nodes,
                                            const std::vector<Edge>& edges) {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<Triple> inferred;

    // Build lookup maps
    std::map<std::string, const Node*> node_map;
    for (const auto& n : nodes) {
        node_map[n.id] = &n;
    }

    // Group edges by type and endpoints
    std::map<std::pair<EdgeType, std::string>, std::vector<std::string>> edge_map;
    for (const auto& e : edges) {
        edge_map[{e.type, e.from_id}].push_back(e.to_id);
    }

    // Apply each rule
    for (const auto& rule : rules_) {
        if (!rule.enabled) continue;

        // Handle simple symmetry rules
        if (rule.antecedent.size() == 1 && rule.consequent.predicate == rule.antecedent[0].predicate) {
            for (const auto& e : edges) {
                if (rule.antecedent[0].predicate && e.type == *rule.antecedent[0].predicate) {
                    Triple t;
                    auto from_it = node_map.find(e.to_id);
                    auto to_it = node_map.find(e.from_id);
                    if (from_it != node_map.end() && to_it != node_map.end()) {
                        t.subject = from_it->second->label;
                        t.predicate = edge_type_to_string(e.type);
                        t.object = to_it->second->label;
                        t.confidence = e.confidence * rule.confidence_factor;
                        inferred.push_back(t);
                    }
                }
            }
        }

        // Handle transitivity rules (2 antecedents)
        if (rule.antecedent.size() == 2 &&
            rule.antecedent[0].predicate == rule.antecedent[1].predicate &&
            rule.consequent.predicate == rule.antecedent[0].predicate) {

            EdgeType etype = *rule.antecedent[0].predicate;
            for (const auto& e1 : edges) {
                if (e1.type != etype) continue;
                for (const auto& e2 : edges) {
                    if (e2.type != etype) continue;
                    if (e1.to_id == e2.from_id && e1.from_id != e2.to_id) {
                        Triple t;
                        auto from_it = node_map.find(e1.from_id);
                        auto to_it = node_map.find(e2.to_id);
                        if (from_it != node_map.end() && to_it != node_map.end()) {
                            t.subject = from_it->second->label;
                            t.predicate = edge_type_to_string(etype);
                            t.object = to_it->second->label;
                            t.confidence = e1.confidence * e2.confidence * rule.confidence_factor;
                            inferred.push_back(t);
                        }
                    }
                }
            }
        }
    }

    inference_count_ += inferred.size();
    return inferred;
}

std::vector<Triple> InferenceEngine::forward_chain(const std::vector<Node>& nodes,
                                                    const std::vector<Edge>& edges,
                                                    int max_iterations) {
    std::vector<Triple> all_inferred;
    std::set<std::string> seen;

    std::vector<Edge> current_edges = edges;

    for (int i = 0; i < max_iterations; ++i) {
        auto new_triples = infer(nodes, current_edges);
        bool added_new = false;

        for (const auto& t : new_triples) {
            std::string key = t.subject + "|" + t.predicate + "|" + t.object;
            if (seen.find(key) == seen.end()) {
                seen.insert(key);
                all_inferred.push_back(t);
                added_new = true;

                // Convert to edge for next iteration
                Edge e;
                e.id = "inferred_" + std::to_string(all_inferred.size());
                // Find node IDs by label
                for (const auto& n : nodes) {
                    if (n.label == t.subject) e.from_id = n.id;
                    if (n.label == t.object) e.to_id = n.id;
                }
                e.type = string_to_edge_type(t.predicate);
                e.confidence = t.confidence;
                if (!e.from_id.empty() && !e.to_id.empty()) {
                    current_edges.push_back(e);
                }
            }
        }

        if (!added_new) break;
    }

    return all_inferred;
}

std::vector<std::string> InferenceEngine::explain_inference(const Triple& inferred) {
    std::vector<std::string> explanation;
    explanation.push_back("Triple: " + inferred.to_string());
    explanation.push_back("Inferred with confidence: " +
        std::to_string(inferred.confidence));
    // In a full implementation, would track which rules produced this
    explanation.push_back("Applied rules: transitivity, symmetry, or inverse mapping");
    return explanation;
}

size_t InferenceEngine::rule_count() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return rules_.size();
}

size_t InferenceEngine::inferences_made() const {
    return inference_count_;
}

bool InferenceEngine::matches_pattern(const Node& node, const Edge& edge,
                                       const QueryPattern& pattern) const {
    if (pattern.predicate && edge.type != *pattern.predicate) return false;
    // Additional matching logic
    return true;
}

// ═══════════════════════════════════════════════════════════════════════════
//  ONTOLOGY
// ═══════════════════════════════════════════════════════════════════════════

Ontology::Ontology(const std::string& namespace_uri)
    : namespace_(namespace_uri) {}

void Ontology::add_class(const OntologyClass& cls) {
    std::lock_guard<std::mutex> lock(mutex_);
    classes_[cls.uri] = cls;
}

void Ontology::remove_class(const std::string& uri) {
    std::lock_guard<std::mutex> lock(mutex_);
    classes_.erase(uri);
}

std::optional<OntologyClass> Ontology::get_class(const std::string& uri) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = classes_.find(uri);
    if (it != classes_.end()) return it->second;
    return std::nullopt;
}

std::vector<OntologyClass> Ontology::list_classes() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<OntologyClass> result;
    for (const auto& [_, cls] : classes_) {
        result.push_back(cls);
    }
    return result;
}

std::vector<std::string> Ontology::get_subclasses(const std::string& uri) const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<std::string> result;
    for (const auto& [class_uri, cls] : classes_) {
        for (const auto& parent : cls.parent_classes) {
            if (parent == uri) {
                result.push_back(class_uri);
                break;
            }
        }
    }
    return result;
}

std::vector<std::string> Ontology::get_superclasses(const std::string& uri) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = classes_.find(uri);
    if (it != classes_.end()) {
        return it->second.parent_classes;
    }
    return {};
}

bool Ontology::is_subclass_of(const std::string& child, const std::string& parent) const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::set<std::string> visited;
    std::queue<std::string> to_check;
    to_check.push(child);

    while (!to_check.empty()) {
        std::string current = to_check.front();
        to_check.pop();

        if (visited.count(current)) continue;
        visited.insert(current);

        auto it = classes_.find(current);
        if (it == classes_.end()) continue;

        for (const auto& p : it->second.parent_classes) {
            if (p == parent) return true;
            to_check.push(p);
        }
    }
    return false;
}

void Ontology::add_property(const OntologyProperty& prop) {
    std::lock_guard<std::mutex> lock(mutex_);
    properties_[prop.uri] = prop;
}

void Ontology::remove_property(const std::string& uri) {
    std::lock_guard<std::mutex> lock(mutex_);
    properties_.erase(uri);
}

std::optional<OntologyProperty> Ontology::get_property(const std::string& uri) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = properties_.find(uri);
    if (it != properties_.end()) return it->second;
    return std::nullopt;
}

std::vector<OntologyProperty> Ontology::list_properties() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<OntologyProperty> result;
    for (const auto& [_, prop] : properties_) {
        result.push_back(prop);
    }
    return result;
}

std::vector<std::string> Ontology::get_properties_for_class(const std::string& class_uri) const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<std::string> result;
    for (const auto& [uri, prop] : properties_) {
        if (prop.domain == class_uri) {
            result.push_back(uri);
        }
    }
    return result;
}

std::vector<Ontology::ValidationError> Ontology::validate(
    const std::vector<Node>& nodes,
    const std::vector<Edge>& edges) const {

    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<ValidationError> errors;

    // Check that all nodes have valid types
    for (const auto& node : nodes) {
        // Could validate against ontology classes
        if (node.label.empty()) {
            errors.push_back({node.id, "Node has empty label", ""});
        }
    }

    // Check edge constraints
    for (const auto& edge : edges) {
        if (edge.from_id.empty() || edge.to_id.empty()) {
            errors.push_back({edge.id, "Edge has missing endpoint", ""});
        }
    }

    return errors;
}

std::string Ontology::export_owl() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::ostringstream oss;
    oss << "<?xml version=\"1.0\"?>\n";
    oss << "<rdf:RDF xmlns:rdf=\"http://www.w3.org/1999/02/22-rdf-syntax-ns#\"\n";
    oss << "         xmlns:owl=\"http://www.w3.org/2002/07/owl#\"\n";
    oss << "         xmlns:rdfs=\"http://www.w3.org/2000/01/rdf-schema#\"\n";
    oss << "         xmlns:rael=\"" << namespace_ << "\">\n\n";

    for (const auto& [uri, cls] : classes_) {
        oss << "  <owl:Class rdf:about=\"" << uri << "\">\n";
        oss << "    <rdfs:label>" << cls.label << "</rdfs:label>\n";
        for (const auto& parent : cls.parent_classes) {
            oss << "    <rdfs:subClassOf rdf:resource=\"" << parent << "\"/>\n";
        }
        oss << "  </owl:Class>\n\n";
    }

    for (const auto& [uri, prop] : properties_) {
        oss << "  <owl:ObjectProperty rdf:about=\"" << uri << "\">\n";
        oss << "    <rdfs:label>" << prop.label << "</rdfs:label>\n";
        if (!prop.domain.empty()) {
            oss << "    <rdfs:domain rdf:resource=\"" << prop.domain << "\"/>\n";
        }
        if (!prop.range.empty()) {
            oss << "    <rdfs:range rdf:resource=\"" << prop.range << "\"/>\n";
        }
        oss << "  </owl:ObjectProperty>\n\n";
    }

    oss << "</rdf:RDF>\n";
    return oss.str();
}

std::string Ontology::export_rdfs() const {
    // Simplified RDFS export
    return export_owl();
}

bool Ontology::import_owl(const std::string& owl) {
    // Parse OWL/RDF-XML - simplified implementation
    (void)owl;
    return true;
}

std::string Ontology::namespace_uri() const {
    return namespace_;
}

// ═══════════════════════════════════════════════════════════════════════════
//  GRAPH ALGORITHMS
// ═══════════════════════════════════════════════════════════════════════════

std::vector<std::string> GraphAlgorithms::shortest_path(
    const std::string& from,
    const std::string& to,
    const std::map<std::string, std::vector<std::pair<std::string, double>>>& graph) {

    if (from == to) return {from};

    std::map<std::string, double> dist;
    std::map<std::string, std::string> prev;
    std::priority_queue<std::pair<double, std::string>,
                        std::vector<std::pair<double, std::string>>,
                        std::greater<>> pq;

    dist[from] = 0;
    pq.push({0, from});

    while (!pq.empty()) {
        auto [d, u] = pq.top();
        pq.pop();

        if (u == to) break;
        if (d > dist[u]) continue;

        auto it = graph.find(u);
        if (it == graph.end()) continue;

        for (const auto& [v, weight] : it->second) {
            double new_dist = dist[u] + weight;
            if (dist.find(v) == dist.end() || new_dist < dist[v]) {
                dist[v] = new_dist;
                prev[v] = u;
                pq.push({new_dist, v});
            }
        }
    }

    if (prev.find(to) == prev.end()) return {};

    std::vector<std::string> path;
    for (std::string cur = to; !cur.empty(); cur = prev.count(cur) ? prev[cur] : "") {
        path.push_back(cur);
        if (cur == from) break;
    }
    std::reverse(path.begin(), path.end());
    return path;
}

std::vector<std::vector<std::string>> GraphAlgorithms::all_paths(
    const std::string& from,
    const std::string& to,
    const std::map<std::string, std::vector<std::pair<std::string, double>>>& graph,
    int max_depth) {

    std::vector<std::vector<std::string>> result;
    std::vector<std::string> current_path;
    std::set<std::string> visited;

    std::function<void(const std::string&, int)> dfs = [&](const std::string& node, int depth) {
        if (depth > max_depth) return;
        if (node == to) {
            current_path.push_back(node);
            result.push_back(current_path);
            current_path.pop_back();
            return;
        }

        visited.insert(node);
        current_path.push_back(node);

        auto it = graph.find(node);
        if (it != graph.end()) {
            for (const auto& [next, _] : it->second) {
                if (!visited.count(next)) {
                    dfs(next, depth + 1);
                }
            }
        }

        current_path.pop_back();
        visited.erase(node);
    };

    dfs(from, 0);
    return result;
}

std::map<std::string, double> GraphAlgorithms::pagerank(
    const std::map<std::string, std::vector<std::string>>& graph,
    double damping,
    int iterations) {

    std::map<std::string, double> rank;
    std::set<std::string> all_nodes;

    for (const auto& [node, neighbors] : graph) {
        all_nodes.insert(node);
        for (const auto& n : neighbors) {
            all_nodes.insert(n);
        }
    }

    double initial = 1.0 / all_nodes.size();
    for (const auto& n : all_nodes) {
        rank[n] = initial;
    }

    for (int i = 0; i < iterations; ++i) {
        std::map<std::string, double> new_rank;
        double sink_rank = 0;

        for (const auto& n : all_nodes) {
            auto it = graph.find(n);
            if (it == graph.end() || it->second.empty()) {
                sink_rank += rank[n];
            }
        }

        for (const auto& n : all_nodes) {
            double incoming = 0;
            for (const auto& [source, neighbors] : graph) {
                for (const auto& target : neighbors) {
                    if (target == n) {
                        incoming += rank[source] / neighbors.size();
                    }
                }
            }
            new_rank[n] = (1 - damping) / all_nodes.size() +
                          damping * (incoming + sink_rank / all_nodes.size());
        }
        rank = new_rank;
    }

    return rank;
}

std::map<std::string, double> GraphAlgorithms::betweenness_centrality(
    const std::map<std::string, std::vector<std::string>>& graph) {

    std::map<std::string, double> centrality;
    std::set<std::string> all_nodes;

    for (const auto& [node, neighbors] : graph) {
        all_nodes.insert(node);
        centrality[node] = 0;
        for (const auto& n : neighbors) {
            all_nodes.insert(n);
            centrality[n] = 0;
        }
    }

    // Simplified betweenness (full algorithm is O(V*E))
    for (const auto& s : all_nodes) {
        std::map<std::string, std::vector<std::string>> pred;
        std::map<std::string, int> dist;
        std::map<std::string, double> sigma;

        std::queue<std::string> q;
        std::stack<std::string> stack;

        sigma[s] = 1;
        dist[s] = 0;
        q.push(s);

        while (!q.empty()) {
            std::string v = q.front();
            q.pop();
            stack.push(v);

            auto it = graph.find(v);
            if (it == graph.end()) continue;

            for (const auto& w : it->second) {
                if (dist.find(w) == dist.end()) {
                    q.push(w);
                    dist[w] = dist[v] + 1;
                }
                if (dist[w] == dist[v] + 1) {
                    sigma[w] += sigma[v];
                    pred[w].push_back(v);
                }
            }
        }

        std::map<std::string, double> delta;
        while (!stack.empty()) {
            std::string w = stack.top();
            stack.pop();
            for (const auto& v : pred[w]) {
                delta[v] += (sigma[v] / sigma[w]) * (1 + delta[w]);
            }
            if (w != s) {
                centrality[w] += delta[w];
            }
        }
    }

    // Normalize
    double n = all_nodes.size();
    if (n > 2) {
        for (auto& [_, c] : centrality) {
            c /= ((n - 1) * (n - 2));
        }
    }

    return centrality;
}

std::map<std::string, double> GraphAlgorithms::closeness_centrality(
    const std::map<std::string, std::vector<std::string>>& graph) {

    std::map<std::string, double> centrality;

    for (const auto& [node, _] : graph) {
        // BFS to find distances
        std::map<std::string, int> dist;
        std::queue<std::string> q;
        q.push(node);
        dist[node] = 0;

        while (!q.empty()) {
            std::string u = q.front();
            q.pop();

            auto it = graph.find(u);
            if (it == graph.end()) continue;

            for (const auto& v : it->second) {
                if (dist.find(v) == dist.end()) {
                    dist[v] = dist[u] + 1;
                    q.push(v);
                }
            }
        }

        double sum = 0;
        for (const auto& [_, d] : dist) {
            sum += d;
        }
        centrality[node] = sum > 0 ? (dist.size() - 1.0) / sum : 0;
    }

    return centrality;
}

std::vector<std::set<std::string>> GraphAlgorithms::louvain_communities(
    const std::map<std::string, std::vector<std::pair<std::string, double>>>& graph) {

    // Simplified Louvain - each node starts in its own community
    std::map<std::string, int> community;
    int next_comm = 0;

    for (const auto& [node, _] : graph) {
        community[node] = next_comm++;
    }

    // One pass of optimization
    bool changed = true;
    while (changed) {
        changed = false;
        for (const auto& [node, neighbors] : graph) {
            std::map<int, double> comm_weights;
            for (const auto& [neighbor, weight] : neighbors) {
                if (community.count(neighbor)) {
                    comm_weights[community[neighbor]] += weight;
                }
            }

            int best_comm = community[node];
            double best_gain = 0;
            for (const auto& [c, w] : comm_weights) {
                if (w > best_gain) {
                    best_gain = w;
                    best_comm = c;
                }
            }

            if (best_comm != community[node]) {
                community[node] = best_comm;
                changed = true;
            }
        }
    }

    // Group by community
    std::map<int, std::set<std::string>> groups;
    for (const auto& [node, comm] : community) {
        groups[comm].insert(node);
    }

    std::vector<std::set<std::string>> result;
    for (const auto& [_, nodes] : groups) {
        result.push_back(nodes);
    }
    return result;
}

std::vector<std::set<std::string>> GraphAlgorithms::label_propagation(
    const std::map<std::string, std::vector<std::string>>& graph) {

    std::map<std::string, std::string> label;
    for (const auto& [node, _] : graph) {
        label[node] = node;
    }

    std::vector<std::string> nodes;
    for (const auto& [node, _] : graph) {
        nodes.push_back(node);
    }

    std::random_device rd;
    std::mt19937 gen(rd());

    for (int iter = 0; iter < 10; ++iter) {
        std::shuffle(nodes.begin(), nodes.end(), gen);
        bool changed = false;

        for (const auto& node : nodes) {
            auto it = graph.find(node);
            if (it == graph.end() || it->second.empty()) continue;

            std::map<std::string, int> label_counts;
            for (const auto& neighbor : it->second) {
                label_counts[label[neighbor]]++;
            }

            std::string best_label = label[node];
            int best_count = 0;
            for (const auto& [l, c] : label_counts) {
                if (c > best_count) {
                    best_count = c;
                    best_label = l;
                }
            }

            if (best_label != label[node]) {
                label[node] = best_label;
                changed = true;
            }
        }

        if (!changed) break;
    }

    std::map<std::string, std::set<std::string>> groups;
    for (const auto& [node, l] : label) {
        groups[l].insert(node);
    }

    std::vector<std::set<std::string>> result;
    for (const auto& [_, nodes] : groups) {
        result.push_back(nodes);
    }
    return result;
}

double GraphAlgorithms::jaccard_similarity(
    const std::set<std::string>& neighbors1,
    const std::set<std::string>& neighbors2) {

    std::set<std::string> intersection, union_set;
    std::set_intersection(neighbors1.begin(), neighbors1.end(),
                          neighbors2.begin(), neighbors2.end(),
                          std::inserter(intersection, intersection.begin()));
    std::set_union(neighbors1.begin(), neighbors1.end(),
                   neighbors2.begin(), neighbors2.end(),
                   std::inserter(union_set, union_set.begin()));

    if (union_set.empty()) return 0;
    return static_cast<double>(intersection.size()) / union_set.size();
}

double GraphAlgorithms::adamic_adar(
    const std::string& node1,
    const std::string& node2,
    const std::map<std::string, std::set<std::string>>& graph) {

    auto it1 = graph.find(node1);
    auto it2 = graph.find(node2);
    if (it1 == graph.end() || it2 == graph.end()) return 0;

    std::set<std::string> common;
    std::set_intersection(it1->second.begin(), it1->second.end(),
                          it2->second.begin(), it2->second.end(),
                          std::inserter(common, common.begin()));

    double score = 0;
    for (const auto& neighbor : common) {
        auto it = graph.find(neighbor);
        if (it != graph.end() && it->second.size() > 1) {
            score += 1.0 / std::log(it->second.size());
        }
    }
    return score;
}

double GraphAlgorithms::clustering_coefficient(
    const std::string& node,
    const std::map<std::string, std::set<std::string>>& graph) {

    auto it = graph.find(node);
    if (it == graph.end() || it->second.size() < 2) return 0;

    const auto& neighbors = it->second;
    int edges = 0;
    int possible = neighbors.size() * (neighbors.size() - 1) / 2;

    for (auto i = neighbors.begin(); i != neighbors.end(); ++i) {
        for (auto j = std::next(i); j != neighbors.end(); ++j) {
            auto ni = graph.find(*i);
            if (ni != graph.end() && ni->second.count(*j)) {
                edges++;
            }
        }
    }

    return possible > 0 ? static_cast<double>(edges) / possible : 0;
}

double GraphAlgorithms::global_clustering_coefficient(
    const std::map<std::string, std::set<std::string>>& graph) {

    double sum = 0;
    int count = 0;

    for (const auto& [node, _] : graph) {
        double c = clustering_coefficient(node, graph);
        if (c > 0) {
            sum += c;
            count++;
        }
    }

    return count > 0 ? sum / count : 0;
}

std::vector<std::set<std::string>> GraphAlgorithms::connected_components(
    const std::map<std::string, std::vector<std::string>>& graph) {

    std::set<std::string> visited;
    std::vector<std::set<std::string>> components;

    for (const auto& [start, _] : graph) {
        if (visited.count(start)) continue;

        std::set<std::string> component;
        std::queue<std::string> q;
        q.push(start);

        while (!q.empty()) {
            std::string node = q.front();
            q.pop();

            if (visited.count(node)) continue;
            visited.insert(node);
            component.insert(node);

            auto it = graph.find(node);
            if (it != graph.end()) {
                for (const auto& neighbor : it->second) {
                    if (!visited.count(neighbor)) {
                        q.push(neighbor);
                    }
                }
            }
        }

        components.push_back(component);
    }

    return components;
}

bool GraphAlgorithms::is_connected(
    const std::string& from,
    const std::string& to,
    const std::map<std::string, std::vector<std::string>>& graph) {

    std::set<std::string> visited;
    std::queue<std::string> q;
    q.push(from);

    while (!q.empty()) {
        std::string node = q.front();
        q.pop();

        if (node == to) return true;
        if (visited.count(node)) continue;
        visited.insert(node);

        auto it = graph.find(node);
        if (it != graph.end()) {
            for (const auto& neighbor : it->second) {
                if (!visited.count(neighbor)) {
                    q.push(neighbor);
                }
            }
        }
    }

    return false;
}

// ═══════════════════════════════════════════════════════════════════════════
//  SEMANTIC LAYER
// ═══════════════════════════════════════════════════════════════════════════

SemanticLayer::SemanticLayer() {}

std::vector<std::string> SemanticLayer::find_similar_entities(
    const std::string& label,
    const std::vector<Node>& nodes,
    double threshold) {

    std::vector<std::string> similar;
    std::string lower_label = label;
    std::transform(lower_label.begin(), lower_label.end(), lower_label.begin(), ::tolower);

    for (const auto& node : nodes) {
        double sim = string_similarity(lower_label, node.label);
        if (sim >= threshold) {
            similar.push_back(node.id);
        }
    }

    return similar;
}

std::optional<std::string> SemanticLayer::resolve_entity(
    const std::string& mention,
    const std::vector<Node>& nodes) {

    double best_score = 0;
    std::string best_id;

    for (const auto& node : nodes) {
        double score = string_similarity(mention, node.label);
        if (score > best_score) {
            best_score = score;
            best_id = node.id;
        }
    }

    if (best_score > 0.8) return best_id;
    return std::nullopt;
}

std::vector<std::pair<std::string, std::string>> SemanticLayer::find_duplicates(
    const std::vector<Node>& nodes,
    double threshold) {

    std::vector<std::pair<std::string, std::string>> duplicates;

    for (size_t i = 0; i < nodes.size(); ++i) {
        for (size_t j = i + 1; j < nodes.size(); ++j) {
            double sim = string_similarity(nodes[i].label, nodes[j].label);
            if (sim >= threshold) {
                duplicates.push_back({nodes[i].id, nodes[j].id});
            }
        }
    }

    return duplicates;
}

std::vector<SemanticLayer::ExtractedRelation> SemanticLayer::extract_relations(
    const std::string& text) {

    std::vector<ExtractedRelation> relations;

    // Simple pattern matching for common relation phrases
    // "X is a Y", "X causes Y", "X is part of Y"
    std::vector<std::pair<std::string, std::string>> patterns = {
        {" is a ", "IS_A"},
        {" is an ", "IS_A"},
        {" causes ", "CAUSES"},
        {" caused by ", "CAUSED_BY"},
        {" is part of ", "PART_OF"},
        {" contains ", "CONTAINS"},
        {" is related to ", "RELATED_TO"},
        {" is similar to ", "SIMILAR_TO"},
        {" is located in ", "LOCATED_IN"},
    };

    for (const auto& [pattern, predicate] : patterns) {
        size_t pos = text.find(pattern);
        if (pos != std::string::npos) {
            // Extract subject (before pattern) and object (after pattern)
            size_t start = text.rfind('.', pos);
            start = (start == std::string::npos) ? 0 : start + 1;
            size_t end = text.find('.', pos + pattern.length());
            if (end == std::string::npos) end = text.length();

            std::string subject = text.substr(start, pos - start);
            std::string object = text.substr(pos + pattern.length(), end - pos - pattern.length());

            // Trim whitespace
            auto trim = [](std::string& s) {
                size_t begin = s.find_first_not_of(" \t\n\r");
                size_t last = s.find_last_not_of(" \t\n\r");
                if (begin != std::string::npos) {
                    s = s.substr(begin, last - begin + 1);
                }
            };
            trim(subject);
            trim(object);

            if (!subject.empty() && !object.empty()) {
                ExtractedRelation rel;
                rel.subject = subject;
                rel.predicate = predicate;
                rel.object = object;
                rel.confidence = 0.7;
                rel.source_text = text.substr(start, end - start);
                relations.push_back(rel);
            }
        }
    }

    return relations;
}

std::vector<std::pair<std::string, double>> SemanticLayer::semantic_search(
    const std::string& query,
    const std::vector<Node>& nodes,
    int top_k) {

    std::vector<std::pair<std::string, double>> results;

    for (const auto& node : nodes) {
        double score = string_similarity(query, node.label);

        // Also check properties
        for (const auto& [key, value] : node.properties) {
            if (std::holds_alternative<std::string>(value)) {
                double prop_score = string_similarity(query, std::get<std::string>(value));
                score = std::max(score, prop_score * 0.8);
            }
        }

        if (score > 0.1) {
            results.push_back({node.id, score});
        }
    }

    std::sort(results.begin(), results.end(),
              [](const auto& a, const auto& b) { return a.second > b.second; });

    if (static_cast<int>(results.size()) > top_k) {
        results.resize(top_k);
    }

    return results;
}

std::vector<float> SemanticLayer::compute_embedding(const std::string& text) {
    // Simplified embedding - in practice would use a real model
    std::vector<float> embedding(128, 0.0f);
    std::hash<std::string> hasher;
    size_t hash = hasher(text);

    for (size_t i = 0; i < embedding.size(); ++i) {
        embedding[i] = static_cast<float>((hash >> (i % 64)) & 0xFF) / 255.0f;
        hash = hash * 31 + i;
    }

    // Normalize
    float norm = 0;
    for (float v : embedding) norm += v * v;
    norm = std::sqrt(norm);
    if (norm > 0) {
        for (float& v : embedding) v /= norm;
    }

    return embedding;
}

std::vector<float> SemanticLayer::compute_node_embedding(const Node& node) {
    std::string combined = node.label;
    for (const auto& [key, value] : node.properties) {
        combined += " " + property_to_string(value);
    }
    return compute_embedding(combined);
}

std::string SemanticLayer::answer_question(
    const std::string& question,
    const std::vector<Node>& context_nodes,
    const std::vector<Edge>& context_edges) {

    // Find relevant nodes
    auto search_results = semantic_search(question, context_nodes, 5);

    if (search_results.empty()) {
        return "No relevant information found.";
    }

    std::ostringstream answer;
    answer << "Based on the knowledge graph:\n";

    for (const auto& [node_id, score] : search_results) {
        for (const auto& node : context_nodes) {
            if (node.id == node_id) {
                answer << "- " << node.label;

                // Find related edges
                for (const auto& edge : context_edges) {
                    if (edge.from_id == node_id) {
                        for (const auto& target : context_nodes) {
                            if (target.id == edge.to_id) {
                                answer << " " << edge_type_to_string(edge.type)
                                       << " " << target.label;
                            }
                        }
                    }
                }
                answer << "\n";
                break;
            }
        }
    }

    return answer.str();
}

double SemanticLayer::string_similarity(const std::string& a, const std::string& b) const {
    if (a.empty() || b.empty()) return 0;

    std::string la = a, lb = b;
    std::transform(la.begin(), la.end(), la.begin(), ::tolower);
    std::transform(lb.begin(), lb.end(), lb.begin(), ::tolower);

    if (la == lb) return 1.0;
    if (la.find(lb) != std::string::npos || lb.find(la) != std::string::npos) {
        return 0.8;
    }

    // Levenshtein distance
    size_t m = la.size(), n = lb.size();
    std::vector<std::vector<int>> dp(m + 1, std::vector<int>(n + 1));

    for (size_t i = 0; i <= m; ++i) dp[i][0] = i;
    for (size_t j = 0; j <= n; ++j) dp[0][j] = j;

    for (size_t i = 1; i <= m; ++i) {
        for (size_t j = 1; j <= n; ++j) {
            int cost = la[i-1] == lb[j-1] ? 0 : 1;
            dp[i][j] = std::min({dp[i-1][j] + 1, dp[i][j-1] + 1, dp[i-1][j-1] + cost});
        }
    }

    double max_len = std::max(m, n);
    return 1.0 - dp[m][n] / max_len;
}

// ═══════════════════════════════════════════════════════════════════════════
//  KNOWLEDGE GRAPH
// ═══════════════════════════════════════════════════════════════════════════

KnowledgeGraph::KnowledgeGraph(const KnowledgeGraphConfig& config)
    : config_(config)
    , inference_(std::make_unique<InferenceEngine>())
    , ontology_(std::make_unique<Ontology>())
    , semantic_(std::make_unique<SemanticLayer>()) {}

KnowledgeGraph::~KnowledgeGraph() = default;

KnowledgeGraph& KnowledgeGraph::instance() {
    static KnowledgeGraph instance;
    return instance;
}

std::string KnowledgeGraph::add_node(const Node& node) {
    std::lock_guard<std::mutex> lock(mutex_);
    std::string id = node.id.empty() ? generate_id() : node.id;
    Node n = node;
    n.id = id;
    n.created = std::chrono::system_clock::now();
    n.modified = n.created;

    nodes_[id] = n;
    index_node(n);
    return id;
}

std::string KnowledgeGraph::add_node(const std::string& label, NodeType type) {
    Node n;
    n.label = label;
    n.type = type;
    return add_node(n);
}

bool KnowledgeGraph::update_node(const Node& node) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = nodes_.find(node.id);
    if (it == nodes_.end()) return false;

    unindex_node(node.id);
    Node n = node;
    n.modified = std::chrono::system_clock::now();
    nodes_[node.id] = n;
    index_node(n);
    return true;
}

bool KnowledgeGraph::remove_node(const std::string& id) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = nodes_.find(id);
    if (it == nodes_.end()) return false;

    unindex_node(id);
    nodes_.erase(it);

    // Remove connected edges
    std::vector<std::string> to_remove;
    for (const auto& [eid, edge] : edges_) {
        if (edge.from_id == id || edge.to_id == id) {
            to_remove.push_back(eid);
        }
    }
    for (const auto& eid : to_remove) {
        edges_.erase(eid);
    }

    return true;
}

std::optional<Node> KnowledgeGraph::get_node(const std::string& id) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = nodes_.find(id);
    if (it != nodes_.end()) return it->second;
    return std::nullopt;
}

std::vector<Node> KnowledgeGraph::get_nodes_by_type(NodeType type) const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<Node> result;
    auto it = type_index_.find(type);
    if (it != type_index_.end()) {
        for (const auto& id : it->second) {
            auto nit = nodes_.find(id);
            if (nit != nodes_.end()) {
                result.push_back(nit->second);
            }
        }
    }
    return result;
}

std::vector<Node> KnowledgeGraph::get_nodes_by_label(const std::string& label) const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<Node> result;
    auto it = label_index_.find(label);
    if (it != label_index_.end()) {
        for (const auto& id : it->second) {
            auto nit = nodes_.find(id);
            if (nit != nodes_.end()) {
                result.push_back(nit->second);
            }
        }
    }
    return result;
}

std::vector<Node> KnowledgeGraph::search_nodes(const std::string& query, int limit) const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<std::pair<double, Node>> scored;

    std::string lower_query = query;
    std::transform(lower_query.begin(), lower_query.end(), lower_query.begin(), ::tolower);

    for (const auto& [_, node] : nodes_) {
        std::string lower_label = node.label;
        std::transform(lower_label.begin(), lower_label.end(), lower_label.begin(), ::tolower);

        double score = 0;
        if (lower_label == lower_query) score = 1.0;
        else if (lower_label.find(lower_query) != std::string::npos) score = 0.8;
        else if (lower_query.find(lower_label) != std::string::npos) score = 0.6;

        if (score > 0) {
            scored.push_back({score, node});
        }
    }

    std::sort(scored.begin(), scored.end(),
              [](const auto& a, const auto& b) { return a.first > b.first; });

    std::vector<Node> result;
    for (int i = 0; i < limit && i < static_cast<int>(scored.size()); ++i) {
        result.push_back(scored[i].second);
    }
    return result;
}

std::string KnowledgeGraph::add_edge(const Edge& edge) {
    std::lock_guard<std::mutex> lock(mutex_);
    std::string id = edge.id.empty() ? generate_id() : edge.id;
    Edge e = edge;
    e.id = id;
    e.created = std::chrono::system_clock::now();

    edges_[id] = e;
    adj_out_[e.from_id].insert(id);
    adj_in_[e.to_id].insert(id);

    if (e.bidirectional) {
        adj_out_[e.to_id].insert(id);
        adj_in_[e.from_id].insert(id);
    }

    return id;
}

std::string KnowledgeGraph::add_edge(const std::string& from, EdgeType type, const std::string& to) {
    Edge e;
    e.from_id = from;
    e.to_id = to;
    e.type = type;
    return add_edge(e);
}

std::string KnowledgeGraph::add_triple(const std::string& subject, const std::string& predicate,
                                        const std::string& object) {
    // Find or create nodes
    std::string subj_id, obj_id;
    auto subj_nodes = get_nodes_by_label(subject);
    auto obj_nodes = get_nodes_by_label(object);

    if (subj_nodes.empty()) {
        subj_id = add_node(subject, NodeType::ENTITY);
    } else {
        subj_id = subj_nodes[0].id;
    }

    if (obj_nodes.empty()) {
        obj_id = add_node(object, NodeType::ENTITY);
    } else {
        obj_id = obj_nodes[0].id;
    }

    EdgeType type = string_to_edge_type(predicate);
    return add_edge(subj_id, type, obj_id);
}

bool KnowledgeGraph::update_edge(const Edge& edge) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = edges_.find(edge.id);
    if (it == edges_.end()) return false;
    edges_[edge.id] = edge;
    return true;
}

bool KnowledgeGraph::remove_edge(const std::string& id) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = edges_.find(id);
    if (it == edges_.end()) return false;

    adj_out_[it->second.from_id].erase(id);
    adj_in_[it->second.to_id].erase(id);
    edges_.erase(it);
    return true;
}

std::optional<Edge> KnowledgeGraph::get_edge(const std::string& id) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = edges_.find(id);
    if (it != edges_.end()) return it->second;
    return std::nullopt;
}

std::vector<Edge> KnowledgeGraph::get_edges_from(const std::string& node_id) const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<Edge> result;
    auto it = adj_out_.find(node_id);
    if (it != adj_out_.end()) {
        for (const auto& eid : it->second) {
            auto eit = edges_.find(eid);
            if (eit != edges_.end()) {
                result.push_back(eit->second);
            }
        }
    }
    return result;
}

std::vector<Edge> KnowledgeGraph::get_edges_to(const std::string& node_id) const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<Edge> result;
    auto it = adj_in_.find(node_id);
    if (it != adj_in_.end()) {
        for (const auto& eid : it->second) {
            auto eit = edges_.find(eid);
            if (eit != edges_.end()) {
                result.push_back(eit->second);
            }
        }
    }
    return result;
}

std::vector<Edge> KnowledgeGraph::get_edges_between(const std::string& from, const std::string& to) const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<Edge> result;
    auto it = adj_out_.find(from);
    if (it != adj_out_.end()) {
        for (const auto& eid : it->second) {
            auto eit = edges_.find(eid);
            if (eit != edges_.end() && eit->second.to_id == to) {
                result.push_back(eit->second);
            }
        }
    }
    return result;
}

std::vector<Edge> KnowledgeGraph::get_edges_by_type(EdgeType type) const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<Edge> result;
    for (const auto& [_, edge] : edges_) {
        if (edge.type == type) {
            result.push_back(edge);
        }
    }
    return result;
}

QueryResult KnowledgeGraph::query(const GraphQuery& q) const {
    std::lock_guard<std::mutex> lock(mutex_);
    QueryResult result;
    auto start = std::chrono::high_resolution_clock::now();

    // Simple pattern matching
    for (const auto& pattern : q.patterns) {
        for (const auto& [eid, edge] : edges_) {
            bool matches = true;

            if (pattern.predicate && edge.type != *pattern.predicate) {
                matches = false;
            }
            if (pattern.subject && !pattern.subject->empty() && (*pattern.subject)[0] != '?') {
                auto nit = nodes_.find(edge.from_id);
                if (nit == nodes_.end() || nit->second.label != *pattern.subject) {
                    matches = false;
                }
            }
            if (pattern.object && !pattern.object->empty() && (*pattern.object)[0] != '?') {
                auto nit = nodes_.find(edge.to_id);
                if (nit == nodes_.end() || nit->second.label != *pattern.object) {
                    matches = false;
                }
            }

            if (matches) {
                std::map<std::string, std::string> binding;
                auto from_it = nodes_.find(edge.from_id);
                auto to_it = nodes_.find(edge.to_id);
                if (from_it != nodes_.end()) binding["subject"] = from_it->second.label;
                if (to_it != nodes_.end()) binding["object"] = to_it->second.label;
                binding["predicate"] = edge_type_to_string(edge.type);
                result.bindings.push_back(binding);
            }
        }
    }

    // Apply limit
    if (q.limit && static_cast<int>(result.bindings.size()) > *q.limit) {
        result.bindings.resize(*q.limit);
    }

    auto end = std::chrono::high_resolution_clock::now();
    result.execution_time_ms = std::chrono::duration<double, std::milli>(end - start).count();
    result.total_matches = result.bindings.size();

    return result;
}

QueryResult KnowledgeGraph::query_pattern(const QueryPattern& pattern) const {
    GraphQuery q;
    q.patterns = {pattern};
    return query(q);
}

std::vector<std::vector<std::string>> KnowledgeGraph::find_paths(const PathQuery& pq) const {
    std::lock_guard<std::mutex> lock(mutex_);

    // Build adjacency for path finding
    std::map<std::string, std::vector<std::pair<std::string, double>>> adj;
    for (const auto& [_, edge] : edges_) {
        bool allowed = pq.allowed_edges.empty();
        for (const auto& et : pq.allowed_edges) {
            if (edge.type == et) { allowed = true; break; }
        }
        if (allowed) {
            adj[edge.from_id].push_back({edge.to_id, edge.weight});
        }
    }

    if (pq.all_paths) {
        return GraphAlgorithms::all_paths(pq.start_node, pq.end_node, adj, pq.max_depth);
    } else {
        auto path = GraphAlgorithms::shortest_path(pq.start_node, pq.end_node, adj);
        if (path.empty()) return {};
        return {path};
    }
}

std::vector<Triple> KnowledgeGraph::get_triples(const std::string& subject,
                                                  const std::string& predicate,
                                                  const std::string& object) const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<Triple> result;

    for (const auto& [_, edge] : edges_) {
        auto from_it = nodes_.find(edge.from_id);
        auto to_it = nodes_.find(edge.to_id);
        if (from_it == nodes_.end() || to_it == nodes_.end()) continue;

        bool match = true;
        if (!subject.empty() && from_it->second.label != subject) match = false;
        if (!predicate.empty() && edge_type_to_string(edge.type) != predicate) match = false;
        if (!object.empty() && to_it->second.label != object) match = false;

        if (match) {
            result.push_back(Triple::from_edge(from_it->second, edge, to_it->second));
        }
    }

    return result;
}

std::vector<Node> KnowledgeGraph::get_neighbors(const std::string& node_id,
                                                  std::optional<EdgeType> edge_type) const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::set<std::string> neighbor_ids;

    auto out_it = adj_out_.find(node_id);
    if (out_it != adj_out_.end()) {
        for (const auto& eid : out_it->second) {
            auto eit = edges_.find(eid);
            if (eit != edges_.end()) {
                if (!edge_type || eit->second.type == *edge_type) {
                    neighbor_ids.insert(eit->second.to_id);
                }
            }
        }
    }

    auto in_it = adj_in_.find(node_id);
    if (in_it != adj_in_.end()) {
        for (const auto& eid : in_it->second) {
            auto eit = edges_.find(eid);
            if (eit != edges_.end()) {
                if (!edge_type || eit->second.type == *edge_type) {
                    neighbor_ids.insert(eit->second.from_id);
                }
            }
        }
    }

    std::vector<Node> result;
    for (const auto& nid : neighbor_ids) {
        auto nit = nodes_.find(nid);
        if (nit != nodes_.end()) {
            result.push_back(nit->second);
        }
    }
    return result;
}

std::vector<Node> KnowledgeGraph::traverse_bfs(const std::string& start, int max_depth) const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<Node> result;
    std::set<std::string> visited;
    std::queue<std::pair<std::string, int>> q;

    q.push({start, 0});

    while (!q.empty()) {
        auto [node_id, depth] = q.front();
        q.pop();

        if (depth > max_depth || visited.count(node_id)) continue;
        visited.insert(node_id);

        auto nit = nodes_.find(node_id);
        if (nit != nodes_.end()) {
            result.push_back(nit->second);
        }

        auto out_it = adj_out_.find(node_id);
        if (out_it != adj_out_.end()) {
            for (const auto& eid : out_it->second) {
                auto eit = edges_.find(eid);
                if (eit != edges_.end() && !visited.count(eit->second.to_id)) {
                    q.push({eit->second.to_id, depth + 1});
                }
            }
        }
    }

    return result;
}

std::vector<Node> KnowledgeGraph::traverse_dfs(const std::string& start, int max_depth) const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<Node> result;
    std::set<std::string> visited;
    std::stack<std::pair<std::string, int>> s;

    s.push({start, 0});

    while (!s.empty()) {
        auto [node_id, depth] = s.top();
        s.pop();

        if (depth > max_depth || visited.count(node_id)) continue;
        visited.insert(node_id);

        auto nit = nodes_.find(node_id);
        if (nit != nodes_.end()) {
            result.push_back(nit->second);
        }

        auto out_it = adj_out_.find(node_id);
        if (out_it != adj_out_.end()) {
            for (const auto& eid : out_it->second) {
                auto eit = edges_.find(eid);
                if (eit != edges_.end() && !visited.count(eit->second.to_id)) {
                    s.push({eit->second.to_id, depth + 1});
                }
            }
        }
    }

    return result;
}

KnowledgeGraph::Subgraph KnowledgeGraph::get_subgraph(const std::string& center, int radius) const {
    Subgraph sg;
    sg.nodes = traverse_bfs(center, radius);

    std::set<std::string> node_ids;
    for (const auto& n : sg.nodes) {
        node_ids.insert(n.id);
    }

    std::lock_guard<std::mutex> lock(mutex_);
    for (const auto& [_, edge] : edges_) {
        if (node_ids.count(edge.from_id) && node_ids.count(edge.to_id)) {
            sg.edges.push_back(edge);
        }
    }

    return sg;
}

KnowledgeGraph::Subgraph KnowledgeGraph::get_induced_subgraph(
    const std::vector<std::string>& node_ids) const {

    Subgraph sg;
    std::set<std::string> id_set(node_ids.begin(), node_ids.end());

    std::lock_guard<std::mutex> lock(mutex_);
    for (const auto& id : node_ids) {
        auto it = nodes_.find(id);
        if (it != nodes_.end()) {
            sg.nodes.push_back(it->second);
        }
    }

    for (const auto& [_, edge] : edges_) {
        if (id_set.count(edge.from_id) && id_set.count(edge.to_id)) {
            sg.edges.push_back(edge);
        }
    }

    return sg;
}

void KnowledgeGraph::run_inference() {
    std::vector<Node> nodes;
    std::vector<Edge> edges;

    {
        std::lock_guard<std::mutex> lock(mutex_);
        for (const auto& [_, n] : nodes_) nodes.push_back(n);
        for (const auto& [_, e] : edges_) edges.push_back(e);
    }

    auto inferred = inference_->forward_chain(nodes, edges, config_.inference_depth);

    std::lock_guard<std::mutex> lock(mutex_);
    inferred_triples_ = inferred;
}

std::vector<Triple> KnowledgeGraph::get_inferred_triples() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return inferred_triples_;
}

std::vector<std::string> KnowledgeGraph::explain(const std::string& triple_id) const {
    (void)triple_id;
    return {"Inference explanation not yet implemented"};
}

InferenceEngine& KnowledgeGraph::inference_engine() {
    return *inference_;
}

Ontology& KnowledgeGraph::ontology() {
    return *ontology_;
}

bool KnowledgeGraph::validate() const {
    std::vector<Node> nodes;
    std::vector<Edge> edges;

    {
        std::lock_guard<std::mutex> lock(mutex_);
        for (const auto& [_, n] : nodes_) nodes.push_back(n);
        for (const auto& [_, e] : edges_) edges.push_back(e);
    }

    auto errors = ontology_->validate(nodes, edges);
    return errors.empty();
}

SemanticLayer& KnowledgeGraph::semantic() {
    return *semantic_;
}

std::vector<Node> KnowledgeGraph::semantic_search(const std::string& query, int top_k) {
    std::vector<Node> all_nodes;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        for (const auto& [_, n] : nodes_) all_nodes.push_back(n);
    }

    auto results = semantic_->semantic_search(query, all_nodes, top_k);

    std::vector<Node> nodes;
    for (const auto& [id, _] : results) {
        auto node = get_node(id);
        if (node) nodes.push_back(*node);
    }
    return nodes;
}

std::map<std::string, double> KnowledgeGraph::compute_pagerank() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::map<std::string, std::vector<std::string>> adj;

    for (const auto& [_, edge] : edges_) {
        adj[edge.from_id].push_back(edge.to_id);
    }

    return GraphAlgorithms::pagerank(adj);
}

std::vector<std::set<std::string>> KnowledgeGraph::detect_communities() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::map<std::string, std::vector<std::pair<std::string, double>>> adj;

    for (const auto& [_, edge] : edges_) {
        adj[edge.from_id].push_back({edge.to_id, edge.weight});
    }

    return GraphAlgorithms::louvain_communities(adj);
}

std::map<std::string, double> KnowledgeGraph::compute_centrality() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::map<std::string, std::vector<std::string>> adj;

    for (const auto& [_, edge] : edges_) {
        adj[edge.from_id].push_back(edge.to_id);
    }

    return GraphAlgorithms::betweenness_centrality(adj);
}

bool KnowledgeGraph::import_rdf(const std::string& rdf, const std::string& format) {
    (void)rdf;
    (void)format;
    // Simplified - would parse RDF/Turtle/N-Triples
    return true;
}

std::string KnowledgeGraph::export_rdf(const std::string& format) const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::ostringstream oss;

    if (format == "turtle" || format == "ttl") {
        oss << "@prefix rael: <http://rael.ai/ontology#> .\n\n";
        for (const auto& [_, edge] : edges_) {
            auto from_it = nodes_.find(edge.from_id);
            auto to_it = nodes_.find(edge.to_id);
            if (from_it != nodes_.end() && to_it != nodes_.end()) {
                oss << "rael:" << edge.from_id << " rael:"
                    << edge_type_to_string(edge.type) << " rael:"
                    << edge.to_id << " .\n";
            }
        }
    }

    return oss.str();
}

bool KnowledgeGraph::import_json(const std::string& json) {
    (void)json;
    return true;
}

std::string KnowledgeGraph::export_json() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::ostringstream oss;
    oss << "{\n  \"nodes\": [\n";

    bool first = true;
    for (const auto& [_, node] : nodes_) {
        if (!first) oss << ",\n";
        first = false;
        oss << "    {\"id\": \"" << node.id << "\", \"label\": \"" << node.label
            << "\", \"type\": \"" << node_type_to_string(node.type) << "\"}";
    }

    oss << "\n  ],\n  \"edges\": [\n";

    first = true;
    for (const auto& [_, edge] : edges_) {
        if (!first) oss << ",\n";
        first = false;
        oss << "    {\"id\": \"" << edge.id << "\", \"from\": \"" << edge.from_id
            << "\", \"to\": \"" << edge.to_id << "\", \"type\": \""
            << edge_type_to_string(edge.type) << "\"}";
    }

    oss << "\n  ]\n}";
    return oss.str();
}

bool KnowledgeGraph::import_cypher(const std::string& cypher) {
    (void)cypher;
    return true;
}

std::string KnowledgeGraph::export_cypher() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::ostringstream oss;

    for (const auto& [_, node] : nodes_) {
        oss << "CREATE (n" << node.id << ":" << node_type_to_string(node.type)
            << " {label: \"" << node.label << "\"})\n";
    }

    for (const auto& [_, edge] : edges_) {
        oss << "MATCH (a {id: \"" << edge.from_id << "\"}), (b {id: \"" << edge.to_id << "\"})\n"
            << "CREATE (a)-[:" << edge_type_to_string(edge.type) << "]->(b)\n";
    }

    return oss.str();
}

bool KnowledgeGraph::save(const std::string& path) {
    std::string file_path = path.empty() ? config_.storage_path : path;
    if (file_path.empty()) return false;

    // Would serialize to file
    return true;
}

bool KnowledgeGraph::load(const std::string& path) {
    std::string file_path = path.empty() ? config_.storage_path : path;
    if (file_path.empty()) return false;

    // Would deserialize from file
    return true;
}

std::string KnowledgeGraph::create_snapshot(const std::string& name) {
    std::lock_guard<std::mutex> lock(mutex_);

    Snapshot snap;
    snap.id = generate_id();
    snap.name = name.empty() ? "snapshot_" + snap.id : name;
    snap.timestamp = std::chrono::system_clock::now();
    snap.nodes = nodes_;
    snap.edges = edges_;

    snapshots_.push_back(snap);
    return snap.id;
}

bool KnowledgeGraph::restore_snapshot(const std::string& snapshot_id) {
    std::lock_guard<std::mutex> lock(mutex_);

    for (const auto& snap : snapshots_) {
        if (snap.id == snapshot_id) {
            nodes_ = snap.nodes;
            edges_ = snap.edges;

            // Rebuild indices
            label_index_.clear();
            type_index_.clear();
            adj_out_.clear();
            adj_in_.clear();

            for (const auto& [_, n] : nodes_) {
                index_node(n);
            }
            for (const auto& [id, e] : edges_) {
                adj_out_[e.from_id].insert(id);
                adj_in_[e.to_id].insert(id);
            }

            return true;
        }
    }
    return false;
}

std::vector<std::pair<std::string, std::string>> KnowledgeGraph::list_snapshots() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<std::pair<std::string, std::string>> result;
    for (const auto& snap : snapshots_) {
        result.push_back({snap.id, snap.name});
    }
    return result;
}

KnowledgeGraph::Stats KnowledgeGraph::get_stats() const {
    std::lock_guard<std::mutex> lock(mutex_);
    Stats s;
    s.node_count = nodes_.size();
    s.edge_count = edges_.size();
    s.triple_count = edges_.size();
    s.inferred_count = inferred_triples_.size();

    for (const auto& [_, n] : nodes_) {
        s.nodes_by_type[n.type]++;
    }
    for (const auto& [_, e] : edges_) {
        s.edges_by_type[e.type]++;
    }

    if (!nodes_.empty()) {
        s.avg_out_degree = static_cast<double>(edges_.size()) / nodes_.size();
    }

    return s;
}

void KnowledgeGraph::merge(const KnowledgeGraph& other, bool resolve_conflicts) {
    (void)resolve_conflicts;

    std::lock_guard<std::mutex> lock(mutex_);
    // Simple merge - add all nodes and edges from other
    for (const auto& [id, node] : other.nodes_) {
        if (nodes_.find(id) == nodes_.end()) {
            nodes_[id] = node;
            index_node(node);
        }
    }
    for (const auto& [id, edge] : other.edges_) {
        if (edges_.find(id) == edges_.end()) {
            edges_[id] = edge;
            adj_out_[edge.from_id].insert(id);
            adj_in_[edge.to_id].insert(id);
        }
    }
}

void KnowledgeGraph::clear() {
    std::lock_guard<std::mutex> lock(mutex_);
    nodes_.clear();
    edges_.clear();
    label_index_.clear();
    type_index_.clear();
    adj_out_.clear();
    adj_in_.clear();
    inferred_triples_.clear();
    snapshots_.clear();
}

std::string KnowledgeGraph::generate_id() {
    uint64_t id = next_id_++;
    std::ostringstream oss;
    oss << "kg_" << std::hex << std::setfill('0') << std::setw(16) << id;
    return oss.str();
}

void KnowledgeGraph::index_node(const Node& node) {
    label_index_[node.label].insert(node.id);
    type_index_[node.type].insert(node.id);
}

void KnowledgeGraph::unindex_node(const std::string& id) {
    auto it = nodes_.find(id);
    if (it == nodes_.end()) return;

    label_index_[it->second.label].erase(id);
    type_index_[it->second.type].erase(id);
}

void KnowledgeGraph::build_adjacency() {
    adj_out_.clear();
    adj_in_.clear();
    for (const auto& [id, e] : edges_) {
        adj_out_[e.from_id].insert(id);
        adj_in_[e.to_id].insert(id);
    }
}

// ═══════════════════════════════════════════════════════════════════════════
//  QUERY BUILDER
// ═══════════════════════════════════════════════════════════════════════════

QueryBuilder::QueryBuilder() {}

QueryBuilder& QueryBuilder::match(const std::string& subject,
                                   const std::string& predicate,
                                   const std::string& object) {
    QueryPattern p;
    p.subject = subject;
    p.predicate = string_to_edge_type(predicate);
    p.object = object;
    query_.patterns.push_back(p);
    return *this;
}

QueryBuilder& QueryBuilder::where(const std::string& var,
                                   const std::string& property,
                                   const PropertyValue& value) {
    (void)var;
    if (!query_.patterns.empty()) {
        query_.patterns.back().filters[property] = value;
    }
    return *this;
}

QueryBuilder& QueryBuilder::filter(const std::string& expression) {
    (void)expression;
    return *this;
}

QueryBuilder& QueryBuilder::select(const std::vector<std::string>& vars) {
    query_.select_vars = vars;
    return *this;
}

QueryBuilder& QueryBuilder::select_all() {
    query_.select_vars.clear();
    return *this;
}

QueryBuilder& QueryBuilder::distinct() {
    query_.distinct = true;
    return *this;
}

QueryBuilder& QueryBuilder::limit(int n) {
    query_.limit = n;
    return *this;
}

QueryBuilder& QueryBuilder::offset(int n) {
    query_.offset = n;
    return *this;
}

QueryBuilder& QueryBuilder::order_by(const std::string& var, bool ascending) {
    query_.order_by = var;
    query_.ascending = ascending;
    return *this;
}

QueryBuilder& QueryBuilder::path(const std::string& from, const std::string& to) {
    (void)from;
    (void)to;
    return *this;
}

QueryBuilder& QueryBuilder::via(EdgeType type) {
    path_via_.push_back(edge_type_to_string(type));
    return *this;
}

QueryBuilder& QueryBuilder::max_depth(int depth) {
    (void)depth;
    return *this;
}

GraphQuery QueryBuilder::build() const {
    return query_;
}

QueryResult QueryBuilder::execute(KnowledgeGraph& kg) const {
    return kg.query(query_);
}

std::string QueryBuilder::to_sparql() const {
    std::ostringstream oss;
    oss << "SELECT ";
    if (query_.distinct) oss << "DISTINCT ";
    if (query_.select_vars.empty()) {
        oss << "*";
    } else {
        for (size_t i = 0; i < query_.select_vars.size(); ++i) {
            if (i > 0) oss << " ";
            oss << "?" << query_.select_vars[i];
        }
    }
    oss << " WHERE {\n";
    for (const auto& p : query_.patterns) {
        oss << "  ";
        if (p.subject) oss << *p.subject;
        oss << " ";
        if (p.predicate) oss << edge_type_to_string(*p.predicate);
        oss << " ";
        if (p.object) oss << *p.object;
        oss << " .\n";
    }
    oss << "}";
    if (query_.limit) oss << " LIMIT " << *query_.limit;
    return oss.str();
}

std::string QueryBuilder::to_cypher() const {
    std::ostringstream oss;
    oss << "MATCH ";
    for (size_t i = 0; i < query_.patterns.size(); ++i) {
        const auto& p = query_.patterns[i];
        if (i > 0) oss << ", ";
        oss << "(";
        if (p.subject) oss << *p.subject;
        oss << ")-[";
        if (p.predicate) oss << ":" << edge_type_to_string(*p.predicate);
        oss << "]->(";
        if (p.object) oss << *p.object;
        oss << ")";
    }
    oss << "\nRETURN ";
    if (query_.select_vars.empty()) {
        oss << "*";
    } else {
        for (size_t i = 0; i < query_.select_vars.size(); ++i) {
            if (i > 0) oss << ", ";
            oss << query_.select_vars[i];
        }
    }
    if (query_.limit) oss << " LIMIT " << *query_.limit;
    return oss.str();
}

// ═══════════════════════════════════════════════════════════════════════════
//  HELPER FUNCTIONS
// ═══════════════════════════════════════════════════════════════════════════

std::string edge_type_to_string(EdgeType type) {
    switch (type) {
        case EdgeType::IS_A: return "IS_A";
        case EdgeType::PART_OF: return "PART_OF";
        case EdgeType::HAS_PART: return "HAS_PART";
        case EdgeType::RELATED_TO: return "RELATED_TO";
        case EdgeType::SIMILAR_TO: return "SIMILAR_TO";
        case EdgeType::OPPOSITE_OF: return "OPPOSITE_OF";
        case EdgeType::SYNONYM_OF: return "SYNONYM_OF";
        case EdgeType::CAUSES: return "CAUSES";
        case EdgeType::CAUSED_BY: return "CAUSED_BY";
        case EdgeType::ENABLES: return "ENABLES";
        case EdgeType::PREVENTS: return "PREVENTS";
        case EdgeType::BEFORE: return "BEFORE";
        case EdgeType::AFTER: return "AFTER";
        case EdgeType::DURING: return "DURING";
        case EdgeType::LOCATED_IN: return "LOCATED_IN";
        case EdgeType::NEAR: return "NEAR";
        case EdgeType::CONTAINS: return "CONTAINS";
        case EdgeType::HAS_PROPERTY: return "HAS_PROPERTY";
        case EdgeType::HAS_VALUE: return "HAS_VALUE";
        case EdgeType::DERIVED_FROM: return "DERIVED_FROM";
        case EdgeType::INFERRED_BY: return "INFERRED_BY";
        case EdgeType::CUSTOM: return "CUSTOM";
    }
    return "UNKNOWN";
}

EdgeType string_to_edge_type(const std::string& str) {
    if (str == "IS_A") return EdgeType::IS_A;
    if (str == "PART_OF") return EdgeType::PART_OF;
    if (str == "HAS_PART") return EdgeType::HAS_PART;
    if (str == "RELATED_TO") return EdgeType::RELATED_TO;
    if (str == "SIMILAR_TO") return EdgeType::SIMILAR_TO;
    if (str == "OPPOSITE_OF") return EdgeType::OPPOSITE_OF;
    if (str == "SYNONYM_OF") return EdgeType::SYNONYM_OF;
    if (str == "CAUSES") return EdgeType::CAUSES;
    if (str == "CAUSED_BY") return EdgeType::CAUSED_BY;
    if (str == "ENABLES") return EdgeType::ENABLES;
    if (str == "PREVENTS") return EdgeType::PREVENTS;
    if (str == "BEFORE") return EdgeType::BEFORE;
    if (str == "AFTER") return EdgeType::AFTER;
    if (str == "DURING") return EdgeType::DURING;
    if (str == "LOCATED_IN") return EdgeType::LOCATED_IN;
    if (str == "NEAR") return EdgeType::NEAR;
    if (str == "CONTAINS") return EdgeType::CONTAINS;
    if (str == "HAS_PROPERTY") return EdgeType::HAS_PROPERTY;
    if (str == "HAS_VALUE") return EdgeType::HAS_VALUE;
    if (str == "DERIVED_FROM") return EdgeType::DERIVED_FROM;
    if (str == "INFERRED_BY") return EdgeType::INFERRED_BY;
    return EdgeType::CUSTOM;
}

std::string node_type_to_string(NodeType type) {
    switch (type) {
        case NodeType::ENTITY: return "ENTITY";
        case NodeType::CONCEPT: return "CONCEPT";
        case NodeType::EVENT: return "EVENT";
        case NodeType::PROPERTY: return "PROPERTY";
        case NodeType::LITERAL: return "LITERAL";
        case NodeType::RULE: return "RULE";
        case NodeType::QUERY: return "QUERY";
        case NodeType::CONTEXT: return "CONTEXT";
    }
    return "UNKNOWN";
}

NodeType string_to_node_type(const std::string& str) {
    if (str == "ENTITY") return NodeType::ENTITY;
    if (str == "CONCEPT") return NodeType::CONCEPT;
    if (str == "EVENT") return NodeType::EVENT;
    if (str == "PROPERTY") return NodeType::PROPERTY;
    if (str == "LITERAL") return NodeType::LITERAL;
    if (str == "RULE") return NodeType::RULE;
    if (str == "QUERY") return NodeType::QUERY;
    if (str == "CONTEXT") return NodeType::CONTEXT;
    return NodeType::ENTITY;
}

std::string property_to_string(const PropertyValue& value) {
    if (std::holds_alternative<std::string>(value)) {
        return std::get<std::string>(value);
    }
    if (std::holds_alternative<int64_t>(value)) {
        return std::to_string(std::get<int64_t>(value));
    }
    if (std::holds_alternative<double>(value)) {
        return std::to_string(std::get<double>(value));
    }
    if (std::holds_alternative<bool>(value)) {
        return std::get<bool>(value) ? "true" : "false";
    }
    if (std::holds_alternative<std::vector<std::string>>(value)) {
        const auto& vec = std::get<std::vector<std::string>>(value);
        std::ostringstream oss;
        oss << "[";
        for (size_t i = 0; i < vec.size(); ++i) {
            if (i > 0) oss << ", ";
            oss << "\"" << vec[i] << "\"";
        }
        oss << "]";
        return oss.str();
    }
    return "";
}

KnowledgeGraph& kg() {
    return KnowledgeGraph::instance();
}

} // namespace rael
