// RAEL V49 - Distributed Tracing (#39) & Metrics Dashboard (#40)
// Observability and monitoring
#pragma once

#include <string>
#include <vector>
#include <map>
#include <memory>
#include <functional>
#include <optional>
#include <mutex>
#include <chrono>
#include <atomic>

namespace rael {

// ═══════════════════════════════════════════════════════════════════════════
//  DISTRIBUTED TRACING (#39)
// ═══════════════════════════════════════════════════════════════════════════

struct SpanContext {
    std::string trace_id;
    std::string span_id;
    std::string parent_span_id;
    std::map<std::string, std::string> baggage;
    bool sampled = true;
};

enum class SpanKind {
    INTERNAL,
    SERVER,
    CLIENT,
    PRODUCER,
    CONSUMER
};

struct SpanEvent {
    std::string name;
    std::chrono::system_clock::time_point timestamp;
    std::map<std::string, std::string> attributes;
};

struct SpanLink {
    SpanContext context;
    std::map<std::string, std::string> attributes;
};

class Span {
public:
    Span(const std::string& name, SpanKind kind = SpanKind::INTERNAL);
    ~Span();

    // Context
    SpanContext context() const;
    void set_parent(const SpanContext& parent);

    // Attributes
    void set_attribute(const std::string& key, const std::string& value);
    void set_attribute(const std::string& key, int64_t value);
    void set_attribute(const std::string& key, double value);
    void set_attribute(const std::string& key, bool value);
    std::map<std::string, std::string> attributes() const;

    // Events
    void add_event(const std::string& name,
                   const std::map<std::string, std::string>& attrs = {});
    std::vector<SpanEvent> events() const;

    // Links
    void add_link(const SpanContext& ctx,
                  const std::map<std::string, std::string>& attrs = {});

    // Status
    void set_status(bool ok, const std::string& description = "");
    void record_exception(const std::string& type, const std::string& message);

    // Timing
    void end();
    std::chrono::system_clock::time_point start_time() const;
    std::chrono::system_clock::time_point end_time() const;
    std::chrono::microseconds duration() const;

    // Name
    std::string name() const;
    SpanKind kind() const;

private:
    std::string name_;
    SpanKind kind_;
    SpanContext context_;
    std::map<std::string, std::string> attributes_;
    std::vector<SpanEvent> events_;
    std::vector<SpanLink> links_;
    std::chrono::system_clock::time_point start_;
    std::chrono::system_clock::time_point end_;
    bool ok_ = true;
    std::string status_description_;
    bool ended_ = false;
};

class Tracer {
public:
    Tracer(const std::string& service_name);

    // Span creation
    std::shared_ptr<Span> start_span(const std::string& name,
                                      SpanKind kind = SpanKind::INTERNAL);
    std::shared_ptr<Span> start_span(const std::string& name,
                                      const SpanContext& parent,
                                      SpanKind kind = SpanKind::INTERNAL);

    // Context propagation
    std::string inject(const SpanContext& ctx);  // To HTTP headers
    SpanContext extract(const std::string& carrier);

    // Current span
    void set_current_span(std::shared_ptr<Span> span);
    std::shared_ptr<Span> current_span();

    // Service info
    std::string service_name() const;

private:
    std::string service_name_;
    thread_local static std::shared_ptr<Span> current_span_;
    mutable std::mutex mutex_;
};

class SpanExporter {
public:
    virtual ~SpanExporter() = default;
    virtual void export_span(const Span& span) = 0;
    virtual void flush() = 0;
};

class ConsoleSpanExporter : public SpanExporter {
public:
    void export_span(const Span& span) override;
    void flush() override;
};

class JaegerExporter : public SpanExporter {
public:
    JaegerExporter(const std::string& endpoint);
    void export_span(const Span& span) override;
    void flush() override;

private:
    std::string endpoint_;
    std::vector<Span> buffer_;
    mutable std::mutex mutex_;
};

// ═══════════════════════════════════════════════════════════════════════════
//  METRICS (#40)
// ═══════════════════════════════════════════════════════════════════════════

enum class MetricType {
    COUNTER,
    GAUGE,
    HISTOGRAM,
    SUMMARY
};

struct MetricLabel {
    std::string name;
    std::string value;
};

class Counter {
public:
    Counter(const std::string& name, const std::string& help);

    void inc(double value = 1.0);
    void inc(const std::vector<MetricLabel>& labels, double value = 1.0);
    double value(const std::vector<MetricLabel>& labels = {}) const;

    std::string name() const;
    std::string help() const;

private:
    std::string name_;
    std::string help_;
    std::map<std::string, std::atomic<double>> values_;
    mutable std::mutex mutex_;
};

class Gauge {
public:
    Gauge(const std::string& name, const std::string& help);

    void set(double value);
    void set(const std::vector<MetricLabel>& labels, double value);
    void inc(double value = 1.0);
    void dec(double value = 1.0);
    double value(const std::vector<MetricLabel>& labels = {}) const;

    std::string name() const;
    std::string help() const;

private:
    std::string name_;
    std::string help_;
    std::map<std::string, std::atomic<double>> values_;
    mutable std::mutex mutex_;
};

class Histogram {
public:
    Histogram(const std::string& name, const std::string& help,
              const std::vector<double>& buckets = {});

    void observe(double value);
    void observe(const std::vector<MetricLabel>& labels, double value);

    double sum(const std::vector<MetricLabel>& labels = {}) const;
    uint64_t count(const std::vector<MetricLabel>& labels = {}) const;
    std::map<double, uint64_t> bucket_counts(const std::vector<MetricLabel>& labels = {}) const;

    std::string name() const;
    std::string help() const;

private:
    std::string name_;
    std::string help_;
    std::vector<double> buckets_;

    struct Data {
        std::atomic<double> sum{0};
        std::atomic<uint64_t> count{0};
        std::map<double, std::atomic<uint64_t>> buckets;
    };
    std::map<std::string, Data> data_;
    mutable std::mutex mutex_;
};

class Summary {
public:
    Summary(const std::string& name, const std::string& help,
            const std::vector<double>& quantiles = {0.5, 0.9, 0.99});

    void observe(double value);
    double quantile(double q) const;
    double sum() const;
    uint64_t count() const;

    std::string name() const;
    std::string help() const;

private:
    std::string name_;
    std::string help_;
    std::vector<double> quantiles_;
    std::vector<double> values_;
    std::atomic<double> sum_{0};
    std::atomic<uint64_t> count_{0};
    mutable std::mutex mutex_;
};

// ═══════════════════════════════════════════════════════════════════════════
//  METRICS REGISTRY
// ═══════════════════════════════════════════════════════════════════════════

class MetricsRegistry {
public:
    MetricsRegistry();

    // Create metrics
    Counter& counter(const std::string& name, const std::string& help);
    Gauge& gauge(const std::string& name, const std::string& help);
    Histogram& histogram(const std::string& name, const std::string& help,
                         const std::vector<double>& buckets = {});
    Summary& summary(const std::string& name, const std::string& help,
                     const std::vector<double>& quantiles = {});

    // Export
    std::string export_prometheus() const;
    std::string export_json() const;

    // List
    std::vector<std::string> list_metrics() const;

private:
    std::map<std::string, std::unique_ptr<Counter>> counters_;
    std::map<std::string, std::unique_ptr<Gauge>> gauges_;
    std::map<std::string, std::unique_ptr<Histogram>> histograms_;
    std::map<std::string, std::unique_ptr<Summary>> summaries_;
    mutable std::mutex mutex_;
};

// ═══════════════════════════════════════════════════════════════════════════
//  DASHBOARD
// ═══════════════════════════════════════════════════════════════════════════

struct DashboardPanel {
    std::string id;
    std::string title;
    std::string type;  // graph, gauge, table, text
    std::vector<std::string> metrics;
    std::map<std::string, std::string> options;
    int row = 0;
    int col = 0;
    int width = 6;
    int height = 4;
};

struct Dashboard {
    std::string id;
    std::string title;
    std::string description;
    std::vector<DashboardPanel> panels;
    std::chrono::seconds refresh_interval{30};
    std::map<std::string, std::string> variables;
};

class DashboardManager {
public:
    DashboardManager();

    // Dashboard CRUD
    void create(const Dashboard& dashboard);
    void update(const Dashboard& dashboard);
    void remove(const std::string& id);
    std::optional<Dashboard> get(const std::string& id) const;
    std::vector<Dashboard> list() const;

    // Data queries
    struct TimeSeriesPoint {
        std::chrono::system_clock::time_point timestamp;
        double value;
    };
    std::vector<TimeSeriesPoint> query_metric(const std::string& metric,
                                               std::chrono::system_clock::time_point start,
                                               std::chrono::system_clock::time_point end);

    // Export/Import
    std::string export_json(const std::string& dashboard_id) const;
    bool import_json(const std::string& json);

private:
    std::map<std::string, Dashboard> dashboards_;
    mutable std::mutex mutex_;
};

// ═══════════════════════════════════════════════════════════════════════════
//  OBSERVABILITY MANAGER
// ═══════════════════════════════════════════════════════════════════════════

struct ObservabilityConfig {
    std::string service_name = "rael";
    bool tracing_enabled = true;
    bool metrics_enabled = true;
    std::string metrics_endpoint = "/metrics";
    std::string trace_exporter = "console";
    std::string trace_endpoint;
    double sample_rate = 1.0;
};

class ObservabilityManager {
public:
    explicit ObservabilityManager(const ObservabilityConfig& config = {});
    ~ObservabilityManager();

    static ObservabilityManager& instance();

    // Tracing
    Tracer& tracer();
    void add_exporter(std::unique_ptr<SpanExporter> exporter);

    // Metrics
    MetricsRegistry& metrics();
    std::string scrape_metrics();

    // Dashboards
    DashboardManager& dashboards();

    // Built-in metrics
    void record_request(const std::string& endpoint, int status, double duration_ms);
    void record_error(const std::string& type);

    // Convenience
    std::shared_ptr<Span> start_span(const std::string& name);

    // HTTP endpoint
    void start_server(int port = 9090);
    void stop_server();

private:
    ObservabilityConfig config_;
    std::unique_ptr<Tracer> tracer_;
    std::unique_ptr<MetricsRegistry> metrics_;
    std::unique_ptr<DashboardManager> dashboards_;
    std::vector<std::unique_ptr<SpanExporter>> exporters_;

    // Built-in metrics
    Counter* request_counter_ = nullptr;
    Histogram* request_duration_ = nullptr;
    Counter* error_counter_ = nullptr;

    std::atomic<bool> server_running_{false};
    mutable std::mutex mutex_;
};

// ═══════════════════════════════════════════════════════════════════════════
//  HELPERS
// ═══════════════════════════════════════════════════════════════════════════

// RAII span
class ScopedSpan {
public:
    ScopedSpan(const std::string& name, SpanKind kind = SpanKind::INTERNAL);
    ~ScopedSpan();

    Span& span();
    void set_attribute(const std::string& key, const std::string& value);
    void add_event(const std::string& name);
    void set_error(const std::string& message);

private:
    std::shared_ptr<Span> span_;
};

// Timer for measuring duration
class Timer {
public:
    Timer();
    void start();
    void stop();
    double elapsed_ms() const;
    double elapsed_us() const;

private:
    std::chrono::high_resolution_clock::time_point start_;
    std::chrono::high_resolution_clock::time_point end_;
    bool stopped_ = false;
};

ObservabilityManager& observability();
MetricsRegistry& metrics();
Tracer& tracer();

} // namespace rael
