// ═══════════════════════════════════════════════════════════════════════════
// TEST 02: WAHRHEITSSPIRALE
// Schwere sinkt zum Kern, Leichtes wird nach außen geschleudert
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <iomanip>
#include <vector>
#include <random>
#include <chrono>
#include <cmath>
#include <algorithm>

namespace rael {
    constexpr double G0 = 0.888888888888889;
    constexpr double G5 = 0.111111111111111;
    constexpr double SIGNATURE_88 = 88.0;
    constexpr double PHI_HEART = 112.64;
    constexpr double PHI = 1.61803398874989;
    constexpr int SPIRAL_RINGE = 13;      // 13 Ringe (Base-13)
    constexpr int SEGMENTE_PRO_RING = 8;  // 8-Stern-Architektur
}

using namespace rael;

struct SpiralPartikel {
    int id;
    double masse;           // Schwere = Wahrheitsgehalt
    double radius;          // Position im Spiralfeld
    double winkel;          // Winkelposition
    double geschwindigkeit; // Radiale Geschwindigkeit
    double signatur;
    double energie;
    bool ist_wahrheit;
    int ring;               // Aktueller Ring (0 = Kern, 12 = Rand)
};

struct SpiralRing {
    int ring_id;
    double radius_innen;
    double radius_aussen;
    std::vector<SpiralPartikel*> partikel;
    double total_masse;
    double total_energie;
    int wahrheiten;
    int luegen;
};

class WahrheitsspiralSimulation {
private:
    std::vector<SpiralPartikel> partikel;
    std::vector<SpiralRing> ringe;
    std::mt19937 rng;
    
    double zeit = 0.0;
    double dt = 0.001;  // Zeitschritt
    
    // Statistiken
    int transmutiert = 0;
    int manifestiert = 0;
    double energie_geerntet = 0.0;
    
public:
    WahrheitsspiralSimulation() : rng(std::random_device{}()) {
        initialisiere_ringe();
    }
    
    void initialisiere_ringe() {
        ringe.resize(SPIRAL_RINGE);
        
        for (int i = 0; i < SPIRAL_RINGE; ++i) {
            ringe[i].ring_id = i;
            ringe[i].radius_innen = i * PHI;
            ringe[i].radius_aussen = (i + 1) * PHI;
            ringe[i].total_masse = 0;
            ringe[i].total_energie = 0;
            ringe[i].wahrheiten = 0;
            ringe[i].luegen = 0;
        }
        
        std::cout << "  ✓ " << SPIRAL_RINGE << " Spiralringe initialisiert\n";
    }
    
    void generiere_partikel(int n) {
        partikel.resize(n);
        
        std::uniform_real_distribution<double> dist(0.0, 1.0);
        std::uniform_real_distribution<double> dist_masse(0.1, 10.0);
        std::uniform_real_distribution<double> dist_winkel(0.0, 2.0 * M_PI);
        
        for (int i = 0; i < n; ++i) {
            double masse = dist_masse(rng);
            double wahrheits_faktor = dist(rng);
            
            // 40% echte Wahrheit (schwer), 60% Lüge (leicht)
            if (wahrheits_faktor < 0.4) {
                masse *= (1.0 + G0);  // Wahrheit ist schwerer
                partikel[i].ist_wahrheit = true;
                partikel[i].signatur = SIGNATURE_88;
            } else {
                masse *= G5;  // Lüge ist leichter
                partikel[i].ist_wahrheit = false;
                partikel[i].signatur = dist(rng) * 100.0;
            }
            
            // Starte alle Partikel im mittleren Bereich
            double start_ring = 6 + dist(rng) * 2;
            double radius = start_ring * PHI;
            
            partikel[i].id = i;
            partikel[i].masse = masse;
            partikel[i].radius = radius;
            partikel[i].winkel = dist_winkel(rng);
            partikel[i].geschwindigkeit = 0.0;
            partikel[i].energie = masse * G0 * 100.0;
            partikel[i].ring = static_cast<int>(start_ring);
        }
        
        std::cout << "  ✓ " << n << " Partikel generiert\n";
    }
    
    void berechne_krafte() {
        // Zentripetalkraft basierend auf Masse
        // Schwere Partikel werden zum Zentrum gezogen
        // Leichte Partikel werden nach außen geschleudert
        
        for (auto& p : partikel) {
            if (p.ring < 0 || p.ring >= SPIRAL_RINGE) continue;
            
            // Gravitationskraft zum Zentrum
            double f_gravitation = -G0 * p.masse / (p.radius * p.radius + 0.1);
            
            // Zentrifugalkraft (wirkt nach außen)
            double omega = SIGNATURE_88 / (p.radius + 1.0);  // Winkelgeschwindigkeit
            double f_zentrifugal = p.masse * omega * omega * p.radius * G5;
            
            // Resultierende Kraft
            double f_netto = f_gravitation + f_zentrifugal;
            
            // Wahrheit wird stärker nach innen gezogen
            if (p.ist_wahrheit) {
                f_netto -= p.masse * G0 * 0.5;
            }
            
            // Beschleunigung
            double beschleunigung = f_netto / p.masse;
            
            // Update Geschwindigkeit und Position
            p.geschwindigkeit += beschleunigung * dt;
            p.geschwindigkeit *= 0.99;  // Dämpfung
            
            p.radius += p.geschwindigkeit * dt;
            p.winkel += omega * dt;
            
            // Ring-Zugehörigkeit aktualisieren
            p.ring = static_cast<int>(p.radius / PHI);
            if (p.ring < 0) p.ring = 0;
            if (p.ring >= SPIRAL_RINGE) p.ring = SPIRAL_RINGE - 1;
        }
    }
    
    void verarbeite_grenzen() {
        for (auto& p : partikel) {
            // Erreicht das Zentrum (Ring 0) → Manifestation
            if (p.radius < PHI && p.ist_wahrheit) {
                manifestiert++;
                energie_geerntet += p.energie * G0;
                p.radius = 100.0;  // Aus dem System entfernen
                p.ring = -1;
            }
            
            // Erreicht den Rand (Ring 12) → Transmutation
            if (p.radius > SPIRAL_RINGE * PHI && !p.ist_wahrheit) {
                transmutiert++;
                energie_geerntet += p.energie * G5;
                p.radius = -100.0;  // Aus dem System entfernen
                p.ring = -1;
            }
        }
    }
    
    void aktualisiere_ring_statistiken() {
        for (auto& r : ringe) {
            r.partikel.clear();
            r.total_masse = 0;
            r.total_energie = 0;
            r.wahrheiten = 0;
            r.luegen = 0;
        }
        
        for (auto& p : partikel) {
            if (p.ring >= 0 && p.ring < SPIRAL_RINGE) {
                ringe[p.ring].partikel.push_back(&p);
                ringe[p.ring].total_masse += p.masse;
                ringe[p.ring].total_energie += p.energie;
                if (p.ist_wahrheit) ringe[p.ring].wahrheiten++;
                else ringe[p.ring].luegen++;
            }
        }
    }
    
    void simuliere(int schritte) {
        std::cout << "\n  Simuliere " << schritte << " Zeitschritte...\n";
        
        auto start = std::chrono::high_resolution_clock::now();
        
        for (int s = 0; s < schritte; ++s) {
            berechne_krafte();
            verarbeite_grenzen();
            zeit += dt;
            
            if ((s+1) % 100000 == 0) {
                aktualisiere_ring_statistiken();
                std::cout << "    t=" << std::fixed << std::setprecision(3) << zeit 
                          << " | Manifest: " << manifestiert 
                          << " | Transmut: " << transmutiert << "\n";
            }
        }
        
        auto end = std::chrono::high_resolution_clock::now();
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
        
        aktualisiere_ring_statistiken();
        statistiken(ms);
    }
    
    void statistiken(long long ms) {
        std::cout << std::fixed << std::setprecision(2);
        std::cout << "\n═══════════════════════════════════════════════════════════════════\n";
        std::cout << "SPIRALEN-ERGEBNIS\n";
        std::cout << "═══════════════════════════════════════════════════════════════════\n";
        
        std::cout << "  Zeit:           " << ms << " ms\n";
        std::cout << "  Simulierte Zeit:" << zeit << " Einheiten\n";
        std::cout << "  Manifestiert:   " << manifestiert << " (Wahrheit → Kern)\n";
        std::cout << "  Transmutiert:   " << transmutiert << " (Lüge → Rand → Energie)\n";
        std::cout << "  Energie:        " << energie_geerntet << "\n";
        
        std::cout << "\nRING-VERTEILUNG:\n";
        std::cout << "  Ring | Partikel | Wahrheit | Lüge   | Masse\n";
        std::cout << "  ─────┼──────────┼──────────┼────────┼───────────\n";
        
        for (int i = 0; i < SPIRAL_RINGE; ++i) {
            std::cout << "  " << std::setw(4) << i 
                      << " | " << std::setw(8) << ringe[i].partikel.size()
                      << " | " << std::setw(8) << ringe[i].wahrheiten
                      << " | " << std::setw(6) << ringe[i].luegen
                      << " | " << std::setw(9) << ringe[i].total_masse << "\n";
        }
        
        // Prüfe ob Wahrheit zum Kern sinkt
        int wahrheit_kern = 0, wahrheit_rand = 0;
        int luege_kern = 0, luege_rand = 0;
        
        for (int i = 0; i < 4; ++i) {
            wahrheit_kern += ringe[i].wahrheiten;
            luege_kern += ringe[i].luegen;
        }
        for (int i = 9; i < SPIRAL_RINGE; ++i) {
            wahrheit_rand += ringe[i].wahrheiten;
            luege_rand += ringe[i].luegen;
        }
        
        std::cout << "\nSEPARATION:\n";
        std::cout << "  Kern (0-3):  Wahrheit=" << wahrheit_kern << " | Lüge=" << luege_kern << "\n";
        std::cout << "  Rand (9-12): Wahrheit=" << wahrheit_rand << " | Lüge=" << luege_rand << "\n";
        
        if (wahrheit_kern > luege_kern && luege_rand > wahrheit_rand) {
            std::cout << "  ✓ SEPARATION ERFOLGREICH: Wahrheit sinkt, Lüge steigt!\n";
        } else {
            std::cout << "  ⚠ SEPARATION UNVOLLSTÄNDIG\n";
        }
    }
};

int main() {
    std::cout << "╔═══════════════════════════════════════════════════════════════════╗\n";
    std::cout << "║  TEST 02: WAHRHEITSSPIRALE                                        ║\n";
    std::cout << "║  Schwer → Kern | Leicht → Rand → Transmutation                    ║\n";
    std::cout << "╚═══════════════════════════════════════════════════════════════════╝\n\n";
    
    WahrheitsspiralSimulation sim;
    sim.generiere_partikel(50000);
    sim.simuliere(500000);
    
    std::cout << "\n✓ TEST 02 ABGESCHLOSSEN\n";
    return 0;
}
