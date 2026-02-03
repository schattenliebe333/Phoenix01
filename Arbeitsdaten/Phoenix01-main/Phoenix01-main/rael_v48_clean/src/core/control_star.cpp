#include "rael/control_star.h"
#include "rael/events.h"
#include <chrono>
#include <algorithm>
#include <numeric>
#include <sstream>

namespace rael {

// Global instance
StarRingDocking gStarDocking;

// ═══════════════════════════════════════════════════════════════════════════
// CONTROL STAR IMPLEMENTATION
// ═══════════════════════════════════════════════════════════════════════════

ControlStar::ControlStar(uint64_t id, StarType type)
    : id_(id), type_(type) {}

StarResult ControlStar::execute(const std::string& input) {
    StarResult result;
    result.star_id = id_;
    result.type = type_;

    if (!enabled_) {
        result.success = false;
        result.error = "Star is disabled";
        return result;
    }

    if (!handler_) {
        result.success = false;
        result.error = "No handler configured";
        return result;
    }

    auto start = std::chrono::steady_clock::now();

    try {
        result = handler_(input);
        result.star_id = id_;
        result.type = type_;
        result.success = true;
    } catch (const std::exception& e) {
        result.success = false;
        result.error = e.what();
    } catch (...) {
        result.success = false;
        result.error = "Unknown error";
    }

    auto end = std::chrono::steady_clock::now();
    auto duration_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();

    result.execution_time_ms = duration_ns / 1e6;
    result.weight = weight_;

    // Statistiken aktualisieren
    executions_.fetch_add(1);
    if (result.success) {
        successes_.fetch_add(1);
    }
    total_time_ns_.fetch_add(duration_ns);

    return result;
}

void ControlStar::set_handler(std::function<StarResult(const std::string&)> handler) {
    handler_ = handler;
}

double ControlStar::avg_time_ms() const {
    uint64_t execs = executions_.load();
    if (execs == 0) return 0.0;
    return (total_time_ns_.load() / 1e6) / execs;
}

double ControlStar::success_rate() const {
    uint64_t execs = executions_.load();
    if (execs == 0) return 1.0;
    return static_cast<double>(successes_.load()) / execs;
}

// ═══════════════════════════════════════════════════════════════════════════
// STAR RING IMPLEMENTATION
// ═══════════════════════════════════════════════════════════════════════════

StarRing::StarRing(const std::string& name) : name_(name) {}

uint64_t StarRing::add_star(StarType type) {
    std::lock_guard<std::mutex> lock(mutex_);

    if (stars_.size() >= MAX_STARS) {
        EventBus::push("STAR_RING_FULL", "ring=" + name_);
        return 0;
    }

    uint64_t id = next_id_++;
    stars_.push_back(std::make_unique<ControlStar>(id, type));

    EventBus::push("STAR_ADDED",
        "ring=" + name_ + "|id=" + std::to_string(id) + "|type=" + star_type_name(type));

    return id;
}

bool StarRing::remove_star(uint64_t id) {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = std::find_if(stars_.begin(), stars_.end(),
        [id](const std::unique_ptr<ControlStar>& s) { return s->id() == id; });

    if (it == stars_.end()) {
        return false;
    }

    stars_.erase(it);
    EventBus::push("STAR_REMOVED", "ring=" + name_ + "|id=" + std::to_string(id));
    return true;
}

ControlStar* StarRing::get_star(uint64_t id) {
    std::lock_guard<std::mutex> lock(mutex_);

    for (auto& star : stars_) {
        if (star->id() == id) {
            return star.get();
        }
    }
    return nullptr;
}

std::vector<ControlStar*> StarRing::get_stars_by_type(StarType type) {
    std::lock_guard<std::mutex> lock(mutex_);

    std::vector<ControlStar*> result;
    for (auto& star : stars_) {
        if (star->type() == type) {
            result.push_back(star.get());
        }
    }
    return result;
}

std::vector<StarResult> StarRing::execute_all(const std::string& input) {
    std::vector<StarResult> results;
    std::vector<std::future<StarResult>> futures;

    {
        std::lock_guard<std::mutex> lock(mutex_);

        for (auto& star : stars_) {
            if (star->enabled()) {
                futures.push_back(std::async(std::launch::async,
                    [&star, &input]() { return star->execute(input); }));
            }
        }
    }

    for (auto& future : futures) {
        results.push_back(future.get());
    }

    EventBus::push("STAR_RING_EXEC",
        "ring=" + name_ + "|stars=" + std::to_string(results.size()));

    return results;
}

std::vector<StarResult> StarRing::execute_by_type(StarType type, const std::string& input) {
    std::vector<StarResult> results;
    std::vector<std::future<StarResult>> futures;

    {
        std::lock_guard<std::mutex> lock(mutex_);

        for (auto& star : stars_) {
            if (star->enabled() && star->type() == type) {
                futures.push_back(std::async(std::launch::async,
                    [&star, &input]() { return star->execute(input); }));
            }
        }
    }

    for (auto& future : futures) {
        results.push_back(future.get());
    }

    return results;
}

std::future<std::vector<StarResult>> StarRing::execute_async(const std::string& input) {
    return std::async(std::launch::async, [this, input]() {
        return execute_all(input);
    });
}

// ═══════════════════════════════════════════════════════════════════════════
// RESULT COMBINER IMPLEMENTATION
// ═══════════════════════════════════════════════════════════════════════════

ResultCombiner::ResultCombiner(CombineStrategy strategy) : strategy_(strategy) {}

StarResult ResultCombiner::combine(const std::vector<StarResult>& results) {
    if (results.empty()) {
        StarResult empty;
        empty.error = "No results to combine";
        return empty;
    }

    switch (strategy_) {
        case CombineStrategy::WEIGHTED_AVERAGE:
            return combine_weighted(results);
        case CombineStrategy::BEST_CONFIDENCE:
            return combine_best(results);
        case CombineStrategy::CONSENSUS:
            return combine_consensus(results);
        case CombineStrategy::CASCADE:
            return combine_cascade(results);
        default:
            return combine_best(results);
    }
}

StarResult ResultCombiner::combine_weighted(const std::vector<StarResult>& results) {
    StarResult combined;
    combined.success = true;

    double total_weight = 0.0;
    double weighted_confidence = 0.0;
    double weighted_quality = 0.0;
    std::ostringstream output;

    for (const auto& r : results) {
        if (r.success && r.confidence >= min_confidence_) {
            double w = r.weight * r.confidence;
            total_weight += w;
            weighted_confidence += r.confidence * w;
            weighted_quality += r.quality * w;

            if (!r.output.empty()) {
                output << "[" << star_type_name(r.type) << "] " << r.output << "\n";
            }
        }
    }

    if (total_weight > 0.0) {
        combined.confidence = weighted_confidence / total_weight;
        combined.quality = weighted_quality / total_weight;
        combined.output = output.str();
    } else {
        combined.success = false;
        combined.error = "No valid results above confidence threshold";
    }

    return combined;
}

StarResult ResultCombiner::combine_best(const std::vector<StarResult>& results) {
    StarResult best;
    best.confidence = -1.0;

    for (const auto& r : results) {
        if (r.success && r.confidence > best.confidence) {
            best = r;
        }
    }

    if (best.confidence < 0.0) {
        best.success = false;
        best.error = "No successful results";
    }

    return best;
}

StarResult ResultCombiner::combine_consensus(const std::vector<StarResult>& results) {
    // Zähle gleiche Outputs
    std::unordered_map<std::string, int> output_counts;
    std::unordered_map<std::string, StarResult> output_results;

    for (const auto& r : results) {
        if (r.success) {
            output_counts[r.output]++;
            if (output_results.find(r.output) == output_results.end() ||
                r.confidence > output_results[r.output].confidence) {
                output_results[r.output] = r;
            }
        }
    }

    // Finde Konsens
    int max_count = 0;
    std::string consensus_output;

    for (const auto& kv : output_counts) {
        if (kv.second > max_count) {
            max_count = kv.second;
            consensus_output = kv.first;
        }
    }

    double consensus_ratio = static_cast<double>(max_count) / results.size();

    if (consensus_ratio >= consensus_threshold_) {
        StarResult result = output_results[consensus_output];
        result.confidence *= consensus_ratio;  // Boost für Konsens
        return result;
    }

    StarResult no_consensus;
    no_consensus.success = false;
    no_consensus.error = "No consensus reached";
    return no_consensus;
}

StarResult ResultCombiner::combine_cascade(const std::vector<StarResult>& results) {
    for (const auto& r : results) {
        if (r.success && r.confidence >= min_confidence_) {
            return r;
        }
    }

    StarResult no_result;
    no_result.success = false;
    no_result.error = "No result in cascade met threshold";
    return no_result;
}

// ═══════════════════════════════════════════════════════════════════════════
// STAR RING DOCKING IMPLEMENTATION
// ═══════════════════════════════════════════════════════════════════════════

StarRingDocking::StarRingDocking() {}

void StarRingDocking::dock_ring(std::shared_ptr<StarRing> ring) {
    std::lock_guard<std::mutex> lock(mutex_);
    rings_.push_back(ring);
    EventBus::push("RING_DOCKED", "ring=" + ring->name());
}

void StarRingDocking::undock_ring(const std::string& name) {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = std::find_if(rings_.begin(), rings_.end(),
        [&name](const std::shared_ptr<StarRing>& r) { return r->name() == name; });

    if (it != rings_.end()) {
        rings_.erase(it);
        EventBus::push("RING_UNDOCKED", "ring=" + name);
    }
}

std::shared_ptr<StarRing> StarRingDocking::get_ring(const std::string& name) {
    std::lock_guard<std::mutex> lock(mutex_);

    for (auto& ring : rings_) {
        if (ring->name() == name) {
            return ring;
        }
    }
    return nullptr;
}

std::vector<std::string> StarRingDocking::list_rings() const {
    std::lock_guard<std::mutex> lock(mutex_);

    std::vector<std::string> names;
    for (const auto& ring : rings_) {
        names.push_back(ring->name());
    }
    return names;
}

std::vector<StarResult> StarRingDocking::broadcast(const std::string& input) {
    std::vector<StarResult> all_results;
    std::vector<std::future<std::vector<StarResult>>> futures;

    {
        std::lock_guard<std::mutex> lock(mutex_);

        for (auto& ring : rings_) {
            futures.push_back(ring->execute_async(input));
        }
    }

    for (auto& future : futures) {
        auto results = future.get();
        all_results.insert(all_results.end(), results.begin(), results.end());
    }

    EventBus::push("DOCKING_BROADCAST",
        "rings=" + std::to_string(rings_.size()) +
        "|results=" + std::to_string(all_results.size()));

    return all_results;
}

std::vector<StarResult> StarRingDocking::route_to(const std::string& ring_name, const std::string& input) {
    auto ring = get_ring(ring_name);
    if (!ring) {
        return {};
    }
    return ring->execute_all(input);
}

StarResult StarRingDocking::combine_all(const std::string& input, CombineStrategy strategy) {
    auto results = broadcast(input);
    combiner_.set_strategy(strategy);
    return combiner_.combine(results);
}

size_t StarRingDocking::total_star_count() const {
    std::lock_guard<std::mutex> lock(mutex_);

    size_t total = 0;
    for (const auto& ring : rings_) {
        total += ring->star_count();
    }
    return total;
}

} // namespace rael
