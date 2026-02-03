#include "rael/nl_router.h"
#include "rael/improvements.h"
#include "rael/util.h"

#include <string>
#include <vector>

namespace rael {

static std::string lower_ascii(std::string s){
    for(char& c: s){
        if(c>='A' && c<='Z') c = (char)(c - 'A' + 'a');
    }
    return s;
}

static bool contains_any(const std::string& hay, const std::vector<std::string>& needles){
    for(const auto& n: needles){
        if(!n.empty() && hay.find(n) != std::string::npos) return true;
    }
    return false;
}

static void emit_top10(int base_risk){
    auto push = [&](const std::string& title, const std::string& problem, int impv, int risk){
        Improvement x;
        x.src = "USER_TASK";
        x.importance = impv;
        x.risk = risk;
        x.confidence = 0.62;
        x.title = title;
        x.problem = problem;
        x.rationale = "User-requested roadmap suggestion. Apply only after human review.";
        x.testplan = "Create minimal test + rollback plan before integration.";
        x.status = "PENDING";
        ImprovementBus::emit(x);
    };
    push("SemanticCore: ActionSpec Router (freie Sprache→Plan)",
         "RAEL soll freie Sprache robust in Intent/Constraints/Plan übersetzen, ohne starre Kommandos.",
         9, base_risk);
    push("ResonanceIntentEngine: Absicht→Wege 1/2/3",
         "Absichtserkennung + alternative Pfade, um Defense auf Absicht statt Signatur zu bauen.",
         9, base_risk);
    push("Memory Core MIND³ (Kurz/Arbeits/Langzeit)",
         "Fehlendes Gedächtnis verhindert stabile Intent-Wiedererkennung und echte Selbstoptimierung.",
         10, base_risk+1);
    push("ControlStar + StarRing Docking (Breite)",
         "Parallele Sterne für Hypothesen/Coding/Defense-Varianten mit Combiner.",
         8, base_risk);
    push("Depth-Scaling (Tiefe) via Activation Cache",
         "Selbstrechnender Speicher (Tiefe) dynamisch skalieren, ohne Fast-Lanes zu stören.",
         8, base_risk);
    push("VoicePack Installer (signiertes Manifest + Hashes)",
         "Natural-Voice offline nachinstallieren, aber supply-chain sicher (Quarantäne, Signatur, SHA256).",
         8, base_risk);
    push("Code Review Gate (RAEL liest Code, Human approves)",
         "RAEL kann Code analysieren und Risiken erklären; Integration bleibt human-approved.",
         8, base_risk);
    push("Formel-Bibliothek anbinden (RST Master/FO)",
         "Die großen Formeldokumente (Master/FO) als internes Nachschlagewerk und Scoring-Basis in RAEL integrieren (κ-Norm, Threat-Features, Memory-Blueprint).",
         8, base_risk);
    push("Metrics→Auto Improvements (AAR)",
         "Automatisch aus Metriken/Outcomes Verbesserungen ableiten und in Tab listen.",
         7, base_risk);
    push("Rollback/Shadow-Sim für riskante Änderungen",
         "Isolierte Simulation/Shadow-grammar Umgebung für gefährliche Änderungen vor Freigabe.",
         9, base_risk+2);
}

NLRouterResult NaturalLanguageRouter::route_and_emit(const std::string& raw_input) const {
    NLRouterResult out;
    std::string s = trim(raw_input);
    if(s.empty()) return out;
    const std::string low = lower_ascii(s);

    // TOP-10 improvements in free form.
    const bool mentions_top = (low.find("top") != std::string::npos) && (low.find("10") != std::string::npos);
    const bool mentions_improve = contains_any(low, {"improvement", "improvements", "verbesser", "weiterentwick", "ideen", "roadmap"});
    if(mentions_top && mentions_improve){
        int base_risk = 3;
        if(contains_any(low, {"low", "niedrig", "gering"})) base_risk = 2;
        else if(contains_any(low, {"high", "hoch"})) base_risk = 6;
        else if(contains_any(low, {"mid", "mittel"})) base_risk = 4;
        emit_top10(base_risk);
        out.handled = true;
        out.response = "[RAEL] OK: 10 Improvements emitted (USER_TASK).";
        return out;
    }

    // If user mentions formula collection, propose a concrete integration.
    if(contains_any(low, {"formel", "formeln", "350", "rst-master", "master-formeln"})){
        Improvement imp;
        imp.src = "USER_TASK";
        imp.importance = 8;
        imp.risk = 2;
        imp.confidence = 0.65;
        imp.title = "Formel-Katalog: RST-MASTER/FO als Offline-Index integrieren";
        imp.problem = "Große Formelsammlungen liegen vor, werden aber im System noch nicht als Index/Scoring-Basis genutzt.";
        imp.rationale = "Vorschlag: Offline-Formelindex bauen (Titel/Tags/Keywords → Referenz) und κ-Norm (QUELLE=1440, TOR=720) als globalen Normalisierer nutzen. So kann RAEL beim Planen/Reflektieren passende Formeln vorschlagen.";
        imp.testplan = "(1) Formeldokumente als Resources registrieren, (2) Keyword-Suche testen, (3) 5 Beispiel-Queries (Firewall/Memory/Zeit/Entropie/Resonanz) gegen erwartete Treffer prüfen.";
        imp.status = "PENDING";
        ImprovementBus::emit(imp);
        out.handled = true;
        out.response = "[RAEL] OK: Improvement emitted (Formel-Katalog).";
        return out;
    }

    // Learning from URL: DO NOT crawl; emit safe manifest-based approach.
    if(contains_any(low, {"url", "link", "website", "webseite"}) && contains_any(low, {"lern", "lernen", "learn", "train", "training"})){
        Improvement imp;
        imp.src = "USER_TASK";
        imp.importance = 8;
        imp.risk = 4;
        imp.confidence = 0.58;
        imp.title = "LearningPack: manifest-/signaturbasiertes Lernen von URL statt Crawler";
        imp.problem = "Freies Crawlen von Websites erweitert Angriffsfläche stark (Parser, Redirects, Mixed Content).";
        imp.rationale = "Vorschlag: URL nur für signiertes manifest.json akzeptieren. Manifest listet exakte Dateien + SHA256; Download geht in Quarantäne; erst nach Prüfung wird ein Offline-LearningPack indiziert. Das erfüllt 'RAEL ist wach im Internet' bei minimaler Supply-Chain-Fläche.";
        imp.testplan = "(1) Manifest-Schema definieren, (2) Signaturprüfung (Public Key embedded), (3) Quarantäne-Install, (4) Offline-Indexer. Test: manipuliertes Paket muss blockieren.";
        imp.status = "PENDING";
        ImprovementBus::emit(imp);
        out.handled = true;
        out.response = "[RAEL] OK: Improvement emitted (LearningPack URL via manifest).";
        return out;
    }

    return out;
}

} // namespace rael
