#include "rael/depth_scaling.h"
#include "rael/events.h"
#include <algorithm>

namespace rael {

// Global instances
DepthScaler gDepthScaler;
FastLaneProtector gFastLaneProtector;

// ═══════════════════════════════════════════════════════════════════════════
// ACTIVATION CACHE IMPLEMENTATION
// ═══════════════════════════════════════════════════════════════════════════

ActivationCache::ActivationCache(const std::string& name, size_t initial_depth)
    : name_(name)
    , buffer_(initial_depth)
    , current_depth_(initial_depth)
    , last_scale_check_(std::chrono::steady_clock::now()) {

    config_ = get_depth_config(ActivationLevel::STANDARD);
}

void ActivationCache::push(double value) {
    std::lock_guard<std::mutex> lock(mutex_);
    buffer_.push(value);
    total_pushes_.fetch_add(1);

    // Auto-scale check
    if (auto_scale_) {
        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            now - last_scale_check_).count();

        if (elapsed > config_.compute_interval_ms) {
            last_scale_check_ = now;
            // Unlock for auto_scale (it locks internally)
            mutex_.unlock();
            auto_scale();
            mutex_.lock();
        }
    }
}

void ActivationCache::push_batch(const std::vector<double>& values) {
    std::lock_guard<std::mutex> lock(mutex_);
    for (double v : values) {
        buffer_.push(v);
    }
    total_pushes_.fetch_add(values.size());
}

double ActivationCache::utilization() const {
    std::lock_guard<std::mutex> lock(mutex_);
    if (current_depth_ == 0) return 0.0;
    return static_cast<double>(buffer_.size()) / current_depth_;
}

void ActivationCache::scale_to(ActivationLevel new_level) {
    if (gFastLaneProtector.is_protected(name_)) {
        EventBus::push("DEPTH_SCALE_BLOCKED",
            "cache=" + name_ + "|reason=protected");
        return;
    }

    ActivationLevel old_level = level_;
    if (new_level == old_level) return;

    DepthConfig new_config = get_depth_config(new_level);

    std::lock_guard<std::mutex> lock(mutex_);

    level_ = new_level;
    config_ = new_config;

    if (new_config.buffer_size != current_depth_) {
        rebuild_buffer(new_config.buffer_size);

        if (new_config.buffer_size > current_depth_) {
            scale_ups_.fetch_add(1);
        } else {
            scale_downs_.fetch_add(1);
        }
    }

    EventBus::push("DEPTH_SCALED",
        "cache=" + name_ +
        "|from=" + activation_level_name(old_level) +
        "|to=" + activation_level_name(new_level) +
        "|depth=" + std::to_string(new_config.buffer_size));
}

void ActivationCache::auto_scale() {
    if (!auto_scale_ || gFastLaneProtector.is_protected(name_)) {
        return;
    }

    double util = utilization();
    ActivationLevel new_level = level_;

    // Hochskalieren
    if (util > config_.scale_up_threshold) {
        switch (level_) {
            case ActivationLevel::DORMANT:  new_level = ActivationLevel::MINIMAL; break;
            case ActivationLevel::MINIMAL:  new_level = ActivationLevel::STANDARD; break;
            case ActivationLevel::STANDARD: new_level = ActivationLevel::ELEVATED; break;
            case ActivationLevel::ELEVATED: new_level = ActivationLevel::MAXIMUM; break;
            default: break;
        }
    }
    // Runterskalieren
    else if (util < config_.scale_down_threshold) {
        switch (level_) {
            case ActivationLevel::MAXIMUM:  new_level = ActivationLevel::ELEVATED; break;
            case ActivationLevel::ELEVATED: new_level = ActivationLevel::STANDARD; break;
            case ActivationLevel::STANDARD: new_level = ActivationLevel::MINIMAL; break;
            case ActivationLevel::MINIMAL:  new_level = ActivationLevel::DORMANT; break;
            default: break;
        }
    }

    if (new_level != level_) {
        scale_to(new_level);
    }
}

void ActivationCache::rebuild_buffer(size_t new_depth) {
    // Erstelle neuen Buffer und kopiere relevante Daten
    SelfComputingRingBuffer new_buffer(new_depth);

    // Kopiere die letzten Werte (so viele wie möglich)
    // Da SelfComputingRingBuffer keine direkte Iterator-Unterstützung hat,
    // müssen wir die Statistiken übernehmen ohne die Rohdaten

    current_depth_ = new_depth;
    buffer_ = SelfComputingRingBuffer(new_depth);

    // Hinweis: Hier gehen historische Daten verloren
    // In einer vollständigen Implementierung würde man die Daten kopieren
}

// ═══════════════════════════════════════════════════════════════════════════
// DEPTH SCALER IMPLEMENTATION
// ═══════════════════════════════════════════════════════════════════════════

DepthScaler::DepthScaler() {}

ActivationCache* DepthScaler::create_cache(const std::string& name, size_t initial_depth) {
    std::lock_guard<std::mutex> lock(mutex_);

    if (caches_.find(name) != caches_.end()) {
        return caches_[name].get();
    }

    auto cache = std::make_unique<ActivationCache>(name, initial_depth);
    ActivationCache* ptr = cache.get();
    caches_[name] = std::move(cache);

    EventBus::push("CACHE_CREATED",
        "name=" + name + "|depth=" + std::to_string(initial_depth));

    return ptr;
}

ActivationCache* DepthScaler::get_cache(const std::string& name) {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = caches_.find(name);
    if (it != caches_.end()) {
        return it->second.get();
    }
    return nullptr;
}

bool DepthScaler::remove_cache(const std::string& name) {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = caches_.find(name);
    if (it != caches_.end()) {
        caches_.erase(it);
        EventBus::push("CACHE_REMOVED", "name=" + name);
        return true;
    }
    return false;
}

void DepthScaler::scale_all(ActivationLevel level) {
    std::lock_guard<std::mutex> lock(mutex_);

    for (auto& kv : caches_) {
        ActivationLevel old_level = kv.second->level();
        kv.second->scale_to(level);

        if (scale_callback_ && old_level != level) {
            scale_callback_(kv.first, old_level, level);
        }
    }

    EventBus::push("SCALE_ALL",
        "level=" + std::string(activation_level_name(level)) +
        "|caches=" + std::to_string(caches_.size()));
}

void DepthScaler::auto_scale_all() {
    std::lock_guard<std::mutex> lock(mutex_);

    for (auto& kv : caches_) {
        kv.second->auto_scale();
    }
}

size_t DepthScaler::total_depth() const {
    std::lock_guard<std::mutex> lock(mutex_);

    size_t total = 0;
    for (const auto& kv : caches_) {
        total += kv.second->depth();
    }
    return total;
}

double DepthScaler::average_utilization() const {
    std::lock_guard<std::mutex> lock(mutex_);

    if (caches_.empty()) return 0.0;

    double sum = 0.0;
    for (const auto& kv : caches_) {
        sum += kv.second->utilization();
    }
    return sum / caches_.size();
}

// ═══════════════════════════════════════════════════════════════════════════
// FAST LANE PROTECTOR IMPLEMENTATION
// ═══════════════════════════════════════════════════════════════════════════

void FastLaneProtector::protect_lane(const std::string& cache_name) {
    std::lock_guard<std::mutex> lock(mutex_);
    protected_lanes_[cache_name] = true;
    EventBus::push("LANE_PROTECTED", "cache=" + cache_name);
}

void FastLaneProtector::unprotect_lane(const std::string& cache_name) {
    std::lock_guard<std::mutex> lock(mutex_);
    protected_lanes_.erase(cache_name);
    EventBus::push("LANE_UNPROTECTED", "cache=" + cache_name);
}

bool FastLaneProtector::is_protected(const std::string& cache_name) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = protected_lanes_.find(cache_name);
    return it != protected_lanes_.end() && it->second;
}

bool FastLaneProtector::can_scale(const std::string& cache_name) const {
    return !is_protected(cache_name);
}

FastLaneProtector::ScopedProtection::ScopedProtection(
    FastLaneProtector& protector, const std::string& name)
    : protector_(protector), name_(name) {
    protector_.protect_lane(name_);
}

FastLaneProtector::ScopedProtection::~ScopedProtection() {
    protector_.unprotect_lane(name_);
}

} // namespace rael
