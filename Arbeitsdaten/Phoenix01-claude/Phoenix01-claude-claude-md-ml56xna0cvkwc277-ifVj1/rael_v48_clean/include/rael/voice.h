#pragma once
#include <string>
#include <memory>

namespace rael {

// Offline Voice output (TTS). Designed to be non-blocking for the core.
class VoiceTTS {
public:
    virtual ~VoiceTTS() = default;
    virtual bool available() const = 0;
    virtual void start() = 0;
    virtual void stop() = 0;
    virtual void speak_async(const std::string& text) = 0;
    virtual void apply_settings() = 0; // reads Settings::snapshot()
};

// Factory: returns a platform implementation (Windows SAPI when available).
std::unique_ptr<VoiceTTS> make_voice_tts();

} // namespace rael
