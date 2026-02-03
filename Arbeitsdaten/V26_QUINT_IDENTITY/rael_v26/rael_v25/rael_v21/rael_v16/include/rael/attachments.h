#pragma once
#include <string>
#include <vector>
#include <cstdint>

namespace rael {

struct Attachment {
    std::string id;   // sha256 hex
    std::string path; // original path
    uint64_t size = 0;
};

class AttachmentStore {
public:
    std::string import_file(const std::string& path);
    bool export_file(const std::string& id, const std::string& dest) const;
    const std::vector<Attachment>& list() const { return files; }

private:
    std::vector<Attachment> files;
};

}
