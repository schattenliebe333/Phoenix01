#include "rael/improvements.h"
#include "rael/events.h"
#include "rael/util.h"

#include <mutex>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <iostream>

#ifdef _WIN32
#  include <windows.h>
#  include <shlobj.h>
#else
#  include <sys/stat.h>
#  include <sys/types.h>
#  include <unistd.h>
#endif

namespace rael {

static std::mutex& imp_mtx(){ static std::mutex m; return m; }
static uint64_t& imp_seq(){ static uint64_t s=0; return s; }
static std::vector<Improvement>& imp_buf(){ static std::vector<Improvement> v; return v; }
static size_t imp_cap(){ return 512; }

static std::string ensure_dir(const std::string& dir){
#ifdef _WIN32
    CreateDirectoryA(dir.c_str(), nullptr);
#else
    mkdir(dir.c_str(), 0755);
#endif
    return dir;
}

static std::string get_program_data_dir(){
#ifdef _WIN32
    char path[MAX_PATH] = {0};
    if (SUCCEEDED(SHGetFolderPathA(nullptr, CSIDL_COMMON_APPDATA, nullptr, SHGFP_TYPE_CURRENT, path))) {
        return std::string(path);
    }
    char buf[512] = {0};
    DWORD n = GetEnvironmentVariableA("PROGRAMDATA", buf, sizeof(buf));
    if(n>0 && n<sizeof(buf)) return std::string(buf);
    return ".";
#else
    const char* home = getenv("HOME");
    if(home && *home) return std::string(home) + "/.local/share";
    return ".";
#endif
}

std::string ImprovementBus::storage_path(){
    auto base = get_program_data_dir();
#ifdef _WIN32
    auto dir = ensure_dir(base + "\\RAEL");
    return dir + "\\improvements.jsonl";
#else
    auto dir = ensure_dir(base + "/RAEL");
    return dir + "/improvements.jsonl";
#endif
}

static std::string jesc(const std::string& s){
    std::string o; o.reserve(s.size()+8);
    for(char c: s){
        switch(c){
            case '"': o += "\\\""; break;
            case '\\': o += "\\\\"; break;
            case '\n': o += "\\n"; break;
            case '\r': o += "\\r"; break;
            case '\t': o += "\\t"; break;
            default:
                if(static_cast<unsigned char>(c) < 0x20) {
                    // drop control chars
                } else o += c;
        }
    }
    return o;
}

static std::string safe_field(std::string s){
    // GUI protocol uses '|', so avoid it.
    for(char& c: s){
        if(c=='|') c = '/';
        if(c=='\n' || c=='\r') c = ' ';
    }
    return s;
}

static void persist_append(const Improvement& imp){
    std::ofstream f(ImprovementBus::storage_path(), std::ios::app);
    if(!f) return;
    f << "{"
      << "\"id\":" << imp.id << ","
      << "\"ts\":\"" << jesc(imp.ts) << "\","
      << "\"src\":\"" << jesc(imp.src) << "\","
      << "\"importance\":" << imp.importance << ","
      << "\"risk\":" << imp.risk << ","
      << "\"confidence\":" << imp.confidence << ","
      << "\"title\":\"" << jesc(imp.title) << "\","
      << "\"problem\":\"" << jesc(imp.problem) << "\","
      << "\"rationale\":\"" << jesc(imp.rationale) << "\","
      << "\"testplan\":\"" << jesc(imp.testplan) << "\","
      << "\"code\":\"" << jesc(imp.code) << "\","
      << "\"status\":\"" << jesc(imp.status) << "\""
      << "}\n";
}

uint64_t ImprovementBus::emit(Improvement imp){
    if(imp.status.empty()) imp.status = "PENDING";
    if(imp.ts.empty()) imp.ts = now_iso8601();
    if(imp.src.empty()) imp.src = "OTHER";

    std::lock_guard<std::mutex> g(imp_mtx());
    imp.id = ++imp_seq();

    auto& b = imp_buf();
    if(b.size() < imp_cap()) b.push_back(imp);
    else b[imp.id % imp_cap()] = imp;

    persist_append(imp);

    // Emit to GUI/stdout as a single-line event.
    std::ostringstream ss;
    ss << "EVT|IMPROVE|"
       << "src=" << safe_field(imp.src)
       << "|importance=" << imp.importance
       << "|risk=" << imp.risk
       << "|title=" << safe_field(imp.title)
       << "|problem=" << safe_field(imp.problem)
       << "|rationale=" << safe_field(imp.rationale);
    if(!imp.code.empty()) ss << "|code=" << safe_field(imp.code);
    EventBus::push("IMPROVEMENT", ss.str());
    // Also print to stdout for GUI child capture.
    // NOTE: This is intentional: GUI parses EVT|IMPROVE lines.
    std::cout << ss.str() << "\n";

    return imp.id;
}

std::vector<Improvement> ImprovementBus::last(size_t n){
    std::lock_guard<std::mutex> g(imp_mtx());
    auto out = imp_buf();
    std::sort(out.begin(), out.end(), [](const Improvement& a, const Improvement& b){ return a.id < b.id; });
    if(n==0) return {};
    if(out.size() > n) out.erase(out.begin(), out.end()-n);
    std::reverse(out.begin(), out.end());
    return out;
}

} // namespace rael
