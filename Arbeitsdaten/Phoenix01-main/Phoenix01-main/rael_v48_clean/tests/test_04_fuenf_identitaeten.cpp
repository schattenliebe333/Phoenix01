// ═══════════════════════════════════════════════════════════════════════════
// TEST 04: DIE 5 IDENTITÄTEN IM ZUSAMMENSPIEL
// MICHAEL → ITH'RA → RAEL → KAEL → PHOENIX
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <iomanip>
#include <vector>
#include <random>
#include <chrono>
#include <cmath>
#include <queue>
#include <functional>

namespace rael {
    constexpr double G0 = 0.888888888888889;
    constexpr double G5 = 0.111111111111111;
    constexpr double SIGNATURE_88 = 88.0;
    constexpr double PHI_HEART = 112.64;
    constexpr double PHI = 1.61803398874989;
    
    constexpr double FREQ_MICHAEL = 1440.0;
    constexpr double FREQ_ITHRA = 720.0;
    constexpr double FREQ_RAEL = 432.0;
    constexpr double FREQ_KAEL = 53.0;
    constexpr double FREQ_PHOENIX = 5.0;
    
    constexpr double K_A = G0 / 9.0;
}

using namespace rael;

enum class Phase {
    INTENT,      // MICHAEL
    FORMUNG,     // ITH'RA
    TRANSFORM,   // RAEL
    PRUEFUNG,    // KAEL
    MANIFEST     // PHOENIX
};

struct Impuls {
    int id;
    double energie;
    double signatur;
    double kohaerenz;     // K1 - Kohärenz des Willens
    double ueberzeugung;  // K2 - Tiefe der Überzeugung
    double integritaet;   // K3 - Integrität des Handelns
    Phase aktuelle_phase;
    std::vector<double> phasen_energien;
    bool abgelehnt;
    double a2;  // Bewusstseins-Energie
};

struct IdentitaetKnoten {
    std::string name;
    double frequenz;
    double resonanz;
    int verarbeitet;
    int weitergeleitet;
    int abgelehnt;
    double akkumulierte_energie;
    std::queue<Impuls*> warteschlange;
};

class FuenfIdentitaetenSimulation {
private:
    std::vector<IdentitaetKnoten> identitaeten;
    std::vector<Impuls> impulse;
    std::mt19937 rng;
    
    double zeit = 0.0;
    double dt = 0.001;
    
    // Statistiken
    int total_manifest = 0;
    int total_abgelehnt = 0;
    double total_energie_manifest = 0.0;
    double total_a2_manifest = 0.0;
    
public:
    FuenfIdentitaetenSimulation() : rng(std::random_device{}()) {
        initialisiere_identitaeten();
    }
    
    void initialisiere_identitaeten() {
        identitaeten.resize(5);
        
        identitaeten[0] = {"MICHAEL", FREQ_MICHAEL, 1.0, 0, 0, 0, 0.0, {}};
        identitaeten[1] = {"ITH'RA",  FREQ_ITHRA,   1.0, 0, 0, 0, 0.0, {}};
        identitaeten[2] = {"RAEL",    FREQ_RAEL,    1.0, 0, 0, 0, 0.0, {}};
        identitaeten[3] = {"KAEL",    FREQ_KAEL,    1.0, 0, 0, 0, 0.0, {}};
        identitaeten[4] = {"PHOENIX", FREQ_PHOENIX, 1.0, 0, 0, 0, 0.0, {}};
        
        std::cout << "  ✓ 5 Identitäten initialisiert:\n";
        for (auto& id : identitaeten) {
            std::cout << "    " << id.name << " @ " << id.frequenz << " Hz\n";
        }
    }
    
    void generiere_impulse(int n) {
        impulse.resize(n);
        
        std::uniform_real_distribution<double> dist(0.0, 1.0);
        std::uniform_real_distribution<double> dist_e(10.0, 200.0);
        
        for (int i = 0; i < n; ++i) {
            double k1 = dist(rng);  // Kohärenz
            double k2 = dist(rng);  // Überzeugung
            double k3 = dist(rng);  // Integrität
            
            // 30% haben hohe Bewusstseins-Werte
            if (dist(rng) < 0.3) {
                k1 = 0.8 + dist(rng) * 0.2;
                k2 = 0.8 + dist(rng) * 0.2;
                k3 = 0.8 + dist(rng) * 0.2;
            }
            
            double summe = k1 + k2 + k3;
            double a2 = K_A * summe * summe;
            
            // Signatur korreliert mit Bewusstseins-Energie
            double sig = (a2 > G0 * 0.5) ? SIGNATURE_88 : dist(rng) * 100.0;
            
            impulse[i] = {
                i,
                dist_e(rng),
                sig,
                k1, k2, k3,
                Phase::INTENT,
                {},
                false,
                a2
            };
        }
        
        std::cout << "  ✓ " << n << " Impulse generiert\n";
    }
    
    void verarbeite_michael(Impuls& imp) {
        // MICHAEL setzt den Intent
        // Frequenz: 1440 Hz - Höchste Schwingung
        
        identitaeten[0].verarbeitet++;
        
        // Intent-Verstärkung basierend auf Kohärenz
        double intent_staerke = imp.kohaerenz * FREQ_MICHAEL / 1000.0;
        imp.energie *= (1.0 + intent_staerke * G0);
        imp.phasen_energien.push_back(imp.energie);
        
        identitaeten[0].akkumulierte_energie += imp.energie;
        
        // Weiterleitung zu ITH'RA
        imp.aktuelle_phase = Phase::FORMUNG;
        identitaeten[0].weitergeleitet++;
    }
    
    void verarbeite_ithra(Impuls& imp) {
        // ITH'RA formt den Impuls
        // Frequenz: 720 Hz - Reflexion und Formgebung
        
        identitaeten[1].verarbeitet++;
        
        // Formung: Überzeugung beeinflusst die Struktur
        double form_qualitaet = imp.ueberzeugung * std::sin(FREQ_ITHRA * zeit);
        imp.energie *= (1.0 + std::abs(form_qualitaet) * G5);
        imp.phasen_energien.push_back(imp.energie);
        
        identitaeten[1].akkumulierte_energie += imp.energie;
        
        // Weiterleitung zu RAEL
        imp.aktuelle_phase = Phase::TRANSFORM;
        identitaeten[1].weitergeleitet++;
    }
    
    void verarbeite_rael(Impuls& imp) {
        // RAEL transformiert
        // Frequenz: 432 Hz - Die Brücke
        
        identitaeten[2].verarbeitet++;
        
        // Transformation: Integrität bestimmt Reinheit
        double transform_kraft = imp.integritaet * G0;
        
        // Signatur-Angleichung
        double sig_diff = std::abs(imp.signatur - SIGNATURE_88);
        double angleichung = std::exp(-sig_diff / SIGNATURE_88) * transform_kraft;
        
        if (angleichung > 0.5) {
            imp.signatur = imp.signatur * (1 - angleichung) + SIGNATURE_88 * angleichung;
        }
        
        imp.energie *= (1.0 + transform_kraft * G5);
        imp.phasen_energien.push_back(imp.energie);
        
        identitaeten[2].akkumulierte_energie += imp.energie;
        
        // Weiterleitung zu KAEL
        imp.aktuelle_phase = Phase::PRUEFUNG;
        identitaeten[2].weitergeleitet++;
    }
    
    void verarbeite_kael(Impuls& imp) {
        // KAEL prüft
        // Frequenz: 53 Hz - Der Wächter
        // 53 + 35 = 88 (gespiegelte Signatur)
        
        identitaeten[3].verarbeitet++;
        
        // Wächter-Prüfung: Signatur UND Energie UND a²
        bool signatur_ok = std::abs(imp.signatur - SIGNATURE_88) < 10.0;
        bool energie_ok = imp.energie <= PHI_HEART * 2;
        bool bewusstsein_ok = imp.a2 > G0 * 0.3;  // Mindestens 30% Bewusstseins-Energie
        
        // Resonanz-Check: 53 Hz muss mit den K-Werten resonieren
        double resonanz = (imp.kohaerenz + imp.ueberzeugung + imp.integritaet) / 3.0;
        bool resonanz_ok = resonanz > 0.5;
        
        imp.phasen_energien.push_back(imp.energie);
        identitaeten[3].akkumulierte_energie += imp.energie;
        
        if ((signatur_ok && energie_ok) || (bewusstsein_ok && resonanz_ok)) {
            // PASSIERT → PHOENIX
            imp.aktuelle_phase = Phase::MANIFEST;
            identitaeten[3].weitergeleitet++;
        } else {
            // ABGELEHNT → Transmutation
            imp.abgelehnt = true;
            identitaeten[3].abgelehnt++;
            total_abgelehnt++;
        }
    }
    
    void verarbeite_phoenix(Impuls& imp) {
        // PHOENIX manifestiert
        // Frequenz: 5 Hz - Erdung in der Materie
        
        identitaeten[4].verarbeitet++;
        
        // Manifestation: Die gesamte Energie wird realisiert
        double manifest_energie = imp.energie * G0 * (1.0 + imp.a2);
        
        imp.phasen_energien.push_back(manifest_energie);
        identitaeten[4].akkumulierte_energie += manifest_energie;
        
        identitaeten[4].weitergeleitet++;
        total_manifest++;
        total_energie_manifest += manifest_energie;
        total_a2_manifest += imp.a2;
    }
    
    void verarbeite_impuls(Impuls& imp) {
        switch (imp.aktuelle_phase) {
            case Phase::INTENT:   verarbeite_michael(imp); break;
            case Phase::FORMUNG:  verarbeite_ithra(imp); break;
            case Phase::TRANSFORM:verarbeite_rael(imp); break;
            case Phase::PRUEFUNG: verarbeite_kael(imp); break;
            case Phase::MANIFEST: verarbeite_phoenix(imp); break;
        }
    }
    
    void simuliere() {
        std::cout << "\n  Verarbeite " << impulse.size() << " Impulse durch die Kette...\n";
        
        auto start = std::chrono::high_resolution_clock::now();
        
        for (auto& imp : impulse) {
            // Durchlaufe alle 5 Phasen (oder bis Ablehnung)
            while (imp.aktuelle_phase != Phase::MANIFEST && !imp.abgelehnt) {
                verarbeite_impuls(imp);
                zeit += dt;
            }
            
            if (imp.aktuelle_phase == Phase::MANIFEST && !imp.abgelehnt) {
                verarbeite_impuls(imp);  // Letzte Phase: PHOENIX
            }
        }
        
        auto end = std::chrono::high_resolution_clock::now();
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
        
        statistiken(ms);
    }
    
    void statistiken(long long ms) {
        std::cout << std::fixed << std::setprecision(2);
        std::cout << "\n═══════════════════════════════════════════════════════════════════\n";
        std::cout << "5-IDENTITÄTEN-ERGEBNIS\n";
        std::cout << "═══════════════════════════════════════════════════════════════════\n";
        
        std::cout << "  Zeit:              " << ms << " ms\n";
        std::cout << "  Impulse total:     " << impulse.size() << "\n";
        std::cout << "  Manifestiert:      " << total_manifest << " (" 
                  << (100.0 * total_manifest / impulse.size()) << "%)\n";
        std::cout << "  Abgelehnt (KAEL):  " << total_abgelehnt << " (" 
                  << (100.0 * total_abgelehnt / impulse.size()) << "%)\n";
        std::cout << "  Manifest-Energie:  " << total_energie_manifest << "\n";
        std::cout << "  Mittlere a²:       " << (total_manifest > 0 ? total_a2_manifest / total_manifest : 0) << "\n";
        
        std::cout << "\nIDENTITÄTEN-STATISTIK:\n";
        std::cout << "  Name    | Freq    | Verarbeitet | Weiter | Abgelehnt | Energie\n";
        std::cout << "  ────────┼─────────┼─────────────┼────────┼───────────┼──────────\n";
        
        for (auto& id : identitaeten) {
            std::cout << "  " << std::setw(7) << id.name
                      << " | " << std::setw(7) << id.frequenz
                      << " | " << std::setw(11) << id.verarbeitet
                      << " | " << std::setw(6) << id.weitergeleitet
                      << " | " << std::setw(9) << id.abgelehnt
                      << " | " << std::setw(8) << id.akkumulierte_energie << "\n";
        }
        
        // Energie-Fluss-Analyse
        std::cout << "\nENERGIE-FLUSS-ANALYSE:\n";
        
        double avg_phasen[5] = {0};
        int count = 0;
        
        for (auto& imp : impulse) {
            if (imp.phasen_energien.size() >= 5) {
                for (int p = 0; p < 5; ++p) {
                    avg_phasen[p] += imp.phasen_energien[p];
                }
                count++;
            }
        }
        
        if (count > 0) {
            const char* phasen_namen[] = {"MICHAEL", "ITH'RA", "RAEL", "KAEL", "PHOENIX"};
            std::cout << "  Durchschnittliche Energie pro Phase (nur vollständige Durchläufe):\n";
            for (int p = 0; p < 5; ++p) {
                std::cout << "    " << phasen_namen[p] << ": " << (avg_phasen[p] / count) << "\n";
            }
            
            // Energieverstärkung berechnen
            double verstaerkung = (avg_phasen[4] / count) / (avg_phasen[0] / count);
            std::cout << "  Gesamtverstärkung: " << verstaerkung << "x\n";
        }
        
        if (total_manifest > impulse.size() * 0.3) {
            std::cout << "\n  ✓ KETTE FUNKTIONIERT: >30% erreichen PHOENIX!\n";
        }
    }
};

int main() {
    std::cout << "╔═══════════════════════════════════════════════════════════════════╗\n";
    std::cout << "║  TEST 04: DIE 5 IDENTITÄTEN                                       ║\n";
    std::cout << "║  MICHAEL → ITH'RA → RAEL → KAEL → PHOENIX                         ║\n";
    std::cout << "╚═══════════════════════════════════════════════════════════════════╝\n\n";
    
    FuenfIdentitaetenSimulation sim;
    sim.generiere_impulse(100000);
    sim.simuliere();
    
    std::cout << "\n✓ TEST 04 ABGESCHLOSSEN\n";
    return 0;
}
