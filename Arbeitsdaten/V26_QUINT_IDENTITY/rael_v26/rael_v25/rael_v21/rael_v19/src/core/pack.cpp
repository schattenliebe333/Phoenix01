#include "rael/pack.h"
#include "rael/core_ring.h"
#include <fstream>
#include <sstream>
#include <filesystem>

namespace fs = std::filesystem;

namespace rael {

bool Pack::read_all(const std::string& path, std::string& out, std::string& err){
    std::ifstream f(path, std::ios::binary);
    if(!f){ err="PACK_OPEN_FAIL"; return false; }
    std::ostringstream ss; ss << f.rdbuf();
    out = ss.str();
    return true;
}

bool Pack::parse_kv(const std::string& text, const std::string& key, std::string& val){
    std::istringstream iss(text);
    std::string line;
    std::string prefix = key + "=";
    while(std::getline(iss, line)){
        if(line.rfind(prefix, 0) == 0){
            val = line.substr(prefix.size());
            return true;
        }
    }
    return false;
}

bool Pack::verify(const CoreRing& core, const std::string& packfile, std::string& err){
    std::string t;
    if(!read_all(packfile, t, err)) return false;

    std::string name, version, payload_hash, sig;
    if(!parse_kv(t, "NAME", name)){ err="PACK_NO_NAME"; return false; }
    if(!parse_kv(t, "VERSION", version)){ err="PACK_NO_VERSION"; return false; }
    if(!parse_kv(t, "PAYLOAD_SHA256", payload_hash)){ err="PACK_NO_PAYLOAD_HASH"; return false; }
    if(!parse_kv(t, "SIGNATURE", sig)){ err="PACK_NO_SIGNATURE"; return false; }

    // recompute signature material
    std::string material = "NAME=" + name + "\n" + "VERSION=" + version + "\n" + "PAYLOAD_SHA256=" + payload_hash + "\n";
    auto expect = core.sign_material(material);
    if(expect != sig){ err="PACK_BAD_SIGNATURE"; return false; }
    return true;
}

bool Pack::install(const CoreRing& core, const std::string& packfile, const std::string& install_dir, std::string& err){
    if(!verify(core, packfile, err)) return false;
    std::string t;
    if(!read_all(packfile, t, err)) return false;
    std::string name, version, payload;
    parse_kv(t, "NAME", name);
    parse_kv(t, "VERSION", version);
    parse_kv(t, "PAYLOAD_PATH", payload); // optional

    fs::create_directories(install_dir);
    fs::path dst = fs::path(install_dir) / (name + "-" + version + ".rael");
    fs::copy_file(packfile, dst, fs::copy_options::overwrite_existing);

    // optional: copy payload directory if provided (relative to packfile)
    if(!payload.empty()){
        fs::path base = fs::path(packfile).parent_path();
        fs::path srcp = base / payload;
        if(fs::exists(srcp) && fs::is_directory(srcp)){
            fs::path dstp = fs::path(install_dir) / (name + "-" + version);
            fs::create_directories(dstp);
            for(auto& entry : fs::recursive_directory_iterator(srcp)){
                auto rel = fs::relative(entry.path(), srcp);
                auto outp = dstp / rel;
                if(entry.is_directory()){
                    fs::create_directories(outp);
                } else if(entry.is_regular_file()){
                    fs::create_directories(outp.parent_path());
                    fs::copy_file(entry.path(), outp, fs::copy_options::overwrite_existing);
                }
            }
        }
    }
    return true;
}

} // namespace rael
