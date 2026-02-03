#include "rael/voice.h"

#ifdef _WIN32

#include <windows.h>
#include <sapi.h>
#include <sphelper.h>

#include <atomic>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>

#include "rael/settings.h"
#include "rael/events.h"
#include "rael/util.h"

namespace rael {

static std::wstring to_w(const std::string& s){
    if(s.empty()) return L"";
    int len = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, nullptr, 0);
    std::wstring out;
    out.resize((size_t)len);
    MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, &out[0], len);
    // drop trailing null
    if(!out.empty() && out.back() == L'\0') out.pop_back();
    return out;
}

static std::string viseme_char_from_id(int id){
    // Rough mapping for GUI lip indicator. SAPI viseme ids are 0..21.
    // We keep it minimal and stable.
    switch(id){
        case 0: return "N"; // silence
        case 1: case 2: return "A";
        case 3: case 4: return "E";
        case 5: case 6: return "I";
        case 7: case 8: return "O";
        case 9: case 10: return "U";
        case 11: return "M";
        case 12: return "F";
        case 13: return "S";
        case 14: return "T";
        case 15: return "R";
        case 16: return "L";
        default: return "N";
    }
}

class SapiVoiceTTS final : public VoiceTTS {
public:
    SapiVoiceTTS() = default;
    ~SapiVoiceTTS() override { stop(); }

    bool available() const override { return true; }

    void start() override {
        if(running_.exchange(true)) return;
        worker_ = std::thread([this](){ this->run(); });
    }

    void stop() override {
        if(!running_.exchange(false)) return;
        {
            std::lock_guard<std::mutex> lk(mu_);
            while(!q_.empty()) q_.pop();
        }
        cv_.notify_all();
        if(worker_.joinable()) worker_.join();
    }

    void speak_async(const std::string& text) override {
        auto snap = Settings::instance().snapshot();
        if(!snap.voice_enabled) return;
        if(text.empty()) return;
        {
            std::lock_guard<std::mutex> lk(mu_);
            q_.push(text);
        }
        cv_.notify_one();
    }

    void apply_settings() override {
        // no-op here; we pull settings per job.
    }

private:
    std::atomic<bool> running_{false};
    std::thread worker_;
    std::mutex mu_;
    std::condition_variable cv_;
    std::queue<std::string> q_;

    void run(){
        // COM init for this thread.
        HRESULT hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
        if(FAILED(hr)){
            EventBus::push("VOICE_INIT_FAIL", "CoInitializeEx failed");
            return;
        }

        ISpVoice* voice = nullptr;
        hr = CoCreateInstance(CLSID_SpVoice, nullptr, CLSCTX_ALL, IID_ISpVoice, (void**)&voice);
        if(FAILED(hr) || !voice){
            EventBus::push("VOICE_INIT_FAIL", "CoCreateInstance SpVoice failed");
            CoUninitialize();
            return;
        }

        // receive viseme + end events
        voice->SetInterest(SPFEI(SPEI_VISEME) | SPFEI(SPEI_END_INPUT_STREAM),
                           SPFEI(SPEI_VISEME) | SPFEI(SPEI_END_INPUT_STREAM));

        while(running_.load()){
            std::string job;
            {
                std::unique_lock<std::mutex> lk(mu_);
                cv_.wait(lk, [&]{ return !running_.load() || !q_.empty(); });
                if(!running_.load()) break;
                job = q_.front();
                q_.pop();
            }

            auto snap = Settings::instance().snapshot();
            if(!snap.voice_enabled) continue;

            voice->SetRate((long)snap.voice_rate);
            voice->SetVolume((unsigned short)snap.voice_volume);
            if(!snap.voice_name_hint.empty()){
                select_voice(voice, snap.voice_name_hint);
            }

            EventBus::push("VOICE", "state=talking");
            // Kick GUI with initial viseme.
            EventBus::push("VOICE", "state=talking|viseme=N|mouth=0.25");

            std::wstring w = to_w(job);
            // speak async so we can poll events
            hr = voice->Speak(w.c_str(), SPF_ASYNC, nullptr);
            if(FAILED(hr)){
                EventBus::push("VOICE_FAIL", "Speak failed");
                continue;
            }

            bool done = false;
            while(!done && running_.load()){
                // pump voice events
                SPEVENT evt;
                ULONG fetched = 0;
                while(SUCCEEDED(voice->GetEvents(1, &evt, &fetched)) && fetched){
                    if(evt.eEventId == SPEI_VISEME){
                        int vid = (int)evt.lParam;
                        auto vch = viseme_char_from_id(vid);
                        // crude mouth openness by viseme class
                        double mouth = (vid == 0) ? 0.0 : 0.45;
                        EventBus::push("VOICE", std::string("state=talking|viseme=") + vch + "|mouth=" + std::to_string(mouth));
                    } else if(evt.eEventId == SPEI_END_INPUT_STREAM){
                        done = true;
                    }
                    SpClearEvent(&evt);
                }
                Sleep(10);
            }

            EventBus::push("VOICE", "state=idle|viseme=N|mouth=0.0");
        }

        voice->Release();
        CoUninitialize();
    }

    static void select_voice(ISpVoice* voice, const std::string& hint){
        ISpObjectToken* pToken = nullptr;
        IEnumSpObjectTokens* pEnum = nullptr;
        ULONG count = 0;
        HRESULT hr = SpEnumTokens(SPCAT_VOICES, nullptr, nullptr, &pEnum);
        if(FAILED(hr) || !pEnum) return;
        pEnum->GetCount(&count);
        std::wstring whint = to_w(hint);

        for(ULONG i=0;i<count;i++){
            if(SUCCEEDED(pEnum->Item(i, &pToken)) && pToken){
                WCHAR* desc = nullptr;
                if(SUCCEEDED(SpGetDescription(pToken, &desc)) && desc){
                    std::wstring d(desc);
                    CoTaskMemFree(desc);
                    if(d.find(whint) != std::wstring::npos){
                        voice->SetVoice(pToken);
                        pToken->Release();
                        break;
                    }
                }
                pToken->Release();
            }
        }
        pEnum->Release();
    }
};

std::unique_ptr<VoiceTTS> make_voice_tts(){
    return std::make_unique<SapiVoiceTTS>();
}

} // namespace rael

#else

namespace rael {
class NullVoiceTTS final : public VoiceTTS {
public:
    bool available() const override { return false; }
    void start() override {}
    void stop() override {}
    void speak_async(const std::string&) override {}
    void apply_settings() override {}
};
std::unique_ptr<VoiceTTS> make_voice_tts(){ return std::make_unique<NullVoiceTTS>(); }
} // namespace rael

#endif
