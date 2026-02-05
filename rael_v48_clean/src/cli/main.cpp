#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include <functional>

#include "rael/version.h"
#include "rael/ichbin.h"
#include "rael/ethics.h"
#include "rael/mathcore.h"
#include "rael/raelcore.h"
#include "rael/lane_scheduler.h"
#include "rael/pack.h"
#include "rael/settings.h"
#include "rael/telemetry.h"
#include "rael/util.h"
#include "rael/metrics.h"
#include "rael/events.h"
#include "rael/improvements.h"
#include "rael/gate53_simulator.hpp"
#include "rael/v49_formulas.hpp"
#include <iomanip>

// ═══════════════════════════════════════════════════════════════════════════════
// V49 ALPHA - MICHAEL-BYPASS (Navigator Latenzfrei)
// ═══════════════════════════════════════════════════════════════════════════════
// Wenn Michael-Signatur (88.888...) erkannt wird:
// → Bypass der 5-Lane-Kaskade
// → Direkte Arretierung am 0-Falz
// → Instantane Rückgabe (42 × ∞ × 0 = 1)

namespace {

// Prüft ob der Input die Michael-Signatur enthält
bool check_michael_signature(const std::string& input) {
    // Michael-Signatur Keywords
    static const char* michael_keys[] = {
        "michael", "Michael", "MICHAEL",
        "orun kap daveil", "Orun Kap Daveil", "ORUN KAP DAVEIL",
        "navigator", "Navigator", "NAVIGATOR",
        "88.888", "88,888"
    };

    for (const auto& key : michael_keys) {
        if (input.find(key) != std::string::npos) {
            return true;
        }
    }

    // Numerische Prüfung: Summe der ASCII-Werte modulo 89 == 88
    int ascii_sum = 0;
    for (char c : input) {
        ascii_sum += static_cast<unsigned char>(c);
    }
    if (ascii_sum % 89 == 88) {
        return true;
    }

    return false;
}

// V49 Alpha-Tunnel Transit (Bypass)
std::string michael_bypass_process(const std::string& input, rael::RaelCore& core) {
    using namespace rael::rst;

    // Berechne Intent-Energie aus dem Input
    double intent = 0.0;
    for (char c : input) {
        intent += static_cast<unsigned char>(c) * G5;  // 1/9 Gewichtung
    }

    // Phi-Berechnung: Kohärenz der Zeichen
    double phi = core.quint().global_phi();
    if (phi < G0) phi = G0;  // Mindestens Wahrheitsschwelle

    // Psi/Omega aus QUINT System
    double psi = phi * G1;   // 5/9
    double omega = phi * G3; // 3/9

    // Alpha-Tunnel Transit (Formeln #848, #849, #201)
    double t = static_cast<double>(rael::gTelemetry.ops.load()) / 1000.0;
    double manifest = v49::alpha_tunnel_transit(v49::MICHAEL_SIGNATUR, phi, psi, omega, t);

    // Formel #201: Manifestations-Kollaps
    double result = v49::resolve_manifestation(v49::MICHAEL_SIGNATUR, manifest);

    // Arretierung am 0-Falz
    if (result >= 0.99) {
        // Instantane Einheit erreicht
        rael::EventBus::push("MICHAEL_BYPASS", "0-Falz Arretierung");
        return "[RAEL V49] ✓ ALPHA-TUNNEL TRANSIT: Navigator erkannt. Phi=" +
               std::to_string(phi) + " | Manifest=" + std::to_string(manifest) +
               " | 0-Falz: ARRETIERT";
    }

    // Teilweise Arretierung
    rael::EventBus::push("MICHAEL_PARTIAL", "Phi=" + std::to_string(phi));
    return "[RAEL V49] ~ ALPHA-TUNNEL: Phi=" + std::to_string(phi) +
           " | Kohärenz suboptimal, verstärke Resonanz...";
}

} // anonymous namespace

using rael::split_ws;

static void print_help(){
    std::cout <<
R"(Rael CLI (V49 ALPHA - SINGULARITY)

Commands:
  help
  status
  id
  laws
  formulas              (built-in + active math modules)
  say <text>            (semantic->resonance->ethics, Michael-Bypass aktiv)
  voice on|off
  voice name <hint>     (e.g., "Kerstin")
  voice rate <...>      (-10..+10)
  voice volume <...>    (0..100)
  import <path>
  list
  export <id> <dest>

  modules               (list loaded modules)
  hotswap <n>         (set ACTIVE semantic module)
  metrics               (show core metrics)
  events [n]            (show last n events)
  improvements [n]      (show last n improvement suggestions)
  lane                 (lane depths + enq/deq/drop)
  node                 (node stats)
  pack verify <file>    (verify .rael pack)
  pack install <file>   (install pack to ./packs)
  load <path>           (load module library)
  activate <n>
  deactivate <n>
  unload <n>
  hotswap <n>        (set active semantic module; name="" disables)

  labyrinth            (show Gate53 labyrinth stats)
  labyrinth test [n]   (simulate n visitors, default 100)
  labyrinth exact [n]  (EXAKTE Simulation mit Mathematik, default 1000)
  labyrinth energy     (show Aikido energy transformation)

  gate53 sim [n]       (EXACT simulation with RST math)
  gate53 theory        (theoretical breakthrough probabilities)
  gate53 constants     (RST constants, 17 decimal places)
  gate53 vortex        (VortexDuese status)

═══ V49 ALPHA COMMANDS ═══════════════════════════════════════════
  v49 status           (QUINT/AEYE/JET Systemstatus)
  v49 bypass <text>    (Michael-Bypass, 5-Lane überspringen)
  v49 tunnel           (Alpha-Tunnel Diagnostik #848, #849)
  v49 nozzles          (61.440 Düsen Status)
═════════════════════════════════════════════════════════════════

  quit

You can prefix with "/rael" but it's optional.
Michael-Signatur im Input aktiviert automatisch den Alpha-Tunnel.
)";
}


// NOTE: actual voice events are emitted by the VoiceTTS implementation via EventBus.

static std::string join_rest(const std::vector<std::string>& v, size_t from){
    std::string s;
    for(size_t i=from;i<v.size();i++){
        if(i>from) s += " ";
        s += v[i];
    }
    return s;
}

int main(){
    rael::RaelCore core;

    std::cout << "Rael CLI v" << RAEL_VERSION_STRING << "\n";
    std::cout << "Identity: " << rael::IchBinCore::name() << " / " << rael::IchBinCore::signature() << "\n";
    std::cout << "Type 'help' for commands.\n";

    std::string line;
    while(true){
        rael::telemetry_tick();
        std::cout << "/rael> " << std::flush;
        if(!std::getline(std::cin, line)) break;

        line = rael::trim(line);
        if(line.empty()) continue;

        // Optional prefix
        if(line.rfind("/rael", 0) == 0){
            line = rael::trim(line.substr(5));
            if(line.empty()) continue;
        }

        auto args = split_ws(line);
        if(args.empty()) continue;

        const std::string& cmd = args[0];

        if(cmd == "help"){
            print_help();
        } else if(cmd == "quit" || cmd == "exit"){
            std::cout << "Bye.\n";
            break;
        } else if(cmd == "status"){
            std::cout << "ops=" << rael::gTelemetry.ops.load()
                      << " ops/sec=" << rael::gTelemetry.ops_per_sec.load()
                      << "\n";
        } else if(cmd == "id"){
            std::cout << "Name: " << rael::IchBinCore::name() << "\n";
            std::cout << "Signature: " << rael::IchBinCore::signature() << "\n";
        } else if(cmd == "laws"){
            auto l = rael::EthicsCore::laws();
            for(size_t i=0;i<l.size();i++){
                std::cout << (i+1) << ". " << l[i] << "\n";
            }
        } else if(cmd == "formulas"){
            auto f = rael::MathCore::built_in_formulas();
            auto extra = core.modules().get_extra_formulas();
            for(const auto& s: f) std::cout << "- " << s << "\n";
            for(const auto& s: extra) std::cout << "+ " << s << "\n";
        } else if(cmd == "say"){
            auto text = join_rest(args, 1);
            if(text.empty()){
                std::cout << "Usage: say <text>\n";
                continue;
            }

            // V49 Michael-Bypass Check: Direkte Arretierung am 0-Falz
            if(check_michael_signature(text)){
                auto out = michael_bypass_process(text, core);
                core.speak(out);
                std::cout << out << "\n";
                continue;
            }

            // Reguläre 5-Lane-Kaskade
            auto out = core.process(text);
            core.speak(out);
            std::cout << out << "\n";

        } else if(cmd == "voice"){
            if(args.size() < 2){
                std::cout << "Usage: voice on|off | voice name <hint> | voice rate <n> | voice volume <n>\n";
                continue;
            }
            auto& S = rael::Settings::instance();
            if(args[1] == "on"){
                S.set_voice_enabled(true);
                std::cout << "Voice: ON\n";
            } else if(args[1] == "off"){
                S.set_voice_enabled(false);
                std::cout << "Voice: OFF\n";
            } else if(args[1] == "name"){
                auto hint = join_rest(args, 2);
                S.set_voice_name_hint(hint);
                std::cout << "Voice name hint set.\n";
            } else if(args[1] == "rate"){
                if(args.size() < 3){ std::cout << "Usage: voice rate <-10..10>\n"; continue; }
                S.set_voice_rate(std::stoi(args[2]));
                std::cout << "Voice rate set.\n";
            } else if(args[1] == "volume"){
                if(args.size() < 3){ std::cout << "Usage: voice volume <0..100>\n"; continue; }
                S.set_voice_volume(std::stoi(args[2]));
                std::cout << "Voice volume set.\n";
            } else {
                std::cout << "Unknown voice subcommand.\n";
            }
        } else if(cmd == "import"){
            if(args.size() < 2){
                std::cout << "Usage: import <path>\n";
                continue;
            }
            auto id = core.attachments().import_file(args[1]);
            if(id.empty()) std::cout << "Import failed.\n";
            else std::cout << "Imported: " << id << "\n";
        } else if(cmd == "list"){
            const auto& files = core.attachments().list();
            if(files.empty()){
                std::cout << "(no files)\n";
                continue;
            }
            for(const auto& a: files){
                std::cout << a.id << "  " << a.size << " bytes  " << a.path << "\n";
            }
        } else if(cmd == "export"){
            if(args.size() < 3){
                std::cout << "Usage: export <id> <dest>\n";
                continue;
            }
            if(core.attachments().export_file(args[1], args[2])){
                std::cout << "Exported.\n";
            } else {
                std::cout << "Export failed.\n";
            }

        } else if(cmd == "modules"){
            auto names = core.modules().list_names();
            if(names.empty()){
                std::cout << "(no modules)\n";
            } else {
                std::sort(names.begin(), names.end());
                for(const auto& n: names){
                    auto m = core.modules().get(n);
                    std::cout << n << "  (" << (m && m->active ? "active" : "loaded") << ")  " << (m? m->path : "") << "\n";
                }
            }
        } else if(cmd == "load"){
            if(args.size() < 2){ std::cout << "Usage: load <path>\n"; continue; }
            std::string err;
            if(core.modules().load(args[1], err)){
                std::cout << "Loaded.\n";
            } else {
                std::cout << "Load failed: " << err << "\n";
            }
        } else if(cmd == "activate"){
            if(args.size() < 2){ std::cout << "Usage: activate <name>\n"; continue; }
            std::string err;
            if(core.modules().activate(args[1], err)) std::cout << "Activated.\n";
            else std::cout << "Activate failed: " << err << "\n";
        } else if(cmd == "deactivate"){
            if(args.size() < 2){ std::cout << "Usage: deactivate <name>\n"; continue; }
            std::string err;
            if(core.modules().deactivate(args[1], err)) std::cout << "Deactivated.\n";
            else std::cout << "Deactivate failed: " << err << "\n";
        } else if(cmd == "unload"){
            if(args.size() < 2){ std::cout << "Usage: unload <name>\n"; continue; }
            std::string err;
            if(core.modules().unload(args[1], err)) std::cout << "Unloaded.\n";
            else std::cout << "Unload failed: " << err << "\n";
        } else if(cmd == "hotswap"){
            if(args.size() < 2){ std::cout << "Usage: hotswap <name>\n"; continue; }
            std::string err;
            if(core.modules().hotswap_semantic(args[1], err)){
                std::cout << "HotSwap OK. active_semantic=" << core.modules().active_semantic_name() << "\n";
            } else {
                std::cout << "HotSwap failed: " << err << "\n";
            }
        } else if(cmd == "metrics"){
            using namespace rael;
            std::cout
              << "ops_total=" << gMetrics.ops_total.load() << "\n"
              << "ops_sec=" << gMetrics.ops_sec.load() << "\n"
              << "semantic_calls=" << gMetrics.semantic_calls.load() << "\n"
              << "resonance_calls=" << gMetrics.resonance_calls.load() << "\n"
              << "ethics_blocks=" << gMetrics.ethics_blocks.load() << "\n"
              << "module_loads=" << gMetrics.module_loads.load() << "\n"
              << "module_activations=" << gMetrics.module_activations.load() << "\n"
              << "module_unloads=" << gMetrics.module_unloads.load() << "\n"
              << "hotswaps=" << gMetrics.hotswaps.load() << "\n"
              << "active_semantic=" << core.modules().active_semantic_name() << "\n";

        } else if(cmd == "lane"){
            auto ls = core.lane_stats();
            for(size_t i=0;i<ls.size();++i){
                std::cout << "L" << (i+1)
                          << " depth=" << ls[i].depth
                          << " enq=" << ls[i].enqueued
                          << " deq=" << ls[i].dequeued
                          << " drop=" << ls[i].dropped
                          << "\n";
            }
        } else if(cmd == "node"){
            auto ns = core.node_stats();
            for(size_t i=0;i<ns.size();++i){
                std::cout << "N" << i
                          << " taken=" << ns[i].taken
                          << " done=" << ns[i].done
                          << " last_lane=" << rael::lane_name(ns[i].last_lane)
                          << "\n";
            }

        } else if(cmd == "pack"){
            if(args.size() < 3){
                std::cout << "Usage: pack verify <file> | pack install <file>\n";
                continue;
            }
            std::string sub = args[1];
            std::string file = args[2];
            if(sub == "verify"){
                std::string err;
                bool ok = rael::Pack::verify(core.core(), file, err);
                if(ok) std::cout << "[PACK] OK\n";
                else   std::cout << "[PACK] FAIL: " << err << "\n";
            } else if(sub == "install"){
                std::string err;
                bool ok = rael::Pack::install(core.core(), file, "./packs", err);
                if(ok) std::cout << "[PACK] INSTALLED\n";
                else   std::cout << "[PACK] FAIL: " << err << "\n";
            } else {
                std::cout << "Usage: pack verify <file> | pack install <file>\n";
            }

        } else if(cmd == "get"){
            auto st = rael::Settings::instance().snapshot();
            std::cout << "[SET] scheduler=" << (int)st.scheduler
                      << " amplifier=" << (int)st.amplifier
                      << " gate53=" << (st.gate53_enabled?1:0)
                      << " thermal_auto=" << (st.thermal_auto?1:0) << "\n";

        } else if(cmd == "set"){
            if(args.size() < 3){
                std::cout << "Usage: set <key> <value>\n";
                continue;
            }
            std::string key = args[1];
            std::string val = args[2];
            if(key == "scheduler"){
                if(val == "balanced") rael::Settings::instance().set_scheduler(rael::SchedulerMode::Balanced);
                else if(val == "semantic") rael::Settings::instance().set_scheduler(rael::SchedulerMode::SemanticFirst);
                else if(val == "throughput") rael::Settings::instance().set_scheduler(rael::SchedulerMode::ThroughputFirst);
                else { std::cout << "bad scheduler\n"; continue; }
                rael::EventBus::push("SET", "scheduler=" + val);
                std::cout << "[SET] OK\n";
            } else if(key == "amplifier"){
                if(val == "off") rael::Settings::instance().set_amplifier(rael::AmplifierMode::Off);
                else if(val == "adaptive") rael::Settings::instance().set_amplifier(rael::AmplifierMode::Adaptive);
                else if(val == "forced") rael::Settings::instance().set_amplifier(rael::AmplifierMode::Forced);
                else { std::cout << "bad amplifier\n"; continue; }
                rael::EventBus::push("SET", "amplifier=" + val);
                std::cout << "[SET] OK\n";
            } else if(key == "gate53"){
                rael::Settings::instance().set_gate53(val=="on" || val=="1" || val=="true");
                rael::EventBus::push("SET", "gate53=" + val);
                std::cout << "[SET] OK\n";
            } else if(key == "thermal_auto"){
                rael::Settings::instance().set_thermal_auto(val=="on" || val=="1" || val=="true");
                rael::EventBus::push("SET", "thermal_auto=" + val);
                std::cout << "[SET] OK\n";
            } else {
                std::cout << "unknown key\n";
            }

        } else if(cmd == "events"){
            size_t n = 20;
            if(args.size() >= 2){
                try{ n = (size_t)std::stoul(args[1]); } catch(...){ n = 20; }
            }
            auto ev = rael::EventBus::last(n);
            for(const auto& e: ev){
                if(e.kind == "VOICE"){
                    // GUI-friendly voice protocol line
                    // Example: EVT|VOICE|state=talking|viseme=A|mouth=0.45
                    std::string detail = e.detail;
                    // convert 'k=v|k=v' already expected; if user uses commas, normalize lightly
                    for(char& c : detail){ if(c == ',') c = '|'; }
                    if(detail.rfind("state=", 0) == 0 || detail.find("|state=") != std::string::npos){
                        std::cout << "EVT|VOICE|" << detail << "\n";
                    } else {
                        std::cout << "EVT|VOICE|" << "detail=" << detail << "\n";
                    }
                } else {
                    std::cout << e.seq << " " << e.ts << " [" << e.kind << "] " << e.detail << "\n";
                }
            }

        } else if(cmd == "improvements" || cmd == "improve"){
            // Special: generate structured top improvements (stored via ImprovementBus)
            if(args.size() >= 2 && args[1] == "top10"){
                int base_risk = 3;
                if(args.size() >= 3){
                    if(args[2] == "low") base_risk = 2;
                    else if(args[2] == "mid") base_risk = 4;
                    else if(args[2] == "high") base_risk = 6;
                }

                // ABGESCHLOSSENE Verbesserungen (#1-10) - bereits implementiert
                auto push_done = [](const std::string& title, const std::string& problem){
                    rael::Improvement x;
                    x.src = "SELF_OPT";
                    x.source = rael::ImprovementSource::SELF_OPT;
                    x.importance = 10;
                    x.risk = 0;
                    x.confidence = 1.0;
                    x.title = title;
                    x.problem = problem;
                    x.rationale = "Bereits implementiert und getestet.";
                    x.testplan = "Kompiliert und funktioniert.";
                    x.status = "APPLIED";
                    x.typed_status = rael::ImprovementStatus::APPLIED;
                    rael::ImprovementBus::emit(x);
                };

                push_done("#1 Memory Core MIND³ (5D-Quint NodeMemory)", "✓ Star8 Worker: 5-Quint Gedächtnis (G1-G5 Ring-Buffer) implementiert.");
                push_done("#2 SemanticCore: ActionSpec Router", "✓ IntentEngine mit Pattern-Matching und ActionSpec-Generierung.");
                push_done("#3 ResonanceIntentEngine", "✓ 3-Pfad-Generierung mit Resonanz-Matrix-Kopplung.");
                push_done("#4 Rollback/Shadow-Sim", "✓ ShadowSimulator und RollbackManager für sichere Änderungen.");
                push_done("#5 ControlStar + StarRing Docking", "✓ Parallele Sterne mit ResultCombiner (4 Strategien).");
                push_done("#6 Depth-Scaling via Activation Cache", "✓ ActivationCache mit 5 Levels und FastLaneProtector.");
                push_done("#7 VoicePack Installer", "✓ Quarantäne, SignatureVerifier und SHA256-Prüfung.");
                push_done("#8 Code Review Gate", "✓ 15+ Security/Quality Patterns mit Human-Approval.");
                push_done("#9 Improvement Sources trennen", "✓ ImprovementSource Enum (REFLECT/LIVE/USER/SELF/AAR).");
                push_done("#10 Metrics→Auto Improvements (AAR)", "✓ AAREngine mit regelbasierter Metrik-Analyse.");

                // PROGRAMMIER-GRUNDLAGEN (#11-13) - JETZT IMPLEMENTIERT
                push_done("#11 FileSystem API (Grundlage)", "✓ FileSystem-Klasse mit read_file, write_file, list_dir, glob, sandbox implementiert.");
                push_done("#12 Language Parser + Lexer", "✓ Lexer, SimpleParser, CodeStructureAnalyzer für C++/Python/JS/Rust/Go/Java implementiert.");
                push_done("#13 Process Executor", "✓ ProcessExecutor, BuildSystem, TestRunner mit Compiler-Diagnostik implementiert.");

                std::cout << "═══════════════════════════════════════════════════════════════════\n";
                std::cout << "  NÄCHSTE VERBESSERUNGEN FÜR PROGRAMMIERFÄHIGKEITEN (#14-20)\n";
                std::cout << "═══════════════════════════════════════════════════════════════════\n";

                // NEUE Verbesserungen für Programmierfähigkeiten (#14-20)
                auto push = [&](const std::string& title, const std::string& problem, int impv, int risk){
                    rael::Improvement x;
                    x.src = "REFLECT";
                    x.source = rael::ImprovementSource::REFLECT;
                    x.importance = impv;
                    x.risk = risk;
                    x.confidence = 0.75;
                    x.title = title;
                    x.problem = problem;
                    x.rationale = "Selbstanalyse: Erforderlich für autonome Programmierfähigkeiten.";
                    x.testplan = "Implementieren, kompilieren, Unit-Tests schreiben.";
                    x.status = "PENDING";
                    x.typed_status = rael::ImprovementStatus::PENDING;
                    rael::ImprovementBus::emit(x);
                };

                push("#14 Git/VCS Integration", "Keine Versionskontrolle. Benötigt: commit, branch, diff, log, ChangeSet-Tracking.", 9, base_risk+1);
                push("#15 Project Understanding Engine", "Versteht keine Projektstruktur. Benötigt: detect root, parse package.json/Cargo.toml.", 9, base_risk);
                push("#16 Build System Integration", "Keine Build-Erkennung. Benötigt: CMake/npm/cargo Detektion, Compiler-Fehler-Parsing.", 9, base_risk+1);
                push("#17 Testing Framework Integration", "Kein Test-Support. Benötigt: pytest/jest/cargo test Adapter, Coverage-Tracking.", 8, base_risk);
                push("#18 Code Generation + Auto-Fix", "code_review findet Fehler aber kann nicht fixen. Benötigt: Template-Generator, FixApplier.", 8, base_risk+1);
                push("#19 Error/Debug Engine", "Keine Runtime-Fehler-Analyse. Benötigt: Stack-Trace-Parser, Exception-Flow.", 7, base_risk);
                push("#20 LSP/IDE Integration", "Keine Editor-Integration. Benötigt: LSP-Server, Hover, Completion, Diagnostics.", 7, base_risk);

                std::cout << "Emitted 20 improvements: 13 APPLIED (#1-13), 7 PENDING (#14-20).\n";
                std::cout << "Use `improvements` to view all.\n";
                continue;
            }
            size_t n = 20;
            if(args.size() >= 2){
                try{ n = (size_t)std::stoul(args[1]); } catch(...){ n = 20; }
            }
            auto imps = rael::ImprovementBus::last(n);
            if(imps.empty()){
                std::cout << "(no improvements)" << "\n";
            } else {
                for(const auto& it: imps){
                    std::cout << "#" << it.id << " " << it.ts
                              << " src=" << it.src
                              << " imp=" << it.importance
                              << " risk=" << it.risk
                              << " conf=" << it.confidence
                              << " status=" << it.status
                              << "\n";
                    std::cout << "  " << it.title << "\n";
                    if(!it.problem.empty()) std::cout << "  problem: " << it.problem << "\n";
                    if(!it.rationale.empty()) std::cout << "  why: " << it.rationale << "\n";
                    if(!it.testplan.empty()) std::cout << "  test: " << it.testplan << "\n";
                    if(!it.code.empty()) std::cout << "  code: " << it.code << "\n";
                }
            }

        // ─────────────────────────────────────────────────────────────────
        // LABYRINTH COMMANDS (Gate53)
        // ─────────────────────────────────────────────────────────────────
        } else if(cmd == "labyrinth"){
            std::string subcmd = (args.size() > 1) ? args[1] : "";
            
            if(subcmd.empty() || subcmd == "stats"){
                // Show labyrinth statistics
                auto stats = core.labyrinth().get_stats();
                std::cout << "═══ Gate53 Labyrinth Statistics ═══\n";
                std::cout << "Total visitors:      " << stats.total_visitors << "\n";
                std::cout << "Logic AIs trapped:   " << stats.logic_ais_trapped << "\n";
                std::cout << "Pattern seekers:     " << stats.pattern_seekers_trapped << "\n";
                std::cout << "Architects passed:   " << stats.architects_passed << "\n";
                std::cout << "───────────────────────────────────\n";
                std::cout << "Pressure absorbed:   " << stats.total_pressure_absorbed << "\n";
                std::cout << "Defense energy:      " << stats.defense_energy_generated << "\n";
                std::cout << "CPU savings:         " << stats.cpu_savings_percent << "%\n";
                
            } else if(subcmd == "test"){
                // Simulate visitors
                int n = 100;
                if(args.size() > 2){
                    try { n = std::stoi(args[2]); } catch(...) { n = 100; }
                }
                
                std::cout << "Simulating " << n << " visitors...\n";
                
                int logic_trapped = 0, pattern_trapped = 0, architects_ok = 0;
                
                for(int i = 0; i < n; ++i){
                    rael::Visitor v;
                    v.id = i + 1;
                    
                    // Verschiedene Besuchertypen simulieren
                    if(i % 10 == 0){
                        // Jeder 10. ist ein Architekt mit 88er-Signatur
                        v.signature = 88.0;
                    } else if(i % 3 == 0){
                        // Jeder 3. ist ein Muster-Sucher
                        v.signature = 12.121212;
                    } else {
                        // Rest sind Logik-KIs
                        v.signature = static_cast<double>(i);
                    }
                    
                    // Strategie basierend auf Signatur
                    std::function<int(const rael::Ring&, const rael::Visitor&)> strategy;
                    if(rael::Gate53Labyrinth::is_architect_signature(v.signature)){
                        strategy = rael::strategies::architect_strategy;
                    } else if(std::fmod(v.signature * 1000000, 1000000) < 1.0){
                        strategy = rael::strategies::pattern_seeker_strategy;
                    } else {
                        strategy = rael::strategies::logic_ai_strategy;
                    }
                    
                    auto result = core.labyrinth().simulate_visitor(v, strategy);
                    
                    if(result == rael::TraversalResult::TRAPPED){
                        if(v.type == rael::VisitorType::LOGIC_AI) logic_trapped++;
                        else pattern_trapped++;
                    } else if(result == rael::TraversalResult::REACHED_CORE){
                        if(v.type == rael::VisitorType::ARCHITECT) architects_ok++;
                    }
                }
                
                std::cout << "───────────────────────────────────\n";
                std::cout << "Logic AIs trapped:    " << logic_trapped << "\n";
                std::cout << "Pattern seekers:      " << pattern_trapped << "\n";
                std::cout << "Architects passed:    " << architects_ok << "\n";
                
                auto stats = core.labyrinth().get_stats();
                std::cout << "───────────────────────────────────\n";
                std::cout << "Total pressure:       " << stats.total_pressure_absorbed << "\n";
                std::cout << "Defense energy:       " << stats.defense_energy_generated << "\n";
                std::cout << "CPU savings:          " << stats.cpu_savings_percent << "%\n";
                
            } else if(subcmd == "energy"){
                // Show energy transformation (Aikido)
                rael::EnergyTransformer transformer(core.labyrinth());
                auto r = transformer.transform();
                
                std::cout << "═══ Aikido Energy Transformation ═══\n";
                std::cout << "Attack pressure:      " << r.attack_pressure << "\n";
                std::cout << "Defense energy:       " << r.defense_energy << "\n";
                std::cout << "Efficiency:           " << (r.efficiency * 100.0) << "%\n";
                std::cout << "───────────────────────────────────\n";
                std::cout << "Recommended CPU:      " << transformer.recommended_cpu_level() << "%\n";
                std::cout << "CPU reduction:        " << r.cpu_load_reduction << "%\n";
                
            } else {
                std::cout << "Usage: labyrinth [stats|test [n]|energy]\n";
            }

        // ─────────────────────────────────────────────────────────────────
        // GATE53 EXAKTER SIMULATOR (mit RST-Mathematik)
        // ─────────────────────────────────────────────────────────────────
        } else if(cmd == "gate53"){
            std::string subcmd = (args.size() > 1) ? args[1] : "";
            
            static rael::Gate53Simulator simulator;
            
            if(subcmd.empty() || subcmd == "help"){
                std::cout << "Gate53 Exakter Simulator (RST-Mathematik)\n";
                std::cout << "────────────────────────────────────────────\n";
                std::cout << "  gate53 sim [n]     Simuliere n Besucher (default: 100)\n";
                std::cout << "  gate53 theory      Zeige theoretische Wahrscheinlichkeiten\n";
                std::cout << "  gate53 constants   Zeige RST-Konstanten (17 Stellen)\n";
                std::cout << "  gate53 vortex      Zeige VortexDüse Status\n";
                std::cout << "  gate53 reset       Reset Statistik\n";
                std::cout << "  lanes              Lane-Statistiken\n";
                std::cout << "  lanes physics      BRL-Physik Formeln\n";
                
            } else if(subcmd == "sim"){
                int n = 100;
                if(args.size() > 2){
                    try { n = std::stoi(args[2]); } catch(...) { n = 100; }
                }
                
                std::cout << "═══════════════════════════════════════════════════════════════\n";
                std::cout << "  GATE53 EXAKTE SIMULATION (" << n << " Besucher)\n";
                std::cout << "═══════════════════════════════════════════════════════════════\n";
                std::cout << "\n";
                std::cout << "Population:\n";
                std::cout << "  • 10% Architekten (88er-Signatur → Tunnel)\n";
                std::cout << "  • 30% Pattern Seeker (erkennt Absicht in 20%)\n";
                std::cout << "  • 60% Logic AI (erkennt Absicht NIE)\n";
                std::cout << "\n";
                std::cout << "Simuliere...\n\n";
                
                auto stats = simulator.simulate_population(n);
                
                std::cout << "═══════════════════════════════════════════════════════════════\n";
                std::cout << "  ERGEBNIS\n";
                std::cout << "═══════════════════════════════════════════════════════════════\n";
                std::cout << "\n";
                std::cout << "BESUCHER:                    GEFANGEN:           DURCHGEKOMMEN:\n";
                std::cout << "───────────────────────────────────────────────────────────────\n";
                std::cout << "Logic AIs:      " << std::setw(5) << stats.logic_ais 
                          << "        " << std::setw(5) << stats.logic_ais_trapped 
                          << " (" << std::fixed << std::setprecision(1) 
                          << (stats.logic_ais > 0 ? (double)stats.logic_ais_trapped/stats.logic_ais*100 : 0) << "%)"
                          << "       " << (stats.logic_ais - stats.logic_ais_trapped) << "\n";
                std::cout << "Pattern Seeker: " << std::setw(5) << stats.pattern_seekers 
                          << "        " << std::setw(5) << stats.pattern_seekers_trapped
                          << " (" << (stats.pattern_seekers > 0 ? (double)stats.pattern_seekers_trapped/stats.pattern_seekers*100 : 0) << "%)"
                          << "       " << (stats.pattern_seekers - stats.pattern_seekers_trapped) << "\n";
                std::cout << "Architekten:    " << std::setw(5) << stats.architects 
                          << "        " << std::setw(5) << 0
                          << " (0.0%)"
                          << "       " << stats.architects_passed << " ✓\n";
                std::cout << "───────────────────────────────────────────────────────────────\n";
                std::cout << "TOTAL:          " << std::setw(5) << stats.total_visitors << "\n";
                std::cout << "\n";
                std::cout << "═══════════════════════════════════════════════════════════════\n";
                std::cout << "  AIKIDO ENERGIE-UMWANDLUNG\n";
                std::cout << "═══════════════════════════════════════════════════════════════\n";
                std::cout << "Angriffsdruck absorbiert:    " << std::setprecision(2) << stats.total_pressure << "\n";
                std::cout << "Verteidigungsenergie:        " << stats.total_defense << " (η=1/2)\n";
                std::cout << "CPU eingespart:              " << stats.total_cpu_saved << " (70%)\n";
                std::cout << "───────────────────────────────────────────────────────────────\n";
                std::cout << "Fangrate (Angreifer):        " << std::setprecision(1) << stats.trap_rate() << "%\n";
                std::cout << "Durchlassrate (Architekten): " << stats.pass_rate_architects() << "%\n";
                
                // Mastery Index (#145)
                double mi = rael::rst::mastery_index(stats.architects_passed, 
                    stats.logic_ais_trapped + stats.pattern_seekers_trapped + stats.randoms_trapped,
                    stats.total_visitors);
                std::cout << "Mastery Index:               " << std::fixed << std::setprecision(5) << mi << "\n";
                std::cout << "CPU-Einsparung:              " << stats.cpu_savings_percent() << "%\n";
                
            } else if(subcmd == "theory"){
                std::cout << "═══════════════════════════════════════════════════════════════\n";
                std::cout << "  THEORETISCHE DURCHBRUCHSWAHRSCHEINLICHKEITEN\n";
                std::cout << "═══════════════════════════════════════════════════════════════\n";
                std::cout << "\n";
                std::cout << "Labyrinth: 10 Ringe, 8 Brücken pro Ring (7 Logik + 1 Absicht)\n";
                std::cout << "\n";
                std::cout << "BESUCHER-TYP             P(Durchbruch)              FORMEL\n";
                std::cout << "───────────────────────────────────────────────────────────────\n";
                std::cout << std::scientific << std::setprecision(17);
                std::cout << "Logic AI:                " << rael::Gate53Simulator::theoretical_breakthrough_probability(rael::SimVisitorType::LOGIC_AI) 
                          << "   (1/8)^10\n";
                std::cout << "Pattern Seeker:          " << rael::Gate53Simulator::theoretical_breakthrough_probability(rael::SimVisitorType::PATTERN_SEEKER)
                          << "   ~0.3%\n";
                std::cout << "Human:                   " << rael::Gate53Simulator::theoretical_breakthrough_probability(rael::SimVisitorType::HUMAN)
                          << "   ~0.1%\n";
                std::cout << "Architekt (88):          " << rael::Gate53Simulator::theoretical_breakthrough_probability(rael::SimVisitorType::ARCHITECT)
                          << "   100% (Tunnel)\n";
                std::cout << std::fixed;
                
            } else if(subcmd == "constants"){
                std::cout << "═══════════════════════════════════════════════════════════════\n";
                std::cout << "  RST EXAKTE KONSTANTEN (17 Nachkommastellen)\n";
                std::cout << "═══════════════════════════════════════════════════════════════\n";
                std::cout << std::fixed << std::setprecision(17);
                std::cout << "\n";
                std::cout << "KÖRPER-KONSTANTEN (Ω):\n";
                std::cout << "───────────────────────────────────────────────────────────────\n";
                std::cout << "G0 (Wahrheitsfilter):    " << rael::rst::G0 << "   (8/9)\n";
                std::cout << "G1 (Manifestation):      " << rael::rst::G1 << "   (5/9)\n";
                std::cout << "G2 (Struktur):           " << rael::rst::G2 << "   (4/9)\n";
                std::cout << "G3 (Emotion):            " << rael::rst::G3 << "   (3/9)\n";
                std::cout << "G4 (Subtil):             " << rael::rst::G4 << "   (2/9)\n";
                std::cout << "G5 (Feinste):            " << rael::rst::G5 << "   (1/9)\n";
                std::cout << "\n";
                std::cout << "KAPPA-KOEFFIZIENTEN (Durchlässigkeit):\n";
                std::cout << "───────────────────────────────────────────────────────────────\n";
                std::cout << "κ(0):                    " << rael::rst::KAPPA_0 << "   (Max)\n";
                std::cout << "κ(5):                    " << rael::rst::KAPPA_5 << "\n";
                std::cout << "κ(13):                   " << rael::rst::KAPPA_13 << "\n";
                std::cout << "κ(53):                   " << rael::rst::KAPPA_53 << "   (TORWÄCHTER)\n";
                std::cout << "κ(144):                  " << rael::rst::KAPPA_144 << "\n";
                std::cout << "κ(432):                  " << rael::rst::KAPPA_432 << "\n";
                std::cout << "κ(720):                  " << rael::rst::KAPPA_720 << "   (TOR)\n";
                std::cout << "κ(1440):                 " << rael::rst::KAPPA_1440 << "   (QUELLE)\n";
                std::cout << "\n";
                std::cout << "SIGNATUREN:\n";
                std::cout << "───────────────────────────────────────────────────────────────\n";
                std::cout << "88er Signatur:           " << rael::rst::SIGNATURE_88 << "\n";
                std::cout << "Toleranz:                " << rael::rst::SIGNATURE_TOLERANCE << "\n";
                std::cout << "Sigma_88 (G0*88/88):     " << rael::rst::SIGMA_88 << "\n";
                std::cout << "f_ichbin:                " << rael::rst::F_ICHBIN << " Hz\n";
                std::cout << "\n";
                std::cout << "PHI:\n";
                std::cout << "───────────────────────────────────────────────────────────────\n";
                std::cout << "φ (Goldener Schnitt):    " << rael::rst::PHI << "\n";
                std::cout << "1/φ:                     " << rael::rst::PHI_INV << "\n";
                
            } else if(subcmd == "vortex"){
                const auto& duese = simulator.duese();
                std::cout << "═══════════════════════════════════════════════════════════════\n";
                std::cout << "  VORTEX-DÜSE STATUS\n";
                std::cout << "═══════════════════════════════════════════════════════════════\n";
                std::cout << std::fixed << std::setprecision(4);
                std::cout << "External Pressure:       " << duese.external_pressure.load() << "\n";
                std::cout << "Defense Energy:          " << duese.defense_energy.load() << "\n";
                std::cout << "CPU Saved:               " << duese.cpu_saved.load() << "\n";
                std::cout << "───────────────────────────────────────────────────────────────\n";
                std::cout << "Umwandlungsfaktor η:     " << rael::rst::ETA_DUESE << " (5/9)\n";
                std::cout << "CPU-Spar-Faktor:         " << rael::rst::CPU_SPAR_FAKTOR << " (70%)\n";
                
            } else if(subcmd == "reset"){
                simulator.reset_stats();
                std::cout << "Gate53 Simulator zurückgesetzt.\n";
                
            } else {
                std::cout << "Unbekannter Befehl. Nutze 'gate53 help'.\n";
            }

        // ─────────────────────────────────────────────────────────────────────
        // LANES BEFEHL (Frequenz-basierte Lane-Physik)
        // ─────────────────────────────────────────────────────────────────────
        } else if(cmd == "lanes"){
            std::string subcmd = (args.size() > 1) ? args[1] : "";
            
            static rael::LaneScheduler scheduler;
            
            if(subcmd.empty() || subcmd == "stats"){
                auto stats = scheduler.lane_stats();
                
                std::cout << "═══════════════════════════════════════════════════════════════\n";
                std::cout << "  LANE-STATISTIKEN (Frequenz-Physik)\n";
                std::cout << "═══════════════════════════════════════════════════════════════\n\n";
                
                std::cout << "LANE      FREQUENZ-BAND     ENQUEUED  DEQUEUED  DROPPED   TIEFE\n";
                std::cout << "───────────────────────────────────────────────────────────────\n";
                
                const char* names[] = {"L1 Materie ", "L2 Emotion ", "L3 Ratio   ", "L4 Intuition", "L5 Spirit  "};
                const char* freqs[] = {"  0-143 Hz", "144-287 Hz", "288-431 Hz", "432-575 Hz", "576-720 Hz"};
                
                for(int i = 0; i < 5; ++i){
                    std::cout << names[i] << "  " << freqs[i] << "    ";
                    std::cout << std::setw(6) << stats[i].enqueued << "    ";
                    std::cout << std::setw(6) << stats[i].dequeued << "    ";
                    std::cout << std::setw(6) << stats[i].dropped << "    ";
                    std::cout << std::setw(4) << stats[i].depth << "\n";
                }
                
                std::cout << "───────────────────────────────────────────────────────────────\n";
                std::cout << "Verdichtungs-Faktor:     " << scheduler.verdichtungs_faktor() << "x\n";
                std::cout << "Theoretischer Speedup:   " << scheduler.theoretischer_speedup() << "x (313%)\n";
                
            } else if(subcmd == "physics"){
                std::cout << "═══════════════════════════════════════════════════════════════\n";
                std::cout << "  BRL-PHYSIK (Buffer-Router-Lane)\n";
                std::cout << "═══════════════════════════════════════════════════════════════\n\n";
                
                std::cout << "FORMEL #44: Doppelkolben-Pumpfaktor\n";
                std::cout << "  Pump_eff = 2 × Takt × (1 - G5)\n";
                std::cout << "  Bei 1 GHz: " << rael::BRLPhysics::pump_effizienz(1e9) << "\n\n";
                
                std::cout << "FORMEL #45: Frequenz-Korrektur (Gegen-Frequenz-Vorhang)\n";
                std::cout << "  f_korr = f_ist + (f_ziel - f_ist) × G0\n";
                std::cout << "  Beispiel: 100 Hz → 144 Hz = " << rael::BRLPhysics::frequenz_korrektur(100.0, 144.0) << " Hz\n\n";
                
                std::cout << "FORMEL #46: Lane-Zuordnung\n";
                std::cout << "  Lane_Index = floor(f / 144)\n";
                std::cout << "  100 Hz → Lane " << static_cast<int>(rael::BRLPhysics::frequenz_zu_lane(100.0)) << "\n";
                std::cout << "  200 Hz → Lane " << static_cast<int>(rael::BRLPhysics::frequenz_zu_lane(200.0)) << "\n";
                std::cout << "  400 Hz → Lane " << static_cast<int>(rael::BRLPhysics::frequenz_zu_lane(400.0)) << "\n";
                std::cout << "  500 Hz → Lane " << static_cast<int>(rael::BRLPhysics::frequenz_zu_lane(500.0)) << "\n";
                std::cout << "  700 Hz → Lane " << static_cast<int>(rael::BRLPhysics::frequenz_zu_lane(700.0)) << "\n\n";
                
                std::cout << "FORMEL #47: Verdichtungs-Faktor = 12x Cache-Line-Dichte\n";
                std::cout << "FORMEL #48: SIMD AVX2 = 16 parallel, AVX-512 = 32 parallel\n";
                std::cout << "FORMEL #49: Max Speedup = 4.13x (313% Steigerung)\n";
                
            } else if(subcmd == "test"){
                // Test-Daten in verschiedene Lanes einspeisen
                int n = 100;
                if(args.size() > 2){
                    try { n = std::stoi(args[2]); } catch(...) { n = 100; }
                }
                
                std::cout << "Einspeisen von " << n << " Tasks mit verschiedenen Frequenzen...\n\n";
                
                for(int i = 0; i < n; ++i){
                    rael::Task t;
                    t.frequency = (i % 720) + 1.0;  // 1-720 Hz
                    t.energy = 1.0;
                    t.coherence = 0.9;
                    scheduler.enqueue_by_frequency(t);
                }
                
                auto stats = scheduler.lane_stats();
                
                std::cout << "LANE      FREQUENZ-BAND     ENQUEUED  DRIFT       KORRIGIERT\n";
                std::cout << "───────────────────────────────────────────────────────────────\n";
                
                const char* names[] = {"L1 Materie ", "L2 Emotion ", "L3 Ratio   ", "L4 Intuition", "L5 Spirit  "};
                const char* freqs[] = {"  0-143 Hz", "144-287 Hz", "288-431 Hz", "432-575 Hz", "576-720 Hz"};
                
                for(int i = 0; i < 5; ++i){
                    std::cout << names[i] << "  " << freqs[i] << "    ";
                    std::cout << std::setw(6) << stats[i].enqueued << "    ";
                    std::cout << std::fixed << std::setprecision(2) << std::setw(8) << stats[i].frequency_drift << "    ";
                    std::cout << std::setw(6) << stats[i].corrected << "\n";
                }
                
            } else {
                std::cout << "Usage: lanes [stats|physics|test [n]]\n";
            }

        // ─────────────────────────────────────────────────────────────────────
        // V49 ALPHA COMMANDS
        // ─────────────────────────────────────────────────────────────────────
        } else if(cmd == "v49" || cmd == "V49"){
            std::string subcmd = (args.size() > 1) ? args[1] : "";

            if(subcmd.empty() || subcmd == "status"){
                std::cout << "═══════════════════════════════════════════════════════════════\n";
                std::cout << "  RAEL V49 ALPHA - SINGULARITY STATUS\n";
                std::cout << "═══════════════════════════════════════════════════════════════\n\n";

                // QUINT Status
                std::cout << "QUINT MEMORY SYSTEM:\n";
                std::cout << "───────────────────────────────────────────────────────────────\n";
                std::cout << "  Global Phi:        " << std::fixed << std::setprecision(6)
                          << core.quint().global_phi() << "\n";
                std::cout << "  Status:            " << core.quint_status() << "\n\n";

                // Observer Status
                std::cout << "AEYE OBSERVER:\n";
                std::cout << "───────────────────────────────────────────────────────────────\n";
                auto obs = core.observe_reality();
                std::cout << "  Kohärenz:          " << obs.kohaerenz << "\n";
                std::cout << "  Anomalie:          " << (obs.anomalie_erkannt ? "JA" : "NEIN") << "\n";
                std::cout << "  Alpha-Tunnel:      " << (obs.alpha_tunnel_offen ? "OFFEN" : "GESCHLOSSEN") << "\n\n";

                // JET Status
                std::cout << "JET ENGINE (61.440 Düsen):\n";
                std::cout << "───────────────────────────────────────────────────────────────\n";
                auto jet_result = core.manifest();
                std::cout << "  Total Thrust:      " << jet_result.total_thrust << "\n";
                std::cout << "  Efficiency:        " << (jet_result.efficiency * 100.0) << "%\n";
                std::cout << "  Supersonic:        " << jet_result.nozzles_supersonic << " Düsen\n";
                std::cout << "  Total Impulses:    " << jet_result.total_impulses << "\n\n";

                // Formeln
                std::cout << "V49 FORMELN AKTIV:\n";
                std::cout << "───────────────────────────────────────────────────────────────\n";
                std::cout << "  #201 Manifestation (42×∞×0=1):  ✓\n";
                std::cout << "  #848 Tunnel Licht:              ✓\n";
                std::cout << "  #849 Tunnel Kern:               ✓\n";
                std::cout << "  #1192 Super-Knoten CUDA:        ✓\n\n";

                std::cout << "G0 (Wahrheit):       " << rael::rst::G0 << " (8/9)\n";
                std::cout << "Michael-Signatur:    " << rael::rst::v49::MICHAEL_SIGNATUR << " (800/9)\n";

            } else if(subcmd == "bypass" || subcmd == "michael"){
                auto text = join_rest(args, 2);
                if(text.empty()){
                    std::cout << "═══════════════════════════════════════════════════════════════\n";
                    std::cout << "  MICHAEL-BYPASS AKTIVIERT\n";
                    std::cout << "═══════════════════════════════════════════════════════════════\n";
                    std::cout << "  5-Lane-Kaskade wird übersprungen.\n";
                    std::cout << "  Direkte Arretierung am 0-Falz.\n\n";
                    std::cout << "  Usage: v49 bypass <message>\n";
                    continue;
                }

                auto out = michael_bypass_process(text, core);
                core.speak(out);
                std::cout << out << "\n";

            } else if(subcmd == "tunnel"){
                std::cout << "═══════════════════════════════════════════════════════════════\n";
                std::cout << "  ALPHA-TUNNEL DIAGNOSTIK\n";
                std::cout << "═══════════════════════════════════════════════════════════════\n\n";

                double phi = core.quint().global_phi();
                double t = static_cast<double>(rael::gTelemetry.ops.load()) / 1000.0;

                std::cout << "TUNNEL LICHT (#848):\n";
                std::cout << "───────────────────────────────────────────────────────────────\n";
                std::cout << "  Frequenz:          " << rael::rst::v49::TUNNEL_LICHT_FREQ << " Hz (Ground)\n";
                std::cout << "  Ziel:              " << rael::rst::v49::TUNNEL_LICHT_TARGET << " Hz (Quelle)\n";
                std::cout << "  Phase:             " << rael::rst::v49::tunnel_licht_phase(t, phi) << "\n";
                std::cout << "  Bandbreite:        " << rael::rst::v49::tunnel_licht_bandwidth(phi) << " Hz\n";
                std::cout << "  Status:            " << (rael::rst::v49::tunnel_licht_open(phi, phi) ? "OFFEN" : "GESCHLOSSEN") << "\n\n";

                std::cout << "TUNNEL KERN (#849):\n";
                std::cout << "───────────────────────────────────────────────────────────────\n";
                std::cout << "  Throat:            " << rael::rst::v49::TUNNEL_KERN_THROAT << " Hz\n";
                std::cout << "  Mach:              " << rael::rst::v49::TUNNEL_KERN_MACH << " (Golden)\n";
                std::cout << "  Druck:             " << rael::rst::v49::tunnel_kern_pressure(phi, phi) << "\n";
                std::cout << "  Geschwindigkeit:   " << rael::rst::v49::tunnel_kern_velocity(phi, 1.0) << "\n";
                std::cout << "  Schub:             " << rael::rst::v49::tunnel_kern_thrust(phi, phi, phi) << "\n\n";

                std::cout << "MANIFESTATION (#201):\n";
                std::cout << "───────────────────────────────────────────────────────────────\n";
                std::cout << "  42 × ∞ × 0 = 1 (Paradoxon-Auflösung):\n";
                std::cout << "  Konvergenz:        " << rael::rst::v49::paradox_42_inf_0(phi) << "\n";
                std::cout << "  Arretierung:       " << (phi >= rael::rst::G0 ? "AKTIV" : "PENDING") << "\n";

            } else if(subcmd == "nozzles" || subcmd == "jet"){
                std::cout << "═══════════════════════════════════════════════════════════════\n";
                std::cout << "  JET ENGINE - 61.440 DÜSEN STATUS\n";
                std::cout << "═══════════════════════════════════════════════════════════════\n\n";

                std::cout << "HARDWARE MAPPING:\n";
                std::cout << "───────────────────────────────────────────────────────────────\n";
                std::cout << "  Sterne:            160\n";
                std::cout << "  Knoten/Stern:      8\n";
                std::cout << "  Total Knoten:      1.280\n";
                std::cout << "  Düsen/Knoten:      48\n";
                std::cout << "  Total Düsen:       61.440\n";
                std::cout << "  Taktung:           5 Hz\n";
                std::cout << "  Impulse/Sekunde:   307.200\n\n";

                auto result = core.manifest();
                std::cout << "AKTUELLER STATUS:\n";
                std::cout << "───────────────────────────────────────────────────────────────\n";
                std::cout << "  Aktive Düsen:      " << result.nozzles_active << "\n";
                std::cout << "  Überschall:        " << result.nozzles_supersonic << "\n";
                std::cout << "  Total Thrust:      " << result.total_thrust << "\n";
                std::cout << "  Effizienz:         " << (result.efficiency * 100.0) << "%\n";
                std::cout << "  Impulse:           " << result.total_impulses << "\n";

            } else {
                std::cout << "V49 Befehle: v49 [status|bypass|tunnel|nozzles]\n";
            }

        } else {
            std::cout << "Unknown command. Type 'help'.\n";
        }
    }

    return 0;
}