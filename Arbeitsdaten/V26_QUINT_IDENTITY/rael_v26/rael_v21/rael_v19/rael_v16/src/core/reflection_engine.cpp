#include "rael/reflection_engine.h"

#include "rael/semantic.h"
#include "rael/resonance.h"
#include "rael/improvements.h"
#include "rael/mathcore.h"
#include "rael/telemetry.h"

#include <algorithm>
#include <cctype>
#include <string>
#include <unordered_map>

namespace rael {

static std::string lower_ascii(std::string s){
    for(char& c: s) c = (char)std::tolower((unsigned char)c);
    return s;
}

static std::string clip(const std::string& s, size_t n){
    if(s.size() <= n) return s;
    return s.substr(0, n) + "…";
}

// Very lightweight "intent -> plausible alternative paths" generator.
// Defensive use only: we generate *possible attacker paths* so that defenses can be hardened.
static void intent_paths(const std::string& intent, std::string& out_paths){
    const std::string i = lower_ascii(intent);
    // Default generic paths
    std::string p1 = "Pfad 1: Supply-Chain über manipulierte Download-Quelle / Mirror";
    std::string p2 = "Pfad 2: Side-Load/DLL-Hijacking über legitimes Binary + bösartige Neben-Datei";
    std::string p3 = "Pfad 3: Konfig-/Policy-Manipulation (z.B. Registry/Settings) zur Abschwächung von Regeln";

    if(i.find("voice") != std::string::npos || i.find("tts") != std::string::npos || i.find("sprache") != std::string::npos){
        p1 = "Pfad 1: Voice-Pack mit gültig wirkendem Namen, aber manipuliertem Model/Binary";
        p2 = "Pfad 2: Dependency/Runtime-Side-Load (z.B. fake DLL neben piper/engine)";
        p3 = "Pfad 3: Model-Polyglot (Datei, die Parser triggert) -> Crash/Code-Execution";
    } else if(i.find("update") != std::string::npos || i.find("install") != std::string::npos || i.find("download") != std::string::npos){
        p1 = "Pfad 1: Signatur-/Manifest-Spoofing (falsches Manifest, falsche Keys)";
        p2 = "Pfad 2: TOCTOU zwischen Hash-Check und Move-to-active";
        p3 = "Pfad 3: Rollback-Attack (alte, verwundbare Version wird 'installiert')";
    } else if(i.find("firewall") != std::string::npos || i.find("security") != std::string::npos){
        p1 = "Pfad 1: Evasion über Low-and-Slow / Timing-Pattern (unter Thresholds)";
        p2 = "Pfad 2: Payload-Splitting / Fragmentation, die Parser/Signaturen umgeht";
        p3 = "Pfad 3: Intent-Masking: harmlose Telemetry vortäuschen, während Ziel erreicht wird";
    }

    out_paths = std::string("- ") + p1 + "\n- " + p2 + "\n- " + p3;
}

// Minimal rate limiter keyed by intent.
static bool should_emit(const std::string& intent, uint64_t ops_now){
    static std::unordered_map<std::string, uint64_t> last_ops;
    const std::string key = intent.empty() ? "__none__" : lower_ascii(intent);
    const uint64_t prev = last_ops[key];
    // Emit at most once per ~400 ops per intent.
    if(prev != 0 && (ops_now - prev) < 400) return false;
    last_ops[key] = ops_now;
    return true;
}

bool ReflectionEngine::reflect_and_emit(const SemanticResult& s,
                                       const ResonanceResult& r,
                                       ActionOutcome outcome,
                                       const std::string& raw_input){
    const uint64_t ops_now = (uint64_t)gTelemetry.ops.load();
    if(!should_emit(s.intention, ops_now)) return false;

    // Only emit on "interesting" outcomes for now.
    if(outcome == ActionOutcome::OK) return false;

    // Use κ-norm to weight importance a bit (higher disorder -> higher importance).
    // We map coherence (0..1) to a pseudo-frequency and back through κ.
    const double pseudo_f = (1.0 - std::clamp(s.coherence, 0.0, 1.0)) * MathCore::QUELLE;
    const double k = std::clamp(MathCore::kappa(pseudo_f), 0.0, 1.0);
    const double disorder = 1.0 - k;

    Improvement imp;
    imp.src = "REFLECT";
    imp.status = "PENDING";
    imp.confidence = std::clamp((s.coherence * 0.6) + (r.coupling * 0.4), 0.0, 1.0);
    imp.risk = 2;
    imp.importance = (int)std::clamp( (int)(6 + disorder * 4), 1, 10 );

    std::string outcome_s;
    if(outcome == ActionOutcome::ETHIK_BLOCK) outcome_s = "ETHIK_BLOCK";
    else outcome_s = "WEAK_RESONANCE";

    imp.title = "Reflexion: " + outcome_s + " – Absicht & alternative Pfade absichern";
    imp.problem = "Eingabe führte zu " + outcome_s + ". Das ist entweder korrekt (Policy) oder ein Hinweis auf fehlende Klassifikation/Heuristiken. Zusätzlich: ein Angreifer könnte dieselbe Absicht über alternative Pfade erreichen.";

    std::string paths;
    intent_paths(s.intention, paths);

    imp.rationale =
        std::string("Input (gekürzt): ") + clip(raw_input, 160) + "\n"
        + "Erkannte Absicht: " + (s.intention.empty()?"(unbekannt)":s.intention) + "\n"
        + "Kohärenz: " + std::to_string(s.coherence)
        + " | Kopplung: " + std::to_string(r.coupling)
        + " | κ(pseudo): " + std::to_string(k) + "\n\n"
        + "Mögliche alternative Pfade (Absichts-orientiert):\n" + paths + "\n\n"
        + "Defensive Idee: Absicht als Zielobjekt absichern, nicht nur den beobachteten Weg. (Pfad 1–3) priorisiert härten.";

    imp.testplan =
        "(1) Für Pfad 1–3 je einen Testfall definieren (ohne schädliche Payloads). "
        "(2) Prüfen, ob RAEL die Absicht dennoch erkennt und HOLD/Block/Hardening-Vorschläge erzeugt. "
        "(3) Regression: legitime Nutzung darf nicht unnötig leiden.";

    // No code snippet by default. The Coding tab will provide code only after explicit permission.
    ImprovementBus::emit(imp);
    return true;
}

} // namespace rael
