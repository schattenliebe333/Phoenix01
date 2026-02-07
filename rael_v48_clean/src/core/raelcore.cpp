// ═══════════════════════════════════════════════════════════════════════════
// RAELCORE - Haupt-Orchestrator mit 9 Themen-Nodes
// ═══════════════════════════════════════════════════════════════════════════
//
// RAEL steht ueber allem. Versteht. Reflektiert. Verbessert.
// Macht Vorschlaege was sie programmieren kann um ein Problem zu loesen.
// Schuetzt den Rechner und die Entwicklung.
//
// 9 Themen-Stern-Nodes:
//   N0 SPRACHE       → Verstehen & Ausdruck
//   N1 GEDAECHTNIS   → Erinnern & Wissen
//   N2 SICHERHEIT    → Schuetzen & Verteidigen
//   N3 REFLEXION     → Selbstanalyse & Verbesserung
//   N4 MATHEMATIK    → Berechnen & Frequenz
//   N5 BEOBACHTUNG   → Ueberwachen & Erkennen
//   N6 LERNEN        → Muster lernen & Optimieren
//   N7 MANIFESTATION → Ausgabe & Aktion
//   N8 INNERES_AUGE  → Dateisystem, Prozesse, Netzwerk, Isolation
//
// ═══════════════════════════════════════════════════════════════════════════

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
#include "rael/star8_themes.h"

namespace rael {

// ═══════════════════════════════════════════════════════════════════════════
// KONSTRUKTOR - Alle Kerne initialisieren
// ═══════════════════════════════════════════════════════════════════════════

RaelCore::RaelCore(){
    // CoreRing verifizieren (IchBin, Ethik, MathBase)
    std::string err;
    if(!core_.verify(err)){
        EventBus::push("CORE_RING_FAIL", err);
    } else {
        EventBus::push("CORE_RING_OK", "ok");
    }

    // Themen-Stern starten (9 Nodes)
    star8_ = std::make_unique<Star8>(*this);
    star8_->start();

    // Voice (Windows SAPI / Linux offline TTS)
    voice_ = make_voice_tts();
    if(voice_ && voice_->available()){
        voice_->start();
        EventBus::push("VOICE_INIT_OK", "ok");
    } else {
        EventBus::push("VOICE_INIT_SKIP", "unavailable");
    }

    // AetherBus Kanaele (Resonanz-Backbone)
    aether_.ensure_channel(10); // semantic coherence
    aether_.ensure_channel(11); // resonance coupling
    aether_.ensure_channel(12); // ethics allow(1)/block(0)
    aether_.ensure_channel(13); // ops/sec proxy
    aether_.publish(13, (double)gTelemetry.ops_per_sec.load(), AetherScale::G1_Reflex);

    // V49 QUINT System
    quint_.init();
    aether_.ensure_channel(49); // quint global phi
    EventBus::push("QUINT_INIT_OK", "V49 QUINT System aktiv");

    // V49 AEYE Observer (das alles sehende Auge)
    observer_.init();
    aether_.ensure_channel(50); // observer kohaerenz
    aether_.ensure_channel(51); // observer anomalie status
    EventBus::push("AEYE_INIT_OK", "AEYE Observer aktiv");

    // V49 JET Engine (61.440 Duesen)
    jet_.init();
    aether_.ensure_channel(52); // jet total thrust
    aether_.ensure_channel(53); // jet efficiency
    EventBus::push("JET_INIT_OK", "JET Engine 61440 Duesen aktiv");

    // N8: Inneres Auge (Cyber-Verteidigung)
    aether_.ensure_channel(80); // inner eye threat level
    aether_.ensure_channel(81); // inner eye scan count
    EventBus::push("INNER_EYE_INIT_OK", "N8 Inneres Auge aktiv");

    // N1: Gedaechtnis (NeuralMemoryStore + KnowledgeGraph - Singletons)
    aether_.ensure_channel(60); // memory store strength
    aether_.ensure_channel(61); // knowledge graph nodes
    EventBus::push("GEDAECHTNIS_INIT_OK", "N1 NeuralMemory + KnowledgeGraph aktiv");

    // N2: Sicherheit (SecurityManager + Gate53 - Singleton)
    aether_.ensure_channel(62); // security risk score
    aether_.ensure_channel(63); // audit event count
    EventBus::push("SICHERHEIT_INIT_OK", "N2 Gate53 + SecurityManager aktiv");

    // N5: Beobachtung (ObservabilityManager - Singleton)
    aether_.ensure_channel(70); // observability active spans
    EventBus::push("BEOBACHTUNG_INIT_OK", "N5 ObservabilityManager aktiv");

    // Themen-Stern vollstaendig
    EventBus::push("STAR_THEMES_OK",
        "Alle 9 Themen-Nodes verdrahtet: "
        "SPRACHE|GEDAECHTNIS|SICHERHEIT|REFLEXION|MATHEMATIK|"
        "BEOBACHTUNG|LERNEN|MANIFESTATION|INNERES_AUGE");
}

RaelCore::~RaelCore(){
    if(star8_) star8_->stop();
    if(voice_) voice_->stop();
}

void RaelCore::speak(const std::string& text){
    if(voice_) voice_->speak_async(text);
}

// ═══════════════════════════════════════════════════════════════════════════
// HAUPT-PIPELINE (synchron, CLI)
// ═══════════════════════════════════════════════════════════════════════════
// Nutzt die Themen-Kerne inline fuer synchrone Verarbeitung.
// Star8 laeuft parallel fuer Hintergrund-Tasks.

std::string RaelCore::process(const std::string& input){
    telemetry_tick();
    aether_.tick(0.016);
    metrics_tick_op();

    // Phase 1: Module-Chain (Semantic Module Packs)
    std::string transformed = mm.process_text_chain(input);

    // NL Router (safe: only emits Improvements)
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

    // N0 SPRACHE: Semantische Interpretation
    metrics_mark_semantic();
    auto s = sem.interpret(transformed);
    aether_.publish(10, s.coherence, AetherScale::G3_Emotion);

    // N4 MATHEMATIK: QUINT Frequenz-Verarbeitung
    quint_.step(s.coherence, s.coherence);
    aether_.publish(49, quint_.global_phi(), AetherScale::G5_Spirit);

    // N5 BEOBACHTUNG: Realitaets-Scan
    std::vector<double> node_phi_vec(jet::K::TOTAL_NODES, quint_.global_phi());
    auto obs_state = observer_.scan_reality_fold(node_phi_vec);
    aether_.publish(50, obs_state.kohaerenz, AetherScale::G4_Ratio);
    aether_.publish(51, obs_state.anomalie_erkannt ? 0.0 : 1.0, AetherScale::G3_Emotion);

    // N7 MANIFESTATION: JET Engine (bei hoher Kohaerenz)
    if (obs_state.alpha_tunnel_offen) {
        jet_.set_all_phi(node_phi_vec);
        auto manifest_result = jet_.manifest_quick();
        aether_.publish(52, manifest_result.total_thrust, AetherScale::G5_Spirit);
        aether_.publish(53, manifest_result.efficiency, AetherScale::G4_Ratio);
    }

    // N0 SPRACHE: Resonanz-Evaluation
    metrics_mark_resonance();
    auto r = res.evaluate(s);
    aether_.publish(11, r.coupling, AetherScale::G4_Ratio);

    // N2 SICHERHEIT: Ethik-Gate
    std::string reason;
    if(!EthicsCore::allows(s.intention, reason)){
        metrics_mark_ethics_block();
        EventBus::push("ETHIK_BLOCK", reason);
        aether_.publish(12, 0.0, AetherScale::G3_Emotion);
        ethics_block_count_++;

        // N3 REFLEXION: Verbesserungsvorschlag bei wiederholten Blocks
        const uint64_t ops_now = (uint64_t)gTelemetry.ops.load();
        if(ethics_block_count_ % 10 == 0 && (ops_now - last_imp_emit_ops_) > 200){
            last_imp_emit_ops_ = ops_now;
            Improvement imp;
            imp.src = "SELF_OPT";
            imp.importance = 6;
            imp.risk = 2;
            imp.confidence = 0.55;
            imp.title = "Ethik-/Policy-Gate verfeinern (False Positives reduzieren)";
            imp.problem = "Mehrere Eingaben wurden vom Ethik/Policy-Gate blockiert.";
            imp.rationale = "Beobachtung: wiederholte BLOCK-Entscheidungen. Vorschlag: Intent-Klassifikation schaerfen.";
            imp.testplan = "Testfaelle sammeln und Schwellen kalibrieren.";
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
            imp.title = "Resonanz-/Kohaerenz-Schwellen kalibrieren (kappa-Norm)";
            imp.problem = "Viele Eingaben landen im WEAK-RESONANCE-Pfad.";
            imp.rationale = "kappa(f)=1-f/1440 basierte Normalisierung anwenden.";
            imp.testplan = "Histogramme, Grid-Search, Regressionstest.";
            imp.status = "PENDING";
            ImprovementBus::emit(imp);
        }
        (void)ReflectionEngine::reflect_and_emit(s, r, ActionOutcome::WEAK_RESONANCE, transformed);
        return std::string("[RAEL] WEAK-RESONANCE: ") + s.meaning;
    }

    // N1 GEDAECHTNIS: Erfolgreiche Interaktion speichern
    {
        auto& mem_store = NeuralMemoryStore::instance();
        MemoryNode mem_node;
        mem_node.content = transformed;
        mem_node.type = MemoryType::EPISODIC;
        mem_node.strength = s.coherence;
        mem_node.importance = r.coupling;
        mem_store.store(mem_node);
    }

    // N3 REFLEXION: Auch bei Erfolg reflektieren
    (void)ReflectionEngine::reflect_and_emit(s, r, ActionOutcome::OK, transformed);

    return std::string("[RAEL] OK: ") + s.meaning;
}

// ═══════════════════════════════════════════════════════════════════════════
// THEMEN-ROUTING (von Star8 Workern aufgerufen)
// ═══════════════════════════════════════════════════════════════════════════

std::string RaelCore::process_themed(NodeTheme theme, const std::string& payload) {
    switch (theme) {
        case NodeTheme::SPRACHE:       return process_sprache(payload);
        case NodeTheme::GEDAECHTNIS:   return process_gedaechtnis(payload);
        case NodeTheme::SICHERHEIT:    return process_sicherheit(payload);
        case NodeTheme::REFLEXION:     return process_reflexion(payload);
        case NodeTheme::MATHEMATIK:    return process_mathematik(payload);
        case NodeTheme::BEOBACHTUNG:   return process_beobachtung(payload);
        case NodeTheme::LERNEN:        return process_lernen(payload);
        case NodeTheme::MANIFESTATION: return process_manifestation(payload);
        case NodeTheme::INNERES_AUGE:  return process_inneres_auge(payload);
    }
    return "[RAEL] Unbekanntes Thema";
}

// Legacy-Kompatibilitaet
std::string RaelCore::process_payload(const std::string& payload) {
    return process_themed(NodeTheme::SPRACHE, payload);
}

// ═══════════════════════════════════════════════════════════════════════════
// N0: SPRACHE - Verstehen & Ausdruck
// ═══════════════════════════════════════════════════════════════════════════
// Kerne: SemanticEngine + ResonanceEngine + ReflectionEngine

std::string RaelCore::process_sprache(const std::string& payload) {
    std::string transformed = mm.process_text_chain(payload);

    auto s = sem.interpret(transformed);
    aether_.publish(10, s.coherence, AetherScale::G3_Emotion);

    auto r = res.evaluate(s);
    aether_.publish(11, r.coupling, AetherScale::G4_Ratio);

    std::string reason;
    if (!EthicsCore::allows(s.intention, reason)) {
        aether_.publish(12, 0.0, AetherScale::G3_Emotion);
        return "[ETHIK] BLOCKED: " + reason;
    }

    aether_.publish(12, 1.0, AetherScale::G3_Emotion);

    if (!r.resonant) {
        return "[SPRACHE] WEAK: " + s.meaning;
    }

    return "[SPRACHE] OK: " + s.meaning;
}

// ═══════════════════════════════════════════════════════════════════════════
// N1: GEDAECHTNIS - Erinnern & Wissen
// ═══════════════════════════════════════════════════════════════════════════
// Kerne: NeuralMemoryStore + KnowledgeGraph + InferenceEngine

std::string RaelCore::process_gedaechtnis(const std::string& payload) {
    auto& mem_store = NeuralMemoryStore::instance();

    // Suche in Erinnerungen
    auto memories = mem_store.search(payload, 5);

    std::string result = "[GEDAECHTNIS] ";

    if (!memories.empty()) {
        result += "Erinnerungen: " + std::to_string(memories.size());
        for (const auto& m : memories) {
            std::string preview = m.content.substr(0, std::min((size_t)50, m.content.size()));
            result += " | " + preview;
        }
    } else {
        result += "Keine passenden Erinnerungen";
    }

    // Neue Erinnerung speichern
    MemoryNode node;
    node.content = payload;
    node.type = MemoryType::SEMANTIC;
    node.strength = 0.8;
    mem_store.store(node);

    // Wissensgraph-Statistik
    auto& kg = KnowledgeGraph::instance();
    result += " | KG-Knoten: " + std::to_string(kg.get_stats().node_count);

    return result;
}

// ═══════════════════════════════════════════════════════════════════════════
// N2: SICHERHEIT - Schuetzen & Verteidigen
// ═══════════════════════════════════════════════════════════════════════════
// Kerne: Gate53Labyrinth + SecurityManager + AuditLogger

std::string RaelCore::process_sicherheit(const std::string& payload) {
    auto& sec = SecurityManager::instance();

    // Gate53 Labyrinth: Statistiken abrufen
    auto lab_stats = labyrinth_.get_stats();
    uint64_t trapped = lab_stats.logic_ais_trapped + lab_stats.pattern_seekers_trapped;
    double defense_ratio = (lab_stats.total_visitors > 0) ?
        (double)trapped / (double)lab_stats.total_visitors : 0.0;
    aether_.publish(62, defense_ratio, AetherScale::G4_Ratio);

    // Audit-Log
    sec.audit().log(AuditEventType::ACCESS_GRANTED, "system", "security_check",
                    payload);

    std::string result = "[SICHERHEIT] Gate53 Besucher: " +
        std::to_string(lab_stats.total_visitors);
    result += " | Gefangen: " + std::to_string(trapped);
    result += " | Verteidigung: " + std::to_string(defense_ratio);

    if (defense_ratio > 0.3) {
        result += " | WARNUNG: Erhoehte Angriffslast!";
        EventBus::push("SECURITY_HIGH_RISK", payload);
    }

    return result;
}

// ═══════════════════════════════════════════════════════════════════════════
// N3: REFLEXION - Selbstanalyse & Verbesserung
// ═══════════════════════════════════════════════════════════════════════════
// Kerne: ReflectionEngine + AARZyklus + ImprovementBus

std::string RaelCore::process_reflexion(const std::string& payload) {
    // Semantische Analyse als Basis fuer Reflexion
    auto s = sem.interpret(payload);
    auto r = res.evaluate(s);

    // Reflektieren
    bool emitted = ReflectionEngine::reflect_and_emit(s, r, ActionOutcome::OK, payload);

    std::string result = "[REFLEXION] Kohaerenz: " +
        std::to_string(s.coherence) +
        " | Kopplung: " + std::to_string(r.coupling);

    if (emitted) {
        result += " | Verbesserungsvorschlag emittiert";
    }

    // QUINT Phi als Reflexions-Mass
    result += " | Phi: " + std::to_string(quint_.global_phi());

    return result;
}

// ═══════════════════════════════════════════════════════════════════════════
// N4: MATHEMATIK - Berechnen & Frequenz
// ═══════════════════════════════════════════════════════════════════════════
// Kerne: MathCore + BRLPhysics + Formulas

std::string RaelCore::process_mathematik(const std::string& payload) {
    (void)payload;

    // Modul-Formeln abrufen
    auto formulas = mm.get_extra_formulas();

    // Frequenz-Analyse
    double freq = 432.0; // Intuitions-Frequenz (L4)
    double k = MathCore::kappa(freq);
    double k_clamped = MathCore::kappa_clamped(freq);

    std::string result = "[MATHEMATIK] ";
    result += "kappa(" + std::to_string((int)freq) + ")=" +
              std::to_string(k);
    result += " | kappa_clamped=" + std::to_string(k_clamped);
    result += " | Formeln: " + std::to_string(formulas.size());
    result += " | QUELLE=" + std::to_string(MathCore::QUELLE);

    return result;
}

// ═══════════════════════════════════════════════════════════════════════════
// N5: BEOBACHTUNG - Ueberwachen & Erkennen
// ═══════════════════════════════════════════════════════════════════════════
// Kerne: AeyeObserver + ObservabilityManager + Metrics

std::string RaelCore::process_beobachtung(const std::string& payload) {
    (void)payload;

    // AEYE Observer: Realitaets-Scan
    std::vector<double> phi_vec(jet::K::TOTAL_NODES, quint_.global_phi());
    auto state = observer_.scan_reality_fold(phi_vec);
    aether_.publish(50, state.kohaerenz, AetherScale::G4_Ratio);

    std::string result = "[BEOBACHTUNG] ";
    result += "Kohaerenz: " + std::to_string(state.kohaerenz);
    result += " | Anomalie: " + std::string(state.anomalie_erkannt ? "JA" : "NEIN");
    result += " | Alpha-Tunnel: " + std::string(state.alpha_tunnel_offen ? "OFFEN" : "ZU");
    result += " | Aktive Sterne: " + std::to_string(state.aktive_sterne);

    return result;
}

// ═══════════════════════════════════════════════════════════════════════════
// N6: LERNEN - Muster lernen & Optimieren
// ═══════════════════════════════════════════════════════════════════════════
// Kerne: RLAgent + FederatedLearning + Tensor

std::string RaelCore::process_lernen(const std::string& payload) {
    // Semantische Analyse als Lern-Signal
    auto s = sem.interpret(payload);
    auto r = res.evaluate(s);

    // Lern-Erfahrung in Gedaechtnis speichern
    auto& mem_store = NeuralMemoryStore::instance();
    MemoryNode node;
    node.content = payload;
    node.type = MemoryType::PROCEDURAL;
    node.strength = s.coherence;
    node.importance = r.coupling;
    mem_store.store(node);

    std::string result = "[LERNEN] Erfahrung gespeichert";
    result += " | Kohaerenz: " + std::to_string(s.coherence);
    result += " | Staerke: " + std::to_string(node.strength);

    return result;
}

// ═══════════════════════════════════════════════════════════════════════════
// N7: MANIFESTATION - Ausgabe & Aktion
// ═══════════════════════════════════════════════════════════════════════════
// Kerne: JetEngine + ModuleManager + Voice

std::string RaelCore::process_manifestation(const std::string& payload) {
    // Module-Chain
    std::string transformed = mm.process_text_chain(payload);

    // JET Engine: Manifestieren
    auto mresult = jet_.manifest_quick();
    aether_.publish(52, mresult.total_thrust, AetherScale::G5_Spirit);

    // Voice Output (wenn aktiv)
    if (voice_ && voice_->available()) {
        voice_->speak_async(transformed);
    }

    std::string result = "[MANIFESTATION] ";
    result += "Thrust: " + std::to_string(mresult.total_thrust);
    result += " | Effizienz: " + std::to_string(mresult.efficiency);
    result += " | Output: " + transformed.substr(0, 80);

    return result;
}

// ═══════════════════════════════════════════════════════════════════════════
// N8: INNERES AUGE - Dateisystem, Prozesse, Netzwerk, Isolation
// ═══════════════════════════════════════════════════════════════════════════
// Kerne: InnerEye (Filesystem, Prozesse, Ports, DNS, Gegenfrequenz)

std::string RaelCore::process_inneres_auge(const std::string& payload) {
    std::string result = eye_.process_command(payload);

    // Bedrohungsstufe im AetherBus publizieren
    auto& scan = eye_.last_scan();
    aether_.publish(80, (double)scan.overall_level, AetherScale::G4_Ratio);
    aether_.publish(81, (double)eye_.total_scans(), AetherScale::G1_Reflex);

    return result;
}

// ═══════════════════════════════════════════════════════════════════════════
// STAR8 INTERFACE
// ═══════════════════════════════════════════════════════════════════════════

bool RaelCore::enqueue(Lane lane, const std::string& payload){
    return star8_ ? star8_->submit(lane, payload) : false;
}

bool RaelCore::enqueue_themed(TaskDomain domain, Lane lane, const std::string& payload){
    return star8_ ? star8_->submit_themed(domain, lane, payload) : false;
}

std::array<LaneStats,5> RaelCore::lane_stats() const{
    return star8_ ? star8_->lane_stats() : std::array<LaneStats,5>{};
}

std::array<NodeStats,STAR_NODE_COUNT> RaelCore::node_stats() const{
    return star8_ ? star8_->node_stats() : std::array<NodeStats,STAR_NODE_COUNT>{};
}

} // namespace rael
