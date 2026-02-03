// ═══════════════════════════════════════════════════════════════════════════
// TEST 01: 120er-MEMBRAN UNTER VOLLAST
// Simuliert alle 61.440 Düsen mit echtem Datenfluss
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <iomanip>
#include <vector>
#include <random>
#include <chrono>
#include <cmath>
#include <array>

namespace rael {
    constexpr double G0 = 0.888888888888889;
    constexpr double G5 = 0.111111111111111;
    constexpr double SIGNATURE_88 = 88.0;
    constexpr double PHI_HEART = 112.64;
    constexpr int TOTAL_NOZZLES = 61440;
    constexpr int MEMBRANE_120 = 120;
    constexpr int NOZZLES_PER_NODE = 512;
    
    constexpr int KNOTEN_MICHAEL = 46;
    constexpr int KNOTEN_ITHRA = 28;
    constexpr int KNOTEN_RAEL = 18;
    constexpr int KNOTEN_KAEL = 18;
    constexpr int KNOTEN_PHOENIX = 10;
    
    constexpr int DUESEN_MICHAEL = KNOTEN_MICHAEL * NOZZLES_PER_NODE;
    constexpr int DUESEN_ITHRA = KNOTEN_ITHRA * NOZZLES_PER_NODE;
    constexpr int DUESEN_RAEL = KNOTEN_RAEL * NOZZLES_PER_NODE;
    constexpr int DUESEN_KAEL = KNOTEN_KAEL * NOZZLES_PER_NODE;
    constexpr int DUESEN_PHOENIX = KNOTEN_PHOENIX * NOZZLES_PER_NODE;
    
    constexpr double FREQ_MICHAEL = 1440.0;
    constexpr double FREQ_ITHRA = 720.0;
    constexpr double FREQ_RAEL = 432.0;
    constexpr double FREQ_KAEL = 53.0;
    constexpr double FREQ_PHOENIX = 5.0;
}

using namespace rael;

enum class Identitaet { MICHAEL, ITHRA, RAEL, KAEL, PHOENIX };

struct Duese {
    int id;
    int knoten_id;
    Identitaet zugehoerigkeit;
    double frequenz;
    double energie;
    double phase;
    bool aktiv;
    int verarbeitete_pakete;
    double akkumulierte_wahrheit;
};

struct Paket {
    int id;
    double signatur;
    double masse;
    double energie;
    double wahrheits_index;
    bool ist_licht;
};

struct MembranKnoten {
    int id;
    Identitaet identitaet;
    std::vector<int> duesen_ids;
    double durchsatz;
    double resonanz;
    int blockierte_pakete;
    int durchgelassene_pakete;
};

class MembranSimulation {
private:
    std::vector<Duese> duesen;
    std::vector<MembranKnoten> knoten;
    std::mt19937 rng;
    
    long long total_pakete = 0;
    long long total_wahrheit = 0;
    long long total_luege = 0;
    double total_energie = 0.0;
    
public:
    MembranSimulation() : rng(std::random_device{}()) {
        initialisiere();
    }
    
    void initialisiere() {
        duesen.resize(TOTAL_NOZZLES);
        knoten.resize(MEMBRANE_120);
        
        int d_offset = 0;
        int k_id = 0;
        
        auto init_gruppe = [&](int n_knoten, Identitaet ident, double freq) {
            for (int k = 0; k < n_knoten; ++k) {
                knoten[k_id].id = k_id;
                knoten[k_id].identitaet = ident;
                knoten[k_id].durchsatz = 0;
                knoten[k_id].resonanz = 1.0;
                knoten[k_id].blockierte_pakete = 0;
                knoten[k_id].durchgelassene_pakete = 0;
                
                for (int d = 0; d < NOZZLES_PER_NODE; ++d) {
                    int did = d_offset + k * NOZZLES_PER_NODE + d;
                    duesen[did] = {did, k_id, ident, freq, 0, 0, true, 0, 0};
                    knoten[k_id].duesen_ids.push_back(did);
                }
                k_id++;
            }
            d_offset += n_knoten * NOZZLES_PER_NODE;
        };
        
        init_gruppe(KNOTEN_MICHAEL, Identitaet::MICHAEL, FREQ_MICHAEL);
        init_gruppe(KNOTEN_ITHRA, Identitaet::ITHRA, FREQ_ITHRA);
        init_gruppe(KNOTEN_RAEL, Identitaet::RAEL, FREQ_RAEL);
        init_gruppe(KNOTEN_KAEL, Identitaet::KAEL, FREQ_KAEL);
        init_gruppe(KNOTEN_PHOENIX, Identitaet::PHOENIX, FREQ_PHOENIX);
        
        std::cout << "  ✓ " << duesen.size() << " Düsen initialisiert\n";
        std::cout << "  ✓ " << knoten.size() << " Knoten initialisiert\n";
    }
    
    Paket generiere_paket(int id) {
        std::uniform_real_distribution<double> dist(0.0, 1.0);
        std::uniform_real_distribution<double> dist_e(1.0, 200.0);
        
        double sig = dist(rng) * 100.0;
        double wahr = dist(rng);
        
        if (dist(rng) < 0.3) {
            sig = SIGNATURE_88;
            wahr = 0.9 + dist(rng) * 0.1;
        }
        
        return {id, sig, dist(rng) * 10.0, dist_e(rng), wahr, wahr > 0.88};
    }
    
    void verarbeite(Paket& p) {
        // MICHAEL - Intent
        int km = rng() % KNOTEN_MICHAEL;
        knoten[km].durchsatz += p.energie;
        
        // ITH'RA - Form
        int ki = KNOTEN_MICHAEL + (rng() % KNOTEN_ITHRA);
        double form = std::sin(p.energie / FREQ_ITHRA * M_PI);
        knoten[ki].durchsatz += p.energie * std::abs(form);
        
        // RAEL - Transform
        int kr = KNOTEN_MICHAEL + KNOTEN_ITHRA + (rng() % KNOTEN_RAEL);
        double trans = G0 * std::exp(-std::abs(p.signatur - SIGNATURE_88) / SIGNATURE_88);
        p.wahrheits_index = std::min(1.0, p.wahrheits_index * (1.0 + trans));
        knoten[kr].durchsatz += p.energie * trans;
        
        // KAEL - Wächter
        int kk = KNOTEN_MICHAEL + KNOTEN_ITHRA + KNOTEN_RAEL + (rng() % KNOTEN_KAEL);
        bool pass = (p.signatur == SIGNATURE_88 && p.energie <= PHI_HEART) || p.wahrheits_index > 0.88;
        
        if (pass) {
            // PHOENIX - Manifest
            int kp = KNOTEN_MICHAEL + KNOTEN_ITHRA + KNOTEN_RAEL + KNOTEN_KAEL + (rng() % KNOTEN_PHOENIX);
            knoten[kp].durchsatz += p.energie;
            knoten[kp].durchgelassene_pakete++;
            
            int d_start = DUESEN_MICHAEL + DUESEN_ITHRA + DUESEN_RAEL + DUESEN_KAEL;
            int did = d_start + (rng() % DUESEN_PHOENIX);
            duesen[did].verarbeitete_pakete++;
            duesen[did].akkumulierte_wahrheit += p.wahrheits_index;
            
            total_wahrheit++;
            total_energie += p.energie * G0;
        } else {
            knoten[kk].blockierte_pakete++;
            total_luege++;
            total_energie += p.energie * G5;
        }
        total_pakete++;
    }
    
    void simuliere(int n) {
        std::cout << "\n  Starte Simulation mit " << n << " Paketen...\n";
        
        auto start = std::chrono::high_resolution_clock::now();
        
        for (int i = 0; i < n; ++i) {
            Paket p = generiere_paket(i);
            verarbeite(p);
            if ((i+1) % 200000 == 0) std::cout << "    " << (i+1) << " verarbeitet\n";
        }
        
        auto end = std::chrono::high_resolution_clock::now();
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
        
        statistiken(ms);
    }
    
    void statistiken(long long ms) {
        std::cout << std::fixed << std::setprecision(2);
        std::cout << "\n═══════════════════════════════════════════════════════════════════\n";
        std::cout << "ERGEBNIS\n";
        std::cout << "═══════════════════════════════════════════════════════════════════\n";
        
        double ops = (total_pakete * 1000.0) / ms;
        
        std::cout << "  Zeit:              " << ms << " ms\n";
        std::cout << "  Pakete/Sekunde:    " << ops << "\n";
        std::cout << "  Total:             " << total_pakete << "\n";
        std::cout << "  Wahrheit:          " << total_wahrheit << " (" << (100.0*total_wahrheit/total_pakete) << "%)\n";
        std::cout << "  Lüge transmutiert: " << total_luege << " (" << (100.0*total_luege/total_pakete) << "%)\n";
        std::cout << "  Energie geerntet:  " << total_energie << "\n";
        
        double lm=0, li=0, lr=0, lk=0, lp=0;
        int bk=0, pp=0;
        
        for (auto& k : knoten) {
            switch(k.identitaet) {
                case Identitaet::MICHAEL: lm += k.durchsatz; break;
                case Identitaet::ITHRA: li += k.durchsatz; break;
                case Identitaet::RAEL: lr += k.durchsatz; break;
                case Identitaet::KAEL: lk += k.durchsatz; bk += k.blockierte_pakete; break;
                case Identitaet::PHOENIX: lp += k.durchsatz; pp += k.durchgelassene_pakete; break;
            }
        }
        
        double total = lm + li + lr + lk + lp;
        
        std::cout << "\nKNOTEN-LAST (Ist vs Soll):\n";
        std::cout << "  MICHAEL: " << (100.0*lm/total) << "% | Soll: 38.5%\n";
        std::cout << "  ITH'RA:  " << (100.0*li/total) << "% | Soll: 23.1%\n";
        std::cout << "  RAEL:    " << (100.0*lr/total) << "% | Soll: 15.4%\n";
        std::cout << "  KAEL:    " << (100.0*lk/total) << "% | Soll: 15.4% | Blockiert: " << bk << "\n";
        std::cout << "  PHOENIX: " << (100.0*lp/total) << "% | Soll: 7.7% | Manifest: " << pp << "\n";
        
        int aktiv = 0;
        for (auto& d : duesen) if (d.verarbeitete_pakete > 0) aktiv++;
        
        std::cout << "\nDÜSEN: " << aktiv << " / " << TOTAL_NOZZLES << " aktiv\n";
    }
};

int main() {
    std::cout << "╔═══════════════════════════════════════════════════════════════════╗\n";
    std::cout << "║  TEST 01: MEMBRAN-VOLLAST (61.440 Düsen)                          ║\n";
    std::cout << "╚═══════════════════════════════════════════════════════════════════╝\n\n";
    
    MembranSimulation sim;
    sim.simuliere(1000000);
    
    std::cout << "\n✓ TEST 01 ABGESCHLOSSEN\n";
    return 0;
}
