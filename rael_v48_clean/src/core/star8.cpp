#include "rael/star8.h"
#include "rael/raelcore.h"
#include "rael/events.h"
#include <chrono>
#include <sstream>
#include <iomanip>

namespace rael {

// ═══════════════════════════════════════════════════════════════════════════
// KONSTRUKTOR - Themen-Zuweisung
// ═══════════════════════════════════════════════════════════════════════════

Star8::Star8(RaelCore& core) : core_(core) {
    auto now = std::chrono::steady_clock::now();

    // Jeder Node bekommt sein Thema (1:1 Mapping)
    for (size_t i = 0; i < STAR_NODE_COUNT; ++i) {
        node_memories_[i].theme = static_cast<NodeTheme>(i);
        node_memories_[i].last_activity = now;
    }
}

Star8::~Star8(){ stop(); }

// ═══════════════════════════════════════════════════════════════════════════
// START / STOP
// ═══════════════════════════════════════════════════════════════════════════

void Star8::start(){
    bool expected=false;
    if(!running_.compare_exchange_strong(expected, true)) return;
    threads_.clear();
    threads_.reserve(STAR_NODE_COUNT);

    // Starte fuer jeden Themen-Node einen Worker-Thread
    for(size_t i = 0; i < STAR_NODE_COUNT; i++){
        threads_.emplace_back(&Star8::worker, this, i);
    }

    // Themen-Uebersicht loggen
    std::ostringstream oss;
    oss << "nodes=" << STAR_NODE_COUNT << "|memory=5D_QUINT|themes=[";
    for (size_t i = 0; i < STAR_NODE_COUNT; i++) {
        if (i > 0) oss << ",";
        oss << "N" << i << ":" << theme_name(node_memories_[i].theme);
    }
    oss << "]";
    EventBus::push("STAR8_START", oss.str());
}

void Star8::stop(){
    bool expected=true;
    if(!running_.compare_exchange_strong(expected, false)) return;
    for(auto& t: threads_) if(t.joinable()) t.join();
    threads_.clear();

    // Finale Statistiken
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(2);
    oss << "coherence=" << total_coherence()
        << "|energy=" << total_energy()
        << "|health=" << average_health();
    EventBus::push("STAR8_STOP", oss.str());
}

// ═══════════════════════════════════════════════════════════════════════════
// TASK EINREIHEN
// ═══════════════════════════════════════════════════════════════════════════

bool Star8::submit(Lane lane, const std::string& payload){
    Task t;
    t.lane = lane;
    t.payload = payload;
    t.domain = TaskDomain::AUTO;
    return sched_.enqueue(std::move(t));
}

bool Star8::submit_themed(TaskDomain domain, Lane lane, const std::string& payload){
    Task t;
    t.lane = lane;
    t.payload = payload;
    t.domain = domain;
    return sched_.enqueue(std::move(t));
}

// ═══════════════════════════════════════════════════════════════════════════
// THEMEN-WORKER
// ═══════════════════════════════════════════════════════════════════════════
// Jeder Worker hat sein Thema. Er nimmt bevorzugt Tasks seiner Domaene,
// kann aber auch generische Tasks (AUTO) verarbeiten.

void Star8::worker(size_t node_id){
    NodeMemory& mem = node_memories_[node_id];
    NodeTheme my_theme = mem.theme;

    while(running_.load()){
        Task t;
        if(sched_.try_dequeue(t)){
            // Themen-Routing: Gehoert dieser Task zu meinem Thema?
            NodeTheme target_theme = domain_to_theme(t.domain);

            if (t.domain != TaskDomain::AUTO && target_theme != my_theme) {
                // Nicht mein Thema - zurueck in die Queue
                sched_.enqueue(std::move(t));
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
                continue;
            }

            auto start_time = std::chrono::steady_clock::now();
            mem.last_activity = start_time;
            sched_.mark_taken(node_id, t.lane);

            if(t.slow){
                std::this_thread::sleep_for(std::chrono::milliseconds(2));
            }

            // Themen-spezifische Verarbeitung
            bool success = true;
            std::string out;
            try {
                out = core_.process_themed(my_theme, t.payload);
            } catch (...) {
                success = false;
                out = "[ERROR:" + std::string(theme_name(my_theme)) + "]";
            }

            auto end_time = std::chrono::steady_clock::now();
            auto duration_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(
                end_time - start_time).count();
            double duration_ms = duration_ns / 1e6;

            // ═══════════════════════════════════════════════════════════════
            // UPDATE NODE MEMORY (5D-Speicher)
            // ═══════════════════════════════════════════════════════════════

            mem.reflex.push(duration_ms);
            mem.instinct.push(static_cast<double>(t.payload.size()));
            mem.emotion.push(success ? 1.0 : 0.0);

            double throughput = duration_ms > 0.0 ?
                static_cast<double>(t.payload.size() + out.size()) / duration_ms : 0.0;
            mem.ratio.push(throughput);

            double perf_score = success ? (1.0 / (1.0 + duration_ms / 100.0)) : 0.0;
            mem.spirit.push(perf_score);

            if (success) {
                mem.tasks_completed.fetch_add(1);
            } else {
                mem.tasks_failed.fetch_add(1);
            }
            mem.total_processing_ns.fetch_add(duration_ns);

            double variance = mem.reflex.variance();
            double coherence = 1.0 / (1.0 + variance);
            mem.coherence.store(coherence);

            // Event mit Themen-Info
            std::string event_tag = std::string("ANSWER:") + theme_name(my_theme);
            EventBus::push(event_tag, out);
            sched_.mark_done(node_id);

        } else {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    }
}

// ═══════════════════════════════════════════════════════════════════════════
// GLOBALE STERN-STATISTIKEN
// ═══════════════════════════════════════════════════════════════════════════

double Star8::total_coherence() const {
    double sum = 0.0;
    for (const auto& mem : node_memories_) {
        sum += mem.coherence.load();
    }
    return sum / (double)STAR_NODE_COUNT;
}

double Star8::total_energy() const {
    double sum = 0.0;
    for (const auto& mem : node_memories_) {
        sum += mem.total_energy();
    }
    return sum;
}

double Star8::average_health() const {
    double sum = 0.0;
    for (const auto& mem : node_memories_) {
        sum += mem.health();
    }
    return sum / (double)STAR_NODE_COUNT;
}

} // namespace rael
