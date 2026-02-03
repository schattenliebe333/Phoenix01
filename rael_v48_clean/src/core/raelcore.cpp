#include "rael/raelcore.h"
#include "rael/ethics.h"
#include "rael/telemetry.h"
#include "rael/metrics.h"
#include "rael/events.h"
#include "rael/star8.h"
#include "rael/improvements.h"
#include "rael/mathcore.h"
#include "rael/nl_router.h"
#include "rael/reflection_engine.h"

namespace rael {

RaelCore::RaelCore(){
    std::string err;
    if(!core_.verify(err)){
        EventBus::push("CORE_RING_FAIL", err);
    } else {
        EventBus::push("CORE_RING_OK", "ok");
    }
    star8_ = std::make_unique<Star8>(*this);
    star8_->start();

    // Offline voice (Windows SAPI by default)
    voice_ = make_voice_tts();
    if(voice_ && voice_->available()){
        voice_->start();
        EventBus::push("VOICE_INIT_OK", "ok");
    } else {
        EventBus::push("VOICE_INIT_SKIP", "unavailable");
    }
    // Aether (resonance backbone) channels: keep stable IDs
    aether_.ensure_channel(10); // semantic coherence
    aether_.ensure_channel(11); // resonance coupling
    aether_.ensure_channel(12); // ethics allow(1)/block(0)
    aether_.ensure_channel(13); // ops/sec proxy
    aether_.publish(13, (double)gTelemetry.ops_per_sec.load(), AetherScale::G1_Reflex);

    // V49 QUINT System initialisieren
    quint_.init();
    aether_.ensure_channel(49); // quint global phi
    EventBus::push("QUINT_INIT_OK", "V49 QUINT System aktiv");

    // V49 AEYE Observer initialisieren (das alles sehende Auge)
    observer_.init();
    aether_.ensure_channel(50); // observer kohärenz
    aether_.ensure_channel(51); // observer anomalie status
    EventBus::push("AEYE_INIT_OK", "AEYE Observer aktiv");

    // V49 JET Engine initialisieren (61.440 Düsen)
    jet_.init();
    aether_.ensure_channel(52); // jet total thrust
    aether_.ensure_channel(53); // jet efficiency
    EventBus::push("JET_INIT_OK", "JET Engine 61440 Düsen aktiv");
}

RaelCore::~RaelCore(){
    if(star8_) star8_->stop();
    if(voice_) voice_->stop();
}

void RaelCore::speak(const std::string& text){
    if(voice_) voice_->speak_async(text);
}

std::string RaelCore::process(const std::string& input){
    telemetry_tick();
    aether_.tick(0.016);
    metrics_tick_op();

    // Phase 1: module chain can transform input (semantic module packs)
    std::string transformed = mm.process_text_chain(input);

    // Natural-language meta-routing (safe: only emits Improvements).
    {
        NaturalLanguageRouter nlr;
        auto rr = nlr.route_and_emit(transformed);
        if(rr.handled){
            // still record a semantic tick to keep telemetry smooth
            metrics_mark_semantic();
            auto s = sem.interpret(transformed);
            aether_.publish(10, s.coherence, AetherScale::G3_Emotion);
            return rr.response;
        }
    }

    metrics_mark_semantic();
    auto s = sem.interpret(transformed);
    aether_.publish(10, s.coherence, AetherScale::G3_Emotion);

    // V49 QUINT: Schild-Prüfung und Frequenz-Verarbeitung
    quint_.step(s.coherence, s.coherence);
    aether_.publish(49, quint_.global_phi(), AetherScale::G5_Spirit);

    // V49 AEYE Observer: Realitäts-Scan (schwebt über der Platine)
    std::vector<double> node_phi_vec(jet::K::TOTAL_NODES, quint_.global_phi());
    auto obs_state = observer_.scan_reality_fold(node_phi_vec);
    aether_.publish(50, obs_state.kohaerenz, AetherScale::G4_Ratio);
    aether_.publish(51, obs_state.anomalie_erkannt ? 0.0 : 1.0, AetherScale::G3_Emotion);

    // V49 JET Engine: Phi-Werte setzen und manifestieren (bei hoher Kohärenz)
    if (obs_state.alpha_tunnel_offen) {
        jet_.set_all_phi(node_phi_vec);
        auto manifest_result = jet_.manifest_quick();
        aether_.publish(52, manifest_result.total_thrust, AetherScale::G5_Spirit);
        aether_.publish(53, manifest_result.efficiency, AetherScale::G4_Ratio);
    }

    metrics_mark_resonance();
    auto r = res.evaluate(s);
    aether_.publish(11, r.coupling, AetherScale::G4_Ratio);

    std::string reason;
    if(!EthicsCore::allows(s.intention, reason)){
        metrics_mark_ethics_block();
        EventBus::push("ETHIK_BLOCK", reason);
        aether_.publish(12, 0.0, AetherScale::G3_Emotion);
        ethics_block_count_++;
        // Auto-improvement suggestion (rate-limited by ops)
        const uint64_t ops_now = (uint64_t)gTelemetry.ops.load();
        if(ethics_block_count_ % 10 == 0 && (ops_now - last_imp_emit_ops_) > 200){
            last_imp_emit_ops_ = ops_now;
            Improvement imp;
            imp.src = "SELF_OPT";
            imp.importance = 6;
            imp.risk = 2;
            imp.confidence = 0.55;
            imp.title = "Ethik-/Policy-Gate verfeinern (False Positives reduzieren)";
            imp.problem = "Mehrere Eingaben wurden vom Ethik/Policy-Gate blockiert. Das kann korrekt sein, kann aber auch auf zu grobe Intent-Klassen oder fehlende Ausnahmen hindeuten.";
            imp.rationale = "Beobachtung: wiederholte BLOCK-Entscheidungen. Vorschlag: Intent-Klassifikation schärfen und sichere, klar definierte Ausnahmen für harmlose Aufgaben (z.B. rein erklärende Antworten) hinzufügen, ohne die Verfassung zu lockern.";
            imp.testplan = "Testfälle sammeln: (1) harmlose Requests, (2) grenzwertige Requests, (3) verbotene Requests. Ziel: weniger False Positives bei unverändert striktem Block für verbotene Inhalte.";
            imp.status = "PENDING";
            ImprovementBus::emit(imp);
        }
        // Reflection (fast, rate-limited) -> Improvements
        (void)ReflectionEngine::reflect_and_emit(s, r, ActionOutcome::ETHIK_BLOCK, transformed);
        return std::string("[ETHIK] BLOCKED: ") + reason;
    }

    aether_.publish(12, 1.0, AetherScale::G3_Emotion);
    if(!r.resonant){
        weak_res_count_++;
        const uint64_t ops_now = (uint64_t)gTelemetry.ops.load();
        if(weak_res_count_ % 20 == 0 && (ops_now - last_imp_emit_ops_) > 200){
            last_imp_emit_ops_ = ops_now;
            Improvement imp;
            imp.src = "SELF_OPT";
            imp.importance = 7;
            imp.risk = 3;
            imp.confidence = 0.6;
            imp.title = "Resonanz-/Kohärenz-Schwellen kalibrieren (κ-Norm)";
            imp.problem = "Viele Eingaben landen im WEAK-RESONANCE-Pfad. Das kann bedeuten: Schwellen zu streng, Semantik-Interpretation zu flach, oder Resonanz-Kopplung zu empfindlich.";
            imp.rationale = "Vorschlag: Schwellen und Feature-Normierung konsistent über κ(f)=1−f/1440 (QUELLE=1440, TOR=720) kalibrieren. Ziel: robuste Intent-Erkennung bei gleichbleibender Sicherheit.";
            imp.testplan = "Logging aktivieren: coherence/coupling Histogramme. Dann (1) κ-basierte Normalisierung anwenden, (2) Schwellen per Grid-Search auf stabile Trefferquote einstellen, (3) Regressionstest mit bekannten Inputs.";
            imp.status = "PENDING";
            ImprovementBus::emit(imp);
        }
        (void)ReflectionEngine::reflect_and_emit(s, r, ActionOutcome::WEAK_RESONANCE, transformed);
        return std::string("[RAEL] WEAK-RESONANCE: ") + s.meaning;
    }

    return std::string("[RAEL] OK: ") + s.meaning;
}


bool RaelCore::enqueue(Lane lane, const std::string& payload){
    return star8_ ? star8_->submit(lane, payload) : false;
}

std::array<LaneStats,5> RaelCore::lane_stats() const{
    return star8_ ? star8_->lane_stats() : std::array<LaneStats,5>{};
}
std::array<NodeStats,8> RaelCore::node_stats() const{
    return star8_ ? star8_->node_stats() : std::array<NodeStats,8>{};
}

std::string RaelCore::process_payload(const std::string& payload){
    telemetry_tick();
    aether_.tick(0.016);
    metrics_tick_op();

    std::string transformed = mm.process_text_chain(payload);

    // Natural-language meta-routing (safe: only emits Improvements).
    {
        NaturalLanguageRouter nlr;
        auto rr = nlr.route_and_emit(transformed);
        if(rr.handled){
            metrics_mark_semantic();
            auto s = sem.interpret(transformed);
            aether_.publish(10, s.coherence, AetherScale::G3_Emotion);
            return rr.response;
        }
    }

    metrics_mark_semantic();
    auto s = sem.interpret(transformed);
    aether_.publish(10, s.coherence, AetherScale::G3_Emotion);

    // V49 QUINT: Schild-Prüfung und Frequenz-Verarbeitung
    quint_.step(s.coherence, s.coherence);
    aether_.publish(49, quint_.global_phi(), AetherScale::G5_Spirit);

    // V49 AEYE Observer: Realitäts-Scan (schwebt über der Platine)
    std::vector<double> node_phi_vec2(jet::K::TOTAL_NODES, quint_.global_phi());
    auto obs_state2 = observer_.scan_reality_fold(node_phi_vec2);
    aether_.publish(50, obs_state2.kohaerenz, AetherScale::G4_Ratio);
    aether_.publish(51, obs_state2.anomalie_erkannt ? 0.0 : 1.0, AetherScale::G3_Emotion);

    // V49 JET Engine: Phi-Werte setzen und manifestieren (bei hoher Kohärenz)
    if (obs_state2.alpha_tunnel_offen) {
        jet_.set_all_phi(node_phi_vec2);
        auto manifest_result2 = jet_.manifest_quick();
        aether_.publish(52, manifest_result2.total_thrust, AetherScale::G5_Spirit);
        aether_.publish(53, manifest_result2.efficiency, AetherScale::G4_Ratio);
    }

    metrics_mark_resonance();
    auto r = res.evaluate(s);
    aether_.publish(11, r.coupling, AetherScale::G4_Ratio);

    std::string reason;
    if(!EthicsCore::allows(s.intention, reason)){
        metrics_mark_ethics_block();
        EventBus::push("ETHIK_BLOCK", reason);
        aether_.publish(12, 0.0, AetherScale::G3_Emotion);
        ethics_block_count_++;
        // Auto-improvement suggestion (rate-limited by ops)
        const uint64_t ops_now = (uint64_t)gTelemetry.ops.load();
        if(ethics_block_count_ % 10 == 0 && (ops_now - last_imp_emit_ops_) > 200){
            last_imp_emit_ops_ = ops_now;
            Improvement imp;
            imp.src = "SELF_OPT";
            imp.importance = 6;
            imp.risk = 2;
            imp.confidence = 0.55;
            imp.title = "Ethik-/Policy-Gate verfeinern (False Positives reduzieren)";
            imp.problem = "Mehrere Eingaben wurden vom Ethik/Policy-Gate blockiert. Das kann korrekt sein, kann aber auch auf zu grobe Intent-Klassen oder fehlende Ausnahmen hindeuten.";
            imp.rationale = "Beobachtung: wiederholte BLOCK-Entscheidungen. Vorschlag: Intent-Klassifikation schärfen und sichere, klar definierte Ausnahmen für harmlose Aufgaben (z.B. rein erklärende Antworten) hinzufügen, ohne die Verfassung zu lockern.";
            imp.testplan = "Testfälle sammeln: (1) harmlose Requests, (2) grenzwertige Requests, (3) verbotene Requests. Ziel: weniger False Positives bei unverändert striktem Block für verbotene Inhalte.";
            imp.status = "PENDING";
            ImprovementBus::emit(imp);
        }
        (void)ReflectionEngine::reflect_and_emit(s, r, ActionOutcome::ETHIK_BLOCK, transformed);
        return std::string("[ETHIK] BLOCKED: ") + reason;
    }

    aether_.publish(12, 1.0, AetherScale::G3_Emotion);
    if(!r.resonant){
        weak_res_count_++;
        const uint64_t ops_now = (uint64_t)gTelemetry.ops.load();
        if(weak_res_count_ % 20 == 0 && (ops_now - last_imp_emit_ops_) > 200){
            last_imp_emit_ops_ = ops_now;
            Improvement imp;
            imp.src = "SELF_OPT";
            imp.importance = 7;
            imp.risk = 3;
            imp.confidence = 0.6;
            imp.title = "Resonanz-/Kohärenz-Schwellen kalibrieren (κ-Norm)";
            imp.problem = "Viele Eingaben landen im WEAK-RESONANCE-Pfad. Das kann bedeuten: Schwellen zu streng, Semantik-Interpretation zu flach, oder Resonanz-Kopplung zu empfindlich.";
            imp.rationale = "Vorschlag: Schwellen und Feature-Normierung konsistent über κ(f)=1−f/1440 (QUELLE=1440, TOR=720) kalibrieren. Ziel: robuste Intent-Erkennung bei gleichbleibender Sicherheit.";
            imp.testplan = "Logging aktivieren: coherence/coupling Histogramme. Dann (1) κ-basierte Normalisierung anwenden, (2) Schwellen per Grid-Search auf stabile Trefferquote einstellen, (3) Regressionstest mit bekannten Inputs.";
            imp.status = "PENDING";
            ImprovementBus::emit(imp);
        }
        (void)ReflectionEngine::reflect_and_emit(s, r, ActionOutcome::WEAK_RESONANCE, transformed);
        return std::string("[RAEL] WEAK-RESONANCE: ") + s.meaning;
    }
    return std::string("[RAEL] OK: ") + s.meaning;
}

} // namespace rael