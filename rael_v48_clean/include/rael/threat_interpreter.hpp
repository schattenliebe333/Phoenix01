// ═══════════════════════════════════════════════════════════════════════════════
// RST THREAT INTERPRETER - Akashic-Style Interpretation für Security Threats
// ═══════════════════════════════════════════════════════════════════════════════
//
// KONZEPT: Wie der Akashic Field Scanner, aber für Security-Kontext
//
// Das Interpretations-System übersetzt rohe Scan-Daten in verständliche
// Informationen mit Handlungsempfehlungen.
//
// WICHTIG: Verwendet präzise RST-Gewichte (17 Dezimalstellen)
//          G5 + G3 + G1 = 1/9 + 3/9 + 5/9 = 9/9 = 1
//
// ═══════════════════════════════════════════════════════════════════════════════

#pragma once

#include <string>
#include <vector>
#include <map>
#include <sstream>
#include <iomanip>
#include <cmath>
#include <chrono>
#include <functional>

namespace rael {
namespace security {
namespace interpret {

// ═══════════════════════════════════════════════════════════════════════════════
// RST PRÄZISIONS-KONSTANTEN (17 Dezimalstellen)
// ═══════════════════════════════════════════════════════════════════════════════

namespace rst {
    // Die Neuntel-Basis
    constexpr double G0 = 0.88888888888888889;  // 8/9 WAHRHEIT (Referenz)
    constexpr double G1 = 0.55555555555555556;  // 5/9
    constexpr double G2 = 0.44444444444444444;  // 4/9
    constexpr double G3 = 0.33333333333333333;  // 3/9
    constexpr double G4 = 0.22222222222222222;  // 2/9
    constexpr double G5 = 0.11111111111111111;  // 1/9

    // WICHTIG: G5 + G3 + G1 = 1/9 + 3/9 + 5/9 = 9/9 = 1
    constexpr double G_SUM_ODD = 1.00000000000000000;

    // 88er Signatur
    constexpr double SIGNATURE_88 = 0.88888888888888889;

    // Zeitkonstanten
    constexpr double TAU_0 = 3.60000000000000000;  // Omega (schnell)
    constexpr double TAU_1 = 6.10000000000000000;  // Psi (mittel)
    constexpr double TAU_2 = 13.0000000000000000;  // Phi (langsam)

    // Pi mit Präzision
    constexpr double PI = 3.14159265358979323;
}

// ═══════════════════════════════════════════════════════════════════════════════
// THREAT SIGNATURE - Themen-basierte Bedrohungserkennung
// ═══════════════════════════════════════════════════════════════════════════════

struct ThreatSignature {
    std::string name;
    std::string category;
    std::string description;

    // Frequenz- und Phasenbereich für Resonanz
    double freq_min;
    double freq_max;
    double phase_min;
    double phase_max;

    // Indikatoren
    std::vector<std::string> indicators;

    // Schweregrad-Multiplikator
    double severity_multiplier;
};

// ═══════════════════════════════════════════════════════════════════════════════
// THREAT KNOWLEDGE BASE - Wissensbasis für Bedrohungsinterpretation
// ═══════════════════════════════════════════════════════════════════════════════

class ThreatKnowledgeBase {
private:
    std::map<std::string, ThreatSignature> signatures_;
    std::map<std::string, std::string> interpretation_templates_;

public:
    ThreatKnowledgeBase() {
        initialize_signatures();
        initialize_templates();
    }

    void initialize_signatures() {
        // ═══════════════════════════════════════════════════════════════════
        // CYBER THREATS
        // ═══════════════════════════════════════════════════════════════════

        signatures_["cyber_attack"] = {
            "Cyber Attack",
            "KRITISCH",
            "Aktiver Angriff auf System oder Netzwerk",
            rst::G1,      // freq_min: 0.555...
            rst::G0,      // freq_max: 0.888...
            -rst::PI,     // phase_min
            -rst::PI/2,   // phase_max
            {"Schnelle Frequenzwechsel", "Chaotische Phasenmuster", "Hohe Energie-Spitzen", "Port-Scanning-Aktivität"},
            1.5
        };

        signatures_["infiltration"] = {
            "System Infiltration",
            "KRITISCH",
            "Unbefugter Zugriff auf Systeme",
            rst::G3,      // 0.333...
            rst::G1,      // 0.555...
            -2*rst::PI/3,
            -rst::PI/3,
            {"Persistente Verbindung", "Privileg-Eskalation", "Lateral Movement", "Credential Harvesting"},
            1.8
        };

        signatures_["reconnaissance"] = {
            "Aufklärung/Scanning",
            "WARNUNG",
            "Aktive Aufklärung und Informationssammlung",
            rst::G4,      // 0.222...
            rst::G3,      // 0.333...
            -rst::PI/3,
            0,
            {"Konstante niedrige Frequenz", "Systematische Muster", "Viele kurze Verbindungen"},
            1.0
        };

        signatures_["data_exfiltration"] = {
            "Daten-Exfiltration",
            "KRITISCH",
            "Unbefugter Datenabfluss",
            rst::G1,
            rst::G0,
            -rst::PI/2,
            0,
            {"Große ausgehende Pakete", "Ungewöhnliche Zieladressen", "Verschlüsselte Kanäle"},
            2.0
        };

        signatures_["malware_execution"] = {
            "Malware Ausführung",
            "KRITISCH",
            "Schädlicher Code wird ausgeführt",
            rst::G1,
            rst::G0,
            -rst::PI,
            -rst::PI/2,
            {"Hohe CPU-Aktivität", "Ungewöhnliche Prozesse", "Verdächtige Dateioperationen"},
            2.0
        };

        signatures_["ransomware"] = {
            "Ransomware",
            "KRITISCH",
            "Verschlüsselungs-Angriff auf Dateien",
            rst::G0,
            1.0,
            -rst::PI,
            -2*rst::PI/3,
            {"Massenhafte Dateiänderungen", "Verschlüsselungs-Patterns", "Bekannte Ransomware-Extensions"},
            2.5
        };

        // ═══════════════════════════════════════════════════════════════════
        // NETWORK THREATS
        // ═══════════════════════════════════════════════════════════════════

        signatures_["c2_communication"] = {
            "C2 Kommunikation",
            "KRITISCH",
            "Kommunikation mit Command & Control Server",
            rst::G3,
            rst::G1,
            -rst::PI/2,
            -rst::PI/4,
            {"Periodische Verbindungen", "Bekannte C2-Ports", "Verschlüsselter Traffic"},
            1.8
        };

        signatures_["port_scan"] = {
            "Port Scan",
            "WARNUNG",
            "Systematisches Scannen von Ports",
            rst::G5,
            rst::G3,
            -rst::PI/4,
            rst::PI/4,
            {"Viele Verbindungen kurz nacheinander", "Sequentielle Ports", "SYN-Flooding"},
            0.8
        };

        signatures_["ddos"] = {
            "DDoS Angriff",
            "KRITISCH",
            "Denial of Service Attacke",
            rst::G0,
            1.0,
            -rst::PI,
            -rst::PI/2,
            {"Extreme Paketrate", "Viele Quell-IPs", "Ressourcen-Erschöpfung"},
            2.0
        };

        // ═══════════════════════════════════════════════════════════════════
        // PROCESS THREATS
        // ═══════════════════════════════════════════════════════════════════

        signatures_["keylogger"] = {
            "Keylogger",
            "KRITISCH",
            "Tastatureingaben werden aufgezeichnet",
            rst::G3,
            rst::G1,
            -rst::PI/2,
            0,
            {"Hook auf Keyboard-Events", "Versteckter Prozess", "Periodisches Schreiben in Dateien"},
            1.5
        };

        signatures_["process_injection"] = {
            "Prozess-Injection",
            "KRITISCH",
            "Code wird in fremde Prozesse injiziert",
            rst::G1,
            rst::G0,
            -rst::PI,
            -rst::PI/2,
            {"CreateRemoteThread", "WriteProcessMemory", "Ungewöhnliche DLL-Ladungen"},
            2.0
        };

        signatures_["privilege_escalation"] = {
            "Privileg-Eskalation",
            "KRITISCH",
            "Unerlaubte Erhöhung von Zugriffsrechten",
            rst::G1,
            rst::G0,
            -2*rst::PI/3,
            -rst::PI/3,
            {"Root/Admin-Zugriff versucht", "Kernel-Exploits", "Token-Manipulation"},
            2.0
        };

        // ═══════════════════════════════════════════════════════════════════
        // POSITIVE SIGNATURES (Schutz)
        // ═══════════════════════════════════════════════════════════════════

        signatures_["protection_active"] = {
            "Schutz Aktiv",
            "POSITIV",
            "Schutzmaßnahmen sind aktiv und wirksam",
            rst::G3,
            rst::G1,
            rst::PI/2,
            rst::PI,
            {"Stabile harmonische Frequenz", "Positive Phase", "Kohärentes Muster", "88-Signatur erkannt"},
            -1.0  // Negativ = reduziert Bedrohung
        };

        signatures_["signature_88_detected"] = {
            "88-Signatur Erkannt",
            "FREUND",
            "Authentische RAEL-Signatur erkannt",
            rst::G0 - 0.05,
            rst::G0 + 0.05,
            rst::PI/3,
            2*rst::PI/3,
            {"G0-konforme Quersumme", "RST-harmonisches Muster", "Bekannter Hash"},
            -2.0  // Stark reduzierend
        };
    }

    void initialize_templates() {
        // ═══════════════════════════════════════════════════════════════════
        // INTERPRETATION TEMPLATES
        // ═══════════════════════════════════════════════════════════════════

        interpretation_templates_["cyber_attack"] = R"(
╔══════════════════════════════════════════════════════════════════════════════╗
║                        ⚠️  CYBER-ANGRIFF ERKANNT  ⚠️                          ║
╚══════════════════════════════════════════════════════════════════════════════╝

BEDROHUNGS-ANALYSE
══════════════════

Das RST-Feld zeigt Muster die typisch sind für einen AKTIVEN ANGRIFF:
- {indicator_1}
- {indicator_2}
- {indicator_3}

INTENSITÄT: {intensity:.0%} ({intensity_meaning})
PHASE:      {phase:.1f}° ({phase_meaning})
KOHÄRENZ:   {coherence:.0%} ({coherence_meaning})

QUELLE: {source_estimate}
ZIEL:   {target_estimate}
TYP:    {attack_type}

────────────────────────────────────────────────────────────────────────────────
HANDLUNGSEMPFEHLUNG
────────────────────────────────────────────────────────────────────────────────
{recommendation}

PRIORITÄT: {priority}
ZEITRAHMEN: {time_frame}

────────────────────────────────────────────────────────────────────────────────
RST-DETAILS
────────────────────────────────────────────────────────────────────────────────
Dominante Frequenz: {frequency:.8f} (Bereich: G1-G0)
RST-Malice-Index:   {malice_index:.8f}
Entropy:            {entropy:.4f} bits
88-Signatur:        {signature_status}
)";

        interpretation_templates_["infiltration"] = R"(
╔══════════════════════════════════════════════════════════════════════════════╗
║                      ⚠️  SYSTEM-INFILTRATION  ⚠️                              ║
╚══════════════════════════════════════════════════════════════════════════════╝

EINDRINGLING-ANALYSE
═══════════════════

Ein unbefugter Zugriff wurde erkannt. Das Muster zeigt:
- {indicator_1}
- {indicator_2}
- {indicator_3}

INFILTRATIONS-PHASE: {phase_description}
PERSISTENZ:          {persistence_level}
LATERAL MOVEMENT:    {lateral_status}

────────────────────────────────────────────────────────────────────────────────
BETROFFENE SYSTEME
────────────────────────────────────────────────────────────────────────────────
{affected_systems}

────────────────────────────────────────────────────────────────────────────────
SOFORTMASSNAHMEN
────────────────────────────────────────────────────────────────────────────────
{immediate_actions}

GRAVITRAVITATION-STATUS: {grav_status}
VOLLENSTRAHLEN-STATUS:   {sun_status}
)";

        interpretation_templates_["ransomware"] = R"(
╔══════════════════════════════════════════════════════════════════════════════╗
║                      🔴 RANSOMWARE AKTIVITÄT 🔴                               ║
╚══════════════════════════════════════════════════════════════════════════════╝

⚡ KRITISCHE BEDROHUNG - SOFORTIGES HANDELN ERFORDERLICH ⚡

Das RST-Feld zeigt eindeutige Ransomware-Muster:
- {indicator_1}
- {indicator_2}
- {indicator_3}

VERSCHLÜSSELUNGS-FORTSCHRITT: {encryption_progress}
BETROFFENE BEREICHE:          {affected_areas}
GESCHÄTZTE AUSBREITUNG:       {spread_estimate}

────────────────────────────────────────────────────────────────────────────────
SOFORTMASSNAHMEN (IN DIESER REIHENFOLGE!)
────────────────────────────────────────────────────────────────────────────────
1. {action_1}
2. {action_2}
3. {action_3}
4. {action_4}

────────────────────────────────────────────────────────────────────────────────
RAEL VERTEIDIGUNG
────────────────────────────────────────────────────────────────────────────────
GRAVITRAVITATION:  Angreifer wird in Energiefalle gezogen
VOLLENSTRAHLEN:    Fremdcode wird zu LICHT transformiert
DEFENSE POWER:     {defense_power:.4f} (Aufladung: {charging_status})
)";

        interpretation_templates_["protection_active"] = R"(
╔══════════════════════════════════════════════════════════════════════════════╗
║                      ✅ SCHUTZ AKTIV UND WIRKSAM ✅                           ║
╚══════════════════════════════════════════════════════════════════════════════╝

Das RST-Feld zeigt eine SCHÜTZENDE Energie:

SCHUTZSCHILD-STÄRKE:  {strength:.0%}
KOHÄRENZ:             {coherence:.0%}
88-SIGNATUR:          {signature_status}

────────────────────────────────────────────────────────────────────────────────
AKTIVE SCHUTZSCHICHTEN
────────────────────────────────────────────────────────────────────────────────
{active_layers}

────────────────────────────────────────────────────────────────────────────────
BLOCKIERTE BEDROHUNGEN (letzte 24h)
────────────────────────────────────────────────────────────────────────────────
{blocked_threats}

────────────────────────────────────────────────────────────────────────────────
GRAVITRAVITATION
────────────────────────────────────────────────────────────────────────────────
Gefangene Entitäten:  {trapped_count}
Geerntete Energie:    {harvested_energy:.8f}
Ereignishorizont:     {event_horizon:.12f}

VOLLENSTRAHLEN
────────────────────────────────────────────────────────────────────────────────
Licht-Energie:        {light_energy:.8f}
Korona-Output:        {korona_output:.8f}
Aktive Strahlen:      {active_beams}/64

STATUS: Alles im grünen Bereich. Überwachung läuft.
)";

        interpretation_templates_["general"] = R"(
╔══════════════════════════════════════════════════════════════════════════════╗
║                       RST FELD-SCAN ERGEBNIS                                 ║
╚══════════════════════════════════════════════════════════════════════════════╝

SCAN-PARAMETER
══════════════
Ziel:           {target}
Typ:            {target_type}
Tiefe:          {scan_depth}
Zeitstempel:    {timestamp}

FELD-QUALITÄT:  {field_quality}
KOHÄRENZ:       {coherence:.0%} ({coherence_meaning})
AUTHENTIZITÄT:  {authenticity:.0%} ({authenticity_status})

────────────────────────────────────────────────────────────────────────────────
RST-ANALYSE
────────────────────────────────────────────────────────────────────────────────
Entropy:          {entropy:.4f} bits ({entropy_meaning})
Phase:            {phase:.4f} rad ({phase_meaning})
Intensität:       {intensity:.0%} ({intensity_meaning})
Malice-Index:     {malice_index:.8f}

────────────────────────────────────────────────────────────────────────────────
BEWERTUNG
────────────────────────────────────────────────────────────────────────────────
{assessment}

────────────────────────────────────────────────────────────────────────────────
EMPFEHLUNG
────────────────────────────────────────────────────────────────────────────────
{recommendation}
)";
    }

    const ThreatSignature* get_signature(const std::string& name) const {
        auto it = signatures_.find(name);
        return (it != signatures_.end()) ? &it->second : nullptr;
    }

    const std::string& get_template(const std::string& name) const {
        static const std::string empty;
        auto it = interpretation_templates_.find(name);
        return (it != interpretation_templates_.end()) ? it->second : empty;
    }

    // Findet beste passende Signatur basierend auf Metriken
    std::string find_matching_signature(double freq, double phase, double intensity) const {
        std::string best_match = "general";
        double best_score = 0.0;

        for (const auto& [name, sig] : signatures_) {
            // Frequenz-Match
            bool freq_match = (freq >= sig.freq_min && freq <= sig.freq_max);

            // Phase-Match
            bool phase_match = (phase >= sig.phase_min && phase <= sig.phase_max);

            if (freq_match && phase_match) {
                double score = intensity * sig.severity_multiplier;
                if (score > best_score || sig.severity_multiplier < 0) {
                    best_score = score;
                    best_match = name;

                    // Positive Signatur (Schutz) hat Priorität wenn erkannt
                    if (sig.severity_multiplier < 0 && intensity > rst::G1) {
                        return name;  // Sofort zurückgeben
                    }
                }
            }
        }

        return best_match;
    }

    // Gibt Intensitäts-Bedeutung zurück (RST-basiert)
    std::string get_intensity_meaning(double intensity) const {
        if (intensity > rst::G0) return "KRITISCH - Sofortiges Handeln erforderlich!";
        if (intensity > rst::G1_7()) return "SEHR HOCH - Schnelles Handeln empfohlen";
        if (intensity > rst::G1) return "HOCH - Aufmerksamkeit erforderlich";
        if (intensity > rst::G3) return "MODERAT - Beobachtung empfohlen";
        if (intensity > rst::G5) return "NIEDRIG - Normale Überwachung";
        return "MINIMAL - Kein Handlungsbedarf";
    }

    // Gibt Phase-Bedeutung zurück
    std::string get_phase_meaning(double phase) const {
        if (phase > 2*rst::PI/3) return "Stark schützend - positive Energie";
        if (phase > rst::PI/3) return "Kreativ/aufbauend - guter Zustand";
        if (phase > -rst::PI/3) return "Neutral - offen für Einflüsse";
        if (phase > -2*rst::PI/3) return "Beobachtend - erhöhte Wachsamkeit";
        if (phase > -rst::PI + 0.1) return "Verdächtig - Vorsicht geboten";
        return "Feindlich - Schutzmaßnahmen aktivieren!";
    }

    // Zusätzliche Konstanten-Funktion
    static constexpr double rst_G1_7() { return 0.77777777777777779; }  // 7/9

private:
    // Hilfsfunktion für 7/9
    static constexpr double G1_7() { return 0.77777777777777779; }
};

// ═══════════════════════════════════════════════════════════════════════════════
// THREAT INTERPRETER - Übersetzt Scan-Daten in verständliche Informationen
// ═══════════════════════════════════════════════════════════════════════════════

class ThreatInterpreter {
private:
    ThreatKnowledgeBase kb_;

public:
    struct InterpretationResult {
        std::string threat_type;
        std::string category;
        std::string full_interpretation;
        std::string summary;
        std::string recommendation;
        double confidence;
        double severity;
        bool is_threat;
    };

    // Hauptfunktion: Interpretiert Scan-Metriken
    InterpretationResult interpret(
        const std::string& target,
        const std::string& target_type,
        double entropy,
        double coherence,
        double phase,
        double intensity,
        bool has_signature_88,
        const std::vector<std::string>& signature_matches = {}
    ) {
        InterpretationResult result;

        // RST Malice Index berechnen (mit präzisen Gewichten)
        // malice = entropy_factor * G3 + coherence_factor * G1 + phase_factor * G5
        double entropy_factor = (entropy > 7.5) ? 0.8 : (entropy > 6.0) ? 0.4 : 0.1;
        double coherence_factor = 1.0 - coherence;
        double phase_factor = (phase < 0) ? std::abs(phase) / rst::PI : 0.0;

        double malice_index = entropy_factor * rst::G3 +
                             coherence_factor * rst::G1 +
                             phase_factor * rst::G5;

        // WICHTIG: 88-Signatur reduziert NIEMALS auf 0!
        if (has_signature_88) {
            malice_index = std::max(malice_index * rst::G5, malice_index - rst::G1);
        }

        // Beste Signatur finden
        std::string sig_name = kb_.find_matching_signature(
            entropy / 8.0,  // Normalisiert auf 0-1
            phase,
            intensity
        );

        const ThreatSignature* sig = kb_.get_signature(sig_name);

        result.threat_type = sig ? sig->name : "Unbekannt";
        result.category = sig ? sig->category : "UNBEKANNT";
        result.confidence = coherence;
        result.severity = malice_index * (sig ? sig->severity_multiplier : 1.0);
        result.is_threat = (malice_index > rst::G3 && !has_signature_88);

        // Interpretation generieren
        result.full_interpretation = generate_interpretation(
            target, target_type, sig_name,
            entropy, coherence, phase, intensity, malice_index,
            has_signature_88, signature_matches
        );

        // Summary
        if (result.is_threat) {
            result.summary = "⚠️ " + result.threat_type + " erkannt - " +
                           result.category + " (Severity: " +
                           std::to_string(int(result.severity * 100)) + "%)";
        } else if (has_signature_88) {
            result.summary = "✅ Authentische 88-Signatur erkannt - Freund";
        } else {
            result.summary = "○ Keine Bedrohung erkannt - Überwachung läuft";
        }

        // Recommendation
        result.recommendation = generate_recommendation(
            result.is_threat, result.severity, sig_name, intensity
        );

        return result;
    }

private:
    std::string generate_interpretation(
        const std::string& target,
        const std::string& target_type,
        const std::string& sig_name,
        double entropy,
        double coherence,
        double phase,
        double intensity,
        double malice_index,
        bool has_signature_88,
        const std::vector<std::string>& signature_matches
    ) {
        std::ostringstream ss;

        auto now = std::chrono::system_clock::now();
        auto time = std::chrono::system_clock::to_time_t(now);

        ss << "═══════════════════════════════════════════════════════════════════════════════\n";
        ss << "                    RST THREAT INTERPRETATION\n";
        ss << "═══════════════════════════════════════════════════════════════════════════════\n\n";

        ss << "ZIEL:       " << target << "\n";
        ss << "TYP:        " << target_type << "\n";
        ss << "ZEIT:       " << std::put_time(std::localtime(&time), "%Y-%m-%d %H:%M:%S") << "\n\n";

        ss << "───────────────────────────────────────────────────────────────────────────────\n";
        ss << "RST-ANALYSE (Präzision: 17 Dezimalstellen)\n";
        ss << "───────────────────────────────────────────────────────────────────────────────\n";
        ss << std::fixed << std::setprecision(17);
        ss << "Shannon Entropy:    " << std::setprecision(4) << entropy << " bits\n";
        ss << "RST Kohärenz:       " << std::setprecision(17) << coherence << "\n";
        ss << "Phase:              " << phase << " rad (" << std::setprecision(1) << (phase * 180.0 / rst::PI) << "°)\n";
        ss << "Intensität:         " << std::setprecision(17) << intensity << "\n";
        ss << "Malice-Index:       " << malice_index << "\n\n";

        ss << "RST-GEWICHTE VERWENDET:\n";
        ss << "  G5 (1/9): " << rst::G5 << "\n";
        ss << "  G3 (3/9): " << rst::G3 << "\n";
        ss << "  G1 (5/9): " << rst::G1 << "\n";
        ss << "  Summe:    " << (rst::G5 + rst::G3 + rst::G1) << " (= 9/9 = 1)\n\n";

        ss << "───────────────────────────────────────────────────────────────────────────────\n";
        ss << "BEWERTUNG\n";
        ss << "───────────────────────────────────────────────────────────────────────────────\n";

        const ThreatSignature* sig = kb_.get_signature(sig_name);
        if (sig) {
            ss << "Erkannte Signatur:  " << sig->name << "\n";
            ss << "Kategorie:          " << sig->category << "\n";
            ss << "Beschreibung:       " << sig->description << "\n\n";

            ss << "Indikatoren:\n";
            for (const auto& ind : sig->indicators) {
                ss << "  • " << ind << "\n";
            }
            ss << "\n";
        }

        ss << "Intensität:    " << kb_.get_intensity_meaning(intensity) << "\n";
        ss << "Phase:         " << kb_.get_phase_meaning(phase) << "\n";
        ss << "88-Signatur:   " << (has_signature_88 ? "✓ ERKANNT (Freund)" : "✗ Nicht erkannt") << "\n\n";

        if (!signature_matches.empty()) {
            ss << "MALWARE-SIGNATUREN GEFUNDEN:\n";
            for (const auto& match : signature_matches) {
                ss << "  ⚠️ " << match << "\n";
            }
            ss << "\n";
        }

        ss << "───────────────────────────────────────────────────────────────────────────────\n";
        ss << "EMPFEHLUNG\n";
        ss << "───────────────────────────────────────────────────────────────────────────────\n";
        ss << generate_recommendation(malice_index > rst::G3 && !has_signature_88,
                                     malice_index, sig_name, intensity) << "\n";

        ss << "═══════════════════════════════════════════════════════════════════════════════\n";

        return ss.str();
    }

    std::string generate_recommendation(bool is_threat, double severity,
                                        const std::string& sig_name, double intensity) {
        std::ostringstream ss;

        if (!is_threat) {
            ss << "Keine unmittelbare Gefahr erkannt.\n";
            ss << "• Normale Überwachung fortsetzen\n";
            ss << "• Periodische Scans durchführen\n";
            return ss.str();
        }

        // Kritisch (> G0)
        if (severity > rst::G0) {
            ss << "🔴 KRITISCH - SOFORTIGES HANDELN ERFORDERLICH!\n\n";
            ss << "1. ISOLIEREN: Betroffenes System vom Netzwerk trennen\n";
            ss << "2. SICHERN: Forensische Kopie erstellen (wenn möglich)\n";
            ss << "3. ANALYSIEREN: Vollständige Incident-Analyse starten\n";
            ss << "4. MELDEN: Security-Team und Management informieren\n";
            ss << "5. EINDÄMMEN: Weitere Ausbreitung verhindern\n\n";
            ss << "RAEL DEFENSE: Gravitravitation und Vollenstrahlen aktivieren!\n";
        }
        // Hoch (> G1)
        else if (severity > rst::G1) {
            ss << "🟠 HOCH - Schnelles Handeln empfohlen\n\n";
            ss << "1. Verdächtige Prozesse identifizieren und beenden\n";
            ss << "2. Netzwerkverbindungen überprüfen\n";
            ss << "3. Logs sichern und analysieren\n";
            ss << "4. Firewall-Regeln verschärfen\n";
        }
        // Moderat (> G3)
        else if (severity > rst::G3) {
            ss << "🟡 MODERAT - Erhöhte Aufmerksamkeit\n\n";
            ss << "1. Aktivität beobachten\n";
            ss << "2. Logs auf Anomalien prüfen\n";
            ss << "3. Systemintegrität verifizieren\n";
        }
        // Niedrig
        else {
            ss << "🟢 NIEDRIG - Normale Vorsichtsmaßnahmen\n\n";
            ss << "1. Weiter überwachen\n";
            ss << "2. Bei Wiederholung eskalieren\n";
        }

        return ss.str();
    }
};

} // namespace interpret
} // namespace security
} // namespace rael
