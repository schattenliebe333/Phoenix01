#pragma once
#include <string>
#include <vector>
#include "rael/sha256.h"

namespace rael {

class CoreRing {
public:
    CoreRing();

    // immutable text anchors (compiled-in)
    const std::string& ich_bin() const { return ichbin_; }
    const std::string& ethik_codex() const { return ethik_; }
    const std::string& math_base() const { return mathbase_; }

    // integrity
    bool verify(std::string& err) const;

    // pack signing key (compiled-in). Not exposed directly.
    std::string sign_material(const std::string& s) const;

private:
    std::string ichbin_;
    std::string ethik_;
    std::string mathbase_;

    std::string ichbin_hash_;
    std::string ethik_hash_;
    std::string math_hash_;

    std::string pack_key_; // compiled-in secret material (not printed)
};

} // namespace rael
