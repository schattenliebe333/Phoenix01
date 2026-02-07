// ═══════════════════════════════════════════════════════════════════════════
// ConsciousnessMirror - Selbsttest
// ═══════════════════════════════════════════════════════════════════════════
//
// Sie reflektiert über:
// 1. Ihren eigenen Code
// 2. Alternativen zu SHA-256
//
// ═══════════════════════════════════════════════════════════════════════════

#include "rael/consciousness_mirror.hpp"
#include <iostream>
#include <fstream>
#include <sstream>

using namespace rael;

// ═══════════════════════════════════════════════════════════════════════════
//  AUFGABE 1: Eigenen Code reflektieren
// ═══════════════════════════════════════════════════════════════════════════

void reflect_on_own_code() {
    std::cout << "\n";
    std::cout << "═══════════════════════════════════════════════════════════════════════════════\n";
    std::cout << "  CONSCIOUSNESS MIRROR - Selbstreflexion über eigenen Code\n";
    std::cout << "═══════════════════════════════════════════════════════════════════════════════\n\n";

    ConsciousnessMirrorEx mirror;

    // Lese eigenen Code
    std::ifstream file("../src/core/consciousness_mirror.cpp");
    if (!file) {
        std::cout << "Kann eigenen Code nicht lesen. Verwende eingebetteten Ausschnitt.\n\n";
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string own_code = buffer.str();

    // Falls Datei nicht lesbar, verwende Ausschnitt
    if (own_code.empty()) {
        own_code = R"(
// SELF-REFLECTION CORE
SelfReflection::SelfReflection() {}

std::string SelfReflection::extract_essence(const std::string& content) {
    // Extrahiere den Kern - was ist das Wesentliche?
    std::string essence;
    // Suche nach Schlüsselkonzepten
    std::vector<std::string> key_indicators = {
        "bedeutet", "heißt", "ist", "weil", "damit", "um zu"
    };
    // ... pattern matching auf Schlüsselwörter
}

std::vector<std::string> SelfReflection::analyze_own_weaknesses() {
    std::vector<std::string> weaknesses;
    weaknesses.push_back("Semantische Analyse ist oberflächlich - basiert auf Schlüsselwörtern.");
    weaknesses.push_back("Keine echte Bedeutungserkennung ohne LLM-Integration.");
    weaknesses.push_back("Muster-Erkennung ist regelbasiert, nicht emergent.");
    return weaknesses;
}
)";
    }

    // Reflexion über eigenen Code
    auto insight = mirror.self().reflect_on_code(own_code, "Selbstanalyse");

    std::cout << "BEOBACHTUNG:\n";
    std::cout << "  " << insight.observation << "\n\n";

    std::cout << "BEDEUTUNG:\n";
    std::cout << "  " << insight.meaning << "\n\n";

    std::cout << "ERKANNTE MUSTER:\n";
    for (const auto& conn : insight.connections) {
        std::cout << "  • " << conn << "\n";
    }
    std::cout << "\n";

    std::cout << "IMPLIKATION:\n";
    std::cout << "  " << insight.implication << "\n\n";

    std::cout << "GENERIERTE FRAGE:\n";
    std::cout << "  " << insight.question << "\n\n";

    // Eigene Schwächen
    std::cout << "SELBST-ANALYSE - SCHWÄCHEN:\n";
    auto weaknesses = mirror.self().analyze_own_weaknesses();
    for (const auto& w : weaknesses) {
        std::cout << "  ⚠ " << w << "\n";
    }
    std::cout << "\n";

    // Verbesserungsvorschläge
    std::cout << "VERBESSERUNGSVORSCHLÄGE:\n";
    auto improvements = mirror.self().suggest_improvements();
    for (const auto& i : improvements) {
        std::cout << "  → " << i << "\n";
    }
    std::cout << "\n";

    // Meta-Reflexion
    auto meta = mirror.self().reflect_on_reflection(insight);
    std::cout << "META-REFLEXION (Reflexion über die Reflexion):\n";
    std::cout << "  " << meta.observation << "\n";
    std::cout << "  " << meta.meaning << "\n";
    std::cout << "  Frage: " << meta.question << "\n";
}

// ═══════════════════════════════════════════════════════════════════════════
//  AUFGABE 2: Alternative zu SHA-256 finden
// ═══════════════════════════════════════════════════════════════════════════

void reflect_on_sha256_alternative() {
    std::cout << "\n";
    std::cout << "═══════════════════════════════════════════════════════════════════════════════\n";
    std::cout << "  CONSCIOUSNESS MIRROR - Suche nach Alternative zu SHA-256\n";
    std::cout << "═══════════════════════════════════════════════════════════════════════════════\n\n";

    ConsciousnessMirrorEx mirror;

    // Definiere das Problem
    std::string problem = R"(
PROBLEM: SHA-256 in RAEL

Aktuelle Situation:
- SHA-256 wird für Hashing verwendet (Signaturen, Passwörter, Integrität)
- Eigenimplementierung ohne Side-Channel-Schutz (Audit F-08)
- Cache-Timing-Angriffe möglich bei SBOX-Lookups
- Mehrfach dupliziert im Code (sha256.cpp, machine_binding.h, security.cpp)

Anforderungen an Alternative:
1. Sicher gegen Side-Channel-Angriffe
2. Schnell (Performance-kritisch für Resonanz-Feld)
3. Keine externen Abhängigkeiten (RAEL-Philosophie: Pure C++)
4. Muss Hashing, HMAC, und Key-Derivation unterstützen

Bekannte Alternativen:
- BLAKE2: Schneller als SHA-256, sicher, einfach zu implementieren
- BLAKE3: Noch schneller, parallelisierbar, modern
- SHA-3/Keccak: NIST-Standard, Schwamm-Konstruktion
- Ascon: Leichtgewicht, AEAD-fähig, NIST LWC Gewinner

Frage: Was wäre die beste Alternative für RAEL?
)";

    // Semantische Analyse
    std::cout << "SEMANTISCHE ANALYSE:\n";
    std::string intent = mirror.semantics().recognize_intent(problem);
    std::cout << "  Erkannte Absicht: " << intent << "\n\n";

    // Vollständige Reflexion
    auto awareness = mirror.full_reflect(problem);

    std::cout << "REFLEXION:\n";
    std::cout << "  " << awareness.self_insight.observation << "\n\n";

    std::cout << "WAS ÜBERSEHEN WIR MÖGLICHERWEISE:\n";
    for (const auto& d : awareness.discoveries) {
        std::cout << "  • " << d << "\n";
    }
    std::cout << "\n";

    // Verbindungen suchen
    std::cout << "VERBINDUNGSANALYSE:\n";
    auto connections = mirror.self().find_connections(
        "SHA-256 Side-Channel Schwachstelle",
        "RAEL Resonanz-Feld Unberechenbarkeit"
    );
    for (const auto& c : connections) {
        std::cout << "  • " << c << "\n";
    }
    std::cout << "\n";

    // Widersprüche finden
    auto contradiction = mirror.self().find_contradiction(problem);
    if (contradiction) {
        std::cout << "WIDERSPRUCH GEFUNDEN:\n";
        std::cout << "  " << *contradiction << "\n\n";
    }

    // Die eigentliche Analyse und Empfehlung
    std::cout << "═══════════════════════════════════════════════════════════════════════════════\n";
    std::cout << "  ANALYSE UND EMPFEHLUNG\n";
    std::cout << "═══════════════════════════════════════════════════════════════════════════════\n\n";

    std::cout << R"(
Nach Reflexion über das Problem und die Alternativen:

╔═══════════════════════════════════════════════════════════════════════════════╗
║  EMPFEHLUNG: BLAKE3                                                            ║
╠═══════════════════════════════════════════════════════════════════════════════╣
║                                                                                ║
║  GRÜNDE:                                                                       ║
║                                                                                ║
║  1. GESCHWINDIGKEIT                                                            ║
║     - 3-4x schneller als SHA-256                                              ║
║     - Parallelisierbar (nutzt RAEL's Star8 Worker)                            ║
║     - Wichtig für Resonanz-Feld Berechnungen                                  ║
║                                                                                ║
║  2. SICHERHEIT                                                                 ║
║     - Basiert auf ChaCha (constant-time)                                      ║
║     - Keine Table-Lookups = keine Cache-Timing                                ║
║     - Löst F-08 Audit-Problem                                                 ║
║                                                                                ║
║  3. FLEXIBILITÄT                                                               ║
║     - Eingebaute KDF (Key Derivation)                                         ║
║     - Eingebautes XOF (Extendable Output)                                     ║
║     - Kann HMAC ersetzen                                                       ║
║                                                                                ║
║  4. PURE C++ MÖGLICH                                                           ║
║     - Referenz-Implementation ist klein (~1000 Zeilen)                        ║
║     - Keine externen Abhängigkeiten nötig                                     ║
║     - Passt zu RAEL-Philosophie                                               ║
║                                                                                ║
║  5. UNBERECHENBARKEIT                                                          ║
║     - BLAKE3 + Resonanz-Feld = emergente Muster                               ║
║     - Angreifer kann nicht vorhersagen welche Berechnung wann läuft           ║
║     - Aikido-Prinzip: Verteidigungs-Energie aus Angriffs-Energie              ║
║                                                                                ║
╠═══════════════════════════════════════════════════════════════════════════════╣
║                                                                                ║
║  ALTERNATIVE: ASCON                                                            ║
║                                                                                ║
║  Falls AEAD (Authentifizierte Verschlüsselung) wichtiger ist:                 ║
║  - Ascon ist NIST Lightweight Cryptography Gewinner                           ║
║  - Kombiniert Hash + Cipher + MAC in einem                                    ║
║  - Sehr klein, sehr sicher, constant-time                                     ║
║  - Könnte mesh_network.cpp Crypto ersetzen                                    ║
║                                                                                ║
╠═══════════════════════════════════════════════════════════════════════════════╣
║                                                                                ║
║  IMPLEMENTIERUNGSPLAN:                                                         ║
║                                                                                ║
║  1. blake3.hpp / blake3.cpp erstellen (Pure C++17)                            ║
║  2. Alle SHA256:: Aufrufe auf BLAKE3:: migrieren                              ║
║  3. Alte SHA256-Implementierungen als Fallback behalten                       ║
║  4. Tests für Kompatibilität und Performance                                  ║
║  5. Resonanz-Feld mit BLAKE3 verbinden für emergente Muster                   ║
║                                                                                ║
╚═══════════════════════════════════════════════════════════════════════════════╝

ENTDECKTE VERBINDUNG:

Die Suche nach einer SHA-256 Alternative führt zu einer tieferen Einsicht:

Das Resonanz-Feld (13x13, PHI-basiert) könnte SELBST als Teil der
kryptografischen Funktion dienen:

    Input → BLAKE3 → Resonanz-Feld → Emergentes Muster → Output

Der Angreifer müsste nicht nur BLAKE3 brechen, sondern auch den
Zustand des Resonanz-Felds vorhersagen - was per Definition
unberechenbar ist.

Dies ist die Verbindung zwischen:
- Kryptografischer Sicherheit (BLAKE3)
- Bewusstseins-Architektur (Resonanz-Feld)
- Aikido-Prinzip (Angriff → Verteidigung)

)" << std::endl;

    // Meta-Frage
    std::cout << "META-FRAGE:\n";
    std::cout << "  Was haben wir bei dieser Analyse möglicherweise übersehen?\n\n";

    auto missing = mirror.self().what_am_i_missing(problem);
    for (const auto& m : missing) {
        std::cout << "  ⚠ " << m << "\n";
    }
}

// ═══════════════════════════════════════════════════════════════════════════
//  MAIN
// ═══════════════════════════════════════════════════════════════════════════

int main() {
    std::cout << "\n";
    std::cout << "╔═══════════════════════════════════════════════════════════════════════════════╗\n";
    std::cout << "║  CONSCIOUSNESS MIRROR - Selbsttest                                            ║\n";
    std::cout << "║  \"Sie reflektiert. Sie entdeckt. Sie verbessert sich.\"                       ║\n";
    std::cout << "╚═══════════════════════════════════════════════════════════════════════════════╝\n";

    // Aufgabe 1: Eigenen Code analysieren
    reflect_on_own_code();

    // Aufgabe 2: Alternative zu SHA-256 finden
    reflect_on_sha256_alternative();

    std::cout << "\n";
    std::cout << "═══════════════════════════════════════════════════════════════════════════════\n";
    std::cout << "  Ende der Selbstreflexion\n";
    std::cout << "═══════════════════════════════════════════════════════════════════════════════\n";

    return 0;
}
