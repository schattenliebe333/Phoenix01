#pragma once
// ═══════════════════════════════════════════════════════════════════════════
// RAEL 10.0 - AETHER RING (Self-Computing Ring Buffer)
// ═══════════════════════════════════════════════════════════════════════════
// O(1) aggregates: min/max/sum/sumsq/trend
// Cached entropy (O(k) over buckets)
// Foundation for the 5-Quint Nervous System
// ═══════════════════════════════════════════════════════════════════════════

#include <deque>
#include <unordered_map>
#include <cmath>
#include <cstdint>
#include <limits>

namespace rael {

class SelfComputingRingBuffer {
public:
    explicit SelfComputingRingBuffer(size_t cap = 64) : cap_(cap) {}

    size_t capacity() const { return cap_; }
    size_t size() const { return buf_.size(); }
    bool empty() const { return buf_.empty(); }

    void push(double v) {
        // Eviction
        if (buf_.size() == cap_) {
            double old = buf_.front();
            buf_.pop_front();
            sum_ -= old;
            sumsq_ -= old * old;
            
            // Update counts bucket
            auto b = bucket(old);
            auto it = counts_.find(b);
            if (it != counts_.end()) {
                if (--it->second <= 0) counts_.erase(it);
            }
            entropy_dirty_ = true;

            // Monotonic queues eviction
            if (!minq_.empty() && minq_.front() == old) minq_.pop_front();
            if (!maxq_.empty() && maxq_.front() == old) maxq_.pop_front();
        }

        buf_.push_back(v);
        sum_ += v;
        sumsq_ += v * v;
        
        // Counts for entropy
        counts_[bucket(v)] += 1;
        entropy_dirty_ = true;

        // Monotonic deques for O(1) min/max
        while (!minq_.empty() && minq_.back() > v) minq_.pop_back();
        minq_.push_back(v);
        while (!maxq_.empty() && maxq_.back() < v) maxq_.pop_back();
        maxq_.push_back(v);
    }

    // ─────────────────────────────────────────────────────────────────────
    // Statistical Aggregates (all O(1) except entropy)
    // ─────────────────────────────────────────────────────────────────────
    
    double sum() const { return sum_; }
    
    double mean() const {
        if (buf_.empty()) return 0.0;
        return sum_ / (double)buf_.size();
    }
    
    double variance() const {
        if (buf_.size() < 2) return 0.0;
        double m = mean();
        double v = (sumsq_ / (double)buf_.size()) - m * m;
        return v < 0.0 ? 0.0 : v;
    }
    
    double stddev() const { return std::sqrt(variance()); }

    double min() const {
        if (minq_.empty()) return 0.0;
        return minq_.front();
    }
    
    double max() const {
        if (maxq_.empty()) return 0.0;
        return maxq_.front();
    }

    double first() const { return buf_.empty() ? 0.0 : buf_.front(); }
    double last() const { return buf_.empty() ? 0.0 : buf_.back(); }

    double trend() const {
        if (buf_.size() < 2) return 0.0;
        return last() - first();
    }
    
    double range() const {
        return max() - min();
    }

    // ─────────────────────────────────────────────────────────────────────
    // Entropy (cached, O(k) on recompute)
    // ─────────────────────────────────────────────────────────────────────
    
    double entropy() const {
        if (!entropy_dirty_) return cached_entropy_;
        if (buf_.empty()) { 
            cached_entropy_ = 0.0; 
            entropy_dirty_ = false; 
            return 0.0; 
        }
        
        double n = (double)buf_.size();
        double H = 0.0;
        for (const auto& kv : counts_) {
            double p = (double)kv.second / n;
            if (p > 0.0) H -= p * std::log(p);
        }
        cached_entropy_ = H;
        entropy_dirty_ = false;
        return cached_entropy_;
    }

    // ─────────────────────────────────────────────────────────────────────
    // Advanced: Rate of change (derivative approximation)
    // ─────────────────────────────────────────────────────────────────────
    
    double velocity() const {
        if (buf_.size() < 2) return 0.0;
        return trend() / (double)(buf_.size() - 1);
    }
    
    double acceleration() const {
        if (buf_.size() < 3) return 0.0;
        // Simple second derivative approximation
        size_t mid = buf_.size() / 2;
        double v1 = 0.0, v2 = 0.0;
        size_t i = 0;
        for (const auto& x : buf_) {
            if (i < mid) v1 += x;
            else v2 += x;
            ++i;
        }
        v1 /= mid;
        v2 /= (buf_.size() - mid);
        return v2 - v1;
    }

private:
    static int64_t bucket(double v) {
        // Quantize to 1e-3 for entropy stability
        return (int64_t)llround(v * 1000.0);
    }

    size_t cap_{64};
    std::deque<double> buf_;
    std::deque<double> minq_;
    std::deque<double> maxq_;
    double sum_{0.0};
    double sumsq_{0.0};

    mutable bool entropy_dirty_{true};
    mutable double cached_entropy_{0.0};
    std::unordered_map<int64_t, int> counts_;
};

} // namespace rael
