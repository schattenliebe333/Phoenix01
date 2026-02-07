#pragma once
// ═══════════════════════════════════════════════════════════════════════════
// STAR-THEMEN: Jeder Node ist ein Themen-Kalkulator
// ═══════════════════════════════════════════════════════════════════════════
//
// Architekten-Vision:
//   - Prozesse stehen sich nicht im Weg
//   - Jeder Stern-Node beinhaltet genau die Kerne, die er braucht
//   - Reichen 9 Nodes nicht, skalieren wir auf 16, 32, ...
//
// N0 SPRACHE       : Semantic + Resonance + Reflection
// N1 GEDAECHTNIS   : NeuralMemory + KnowledgeGraph + Inference
// N2 SICHERHEIT    : Gate53 + SecurityManager + AuditLogger
// N3 REFLEXION     : ReflectionEngine + AARZyklus + Improvements
// N4 MATHEMATIK    : MathCore + BRLPhysics + Formulas
// N5 BEOBACHTUNG   : AeyeObserver + Observability + Metrics
// N6 LERNEN        : RLAgent + FederatedLearning + Tensor
// N7 MANIFESTATION : JetEngine + ModuleManager + Voice
// N8 INNERES_AUGE  : Filesystem + Prozesse + Netzwerk + Gegenfrequenz
//
// ═══════════════════════════════════════════════════════════════════════════

#include <cstdint>
#include <cstddef>

namespace rael {

// Anzahl der Stern-Nodes (skalierbar: 9 -> 16 -> 32)
constexpr size_t STAR_NODE_COUNT = 9;

// ─── Node-Themen ─────────────────────────────────────────────────────────

enum class NodeTheme : uint8_t {
    SPRACHE       = 0,  // N0: Verstehen & Ausdruck
    GEDAECHTNIS   = 1,  // N1: Erinnern & Wissen
    SICHERHEIT    = 2,  // N2: Schuetzen & Verteidigen
    REFLEXION     = 3,  // N3: Selbstanalyse & Verbesserung
    MATHEMATIK    = 4,  // N4: Berechnen & Frequenz
    BEOBACHTUNG   = 5,  // N5: Ueberwachen & Erkennen
    LERNEN        = 6,  // N6: Muster lernen & Optimieren
    MANIFESTATION = 7,  // N7: Ausgabe & Aktion
    INNERES_AUGE  = 8   // N8: Dateisystem, Prozesse, Netzwerk, Isolation
};

// ─── Task-Domaenen ───────────────────────────────────────────────────────
// Was soll getan werden? -> Bestimmt den zustaendigen Themen-Node.

enum class TaskDomain : uint8_t {
    VERSTEHEN      = 0,   // -> SPRACHE
    ERINNERN       = 1,   // -> GEDAECHTNIS
    SCHUETZEN      = 2,   // -> SICHERHEIT
    REFLEKTIEREN   = 3,   // -> REFLEXION
    BERECHNEN      = 4,   // -> MATHEMATIK
    BEOBACHTEN     = 5,   // -> BEOBACHTUNG
    OPTIMIEREN     = 6,   // -> LERNEN
    AUSFUEHREN     = 7,   // -> MANIFESTATION
    SCANNEN        = 8,   // -> INNERES_AUGE
    AUTO           = 255  // -> Automatische Erkennung (Default: SPRACHE)
};

// ─── Hilfsfunktionen ─────────────────────────────────────────────────────

constexpr const char* theme_name(NodeTheme t) {
    switch(t) {
        case NodeTheme::SPRACHE:       return "SPRACHE";
        case NodeTheme::GEDAECHTNIS:   return "GEDAECHTNIS";
        case NodeTheme::SICHERHEIT:    return "SICHERHEIT";
        case NodeTheme::REFLEXION:     return "REFLEXION";
        case NodeTheme::MATHEMATIK:    return "MATHEMATIK";
        case NodeTheme::BEOBACHTUNG:   return "BEOBACHTUNG";
        case NodeTheme::LERNEN:        return "LERNEN";
        case NodeTheme::MANIFESTATION: return "MANIFESTATION";
        case NodeTheme::INNERES_AUGE:  return "INNERES_AUGE";
    }
    return "UNBEKANNT";
}

constexpr const char* theme_description(NodeTheme t) {
    switch(t) {
        case NodeTheme::SPRACHE:       return "Semantic + Resonance + Reflection";
        case NodeTheme::GEDAECHTNIS:   return "NeuralMemory + KnowledgeGraph + Inference";
        case NodeTheme::SICHERHEIT:    return "Gate53 + SecurityManager + AuditLogger";
        case NodeTheme::REFLEXION:     return "ReflectionEngine + AARZyklus + Improvements";
        case NodeTheme::MATHEMATIK:    return "MathCore + BRLPhysics + Formulas";
        case NodeTheme::BEOBACHTUNG:   return "AeyeObserver + Observability + Metrics";
        case NodeTheme::LERNEN:        return "RLAgent + FederatedLearning + Tensor";
        case NodeTheme::MANIFESTATION: return "JetEngine + ModuleManager + Voice";
        case NodeTheme::INNERES_AUGE:  return "Filesystem + Prozesse + Netzwerk + Gegenfrequenz";
    }
    return "";
}

constexpr const char* domain_name(TaskDomain d) {
    switch(d) {
        case TaskDomain::VERSTEHEN:    return "VERSTEHEN";
        case TaskDomain::ERINNERN:     return "ERINNERN";
        case TaskDomain::SCHUETZEN:    return "SCHUETZEN";
        case TaskDomain::REFLEKTIEREN: return "REFLEKTIEREN";
        case TaskDomain::BERECHNEN:    return "BERECHNEN";
        case TaskDomain::BEOBACHTEN:   return "BEOBACHTEN";
        case TaskDomain::OPTIMIEREN:   return "OPTIMIEREN";
        case TaskDomain::AUSFUEHREN:   return "AUSFUEHREN";
        case TaskDomain::SCANNEN:      return "SCANNEN";
        case TaskDomain::AUTO:         return "AUTO";
    }
    return "UNBEKANNT";
}

// Routing: Domaene -> Thema
constexpr NodeTheme domain_to_theme(TaskDomain d) {
    switch(d) {
        case TaskDomain::VERSTEHEN:    return NodeTheme::SPRACHE;
        case TaskDomain::ERINNERN:     return NodeTheme::GEDAECHTNIS;
        case TaskDomain::SCHUETZEN:    return NodeTheme::SICHERHEIT;
        case TaskDomain::REFLEKTIEREN: return NodeTheme::REFLEXION;
        case TaskDomain::BERECHNEN:    return NodeTheme::MATHEMATIK;
        case TaskDomain::BEOBACHTEN:   return NodeTheme::BEOBACHTUNG;
        case TaskDomain::OPTIMIEREN:   return NodeTheme::LERNEN;
        case TaskDomain::AUSFUEHREN:   return NodeTheme::MANIFESTATION;
        case TaskDomain::SCANNEN:      return NodeTheme::INNERES_AUGE;
        case TaskDomain::AUTO:         return NodeTheme::SPRACHE;
    }
    return NodeTheme::SPRACHE;
}

// Thema -> Node-Index (1:1 Mapping fuer erste 9 Nodes)
constexpr size_t theme_to_node(NodeTheme t) {
    return static_cast<size_t>(t);
}

} // namespace rael
