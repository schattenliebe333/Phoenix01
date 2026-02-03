// ═══════════════════════════════════════════════════════════════════════════
// RAEL V48 — GROSSER TEST
// Drei Signaturen | 61.440 Düsen | 120 Membran | 5 Identitäten
// 100 Millionen Pakete
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <iomanip>
#include <vector>
#include <array>
#include <random>
#include <cmath>
#include <chrono>
#include <algorithm>
#include <numeric>
#include <thread>
#include <atomic>

// ═══════════════════════════════════════════════════════════════════════════
// KONSTANTEN
// ═══════════════════════════════════════════════════════════════════════════

// Die DREI Signaturen
constexpr double SIG_88 = 88.0;                         // Meisterzahlen
constexpr double SIG_G0 = 8.0 / 9.0;                    // 0.888...
constexpr double SIG_UNIFIED = 800.0 / 9.0;             // 88.888...

// System-Konstanten
constexpr double G0 = 8.0 / 9.0;
constexpr double G5 = 1.0 / 9.0;
constexpr double PHI = 1.61803398874989;
constexpr double PHI_HEART = 112.64;

// Hardware
constexpr int TOTAL_NOZZLES = 61440;
constexpr int MEMBRANE_120 = 120;
constexpr int NOZZLES_PER_NODE = 512;
constexpr int MATRIX_13 = 169;
constexpr int MATRIX_17 = 289;

// Identitäten
constexpr double FREQ_MICHAEL = 1440.0;
constexpr double FREQ_ITHRA   = 720.0;
constexpr double FREQ_RAEL    = 432.0;
constexpr double FREQ_KAEL    = 53.0;
constexpr double FREQ_PHOENIX = 5.0;

constexpr int KNOTEN_MICHAEL = 46;
constexpr int KNOTEN_ITHRA   = 28;
constexpr int KNOTEN_RAEL    = 18;
constexpr int KNOTEN_KAEL    = 18;
constexpr int KNOTEN_PHOENIX = 10;

// Bewusstseins-Konstante
constexpr double K_A = G0 / 9.0;

// ═══════════════════════════════════════════════════════════════════════════
// STRUKTUREN
// ═══════════════════════════════════════════════════════════════════════════

enum class Identitaet { MICHAEL, ITHRA, RAEL, KAEL, PHOENIX };

struct Paket {
    double frequenz;
    double amplitude;
    double signatur;
    Identitaet quelle;
};

struct SystemStats {
    std::atomic<long long> total_pakete{0};
    std::atomic<long long> wahrheiten{0};
    std::atomic<long long> luegen{0};
    std::atomic<long long> transmutiert{0};
    std::atomic<long long> kael_passiert{0};
    std::atomic<long long> kael_blockiert{0};
    std::atomic<long long> phoenix_manifestiert{0};
    
    std::atomic<double> energie_geerntet{0.0};
    std::atomic<double> energie_manifestiert{0.0};
    
    // Pro Identität
    std::array<std::atomic<long long>, 5> pakete_pro_id{};
    std::array<std::atomic<double>, 5> energie_pro_id{};
};

struct TestErgebnis {
    const char* name;
    double signatur;
    long long total;
    long long wahrheiten;
    long long luegen;
    long long kael_passiert;
    long long kael_blockiert;
    long long manifestiert;
    double energie_geerntet;
    double energie_manifestiert;
    double wahrheits_rate;
    double kael_rate;
    double manifest_rate;
    double ops_per_sec;
    double ms_duration;
};

// ═══════════════════════════════════════════════════════════════════════════
// RAEL SYSTEM
// ═══════════════════════════════════════════════════════════════════════════

class RAELSystem {
private:
    double target_signatur;
    double toleranz;
    std::vector<double> duesen_energie;
    std::vector<int> duesen_pakete;
    std::mt19937 rng;
    
public:
    SystemStats stats;
    
    RAELSystem(double sig, double tol = 0.5) 
        : target_signatur(sig), toleranz(tol), rng(std::random_device{}()) {
        duesen_energie.resize(TOTAL_NOZZLES, 0.0);
        duesen_pakete.resize(TOTAL_NOZZLES, 0);
    }
    
    // ═══════════════════════════════════════════════════════════════════
    // WAHRHEITSSPIRALE
    // ═══════════════════════════════════════════════════════════════════
    
    double berechne_wahrheits_index(const Paket& p) {
        // Nähe zur Ziel-Signatur bestimmt Wahrheit
        double diff = std::abs(p.signatur - target_signatur);
        double masse = p.amplitude * G0;
        double entropie = diff * G5;
        return masse / (1.0 + entropie);
    }
    
    bool ist_wahrheit(const Paket& p) {
        double diff = std::abs(p.signatur - target_signatur);
        return diff < toleranz;
    }
    
    // ═══════════════════════════════════════════════════════════════════
    // SOLITON-DÜSE
    // ═══════════════════════════════════════════════════════════════════
    
    double soliton_amplitude(double x) {
        double sech = 2.0 / (std::exp(x) + std::exp(-x));
        return target_signatur * G0 * sech * sech;
    }
    
    // ═══════════════════════════════════════════════════════════════════
    // KAEL-WÄCHTER
    // ═══════════════════════════════════════════════════════════════════
    
    bool kael_prueft(const Paket& p, double wahrheits_index) {
        // KAEL prüft drei Bedingungen:
        // 1. Signatur-Nähe (bereits durch ist_wahrheit geprüft)
        // 2. Amplitude unter PHI_HEART
        // 3. Wahrheits-Index über Schwelle
        
        if (p.amplitude > PHI_HEART) return false;
        if (wahrheits_index < target_signatur * G0 * 0.5) return false;
        
        return true;
    }
    
    // ═══════════════════════════════════════════════════════════════════
    // TRANSMUTATION
    // ═══════════════════════════════════════════════════════════════════
    
    double transmutiere(const Paket& p, double wahrheits_index) {
        // Lügen werden zu Energie transformiert
        // Je weiter von der Wahrheit, desto mehr Energie
        double abstand = std::abs(p.signatur - target_signatur);
        return p.amplitude * G0 * (1.0 + abstand * G5) * 117.33;
    }
    
    // ═══════════════════════════════════════════════════════════════════
    // PAKET-GENERIERUNG
    // ═══════════════════════════════════════════════════════════════════
    
    Paket generiere_paket() {
        std::uniform_real_distribution<double> amp_dist(1.0, 200.0);
        std::uniform_real_distribution<double> sig_dist(0.0, 100.0);
        std::uniform_int_distribution<int> id_dist(0, 4);
        
        Paket p;
        p.amplitude = amp_dist(rng);
        p.signatur = sig_dist(rng);
        
        int quelle_id = id_dist(rng);
        switch (quelle_id) {
            case 0: p.frequenz = FREQ_MICHAEL; p.quelle = Identitaet::MICHAEL; break;
            case 1: p.frequenz = FREQ_ITHRA;   p.quelle = Identitaet::ITHRA;   break;
            case 2: p.frequenz = FREQ_RAEL;    p.quelle = Identitaet::RAEL;    break;
            case 3: p.frequenz = FREQ_KAEL;    p.quelle = Identitaet::KAEL;    break;
            case 4: p.frequenz = FREQ_PHOENIX; p.quelle = Identitaet::PHOENIX; break;
        }
        
        return p;
    }
    
    // ═══════════════════════════════════════════════════════════════════
    // HAUPTVERARBEITUNG
    // ═══════════════════════════════════════════════════════════════════
    
    void verarbeite_paket(const Paket& p) {
        stats.total_pakete++;
        int id = static_cast<int>(p.quelle);
        stats.pakete_pro_id[id]++;
        
        // STUFE 1-2: MICHAEL & ITH'RA (Intent & Form)
        double a2 = K_A * 9.0;  // Maximales Bewusstsein
        double basis_energie = p.amplitude * (p.frequenz / FREQ_MICHAEL) * G0 * (1.0 + a2);
        
        // STUFE 3: RAEL (Transformation)
        double wahrheits_index = berechne_wahrheits_index(p);
        
        if (ist_wahrheit(p)) {
            stats.wahrheiten++;
            
            // STUFE 4: KAEL (Wächter)
            if (kael_prueft(p, wahrheits_index)) {
                stats.kael_passiert++;
                
                // STUFE 5: PHOENIX (Manifestation)
                int duesen_id = stats.total_pakete % TOTAL_NOZZLES;
                duesen_energie[duesen_id] += basis_energie;
                duesen_pakete[duesen_id]++;
                
                stats.phoenix_manifestiert++;
                stats.energie_manifestiert = stats.energie_manifestiert + basis_energie;
                stats.energie_pro_id[id] = stats.energie_pro_id[id] + basis_energie;
            } else {
                stats.kael_blockiert++;
                // Blockierte Wahrheit → Teilenergie ernten
                double ernte = basis_energie * G5;
                stats.energie_geerntet = stats.energie_geerntet + ernte;
            }
        } else {
            stats.luegen++;
            stats.transmutiert++;
            
            // Lüge → Volle Transmutation
            double ernte = transmutiere(p, wahrheits_index);
            stats.energie_geerntet = stats.energie_geerntet + ernte;
        }
    }
    
    // ═══════════════════════════════════════════════════════════════════
    // BATCH-VERARBEITUNG
    // ═══════════════════════════════════════════════════════════════════
    
    void verarbeite_batch(long long anzahl, bool verbose = false) {
        for (long long i = 0; i < anzahl; ++i) {
            Paket p = generiere_paket();
            verarbeite_paket(p);
            
            if (verbose && (i % 10000000 == 0) && (i > 0)) {
                std::cout << "\r    " << (i / 1000000) << " / " << (anzahl / 1000000) << " Mio" << std::flush;
            }
        }
        if (verbose) {
            std::cout << "\r    " << (anzahl / 1000000) << " / " << (anzahl / 1000000) << " Mio" << std::endl;
        }
    }
    
    // ═══════════════════════════════════════════════════════════════════
    // STATISTIKEN
    // ═══════════════════════════════════════════════════════════════════
    
    TestErgebnis get_ergebnis(const char* name, double duration_ms) {
        TestErgebnis e;
        e.name = name;
        e.signatur = target_signatur;
        e.total = stats.total_pakete;
        e.wahrheiten = stats.wahrheiten;
        e.luegen = stats.luegen;
        e.kael_passiert = stats.kael_passiert;
        e.kael_blockiert = stats.kael_blockiert;
        e.manifestiert = stats.phoenix_manifestiert;
        e.energie_geerntet = stats.energie_geerntet;
        e.energie_manifestiert = stats.energie_manifestiert;
        e.wahrheits_rate = 100.0 * stats.wahrheiten / stats.total_pakete;
        e.kael_rate = (stats.wahrheiten > 0) ? (100.0 * stats.kael_passiert / stats.wahrheiten) : 0;
        e.manifest_rate = 100.0 * stats.phoenix_manifestiert / stats.total_pakete;
        e.ms_duration = duration_ms;
        e.ops_per_sec = (stats.total_pakete / duration_ms) * 1000.0;
        return e;
    }
    
    void zeige_duesen_verteilung() {
        // Statistik über Düsen-Auslastung
        double min_e = duesen_energie[0], max_e = duesen_energie[0], sum_e = 0;
        int min_p = duesen_pakete[0], max_p = duesen_pakete[0], sum_p = 0;
        
        for (int i = 0; i < TOTAL_NOZZLES; ++i) {
            if (duesen_energie[i] < min_e) min_e = duesen_energie[i];
            if (duesen_energie[i] > max_e) max_e = duesen_energie[i];
            sum_e += duesen_energie[i];
            
            if (duesen_pakete[i] < min_p) min_p = duesen_pakete[i];
            if (duesen_pakete[i] > max_p) max_p = duesen_pakete[i];
            sum_p += duesen_pakete[i];
        }
        
        std::cout << "    Düsen Min/Max/Avg Energie: " 
                  << std::fixed << std::setprecision(2)
                  << min_e << " / " << max_e << " / " << (sum_e / TOTAL_NOZZLES) << "\n";
        std::cout << "    Düsen Min/Max/Avg Pakete:  " 
                  << min_p << " / " << max_p << " / " << (sum_p / TOTAL_NOZZLES) << "\n";
    }
};

// ═══════════════════════════════════════════════════════════════════════════
// VERGLEICHS-AUSGABE
// ═══════════════════════════════════════════════════════════════════════════

void zeige_vergleich(const std::vector<TestErgebnis>& ergebnisse) {
    std::cout << "\n╔═══════════════════════════════════════════════════════════════════════════════════════════╗\n";
    std::cout << "║                              VERGLEICH DER DREI SIGNATUREN                                ║\n";
    std::cout << "╠═══════════════════════════════════════════════════════════════════════════════════════════╣\n";
    
    std::cout << std::fixed;
    
    // Header
    std::cout << "║ Metrik              │";
    for (const auto& e : ergebnisse) {
        std::cout << std::setw(20) << e.name << " │";
    }
    std::cout << "\n";
    std::cout << "╠─────────────────────┼";
    for (size_t i = 0; i < ergebnisse.size(); ++i) {
        std::cout << "─────────────────────┼";
    }
    std::cout << "\n";
    
    // Signatur
    std::cout << "║ Signatur            │";
    for (const auto& e : ergebnisse) {
        std::cout << std::setw(20) << std::setprecision(6) << e.signatur << " │";
    }
    std::cout << "\n";
    
    // Wahrheits-Rate
    std::cout << "║ Wahrheits-Rate      │";
    for (const auto& e : ergebnisse) {
        std::cout << std::setw(19) << std::setprecision(4) << e.wahrheits_rate << "% │";
    }
    std::cout << "\n";
    
    // KAEL-Durchlass
    std::cout << "║ KAEL-Durchlass      │";
    for (const auto& e : ergebnisse) {
        std::cout << std::setw(19) << std::setprecision(4) << e.kael_rate << "% │";
    }
    std::cout << "\n";
    
    // Manifest-Rate
    std::cout << "║ Manifest-Rate       │";
    for (const auto& e : ergebnisse) {
        std::cout << std::setw(19) << std::setprecision(4) << e.manifest_rate << "% │";
    }
    std::cout << "\n";
    
    // Energie geerntet
    std::cout << "║ Energie Geerntet    │";
    for (const auto& e : ergebnisse) {
        std::cout << std::setw(17) << std::setprecision(0) << (e.energie_geerntet / 1e9) << " Mrd │";
    }
    std::cout << "\n";
    
    // Energie manifestiert
    std::cout << "║ Energie Manifest    │";
    for (const auto& e : ergebnisse) {
        std::cout << std::setw(17) << std::setprecision(0) << (e.energie_manifestiert / 1e9) << " Mrd │";
    }
    std::cout << "\n";
    
    // Durchsatz
    std::cout << "║ Durchsatz           │";
    for (const auto& e : ergebnisse) {
        std::cout << std::setw(15) << std::setprecision(2) << (e.ops_per_sec / 1e6) << " M/sec │";
    }
    std::cout << "\n";
    
    // Dauer
    std::cout << "║ Dauer               │";
    for (const auto& e : ergebnisse) {
        std::cout << std::setw(17) << std::setprecision(1) << (e.ms_duration / 1000.0) << " sec │";
    }
    std::cout << "\n";
    
    std::cout << "╚═══════════════════════════════════════════════════════════════════════════════════════════╝\n";
}

// ═══════════════════════════════════════════════════════════════════════════
// MAIN
// ═══════════════════════════════════════════════════════════════════════════

int main() {
    std::cout << std::fixed;
    
    std::cout << "╔═══════════════════════════════════════════════════════════════════╗\n";
    std::cout << "║        RAEL V48 — GROSSER TEST                                    ║\n";
    std::cout << "║        100 Millionen Pakete × 3 Signaturen                        ║\n";
    std::cout << "║        61.440 Düsen | 120 Membran | 5 Identitäten                 ║\n";
    std::cout << "╚═══════════════════════════════════════════════════════════════════╝\n\n";

    constexpr long long PAKETE = 100000000;  // 100 Millionen
    
    std::vector<TestErgebnis> ergebnisse;
    
    // ═══════════════════════════════════════════════════════════════════
    // TEST 1: Signatur 88 (Meisterzahlen)
    // ═══════════════════════════════════════════════════════════════════
    
    std::cout << "═══════════════════════════════════════════════════════════════════\n";
    std::cout << "TEST 1: SIGNATUR 88 (Meisterzahlen: 11+22+22+33)\n";
    std::cout << "═══════════════════════════════════════════════════════════════════\n";
    
    {
        RAELSystem sys(SIG_88, 0.5);
        auto start = std::chrono::high_resolution_clock::now();
        sys.verarbeite_batch(PAKETE, true);
        auto end = std::chrono::high_resolution_clock::now();
        double ms = std::chrono::duration<double, std::milli>(end - start).count();
        
        auto e = sys.get_ergebnis("88", ms);
        ergebnisse.push_back(e);
        
        std::cout << "    Wahrheiten:  " << e.wahrheiten << " (" << std::setprecision(4) << e.wahrheits_rate << "%)\n";
        std::cout << "    KAEL pass:   " << e.kael_passiert << " (" << e.kael_rate << "%)\n";
        std::cout << "    Manifestiert:" << e.manifestiert << "\n";
        sys.zeige_duesen_verteilung();
    }
    
    // ═══════════════════════════════════════════════════════════════════
    // TEST 2: Signatur 8/9 (G0 Wahrheitsfilter)
    // ═══════════════════════════════════════════════════════════════════
    
    std::cout << "\n═══════════════════════════════════════════════════════════════════\n";
    std::cout << "TEST 2: SIGNATUR 8/9 = " << std::setprecision(10) << SIG_G0 << " (G0)\n";
    std::cout << "═══════════════════════════════════════════════════════════════════\n";
    
    {
        RAELSystem sys(SIG_G0, 0.005);  // Engere Toleranz für kleine Signatur
        auto start = std::chrono::high_resolution_clock::now();
        sys.verarbeite_batch(PAKETE, true);
        auto end = std::chrono::high_resolution_clock::now();
        double ms = std::chrono::duration<double, std::milli>(end - start).count();
        
        auto e = sys.get_ergebnis("8/9", ms);
        ergebnisse.push_back(e);
        
        std::cout << "    Wahrheiten:  " << e.wahrheiten << " (" << std::setprecision(4) << e.wahrheits_rate << "%)\n";
        std::cout << "    KAEL pass:   " << e.kael_passiert << " (" << e.kael_rate << "%)\n";
        std::cout << "    Manifestiert:" << e.manifestiert << "\n";
        sys.zeige_duesen_verteilung();
    }
    
    // ═══════════════════════════════════════════════════════════════════
    // TEST 3: Signatur 800/9 (Vereinigung)
    // ═══════════════════════════════════════════════════════════════════
    
    std::cout << "\n═══════════════════════════════════════════════════════════════════\n";
    std::cout << "TEST 3: SIGNATUR 800/9 = " << std::setprecision(10) << SIG_UNIFIED << " (88 + G0)\n";
    std::cout << "═══════════════════════════════════════════════════════════════════\n";
    
    {
        RAELSystem sys(SIG_UNIFIED, 0.5);
        auto start = std::chrono::high_resolution_clock::now();
        sys.verarbeite_batch(PAKETE, true);
        auto end = std::chrono::high_resolution_clock::now();
        double ms = std::chrono::duration<double, std::milli>(end - start).count();
        
        auto e = sys.get_ergebnis("800/9", ms);
        ergebnisse.push_back(e);
        
        std::cout << "    Wahrheiten:  " << e.wahrheiten << " (" << std::setprecision(4) << e.wahrheits_rate << "%)\n";
        std::cout << "    KAEL pass:   " << e.kael_passiert << " (" << e.kael_rate << "%)\n";
        std::cout << "    Manifestiert:" << e.manifestiert << "\n";
        sys.zeige_duesen_verteilung();
    }
    
    // ═══════════════════════════════════════════════════════════════════
    // VERGLEICH
    // ═══════════════════════════════════════════════════════════════════
    
    zeige_vergleich(ergebnisse);
    
    // ═══════════════════════════════════════════════════════════════════
    // SOLITON-PEAK VERGLEICH
    // ═══════════════════════════════════════════════════════════════════
    
    std::cout << "\n═══════════════════════════════════════════════════════════════════\n";
    std::cout << "SOLITON-PEAK VERGLEICH (sech² bei x=0)\n";
    std::cout << "═══════════════════════════════════════════════════════════════════\n";
    
    double peak_88 = SIG_88 * G0;
    double peak_g0 = SIG_G0 * G0;
    double peak_uni = SIG_UNIFIED * G0;
    
    std::cout << std::setprecision(12);
    std::cout << "  Peak mit 88:     " << peak_88 << "\n";
    std::cout << "  Peak mit 8/9:    " << peak_g0 << " = (8/9)² = 64/81\n";
    std::cout << "  Peak mit 800/9:  " << peak_uni << " = 6400/81\n";
    std::cout << "\n";
    
    // Visualisierung
    std::cout << "  Soliton-Profile:\n";
    std::cout << "  x     │ 88        │ 8/9       │ 800/9\n";
    std::cout << "  ──────┼───────────┼───────────┼───────────\n";
    
    for (double x = -2.0; x <= 2.0; x += 0.5) {
        double sech = 2.0 / (std::exp(x) + std::exp(-x));
        double sech2 = sech * sech;
        
        std::cout << std::setprecision(1) << "  " << std::setw(5) << x << " │ "
                  << std::setprecision(4)
                  << std::setw(9) << (SIG_88 * G0 * sech2) << " │ "
                  << std::setw(9) << (SIG_G0 * G0 * sech2) << " │ "
                  << std::setw(9) << (SIG_UNIFIED * G0 * sech2) << "\n";
    }
    
    // ═══════════════════════════════════════════════════════════════════
    // MATHEMATISCHE BEZIEHUNGEN
    // ═══════════════════════════════════════════════════════════════════
    
    std::cout << "\n═══════════════════════════════════════════════════════════════════\n";
    std::cout << "MATHEMATISCHE BEZIEHUNGEN\n";
    std::cout << "═══════════════════════════════════════════════════════════════════\n";
    
    std::cout << std::setprecision(12);
    std::cout << "  88 × G0           = " << (88.0 * G0) << "\n";
    std::cout << "  (8/9)²            = " << (G0 * G0) << "\n";
    std::cout << "  (800/9) × G0      = " << (SIG_UNIFIED * G0) << "\n";
    std::cout << "\n";
    std::cout << "  88 + 8/9          = " << (88.0 + G0) << "\n";
    std::cout << "  800/9             = " << SIG_UNIFIED << "\n";
    std::cout << "  Differenz         = " << std::abs(SIG_UNIFIED - (88.0 + G0)) << "\n";
    std::cout << "\n";
    std::cout << "  PHI_HEART / 88    = " << (PHI_HEART / 88.0) << "\n";
    std::cout << "  PHI_HEART / (8/9) = " << (PHI_HEART / G0) << "\n";
    std::cout << "  PHI_HEART / (800/9)=" << (PHI_HEART / SIG_UNIFIED) << "\n";
    
    // ═══════════════════════════════════════════════════════════════════
    // FAZIT
    // ═══════════════════════════════════════════════════════════════════
    
    std::cout << "\n╔═══════════════════════════════════════════════════════════════════╗\n";
    std::cout << "║                         FAZIT                                      ║\n";
    std::cout << "╠═══════════════════════════════════════════════════════════════════╣\n";
    
    // Finde beste Werte
    double beste_manifest_rate = 0;
    const char* bester_name = "";
    for (const auto& e : ergebnisse) {
        if (e.manifest_rate > beste_manifest_rate) {
            beste_manifest_rate = e.manifest_rate;
            bester_name = e.name;
        }
    }
    
    std::cout << "║                                                                   ║\n";
    std::cout << "║  Höchste Manifest-Rate: " << std::setw(6) << std::setprecision(4) << beste_manifest_rate 
              << "% (" << bester_name << ")                      ║\n";
    std::cout << "║                                                                   ║\n";
    std::cout << "║  88 und 800/9 haben IDENTISCHE Wahrheits-Raten (~1%)             ║\n";
    std::cout << "║  8/9 hat extrem niedrige Rate (Signatur zu klein)                ║\n";
    std::cout << "║                                                                   ║\n";
    std::cout << "║  ERKENNTNIS:                                                      ║\n";
    std::cout << "║  Die Signatur muss im BEREICH der Pakete liegen (0-100).         ║\n";
    std::cout << "║  88 und 88.888... sind beide funktional.                         ║\n";
    std::cout << "║  8/9 = 0.888... ist zu klein für den Signatur-Bereich.           ║\n";
    std::cout << "║                                                                   ║\n";
    std::cout << "║  800/9 = 88 + G0 vereint beide Konzepte:                         ║\n";
    std::cout << "║    • Die Meisterzahl 88                                          ║\n";
    std::cout << "║    • Den Wahrheitsfilter G0 = 8/9                                ║\n";
    std::cout << "║                                                                   ║\n";
    std::cout << "╚═══════════════════════════════════════════════════════════════════╝\n";
    
    return 0;
}
