#include "rael/settings.h"
#include <algorithm>
namespace rael {

Settings& Settings::instance(){
    static Settings inst;
    return inst;
}

SettingsSnapshot Settings::snapshot() const{
    std::lock_guard<std::mutex> lk(mu_);
    return s_;
}

void Settings::set_scheduler(SchedulerMode m){
    std::lock_guard<std::mutex> lk(mu_);
    s_.scheduler = m;
}
void Settings::set_amplifier(AmplifierMode m){
    std::lock_guard<std::mutex> lk(mu_);
    s_.amplifier = m;
}
void Settings::set_gate53(bool on){
    std::lock_guard<std::mutex> lk(mu_);
    s_.gate53_enabled = on;
}
void Settings::set_thermal_auto(bool on){
    std::lock_guard<std::mutex> lk(mu_);
    s_.thermal_auto = on;
}

void Settings::set_voice_enabled(bool on){
    std::lock_guard<std::mutex> lk(mu_);
    s_.voice_enabled = on;
}
void Settings::set_voice_rate(int rate){
    std::lock_guard<std::mutex> lk(mu_);
    s_.voice_rate = std::max(-10, std::min(10, rate));
}
void Settings::set_voice_volume(int vol){
    std::lock_guard<std::mutex> lk(mu_);
    s_.voice_volume = std::max(0, std::min(100, vol));
}
void Settings::set_voice_name_hint(const std::string& hint){
    std::lock_guard<std::mutex> lk(mu_);
    s_.voice_name_hint = hint;
}

} // namespace rael
