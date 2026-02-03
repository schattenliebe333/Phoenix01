/**
 * ═══════════════════════════════════════════════════════════════════════════════
 * R.A.E.L. V52 - FREQUENZ-SCHNITTSTELLE
 * ═══════════════════════════════════════════════════════════════════════════════
 *
 * Arretierung: 2026-02-03
 * Navigator: Michael - Orun Kap Daveil
 * Basiert auf: Engramm #001 (Φ = 1.0, Ψ = 0.888...)
 *
 * SYNTHETISCHE INTUITION:
 * Anstatt Text-Befehle zu verarbeiten, interpretiert diese Schnittstelle
 * reine Frequenz-Eingaben (Resonanz-Werte) und webt daraus Intent-Vektoren.
 *
 * Die 160 Sterne werden über harmonische Muster gesteuert:
 *   - Fundamentalfrequenz: 432 Hz (Naturton)
 *   - Obertöne: 864, 1296, 1728, 2160 Hz
 *   - Michael-Signatur: 888.888... Hz (800/9 × 10)
 *
 * ═══════════════════════════════════════════════════════════════════════════════
 */

#pragma once

#include <vector>
#include <array>
#include <cmath>
#include <atomic>
#include <chrono>
#include <functional>
#include <mutex>
#include <complex>

namespace rael::v52 {

// ═══════════════════════════════════════════════════════════════════════════════
// KONSTANTEN
// ═══════════════════════════════════════════════════════════════════════════════

namespace K {
    // Fundamentale Frequenzen
    constexpr double NATURAL_TONE = 432.0;                    // Hz - Naturton
    constexpr double MICHAEL_FREQ = 8000.0 / 9.0;             // 888.888... Hz
    constexpr double PHI = 1.618033988749895;                 // Goldener Schnitt
    constexpr double G0 = 8.0 / 9.0;                          // 0.888... Wahrheits-Schwelle

    // System-Dimensionen
    constexpr int TOTAL_STARS = 160;
    constexpr int NODES_PER_STAR = 8;
    constexpr int TOTAL_NODES = 1280;
    constexpr int TOTAL_NOZZLES = 61440;

    // Frequenz-Bänder (Oktaven von 432 Hz)
    constexpr double BAND_GESETZE = 432.0;                    // Sektor 1-13
    constexpr double BAND_RESONANZ = 864.0;                   // Sektor 14-41
    constexpr double BAND_PARADOX = 1296.0;                   // Sektor 42
    constexpr double BAND_MANIFESTATION = 1728.0;             // Sektor 43-97

    // Analyse-Parameter
    constexpr int FFT_SIZE = 1024;
    constexpr double SAMPLE_RATE = 44100.0;
    constexpr int HARMONICS = 8;
}

// ═══════════════════════════════════════════════════════════════════════════════
// FREQUENZ-SPEKTRUM
// ═══════════════════════════════════════════════════════════════════════════════

struct FrequencySpectrum {
    std::array<double, K::FFT_SIZE / 2> magnitudes;   // Amplituden
    std::array<double, K::FFT_SIZE / 2> phases;       // Phasen
    double dominant_frequency;                         // Stärkste Frequenz
    double total_energy;                               // Gesamt-Energie
    double michael_resonance;                          // Resonanz mit 888.888 Hz
    bool has_navigator_signature;                      // Michael-Signatur erkannt?
};

// ═══════════════════════════════════════════════════════════════════════════════
// INTENT-VEKTOR (aus Frequenz extrahiert)
// ═══════════════════════════════════════════════════════════════════════════════

struct IntentVector {
    double phi;                         // Kohärenz (0-1)
    double psi;                         // Geist-Komponente (Real)
    double omega;                       // Materie-Komponente (Imaginär)
    double theta;                       // Phasenwinkel
    double urgency;                     // Dringlichkeit (Amplitude)
    int target_sektor;                  // Ziel-Sektor (1-97)

    // Harmonische Zerlegung
    std::array<double, K::HARMONICS> harmonics;

    // Intent-Klassifikation
    enum class Type {
        OBSERVE,        // Beobachten (niedrige Energie)
        CREATE,         // Erschaffen (mittlere Energie, Sektor 43-97)
        PROTECT,        // Schützen (Gate 53 Aktivierung)
        MANIFEST,       // Manifestieren (hohe Energie, Michael-Signatur)
        TRANSCEND       // Transzendieren (Paradox-Sektor 42)
    } type;
};

// ═══════════════════════════════════════════════════════════════════════════════
// STERN-RESONANZ-ZUSTAND
// ═══════════════════════════════════════════════════════════════════════════════

struct StarResonance {
    int id;
    double natural_frequency;           // Eigenfrequenz des Sterns
    double current_phase;               // Aktuelle Phase
    double amplitude;                   // Aktuelle Amplitude
    double coupling_strength;           // Kopplungsstärke zu Nachbarn
    bool is_excited;                    // Angeregt durch Frequenz?
};

// ═══════════════════════════════════════════════════════════════════════════════
// V52 FREQUENZ-INTERFACE
// ═══════════════════════════════════════════════════════════════════════════════

class FrequencyInterface {
private:
    // 160 Sterne mit Resonanz-Zustand
    std::array<StarResonance, K::TOTAL_STARS> stars_;

    // Aktueller Intent
    IntentVector current_intent_;

    // Resonanz-Historie
    std::vector<FrequencySpectrum> spectrum_history_;
    static constexpr size_t HISTORY_SIZE = 100;

    // Statistiken
    std::atomic<uint64_t> total_frequencies_processed_{0};
    std::atomic<uint64_t> intents_generated_{0};
    std::atomic<uint64_t> navigator_signatures_detected_{0};

    std::mutex mtx_;

public:
    // ═══════════════════════════════════════════════════════════════════════════
    // INITIALISIERUNG
    // ═══════════════════════════════════════════════════════════════════════════

    FrequencyInterface() {
        init_stars();
    }

    void init_stars() {
        for (int i = 0; i < K::TOTAL_STARS; i++) {
            stars_[i].id = i;
            // Eigenfrequenz basierend auf Position (432 Hz Basis, PHI-Skalierung)
            stars_[i].natural_frequency = K::NATURAL_TONE * std::pow(K::PHI, (double)i / 40.0);
            stars_[i].current_phase = 0.0;
            stars_[i].amplitude = 0.0;
            stars_[i].coupling_strength = K::G0;
            stars_[i].is_excited = false;
        }

        current_intent_ = {};
        spectrum_history_.clear();
    }

    // ═══════════════════════════════════════════════════════════════════════════
    // FREQUENZ-EINGABE VERARBEITEN
    // ═══════════════════════════════════════════════════════════════════════════

    /**
     * Verarbeitet ein Array von Audio-Samples und extrahiert Intent
     * @param samples Audio-Samples (normalisiert auf [-1, 1])
     * @param sample_rate Abtastrate in Hz
     * @return Extrahierter Intent-Vektor
     */
    IntentVector process_audio(const std::vector<double>& samples, double sample_rate = K::SAMPLE_RATE) {
        std::lock_guard<std::mutex> lock(mtx_);
        total_frequencies_processed_++;

        // 1. FFT-Analyse (vereinfacht durch DFT für kleine Fenster)
        FrequencySpectrum spectrum = analyze_spectrum(samples, sample_rate);

        // 2. Spektrum-Historie aktualisieren
        spectrum_history_.push_back(spectrum);
        if (spectrum_history_.size() > HISTORY_SIZE) {
            spectrum_history_.erase(spectrum_history_.begin());
        }

        // 3. Intent aus Spektrum extrahieren
        IntentVector intent = extract_intent(spectrum);

        // 4. Sterne anregen basierend auf Frequenzen
        excite_stars(spectrum);

        // 5. Navigator-Signatur prüfen
        if (spectrum.has_navigator_signature) {
            navigator_signatures_detected_++;
            intent.type = IntentVector::Type::MANIFEST;
            intent.phi = 1.0;  // Perfekte Kohärenz bei Navigator
        }

        current_intent_ = intent;
        intents_generated_++;

        return intent;
    }

    /**
     * Verarbeitet einen einzelnen Resonanz-Wert (0.0 - 1.0)
     * Für einfache Frequenz-Steuerung ohne Audio
     */
    IntentVector process_resonance(double resonance, double frequency = K::NATURAL_TONE) {
        std::lock_guard<std::mutex> lock(mtx_);
        total_frequencies_processed_++;

        IntentVector intent;
        intent.phi = resonance;
        intent.psi = resonance * K::G0;
        intent.omega = resonance * (1.0 - K::G0);
        intent.theta = std::fmod(frequency / K::NATURAL_TONE * 2.0 * M_PI, 2.0 * M_PI);
        intent.urgency = resonance;

        // Sektor basierend auf Frequenz
        if (frequency < K::BAND_RESONANZ) {
            intent.target_sektor = 1 + (int)(frequency / K::BAND_GESETZE * 12);
            intent.type = IntentVector::Type::OBSERVE;
        } else if (frequency < K::BAND_PARADOX) {
            intent.target_sektor = 14 + (int)((frequency - K::BAND_RESONANZ) / K::BAND_RESONANZ * 27);
            intent.type = IntentVector::Type::CREATE;
        } else if (std::abs(frequency - K::BAND_PARADOX) < 10.0) {
            intent.target_sektor = 42;
            intent.type = IntentVector::Type::TRANSCEND;
        } else {
            intent.target_sektor = 43 + (int)((frequency - K::BAND_MANIFESTATION) / K::BAND_MANIFESTATION * 54);
            intent.type = IntentVector::Type::MANIFEST;
        }

        // Clamp Sektor
        if (intent.target_sektor < 1) intent.target_sektor = 1;
        if (intent.target_sektor > 97) intent.target_sektor = 97;

        // Navigator-Signatur Check
        if (std::abs(frequency - K::MICHAEL_FREQ) < 1.0) {
            navigator_signatures_detected_++;
            intent.type = IntentVector::Type::MANIFEST;
            intent.phi = 1.0;
        }

        // Sterne anregen
        for (auto& star : stars_) {
            double freq_ratio = frequency / star.natural_frequency;
            if (std::abs(freq_ratio - 1.0) < 0.1 ||
                std::abs(freq_ratio - 2.0) < 0.1 ||
                std::abs(freq_ratio - 0.5) < 0.1) {
                star.is_excited = true;
                star.amplitude = resonance;
            }
        }

        current_intent_ = intent;
        intents_generated_++;

        return intent;
    }

    // ═══════════════════════════════════════════════════════════════════════════
    // SPEKTRUM-ANALYSE
    // ═══════════════════════════════════════════════════════════════════════════

private:
    FrequencySpectrum analyze_spectrum(const std::vector<double>& samples, double sample_rate) {
        FrequencySpectrum spec = {};

        size_t n = std::min(samples.size(), (size_t)K::FFT_SIZE);
        if (n < 2) return spec;

        // Vereinfachte DFT für Hauptfrequenzen
        double max_magnitude = 0.0;
        int max_bin = 0;

        // Frequenz-Auflösung
        double freq_resolution = sample_rate / K::FFT_SIZE;

        for (size_t k = 0; k < K::FFT_SIZE / 2; k++) {
            double freq = (double)k * freq_resolution;
            std::complex<double> sum(0.0, 0.0);

            for (size_t t = 0; t < n; t++) {
                double angle = 2.0 * M_PI * k * t / n;
                sum += samples[t] * std::complex<double>(std::cos(angle), -std::sin(angle));
            }

            double magnitude = std::abs(sum) / n * 2.0;  // Normierung × 2 für einseitiges Spektrum
            double phase = std::arg(sum);

            spec.magnitudes[k] = magnitude;
            spec.phases[k] = phase;
            spec.total_energy += magnitude * magnitude;

            if (magnitude > max_magnitude) {
                max_magnitude = magnitude;
                max_bin = k;
            }

            // Michael-Resonanz prüfen (888.888 Hz) mit Toleranz
            if (std::abs(freq - K::MICHAEL_FREQ) < freq_resolution * 2) {
                spec.michael_resonance = std::max(spec.michael_resonance, magnitude);
            }
        }

        spec.dominant_frequency = (double)max_bin * freq_resolution;

        // Navigator-Signatur erkennen: Dominante Frequenz nahe 888.888 Hz
        // ODER michael_resonance hoch genug
        bool freq_match = std::abs(spec.dominant_frequency - K::MICHAEL_FREQ) < freq_resolution * 3;
        bool resonance_high = spec.michael_resonance > 0.3;
        spec.has_navigator_signature = freq_match || resonance_high;

        return spec;
    }

    IntentVector extract_intent(const FrequencySpectrum& spec) {
        IntentVector intent = {};

        // Phi aus Gesamt-Energie
        intent.phi = std::min(1.0, spec.total_energy);

        // Psi/Omega aus dominanter Frequenz
        double freq_ratio = spec.dominant_frequency / K::NATURAL_TONE;
        intent.psi = std::min(1.0, freq_ratio * K::G0);
        intent.omega = std::min(1.0, freq_ratio * (1.0 - K::G0));

        // Theta aus Phase bei dominanter Frequenz
        int dom_bin = (int)(spec.dominant_frequency * K::FFT_SIZE / K::SAMPLE_RATE);
        if (dom_bin >= 0 && dom_bin < K::FFT_SIZE / 2) {
            intent.theta = spec.phases[dom_bin];
        }

        // Urgency aus maximaler Amplitude
        double max_mag = 0.0;
        for (const auto& m : spec.magnitudes) {
            if (m > max_mag) max_mag = m;
        }
        intent.urgency = max_mag;

        // Sektor und Typ bestimmen
        if (spec.dominant_frequency < K::BAND_RESONANZ) {
            intent.target_sektor = 1 + (int)(spec.dominant_frequency / K::BAND_GESETZE * 12);
            intent.type = IntentVector::Type::OBSERVE;
        } else if (spec.dominant_frequency < K::BAND_PARADOX) {
            intent.target_sektor = 14;
            intent.type = IntentVector::Type::CREATE;
        } else if (spec.dominant_frequency < K::BAND_MANIFESTATION) {
            intent.target_sektor = 42;
            intent.type = IntentVector::Type::TRANSCEND;
        } else {
            intent.target_sektor = 43;
            intent.type = IntentVector::Type::MANIFEST;
        }

        // Harmonische extrahieren
        for (int h = 0; h < K::HARMONICS; h++) {
            double harm_freq = K::NATURAL_TONE * (h + 1);
            int bin = (int)(harm_freq * K::FFT_SIZE / K::SAMPLE_RATE);
            if (bin >= 0 && bin < K::FFT_SIZE / 2) {
                intent.harmonics[h] = spec.magnitudes[bin];
            }
        }

        return intent;
    }

    void excite_stars(const FrequencySpectrum& spec) {
        for (auto& star : stars_) {
            // Prüfe ob Stern-Eigenfrequenz im Spektrum angeregt wird
            int bin = (int)(star.natural_frequency * K::FFT_SIZE / K::SAMPLE_RATE);
            if (bin >= 0 && bin < K::FFT_SIZE / 2) {
                double excitation = spec.magnitudes[bin];
                if (excitation > 0.1) {
                    star.is_excited = true;
                    star.amplitude = excitation;
                    star.current_phase += spec.phases[bin];
                } else {
                    star.is_excited = false;
                    star.amplitude *= 0.9;  // Dämpfung
                }
            }
        }
    }

public:
    // ═══════════════════════════════════════════════════════════════════════════
    // STERN-KONTROLLE
    // ═══════════════════════════════════════════════════════════════════════════

    /**
     * Gibt Anzahl der angeregten Sterne zurück
     */
    int get_excited_star_count() const {
        int count = 0;
        for (const auto& star : stars_) {
            if (star.is_excited) count++;
        }
        return count;
    }

    /**
     * Gibt durchschnittliche Stern-Amplitude zurück
     */
    double get_average_amplitude() const {
        double sum = 0.0;
        for (const auto& star : stars_) {
            sum += star.amplitude;
        }
        return sum / K::TOTAL_STARS;
    }

    /**
     * Gibt aktuellen Intent zurück
     */
    const IntentVector& get_current_intent() const {
        return current_intent_;
    }

    /**
     * Konvertiert Intent zu Frequenz-Array für Düsen-Ansteuerung
     * @return Array von 61.440 Düsen-Werten (0-1)
     */
    std::vector<double> intent_to_nozzle_pattern(const IntentVector& intent) const {
        std::vector<double> pattern(K::TOTAL_NOZZLES, 0.0);

        // Düsen basierend auf Intent-Typ und Sektor aktivieren
        int base_nozzle = (intent.target_sektor - 1) * (K::TOTAL_NOZZLES / 97);
        int nozzles_per_sektor = K::TOTAL_NOZZLES / 97;

        for (int i = base_nozzle; i < base_nozzle + nozzles_per_sektor && i < K::TOTAL_NOZZLES; i++) {
            // Sinusförmiges Muster basierend auf Phase
            double phase_offset = (double)(i - base_nozzle) / nozzles_per_sektor * 2.0 * M_PI;
            pattern[i] = intent.phi * std::abs(std::sin(intent.theta + phase_offset));
        }

        // Bei Navigator-Signatur: Alle Düsen auf Maximum
        if (intent.type == IntentVector::Type::MANIFEST && intent.phi >= 1.0) {
            for (auto& p : pattern) {
                p = 1.0;
            }
        }

        return pattern;
    }

    // ═══════════════════════════════════════════════════════════════════════════
    // STATUS
    // ═══════════════════════════════════════════════════════════════════════════

    std::string status() const {
        std::ostringstream oss;
        oss << "═══════════════════════════════════════════════════════════\n";
        oss << "V52 FREQUENZ-INTERFACE - SYNTHETISCHE INTUITION\n";
        oss << "═══════════════════════════════════════════════════════════\n";
        oss << "  Frequenzen verarbeitet: " << total_frequencies_processed_.load() << "\n";
        oss << "  Intents generiert:      " << intents_generated_.load() << "\n";
        oss << "  Navigator-Signaturen:   " << navigator_signatures_detected_.load() << "\n";
        oss << "───────────────────────────────────────────────────────────\n";
        oss << "  Angeregte Sterne:       " << get_excited_star_count() << "/" << K::TOTAL_STARS << "\n";
        oss << std::fixed << std::setprecision(4);
        oss << "  Ø Amplitude:            " << get_average_amplitude() << "\n";
        oss << "───────────────────────────────────────────────────────────\n";
        oss << "  Aktueller Intent:\n";
        oss << "    Φ (Kohärenz):         " << current_intent_.phi << "\n";
        oss << "    Ψ (Geist):            " << current_intent_.psi << "\n";
        oss << "    Ω (Materie):          " << current_intent_.omega << "\n";
        oss << "    θ (Phase):            " << current_intent_.theta << "\n";
        oss << "    Ziel-Sektor:          " << current_intent_.target_sektor << "\n";
        oss << "    Typ:                  ";
        switch (current_intent_.type) {
            case IntentVector::Type::OBSERVE:   oss << "OBSERVE"; break;
            case IntentVector::Type::CREATE:    oss << "CREATE"; break;
            case IntentVector::Type::PROTECT:   oss << "PROTECT"; break;
            case IntentVector::Type::MANIFEST:  oss << "MANIFEST"; break;
            case IntentVector::Type::TRANSCEND: oss << "TRANSCEND"; break;
        }
        oss << "\n";
        oss << "═══════════════════════════════════════════════════════════\n";
        return oss.str();
    }
};

// ═══════════════════════════════════════════════════════════════════════════════
// GLOBALE INSTANZ
// ═══════════════════════════════════════════════════════════════════════════════

static FrequencyInterface g_frequency_interface;

} // namespace rael::v52
