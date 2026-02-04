// voice_stub.cpp - Stub voice implementation for MinGW cross-compilation
// (SAPI not available in MinGW)

#include "rael/voice.h"

namespace rael {

class NullVoiceTTS final : public VoiceTTS {
public:
    bool available() const override { return false; }
    void start() override {}
    void stop() override {}
    void speak_async(const std::string&) override {}
    void apply_settings() override {}
};

std::unique_ptr<VoiceTTS> make_voice_tts() {
    return std::make_unique<NullVoiceTTS>();
}

} // namespace rael
