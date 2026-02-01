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

static void emit_completed(){
    // Zeige die bereits implementierten Verbesserungen (#1-10)
    auto push_done = [&](const std::string& title, const std::string& problem){
        Improvement x;
        x.src = "SELF_OPT";
        x.source = ImprovementSource::SELF_OPT;
        x.importance = 10;
        x.risk = 0;
        x.confidence = 1.0;
        x.title = title;
        x.problem = problem;
        x.rationale = "Bereits implementiert und getestet.";
        x.testplan = "Kompiliert und funktioniert.";
        x.status = "APPLIED";
        x.typed_status = ImprovementStatus::APPLIED;
        ImprovementBus::emit(x);
    };
    push_done("#1 Memory Core MIND³ (5D-Quint NodeMemory)",
              "✓ Star8 Worker haben jetzt 5-Quint Gedächtnis (G1-G5 Ring-Buffer).");
    push_done("#2 SemanticCore: ActionSpec Router",
              "✓ IntentEngine mit Pattern-Matching und ActionSpec-Generierung implementiert.");
    push_done("#3 ResonanceIntentEngine",
              "✓ 3-Pfad-Generierung mit Resonanz-Matrix-Kopplung implementiert.");
    push_done("#4 Rollback/Shadow-Sim",
              "✓ ShadowSimulator und RollbackManager für sichere Änderungen implementiert.");
    push_done("#5 ControlStar + StarRing Docking",
              "✓ Parallele Sterne mit ResultCombiner (4 Strategien) implementiert.");
    push_done("#6 Depth-Scaling via Activation Cache",
              "✓ ActivationCache mit 5 Levels und FastLaneProtector implementiert.");
    push_done("#7 VoicePack Installer",
              "✓ Quarantäne, SignatureVerifier und SHA256-Prüfung implementiert.");
    push_done("#8 Code Review Gate",
              "✓ 15+ Security/Quality Patterns mit Human-Approval-Workflow implementiert.");
    push_done("#9 Improvement Sources trennen",
              "✓ ImprovementSource Enum (REFLECT/LIVE/USER/SELF/AAR) implementiert.");
    push_done("#10 Metrics→Auto Improvements (AAR)",
              "✓ AAREngine mit regelbasierter Metrik-Analyse implementiert.");
}

static void emit_top10(int base_risk){
    // NEUE Verbesserungen für Programmierfähigkeiten (#11-20)
    auto push = [&](const std::string& title, const std::string& problem, int impv, int risk){
        Improvement x;
        x.src = "REFLECT";
        x.source = ImprovementSource::REFLECT;
        x.importance = impv;
        x.risk = risk;
        x.confidence = 0.75;
        x.title = title;
        x.problem = problem;
        x.rationale = "Selbstanalyse: Erforderlich für autonome Programmierfähigkeiten.";
        x.testplan = "Implementieren, kompilieren, Unit-Tests schreiben.";
        x.status = "PENDING";
        x.typed_status = ImprovementStatus::PENDING;
        ImprovementBus::emit(x);
    };

    // Zeige zuerst die abgeschlossenen
    emit_completed();

    // Dann die neuen Vorschläge für Programmierfähigkeiten
    push("#11 FileSystem API (Grundlage)",
         "RAEL kann keine Dateien lesen/schreiben. Benötigt: read_file, write_file, list_dir, exists, create_dir.",
         10, base_risk+1);
    push("#12 Language Parser + Lexer",
         "Kein Syntaxverständnis für Code. Benötigt: Tokenizer, SimpleAST, LanguageRegistry für C++/Python/JS.",
         10, base_risk+1);
    push("#13 Process Executor",
         "Kann keine Compiler/Tests aufrufen. Benötigt: run_command, capture_output, exit_code Handling.",
         10, base_risk+2);
    push("#14 Git/VCS Integration",
         "Keine Versionskontrolle. Benötigt: commit, branch, diff, log, merge, ChangeSet-Tracking.",
         9, base_risk+1);
    push("#15 Project Understanding Engine",
         "Versteht keine Projektstruktur. Benötigt: detect root, parse package.json/Cargo.toml/CMakeLists.",
         9, base_risk);
    push("#16 Build System Integration",
         "Keine Build-Erkennung. Benötigt: CMake/npm/cargo Detektion, Compiler-Fehler-Parsing.",
         9, base_risk+1);
    push("#17 Testing Framework Integration",
         "Kein Test-Support. Benötigt: pytest/jest/cargo test Adapter, Coverage-Tracking.",
         8, base_risk);
    push("#18 Code Generation + Auto-Fix",
         "code_review findet Fehler aber kann nicht fixen. Benötigt: Template-Generator, Refactorer, FixApplier.",
         8, base_risk+1);
    push("#19 Error/Debug Engine",
         "Keine Runtime-Fehler-Analyse. Benötigt: Stack-Trace-Parser, Symbol-Reader, Exception-Flow.",
         7, base_risk);
    push("#20 LSP/IDE Integration",
         "Keine Editor-Integration. Benötigt: LSP-Server, Hover, Completion, Diagnostics, GoTo-Definition.",
         7, base_risk);
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
