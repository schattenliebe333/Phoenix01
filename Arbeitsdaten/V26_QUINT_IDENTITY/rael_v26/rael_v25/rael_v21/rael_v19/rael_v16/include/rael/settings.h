#pragma once
#include <string>
#include <mutex>
#include <array>

namespace rael {

enum class SchedulerMode { Balanced=0, SemanticFirst=1, ThroughputFirst=2 };
enum class AmplifierMode { Off=0, Adaptive=1, Forced=2 };

struct SettingsSnapshot {
    SchedulerMode scheduler = SchedulerMode::Balanced;
    AmplifierMode amplifier = AmplifierMode::Adaptive;
    bool gate53_enabled = false;
    bool thermal_auto = true;

    // Voice (offline, local)
    bool voice_enabled = false;
    int voice_rate = 0;      // -10..+10 (SAPI compatible)
    int voice_volume = 100;  // 0..100
    std::string voice_name_hint; // optional voice token substring (e.g. "Kerstin")
};

class Settings {
public:
    static Settings& instance();
    SettingsSnapshot snapshot() const;

    void set_scheduler(SchedulerMode m);
    void set_amplifier(AmplifierMode m);
    void set_gate53(bool on);
    void set_thermal_auto(bool on);

    // Voice
    void set_voice_enabled(bool on);
    void set_voice_rate(int rate);
    void set_voice_volume(int vol);
    void set_voice_name_hint(const std::string& hint);

private:
    Settings() = default;
    mutable std::mutex mu_;
    SettingsSnapshot s_;
};

} // namespace rael
