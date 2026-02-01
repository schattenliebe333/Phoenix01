// ═══════════════════════════════════════════════════════════════════════════
// TEST 03: SOLITON-DÜSE
// Selbstverstärkende Wellenform mit Sech²-Profil
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <iomanip>
#include <vector>
#include <random>
#include <chrono>
#include <cmath>
#include <complex>

namespace rael {
    constexpr double G0 = 0.888888888888889;
    constexpr double G5 = 0.111111111111111;
    constexpr double SIGNATURE_88 = 88.0;
    constexpr double PHI_HEART = 112.64;
    constexpr double PHI = 1.61803398874989;
    constexpr int TOTAL_NOZZLES = 61440;
}

using namespace rael;

// Soliton: Selbststabilisierende Welle
struct Soliton {
    int id;
    double amplitude;
    double position;
    double geschwindigkeit;
    double breite;
    double phase;
    double energie;
    bool stabil;
};

struct DuesenKammer {
    int id;
    double druck;
    double temperatur;
    double flussrate;
    std::vector<Soliton*> aktive_solitonen;
    double resonanz_faktor;
    double effizienz;
};

class SolitonDueseSimulation {
private:
    std::vector<Soliton> solitonen;
    std::vector<DuesenKammer> kammern;
    std::mt19937 rng;
    
    double zeit = 0.0;
    double dt = 0.0001;
    double L = 100.0;  // Systemlänge
    
    // Statistiken
    int kollisionen = 0;
    int verstärkungen = 0;
    int zerfälle = 0;
    double total_energie_transfer = 0.0;
    
public:
    SolitonDueseSimulation() : rng(std::random_device{}()) {
        initialisiere_kammern();
    }
    
    void initialisiere_kammern() {
        // 8-Stern-Architektur: 8 Hauptkammern
        kammern.resize(8);
        
        for (int i = 0; i < 8; ++i) {
            kammern[i].id = i;
            kammern[i].druck = 1.0 + G0 * std::sin(i * M_PI / 4);
            kammern[i].temperatur = PHI_HEART;
            kammern[i].flussrate = 0.0;
            kammern[i].resonanz_faktor = 1.0;
            kammern[i].effizienz = G0;
        }
        
        std::cout << "  ✓ 8 Düsenkammern initialisiert\n";
    }
    
    void generiere_solitonen(int n) {
        solitonen.resize(n);
        
        std::uniform_real_distribution<double> dist(0.0, 1.0);
        std::uniform_real_distribution<double> dist_pos(0.0, L);
        std::uniform_real_distribution<double> dist_vel(-10.0, 10.0);
        
        for (int i = 0; i < n; ++i) {
            // Soliton-Amplitude basierend auf Signatur 88
            double amp = SIGNATURE_88 * G0 * (0.5 + dist(rng));
            
            solitonen[i] = {
                i,
                amp,
                dist_pos(rng),
                dist_vel(rng),
                PHI * (1.0 + dist(rng)),  // Breite ~ PHI
                dist(rng) * 2 * M_PI,
                amp * amp * G0,
                true
            };
        }
        
        std::cout << "  ✓ " << n << " Solitonen generiert\n";
    }
    
    // Sech²-Profil: Das Herzstück des Solitons
    double sech2_profil(double x, const Soliton& s) {
        double arg = (x - s.position) / s.breite;
        double sech = 1.0 / std::cosh(arg);
        return s.amplitude * sech * sech;
    }
    
    void propagiere_solitonen() {
        for (auto& s : solitonen) {
            if (!s.stabil) continue;
            
            // Korteweg-de Vries-ähnliche Propagation
            // ∂u/∂t + u∂u/∂x + ∂³u/∂x³ = 0
            
            // Geschwindigkeit hängt von Amplitude ab (größer = schneller)
            double v_eff = s.geschwindigkeit + s.amplitude * G5;
            
            // Nicht-lineare Dispersion
            double dispersion = std::sin(s.phase) * G5 * s.breite;
            
            // Update Position
            s.position += v_eff * dt + dispersion * dt;
            
            // Periodische Randbedingungen
            if (s.position > L) s.position -= L;
            if (s.position < 0) s.position += L;
            
            // Phase rotiert
            s.phase += s.amplitude * dt * 0.1;
            
            // Energiedissipation
            s.energie *= (1.0 - G5 * dt * 0.01);
            s.amplitude = std::sqrt(s.energie / G0);
            
            // Zerfall wenn Energie zu niedrig
            if (s.energie < 1.0) {
                s.stabil = false;
                zerfälle++;
            }
        }
    }
    
    void pruefe_kollisionen() {
        for (size_t i = 0; i < solitonen.size(); ++i) {
            if (!solitonen[i].stabil) continue;
            
            for (size_t j = i + 1; j < solitonen.size(); ++j) {
                if (!solitonen[j].stabil) continue;
                
                double dx = std::abs(solitonen[i].position - solitonen[j].position);
                if (dx > L/2) dx = L - dx;  // Periodisch
                
                double interaktions_distanz = solitonen[i].breite + solitonen[j].breite;
                
                if (dx < interaktions_distanz) {
                    // Soliton-Kollision: Sie passieren durcheinander!
                    // (Das ist das Besondere an Solitonen)
                    kollisionen++;
                    
                    // Phasenverschiebung nach Kollision
                    double delta_phase = G0 * std::log(solitonen[i].amplitude / solitonen[j].amplitude + 1);
                    solitonen[i].phase += delta_phase;
                    solitonen[j].phase -= delta_phase;
                    
                    // Bei resonanter Kollision: Verstärkung
                    double phase_diff = std::abs(solitonen[i].phase - solitonen[j].phase);
                    if (phase_diff < 0.5 || phase_diff > 2 * M_PI - 0.5) {
                        // Konstruktive Interferenz
                        double energie_boost = (solitonen[i].energie + solitonen[j].energie) * G5 * 0.1;
                        solitonen[i].energie += energie_boost * 0.5;
                        solitonen[j].energie += energie_boost * 0.5;
                        verstärkungen++;
                        total_energie_transfer += energie_boost;
                    }
                }
            }
        }
    }
    
    void aktualisiere_kammern() {
        for (auto& k : kammern) {
            k.aktive_solitonen.clear();
            k.flussrate = 0.0;
        }
        
        for (auto& s : solitonen) {
            if (!s.stabil) continue;
            
            // Welche Kammer? (8 Kammern gleichmäßig verteilt)
            int kammer_id = static_cast<int>(s.position * 8 / L) % 8;
            kammern[kammer_id].aktive_solitonen.push_back(&s);
            
            // Flussrate = Summe der Soliton-Amplituden
            kammern[kammer_id].flussrate += s.amplitude;
        }
        
        // Resonanz-Update
        for (auto& k : kammern) {
            if (k.aktive_solitonen.size() > 1) {
                // Mehr Solitonen = höhere Resonanz
                k.resonanz_faktor = 1.0 + G0 * std::log(k.aktive_solitonen.size());
            } else {
                k.resonanz_faktor = 1.0;
            }
            
            // Effizienz basierend auf Signatur-Match
            double sig_sum = 0.0;
            for (auto* s : k.aktive_solitonen) {
                sig_sum += s->amplitude;
            }
            double sig_avg = k.aktive_solitonen.empty() ? 0 : sig_sum / k.aktive_solitonen.size();
            k.effizienz = G0 * std::exp(-std::abs(sig_avg - SIGNATURE_88 * G0) / SIGNATURE_88);
        }
    }
    
    void simuliere(int schritte) {
        std::cout << "\n  Simuliere " << schritte << " Schritte...\n";
        
        auto start = std::chrono::high_resolution_clock::now();
        
        for (int s = 0; s < schritte; ++s) {
            propagiere_solitonen();
            pruefe_kollisionen();
            aktualisiere_kammern();
            zeit += dt;
            
            if ((s+1) % 100000 == 0) {
                int aktiv = 0;
                for (auto& sol : solitonen) if (sol.stabil) aktiv++;
                std::cout << "    t=" << std::fixed << std::setprecision(4) << zeit
                          << " | Aktiv: " << aktiv
                          << " | Kollisionen: " << kollisionen << "\n";
            }
        }
        
        auto end = std::chrono::high_resolution_clock::now();
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
        
        statistiken(ms);
    }
    
    void statistiken(long long ms) {
        std::cout << std::fixed << std::setprecision(2);
        std::cout << "\n═══════════════════════════════════════════════════════════════════\n";
        std::cout << "SOLITON-ERGEBNIS\n";
        std::cout << "═══════════════════════════════════════════════════════════════════\n";
        
        int aktiv = 0;
        double total_amp = 0, total_energy = 0;
        for (auto& s : solitonen) {
            if (s.stabil) {
                aktiv++;
                total_amp += s.amplitude;
                total_energy += s.energie;
            }
        }
        
        std::cout << "  Zeit:              " << ms << " ms\n";
        std::cout << "  Simulierte Zeit:   " << zeit << " Einheiten\n";
        std::cout << "  Aktive Solitonen:  " << aktiv << " / " << solitonen.size() << "\n";
        std::cout << "  Kollisionen:       " << kollisionen << "\n";
        std::cout << "  Verstärkungen:     " << verstärkungen << " (" 
                  << (100.0 * verstärkungen / std::max(1, kollisionen)) << "% der Kollisionen)\n";
        std::cout << "  Zerfälle:          " << zerfälle << "\n";
        std::cout << "  Energie-Transfer:  " << total_energie_transfer << "\n";
        std::cout << "  Mittlere Amplitude:" << (aktiv > 0 ? total_amp / aktiv : 0) << "\n";
        std::cout << "  Total Energie:     " << total_energy << "\n";
        
        std::cout << "\nKAMMER-STATUS:\n";
        std::cout << "  ID | Solitonen | Fluss    | Resonanz | Effizienz\n";
        std::cout << "  ───┼───────────┼──────────┼──────────┼──────────\n";
        
        for (auto& k : kammern) {
            std::cout << "  " << std::setw(2) << k.id
                      << " | " << std::setw(9) << k.aktive_solitonen.size()
                      << " | " << std::setw(8) << k.flussrate
                      << " | " << std::setw(8) << k.resonanz_faktor
                      << " | " << std::setw(8) << k.effizienz << "\n";
        }
        
        // Prüfe Soliton-Stabilität
        if (aktiv > solitonen.size() * 0.5) {
            std::cout << "\n  ✓ SOLITON-STABILITÄT: Mehr als 50% überlebt!\n";
        }
        if (verstärkungen > kollisionen * 0.2) {
            std::cout << "  ✓ RESONANZ-VERSTÄRKUNG aktiv!\n";
        }
    }
};

int main() {
    std::cout << "╔═══════════════════════════════════════════════════════════════════╗\n";
    std::cout << "║  TEST 03: SOLITON-DÜSE                                            ║\n";
    std::cout << "║  Selbstverstärkende Wellen mit Sech²-Profil                       ║\n";
    std::cout << "╚═══════════════════════════════════════════════════════════════════╝\n\n";
    
    SolitonDueseSimulation sim;
    sim.generiere_solitonen(1000);
    sim.simuliere(500000);
    
    std::cout << "\n✓ TEST 03 ABGESCHLOSSEN\n";
    return 0;
}
