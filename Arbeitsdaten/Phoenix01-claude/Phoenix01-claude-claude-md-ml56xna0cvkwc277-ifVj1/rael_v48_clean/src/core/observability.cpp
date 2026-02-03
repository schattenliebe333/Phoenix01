// RAEL V49 - Observability Implementation (#39, #40)
#include "rael/observability.h"
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <random>
#include <iostream>

namespace rael {

// ═══════════════════════════════════════════════════════════════════════════
//  SPAN
// ═══════════════════════════════════════════════════════════════════════════

thread_local std::shared_ptr<Span> Tracer::current_span_ = nullptr;

static std::string generate_id() {
    static std::atomic<uint64_t> counter{0};
    std::random_device rd;
    std::mt19937_64 gen(rd());
    std::uniform_int_distribution<uint64_t> dis;
    std::ostringstream oss;
    oss << std::hex << dis(gen) << counter++;
    return oss.str();
}

Span::Span(const std::string& name, SpanKind kind)
    : name_(name), kind_(kind), start_(std::chrono::system_clock::now()) {
    context_.trace_id = generate_id();
    context_.span_id = generate_id();
    context_.sampled = true;
}

Span::~Span() {
    if (!ended_) end();
}

SpanContext Span::context() const { return context_; }

void Span::set_parent(const SpanContext& parent) {
    context_.trace_id = parent.trace_id;
    context_.parent_span_id = parent.span_id;
}

void Span::set_attribute(const std::string& key, const std::string& value) {
    attributes_[key] = value;
}

void Span::set_attribute(const std::string& key, int64_t value) {
    attributes_[key] = std::to_string(value);
}

void Span::set_attribute(const std::string& key, double value) {
    attributes_[key] = std::to_string(value);
}

void Span::set_attribute(const std::string& key, bool value) {
    attributes_[key] = value ? "true" : "false";
}

std::map<std::string, std::string> Span::attributes() const { return attributes_; }

void Span::add_event(const std::string& name, const std::map<std::string, std::string>& attrs) {
    SpanEvent event;
    event.name = name;
    event.timestamp = std::chrono::system_clock::now();
    event.attributes = attrs;
    events_.push_back(event);
}

std::vector<SpanEvent> Span::events() const { return events_; }

void Span::add_link(const SpanContext& ctx, const std::map<std::string, std::string>& attrs) {
    SpanLink link;
    link.context = ctx;
    link.attributes = attrs;
    links_.push_back(link);
}

void Span::set_status(bool ok, const std::string& description) {
    ok_ = ok;
    status_description_ = description;
}

void Span::record_exception(const std::string& type, const std::string& message) {
    add_event("exception", {{"type", type}, {"message", message}});
    set_status(false, message);
}

void Span::end() {
    if (ended_) return;
    end_ = std::chrono::system_clock::now();
    ended_ = true;
}

std::chrono::system_clock::time_point Span::start_time() const { return start_; }
std::chrono::system_clock::time_point Span::end_time() const { return end_; }

std::chrono::microseconds Span::duration() const {
    return std::chrono::duration_cast<std::chrono::microseconds>(end_ - start_);
}

std::string Span::name() const { return name_; }
SpanKind Span::kind() const { return kind_; }

// ═══════════════════════════════════════════════════════════════════════════
//  TRACER
// ═══════════════════════════════════════════════════════════════════════════

Tracer::Tracer(const std::string& service_name) : service_name_(service_name) {}

std::shared_ptr<Span> Tracer::start_span(const std::string& name, SpanKind kind) {
    auto span = std::make_shared<Span>(name, kind);
    if (current_span_) {
        span->set_parent(current_span_->context());
    }
    return span;
}

std::shared_ptr<Span> Tracer::start_span(const std::string& name, const SpanContext& parent,
                                          SpanKind kind) {
    auto span = std::make_shared<Span>(name, kind);
    span->set_parent(parent);
    return span;
}

std::string Tracer::inject(const SpanContext& ctx) {
    std::ostringstream oss;
    oss << ctx.trace_id << ":" << ctx.span_id << ":" << (ctx.sampled ? "1" : "0");
    return oss.str();
}

SpanContext Tracer::extract(const std::string& carrier) {
    SpanContext ctx;
    std::istringstream iss(carrier);
    std::string part;
    if (std::getline(iss, part, ':')) ctx.trace_id = part;
    if (std::getline(iss, part, ':')) ctx.span_id = part;
    if (std::getline(iss, part, ':')) ctx.sampled = (part == "1");
    return ctx;
}

void Tracer::set_current_span(std::shared_ptr<Span> span) {
    current_span_ = span;
}

std::shared_ptr<Span> Tracer::current_span() {
    return current_span_;
}

std::string Tracer::service_name() const { return service_name_; }

// ═══════════════════════════════════════════════════════════════════════════
//  SPAN EXPORTERS
// ═══════════════════════════════════════════════════════════════════════════

void ConsoleSpanExporter::export_span(const Span& span) {
    std::cout << "[TRACE] " << span.name()
              << " trace_id=" << span.context().trace_id
              << " span_id=" << span.context().span_id
              << " duration=" << span.duration().count() << "us"
              << std::endl;
}

void ConsoleSpanExporter::flush() {}

JaegerExporter::JaegerExporter(const std::string& endpoint) : endpoint_(endpoint) {}

void JaegerExporter::export_span(const Span& span) {
    std::lock_guard<std::mutex> lock(mutex_);
    buffer_.push_back(span);

    if (buffer_.size() >= 100) {
        flush();
    }
}

void JaegerExporter::flush() {
    std::lock_guard<std::mutex> lock(mutex_);
    // Would send to Jaeger endpoint
    buffer_.clear();
}

// ═══════════════════════════════════════════════════════════════════════════
//  METRICS
// ═══════════════════════════════════════════════════════════════════════════

Counter::Counter(const std::string& name, const std::string& help)
    : name_(name), help_(help) {}

void Counter::inc(double value) {
    inc({}, value);
}

void Counter::inc(const std::vector<MetricLabel>& labels, double value) {
    std::string key;
    for (const auto& l : labels) {
        key += l.name + "=" + l.value + ",";
    }

    std::lock_guard<std::mutex> lock(mutex_);
    if (values_.find(key) == values_.end()) {
        values_[key].store(0);
    }
    values_[key].store(values_[key].load() + value);
}

double Counter::value(const std::vector<MetricLabel>& labels) const {
    std::string key;
    for (const auto& l : labels) {
        key += l.name + "=" + l.value + ",";
    }

    std::lock_guard<std::mutex> lock(mutex_);
    auto it = values_.find(key);
    if (it != values_.end()) return it->second.load();
    return 0;
}

std::string Counter::name() const { return name_; }
std::string Counter::help() const { return help_; }

// Gauge
Gauge::Gauge(const std::string& name, const std::string& help)
    : name_(name), help_(help) {}

void Gauge::set(double value) { set({}, value); }

void Gauge::set(const std::vector<MetricLabel>& labels, double value) {
    std::string key;
    for (const auto& l : labels) {
        key += l.name + "=" + l.value + ",";
    }

    std::lock_guard<std::mutex> lock(mutex_);
    values_[key].store(value);
}

void Gauge::inc(double value) {
    std::lock_guard<std::mutex> lock(mutex_);
    values_[""].store(values_[""].load() + value);
}

void Gauge::dec(double value) {
    std::lock_guard<std::mutex> lock(mutex_);
    values_[""].store(values_[""].load() - value);
}

double Gauge::value(const std::vector<MetricLabel>& labels) const {
    std::string key;
    for (const auto& l : labels) {
        key += l.name + "=" + l.value + ",";
    }

    std::lock_guard<std::mutex> lock(mutex_);
    auto it = values_.find(key);
    if (it != values_.end()) return it->second.load();
    return 0;
}

std::string Gauge::name() const { return name_; }
std::string Gauge::help() const { return help_; }

// Histogram
Histogram::Histogram(const std::string& name, const std::string& help,
                     const std::vector<double>& buckets)
    : name_(name), help_(help)
    , buckets_(buckets.empty() ? std::vector<double>{0.005, 0.01, 0.025, 0.05, 0.1, 0.25, 0.5, 1, 2.5, 5, 10}
                               : buckets) {}

void Histogram::observe(double value) { observe({}, value); }

void Histogram::observe(const std::vector<MetricLabel>& labels, double value) {
    std::string key;
    for (const auto& l : labels) {
        key += l.name + "=" + l.value + ",";
    }

    std::lock_guard<std::mutex> lock(mutex_);
    auto& d = data_[key];
    d.sum.store(d.sum.load() + value);
    d.count.fetch_add(1);

    for (double bucket : buckets_) {
        if (value <= bucket) {
            d.buckets[bucket].fetch_add(1);
        }
    }
}

double Histogram::sum(const std::vector<MetricLabel>& labels) const {
    std::string key;
    for (const auto& l : labels) {
        key += l.name + "=" + l.value + ",";
    }

    std::lock_guard<std::mutex> lock(mutex_);
    auto it = data_.find(key);
    if (it != data_.end()) return it->second.sum.load();
    return 0;
}

uint64_t Histogram::count(const std::vector<MetricLabel>& labels) const {
    std::string key;
    for (const auto& l : labels) {
        key += l.name + "=" + l.value + ",";
    }

    std::lock_guard<std::mutex> lock(mutex_);
    auto it = data_.find(key);
    if (it != data_.end()) return it->second.count.load();
    return 0;
}

std::map<double, uint64_t> Histogram::bucket_counts(const std::vector<MetricLabel>& labels) const {
    std::string key;
    for (const auto& l : labels) {
        key += l.name + "=" + l.value + ",";
    }

    std::lock_guard<std::mutex> lock(mutex_);
    std::map<double, uint64_t> result;
    auto it = data_.find(key);
    if (it != data_.end()) {
        for (const auto& [bucket, count] : it->second.buckets) {
            result[bucket] = count.load();
        }
    }
    return result;
}

std::string Histogram::name() const { return name_; }
std::string Histogram::help() const { return help_; }

// Summary
Summary::Summary(const std::string& name, const std::string& help,
                 const std::vector<double>& quantiles)
    : name_(name), help_(help), quantiles_(quantiles) {}

void Summary::observe(double value) {
    std::lock_guard<std::mutex> lock(mutex_);
    values_.push_back(value);
    sum_.store(sum_.load() + value);
    count_.fetch_add(1);

    // Keep only recent values
    if (values_.size() > 10000) {
        values_.erase(values_.begin(), values_.begin() + 5000);
    }
}

double Summary::quantile(double q) const {
    std::lock_guard<std::mutex> lock(mutex_);
    if (values_.empty()) return 0;

    std::vector<double> sorted = values_;
    std::sort(sorted.begin(), sorted.end());

    size_t idx = static_cast<size_t>(q * (sorted.size() - 1));
    return sorted[idx];
}

double Summary::sum() const { return sum_.load(); }
uint64_t Summary::count() const { return count_.load(); }
std::string Summary::name() const { return name_; }
std::string Summary::help() const { return help_; }

// ═══════════════════════════════════════════════════════════════════════════
//  METRICS REGISTRY
// ═══════════════════════════════════════════════════════════════════════════

MetricsRegistry::MetricsRegistry() {}

Counter& MetricsRegistry::counter(const std::string& name, const std::string& help) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (counters_.find(name) == counters_.end()) {
        counters_[name] = std::make_unique<Counter>(name, help);
    }
    return *counters_[name];
}

Gauge& MetricsRegistry::gauge(const std::string& name, const std::string& help) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (gauges_.find(name) == gauges_.end()) {
        gauges_[name] = std::make_unique<Gauge>(name, help);
    }
    return *gauges_[name];
}

Histogram& MetricsRegistry::histogram(const std::string& name, const std::string& help,
                                       const std::vector<double>& buckets) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (histograms_.find(name) == histograms_.end()) {
        histograms_[name] = std::make_unique<Histogram>(name, help, buckets);
    }
    return *histograms_[name];
}

Summary& MetricsRegistry::summary(const std::string& name, const std::string& help,
                                   const std::vector<double>& quantiles) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (summaries_.find(name) == summaries_.end()) {
        summaries_[name] = std::make_unique<Summary>(name, help, quantiles);
    }
    return *summaries_[name];
}

std::string MetricsRegistry::export_prometheus() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::ostringstream oss;

    for (const auto& [name, counter] : counters_) {
        oss << "# HELP " << name << " " << counter->help() << "\n";
        oss << "# TYPE " << name << " counter\n";
        oss << name << " " << counter->value() << "\n";
    }

    for (const auto& [name, gauge] : gauges_) {
        oss << "# HELP " << name << " " << gauge->help() << "\n";
        oss << "# TYPE " << name << " gauge\n";
        oss << name << " " << gauge->value() << "\n";
    }

    for (const auto& [name, hist] : histograms_) {
        oss << "# HELP " << name << " " << hist->help() << "\n";
        oss << "# TYPE " << name << " histogram\n";
        oss << name << "_sum " << hist->sum() << "\n";
        oss << name << "_count " << hist->count() << "\n";
    }

    return oss.str();
}

std::string MetricsRegistry::export_json() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::ostringstream oss;
    oss << "{\n";

    bool first = true;
    for (const auto& [name, counter] : counters_) {
        if (!first) oss << ",\n";
        first = false;
        oss << "  \"" << name << "\": " << counter->value();
    }

    for (const auto& [name, gauge] : gauges_) {
        if (!first) oss << ",\n";
        first = false;
        oss << "  \"" << name << "\": " << gauge->value();
    }

    oss << "\n}";
    return oss.str();
}

std::vector<std::string> MetricsRegistry::list_metrics() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<std::string> names;
    for (const auto& [name, _] : counters_) names.push_back(name);
    for (const auto& [name, _] : gauges_) names.push_back(name);
    for (const auto& [name, _] : histograms_) names.push_back(name);
    for (const auto& [name, _] : summaries_) names.push_back(name);
    return names;
}

// ═══════════════════════════════════════════════════════════════════════════
//  DASHBOARD MANAGER
// ═══════════════════════════════════════════════════════════════════════════

DashboardManager::DashboardManager() {}

void DashboardManager::create(const Dashboard& dashboard) {
    std::lock_guard<std::mutex> lock(mutex_);
    dashboards_[dashboard.id] = dashboard;
}

void DashboardManager::update(const Dashboard& dashboard) {
    std::lock_guard<std::mutex> lock(mutex_);
    dashboards_[dashboard.id] = dashboard;
}

void DashboardManager::remove(const std::string& id) {
    std::lock_guard<std::mutex> lock(mutex_);
    dashboards_.erase(id);
}

std::optional<Dashboard> DashboardManager::get(const std::string& id) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = dashboards_.find(id);
    if (it != dashboards_.end()) return it->second;
    return std::nullopt;
}

std::vector<Dashboard> DashboardManager::list() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<Dashboard> result;
    for (const auto& [_, d] : dashboards_) {
        result.push_back(d);
    }
    return result;
}

std::vector<DashboardManager::TimeSeriesPoint> DashboardManager::query_metric(
    const std::string& metric,
    std::chrono::system_clock::time_point start,
    std::chrono::system_clock::time_point end) {
    (void)metric;
    (void)start;
    (void)end;
    return {};  // Would query from storage
}

std::string DashboardManager::export_json(const std::string& dashboard_id) const {
    auto d = get(dashboard_id);
    if (!d) return "{}";

    std::ostringstream oss;
    oss << "{\n";
    oss << "  \"id\": \"" << d->id << "\",\n";
    oss << "  \"title\": \"" << d->title << "\",\n";
    oss << "  \"panels\": " << d->panels.size() << "\n";
    oss << "}";
    return oss.str();
}

bool DashboardManager::import_json(const std::string& json) {
    (void)json;
    return true;
}

// ═══════════════════════════════════════════════════════════════════════════
//  OBSERVABILITY MANAGER
// ═══════════════════════════════════════════════════════════════════════════

ObservabilityManager::ObservabilityManager(const ObservabilityConfig& config)
    : config_(config)
    , tracer_(std::make_unique<Tracer>(config.service_name))
    , metrics_(std::make_unique<MetricsRegistry>())
    , dashboards_(std::make_unique<DashboardManager>()) {

    // Add default exporter
    if (config.trace_exporter == "console") {
        exporters_.push_back(std::make_unique<ConsoleSpanExporter>());
    } else if (config.trace_exporter == "jaeger" && !config.trace_endpoint.empty()) {
        exporters_.push_back(std::make_unique<JaegerExporter>(config.trace_endpoint));
    }

    // Create built-in metrics
    request_counter_ = &metrics_->counter("rael_requests_total", "Total requests");
    request_duration_ = &metrics_->histogram("rael_request_duration_ms", "Request duration in ms");
    error_counter_ = &metrics_->counter("rael_errors_total", "Total errors");
}

ObservabilityManager::~ObservabilityManager() {
    stop_server();
}

ObservabilityManager& ObservabilityManager::instance() {
    static ObservabilityManager inst;
    return inst;
}

Tracer& ObservabilityManager::tracer() { return *tracer_; }

void ObservabilityManager::add_exporter(std::unique_ptr<SpanExporter> exporter) {
    std::lock_guard<std::mutex> lock(mutex_);
    exporters_.push_back(std::move(exporter));
}

MetricsRegistry& ObservabilityManager::metrics() { return *metrics_; }

std::string ObservabilityManager::scrape_metrics() {
    return metrics_->export_prometheus();
}

DashboardManager& ObservabilityManager::dashboards() { return *dashboards_; }

void ObservabilityManager::record_request(const std::string& endpoint, int status,
                                           double duration_ms) {
    request_counter_->inc({{"endpoint", endpoint}, {"status", std::to_string(status)}});
    request_duration_->observe({{"endpoint", endpoint}}, duration_ms);
}

void ObservabilityManager::record_error(const std::string& type) {
    error_counter_->inc({{"type", type}});
}

std::shared_ptr<Span> ObservabilityManager::start_span(const std::string& name) {
    return tracer_->start_span(name);
}

void ObservabilityManager::start_server(int port) {
    (void)port;
    server_running_ = true;
    // Would start HTTP server for /metrics endpoint
}

void ObservabilityManager::stop_server() {
    server_running_ = false;
}

// ═══════════════════════════════════════════════════════════════════════════
//  HELPERS
// ═══════════════════════════════════════════════════════════════════════════

ScopedSpan::ScopedSpan(const std::string& name, SpanKind kind)
    : span_(observability().tracer().start_span(name, kind)) {}

ScopedSpan::~ScopedSpan() {
    span_->end();
}

Span& ScopedSpan::span() { return *span_; }

void ScopedSpan::set_attribute(const std::string& key, const std::string& value) {
    span_->set_attribute(key, value);
}

void ScopedSpan::add_event(const std::string& name) {
    span_->add_event(name);
}

void ScopedSpan::set_error(const std::string& message) {
    span_->set_status(false, message);
}

Timer::Timer() : stopped_(false) {
    start();
}

void Timer::start() {
    start_ = std::chrono::high_resolution_clock::now();
    stopped_ = false;
}

void Timer::stop() {
    end_ = std::chrono::high_resolution_clock::now();
    stopped_ = true;
}

double Timer::elapsed_ms() const {
    auto e = stopped_ ? end_ : std::chrono::high_resolution_clock::now();
    return std::chrono::duration<double, std::milli>(e - start_).count();
}

double Timer::elapsed_us() const {
    auto e = stopped_ ? end_ : std::chrono::high_resolution_clock::now();
    return std::chrono::duration<double, std::micro>(e - start_).count();
}

ObservabilityManager& observability() {
    return ObservabilityManager::instance();
}

MetricsRegistry& metrics() {
    return observability().metrics();
}

Tracer& tracer() {
    return observability().tracer();
}

} // namespace rael
