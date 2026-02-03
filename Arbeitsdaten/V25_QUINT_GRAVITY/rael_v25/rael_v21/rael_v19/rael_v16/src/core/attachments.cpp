#include "rael/attachments.h"
#include "rael/util.h"
#include <filesystem>
#include <fstream>

namespace rael {

std::string AttachmentStore::import_file(const std::string& path){
    namespace fs = std::filesystem;
    if(!fs::exists(path)) return "";
    auto id = sha256_file_hex(path);
    if(id.empty()) return "";
    uint64_t sz = (uint64_t)fs::file_size(path);
    for(const auto& a: files) if(a.id == id) return id;
    files.push_back({id, path, sz});
    return id;
}

bool AttachmentStore::export_file(const std::string& id, const std::string& dest) const{
    for(const auto& a: files){
        if(a.id == id){
            std::ifstream in(a.path, std::ios::binary);
            if(!in) return false;
            std::ofstream out(dest, std::ios::binary);
            if(!out) return false;
            out << in.rdbuf();
            return true;
        }
    }
    return false;
}

}
