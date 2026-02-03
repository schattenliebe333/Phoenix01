#include "rael/star8.h"
#include "rael/raelcore.h"
#include "rael/events.h"
#include <chrono>
#include <sstream>
#include <iomanip>

namespace rael {

Star8::Star8(RaelCore& core) : core_(core) {
    // Initialisiere alle Node-Memories mit Startzeitpunkt
    auto now = std::chrono::steady_clock::now();
    for (auto& mem : node_memories_) {
        mem.last_activity = now;
    }
}

Star8::~Star8(){ stop(); }

void Star8::start(){
    bool expected=false;
    if(!running_.compare_exchange_strong(expected, true)) return;
    threads_.clear();
    threads_.reserve(8);
    for(size_t i=0;i<8;i++){
        threads_.emplace_back(&Star8::worker, this, i);
    }
    EventBus::push("STAR8_START", "nodes=8|memory=5D_QUINT");
}

void Star8::stop(){
    bool expected=true;
    if(!running_.compare_exchange_strong(expected, false)) return;
    for(auto& t: threads_) if(t.joinable()) t.join();
    threads_.clear();

    // Log finale Statistiken
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(2);
    oss << "coherence=" << total_coherence()
        << "|energy=" << total_energy()
        << "|health=" << average_health();
    EventBus::push("STAR8_STOP", oss.str());
}

bool Star8::submit(Lane lane, const std::string& payload){
    Task t;
    t.lane = lane;
    t.payload = payload;
    return sched_.enqueue(std::move(t));
}

void Star8::worker(size_t node_id){
    NodeMemory& mem = node_memories_[node_id];

    while(running_.load()){
        Task t;
        if(sched_.try_dequeue(t)){
            auto start_time = std::chrono::steady_clock::now();
            mem.last_activity = start_time;

            sched_.mark_taken(node_id, t.lane);

            if(t.slow){
                std::this_thread::sleep_for(std::chrono::milliseconds(2));
            }

            // Verarbeite Task
            bool success = true;
            std::string out;
            try {
                out = core_.process_payload(t.payload);
            } catch (...) {
                success = false;
                out = "[ERROR]";
            }

            auto end_time = std::chrono::steady_clock::now();
            auto duration_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(
                end_time - start_time).count();
            double duration_ms = duration_ns / 1e6;

            // ═══════════════════════════════════════════════════════════════
            // UPDATE NODE MEMORY (5D-Speicher)
            // ═══════════════════════════════════════════════════════════════

            // G1 Reflex: Reaktionszeit in ms
            mem.reflex.push(duration_ms);

            // G2 Instinct: Task-Komplexität (Payload-Länge als Proxy)
            mem.instinct.push(static_cast<double>(t.payload.size()));

            // G3 Emotion: Erfolgsrate (1.0 = Erfolg, 0.0 = Fehler)
            mem.emotion.push(success ? 1.0 : 0.0);

            // G4 Ratio: Durchsatz (Bytes/ms)
            double throughput = duration_ms > 0.0 ?
                static_cast<double>(t.payload.size() + out.size()) / duration_ms : 0.0;
            mem.ratio.push(throughput);

            // G5 Spirit: Kumulative Performance-Score
            double perf_score = success ? (1.0 / (1.0 + duration_ms / 100.0)) : 0.0;
            mem.spirit.push(perf_score);

            // Statistiken aktualisieren
            if (success) {
                mem.tasks_completed.fetch_add(1);
            } else {
                mem.tasks_failed.fetch_add(1);
            }
            mem.total_processing_ns.fetch_add(duration_ns);

            // Kohärenz berechnen (basierend auf Konsistenz der Reaktionszeiten)
            double variance = mem.reflex.variance();
            double coherence = 1.0 / (1.0 + variance);
            mem.coherence.store(coherence);

            // Event senden
            EventBus::push("ANSWER", out);
            sched_.mark_done(node_id);

        } else {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    }
}

// ═══════════════════════════════════════════════════════════════════════════
// GLOBALE STAR8-STATISTIKEN
// ═══════════════════════════════════════════════════════════════════════════

double Star8::total_coherence() const {
    double sum = 0.0;
    for (const auto& mem : node_memories_) {
        sum += mem.coherence.load();
    }
    return sum / 8.0;
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
    return sum / 8.0;
}

} // namespace rael
