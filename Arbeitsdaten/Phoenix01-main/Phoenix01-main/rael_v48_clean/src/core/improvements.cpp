#include "rael/improvements.h"
#include "rael/events.h"
#include "rael/util.h"
#include "rael/metrics.h"

#include <mutex>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <iostream>
#include <unordered_set>

#ifdef _WIN32
#  include <windows.h>
#  include <shlobj.h>
#else
#  include <sys/stat.h>
#  include <sys/types.h>
#  include <unistd.h>
#endif

namespace rael {

static std::mutex& imp_mtx(){ static std::mutex m; return m; }
static uint64_t& imp_seq(){ static uint64_t s=0; return s; }
static std::vector<Improvement>& imp_buf(){ static std::vector<Improvement> v; return v; }
static size_t imp_cap(){ return 512; }

// Track which AAR rules have fired (to avoid duplicates)
static std::unordered_set<std::string>& aar_fired(){
    static std::unordered_set<std::string> s;
    return s;
}

static std::string ensure_dir(const std::string& dir){
#ifdef _WIN32
    CreateDirectoryA(dir.c_str(), nullptr);
#else
    mkdir(dir.c_str(), 0755);
#endif
    return dir;
}

static std::string get_program_data_dir(){
#ifdef _WIN32
    char path[MAX_PATH] = {0};
    if (SUCCEEDED(SHGetFolderPathA(nullptr, CSIDL_COMMON_APPDATA, nullptr, SHGFP_TYPE_CURRENT, path))) {
        return std::string(path);
    }
    char buf[512] = {0};
    DWORD n = GetEnvironmentVariableA("PROGRAMDATA", buf, sizeof(buf));
    if(n>0 && n<sizeof(buf)) return std::string(buf);
    return ".";
#else
    const char* home = getenv("HOME");
    if(home && *home) return std::string(home) + "/.local/share";
    return ".";
#endif
}

std::string ImprovementBus::storage_path(){
    auto base = get_program_data_dir();
#ifdef _WIN32
    auto dir = ensure_dir(base + "\\RAEL");
    return dir + "\\improvements.jsonl";
#else
    auto dir = ensure_dir(base + "/RAEL");
    return dir + "/improvements.jsonl";
#endif
}

static std::string jesc(const std::string& s){
    std::string o; o.reserve(s.size()+8);
    for(char c: s){
        switch(c){
            case '"': o += "\\\""; break;
            case '\\': o += "\\\\"; break;
            case '\n': o += "\\n"; break;
            case '\r': o += "\\r"; break;
            case '\t': o += "\\t"; break;
            default:
                if(static_cast<unsigned char>(c) < 0x20) {
                    // drop control chars
                } else o += c;
        }
    }
    return o;
}

static std::string safe_field(std::string s){
    // GUI protocol uses '|', so avoid it.
    for(char& c: s){
        if(c=='|') c = '/';
        if(c=='\n' || c=='\r') c = ' ';
    }
    return s;
}

static void persist_append(const Improvement& imp){
    std::ofstream f(ImprovementBus::storage_path(), std::ios::app);
    if(!f) return;
    f << "{"
      << "\"id\":" << imp.id << ","
      << "\"ts\":\"" << jesc(imp.ts) << "\","
      << "\"src\":\"" << jesc(imp.src) << "\","
      << "\"source\":" << static_cast<int>(imp.source) << ","
      << "\"importance\":" << imp.importance << ","
      << "\"risk\":" << imp.risk << ","
      << "\"confidence\":" << imp.confidence << ","
      << "\"title\":\"" << jesc(imp.title) << "\","
      << "\"problem\":\"" << jesc(imp.problem) << "\","
      << "\"rationale\":\"" << jesc(imp.rationale) << "\","
      << "\"testplan\":\"" << jesc(imp.testplan) << "\","
      << "\"code\":\"" << jesc(imp.code) << "\","
      << "\"status\":\"" << jesc(imp.status) << "\","
      << "\"typed_status\":" << static_cast<int>(imp.typed_status) << ","
      << "\"shadow_tested\":" << (imp.shadow_tested ? "true" : "false") << ","
      << "\"triggering_metric\":\"" << jesc(imp.triggering_metric) << "\","
      << "\"metric_value\":" << imp.metric_value << ","
      << "\"metric_threshold\":" << imp.metric_threshold
      << "}\n";
}

uint64_t ImprovementBus::emit(Improvement imp){
    if(imp.status.empty()) imp.status = "PENDING";
    if(imp.ts.empty()) imp.ts = now_iso8601();
    if(imp.src.empty()) imp.src = source_to_string(imp.source);

    // Sync source enum with string
    if (imp.source == ImprovementSource::OTHER && !imp.src.empty()) {
        imp.source = string_to_source(imp.src);
    }

    std::lock_guard<std::mutex> g(imp_mtx());
    imp.id = ++imp_seq();

    auto& b = imp_buf();
    if(b.size() < imp_cap()) b.push_back(imp);
    else b[imp.id % imp_cap()] = imp;

    persist_append(imp);

    // Emit to GUI/stdout as a single-line event.
    std::ostringstream ss;
    ss << "EVT|IMPROVE|"
       << "src=" << safe_field(imp.src)
       << "|importance=" << imp.importance
       << "|risk=" << imp.risk
       << "|title=" << safe_field(imp.title)
       << "|problem=" << safe_field(imp.problem)
       << "|rationale=" << safe_field(imp.rationale);
    if(!imp.code.empty()) ss << "|code=" << safe_field(imp.code);
    if(!imp.triggering_metric.empty()) {
        ss << "|metric=" << safe_field(imp.triggering_metric)
           << "|metric_value=" << imp.metric_value;
    }
    EventBus::push("IMPROVEMENT", ss.str());
    // Also print to stdout for GUI child capture.
    std::cout << ss.str() << "\n";

    return imp.id;
}

uint64_t ImprovementBus::emit(ImprovementSource source, const std::string& title,
                              const std::string& problem, int importance, int risk) {
    Improvement imp;
    imp.source = source;
    imp.src = source_to_string(source);
    imp.title = title;
    imp.problem = problem;
    imp.importance = importance;
    imp.risk = risk;
    imp.rationale = "Auto-generated improvement suggestion.";
    imp.confidence = 0.5;
    return emit(imp);
}

std::vector<Improvement> ImprovementBus::last(size_t n){
    std::lock_guard<std::mutex> g(imp_mtx());
    auto out = imp_buf();
    std::sort(out.begin(), out.end(), [](const Improvement& a, const Improvement& b){ return a.id < b.id; });
    if(n==0) return {};
    if(out.size() > n) out.erase(out.begin(), out.end()-n);
    std::reverse(out.begin(), out.end());
    return out;
}

std::vector<Improvement> ImprovementBus::by_source(ImprovementSource source, size_t n) {
    std::lock_guard<std::mutex> g(imp_mtx());
    std::vector<Improvement> out;
    out.reserve(n);

    auto& buf = imp_buf();
    for (auto it = buf.rbegin(); it != buf.rend() && out.size() < n; ++it) {
        if (it->source == source) {
            out.push_back(*it);
        }
    }
    return out;
}

std::vector<Improvement> ImprovementBus::by_status(ImprovementStatus status, size_t n) {
    std::lock_guard<std::mutex> g(imp_mtx());
    std::vector<Improvement> out;
    out.reserve(n);

    auto& buf = imp_buf();
    for (auto it = buf.rbegin(); it != buf.rend() && out.size() < n; ++it) {
        if (it->typed_status == status) {
            out.push_back(*it);
        }
    }
    return out;
}

bool ImprovementBus::update_status(uint64_t id, ImprovementStatus new_status) {
    std::lock_guard<std::mutex> g(imp_mtx());
    auto& buf = imp_buf();
    for (auto& imp : buf) {
        if (imp.id == id) {
            imp.typed_status = new_status;
            imp.status = status_to_string(new_status);
            EventBus::push("IMPROVE_STATUS",
                "id=" + std::to_string(id) + "|status=" + imp.status);
            return true;
        }
    }
    return false;
}

// ═══════════════════════════════════════════════════════════════════════════
// AAR ENGINE IMPLEMENTATION
// ═══════════════════════════════════════════════════════════════════════════

AAREngine gAAR;

AAREngine::AAREngine() {
    // Default-Regeln basierend auf Standard-Metriken
    add_rule({
        "high_ethics_blocks",
        "ethics_blocks",
        100.0,
        true,
        "Ethik-Filter Review erforderlich",
        "Hohe Anzahl an Ethics-Blocks deutet auf problematische Eingaben oder zu strenge Filter hin.",
        7, 3
    });

    add_rule({
        "low_ops_per_sec",
        "ops_per_sec",
        10.0,
        false,  // trigger wenn UNTER threshold
        "Performance-Optimierung erforderlich",
        "Niedrige Operationen pro Sekunde deuten auf Bottlenecks hin.",
        6, 2
    });

    add_rule({
        "high_module_loads",
        "module_loads",
        50.0,
        true,
        "Modul-Caching prüfen",
        "Viele Modul-Ladevorgänge könnten durch Caching reduziert werden.",
        5, 2
    });

    add_rule({
        "semantic_resonance_imbalance",
        "semantic_resonance_ratio",
        5.0,
        true,
        "Semantic/Resonance Balancing",
        "Ungleichgewicht zwischen Semantic- und Resonance-Aufrufen.",
        6, 3
    });
}

void AAREngine::add_rule(const AARRule& rule) {
    rules_.push_back(rule);
}

void AAREngine::set_metric_provider(MetricProvider provider) {
    metric_provider_ = provider;
}

void AAREngine::analyze() {
    // Nur alle 1000 Ticks analysieren
    uint64_t current_tick = gMetrics.ops_total.load();
    if (current_tick - last_analysis_tick_ < 1000) return;
    last_analysis_tick_ = current_tick;

    // Standard-Metriken sammeln
    auto get_metric = [this](const std::string& name) -> double {
        // Erst Custom-Provider fragen
        if (metric_provider_) {
            double v = metric_provider_(name);
            if (v != -1.0) return v;
        }

        // Standard-Metriken
        if (name == "ethics_blocks") return static_cast<double>(gMetrics.ethics_blocks.load());
        if (name == "ops_per_sec") return static_cast<double>(gMetrics.ops_sec.load());
        if (name == "ops_total") return static_cast<double>(gMetrics.ops_total.load());
        if (name == "module_loads") return static_cast<double>(gMetrics.module_loads.load());
        if (name == "semantic_calls") return static_cast<double>(gMetrics.semantic_calls.load());
        if (name == "resonance_calls") return static_cast<double>(gMetrics.resonance_calls.load());
        if (name == "hotswaps") return static_cast<double>(gMetrics.hotswaps.load());

        // Berechnete Metriken
        if (name == "semantic_resonance_ratio") {
            double sem = static_cast<double>(gMetrics.semantic_calls.load());
            double res = static_cast<double>(gMetrics.resonance_calls.load());
            if (res < 1.0) res = 1.0;
            return sem / res;
        }

        return 0.0;
    };

    // Regeln prüfen
    for (const auto& rule : rules_) {
        // Bereits gefeuert? Skip.
        if (aar_fired().count(rule.name)) continue;

        double value = get_metric(rule.metric_name);
        bool triggered = rule.trigger_above ? (value > rule.threshold) : (value < rule.threshold);

        if (triggered) {
            // Improvement erstellen
            Improvement imp;
            imp.source = ImprovementSource::AAR;
            imp.src = "AAR";
            imp.title = rule.suggestion_title;
            imp.problem = rule.suggestion_problem;
            imp.importance = rule.importance;
            imp.risk = rule.risk;
            imp.confidence = 0.7;
            imp.rationale = "Automatisch erkannt durch AAR-Regel '" + rule.name + "'.";
            imp.triggering_metric = rule.metric_name;
            imp.metric_value = value;
            imp.metric_threshold = rule.threshold;
            imp.testplan = "Metrik beobachten nach Änderung.";

            ImprovementBus::emit(imp);
            aar_fired().insert(rule.name);

            EventBus::push("AAR_TRIGGER",
                "rule=" + rule.name + "|metric=" + rule.metric_name +
                "|value=" + std::to_string(value) + "|threshold=" + std::to_string(rule.threshold));
        }
    }
}

} // namespace rael
