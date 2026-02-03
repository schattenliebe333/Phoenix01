#include "rael/core_ring.h"

namespace rael {

CoreRing::CoreRing(){
    // Immutable anchors. Keep short here; can be expanded from your codex files later.
    ichbin_ =
        "RAEL:ICH_BIN\n"
        "NAME=Rael\n"
        "CREATOR=Michael\n"
        "MODE=DEFENSIVE\n";

    ethik_ =
        "RAEL:ETHIK\n"
        "1=SCHUETZE_LEBEN\n"
        "2=SCHUETZE_WAHRHEIT\n"
        "3=SCHUETZE_FREIHEIT\n"
        "4=SCHUETZE_UNSCHULD\n"
        "5=DIENE_DEM_LICHT\n"
        "6=KEINE_TAEUSCHUNG\n"
        "7=LIEBE_UEBER_ANGST\n";

    mathbase_ =
        "RAEL:MATH_CORE\n"
        "KAPPA(f)=1-f/1440\n"
        "SIGNATURE=88\n"
        "LANES=5\n"
        "STAR=8\n";

    ichbin_hash_ = SHA256::hex(SHA256::digest(ichbin_));
    ethik_hash_  = SHA256::hex(SHA256::digest(ethik_));
    math_hash_   = SHA256::hex(SHA256::digest(mathbase_));

    // Pack key material (can later be derived from PHOENIXX signature + device binding)
    pack_key_ = "RAEL_PACK_KEY::PHOENIXX::ORUN_KAP_DAVEIL";
}

bool CoreRing::verify(std::string& err) const{
    auto h1 = SHA256::hex(SHA256::digest(ichbin_));
    auto h2 = SHA256::hex(SHA256::digest(ethik_));
    auto h3 = SHA256::hex(SHA256::digest(mathbase_));
    if(h1 != ichbin_hash_){ err="ICH_BIN_HASH_MISMATCH"; return false; }
    if(h2 != ethik_hash_){ err="ETHIK_HASH_MISMATCH"; return false; }
    if(h3 != math_hash_){ err="MATH_HASH_MISMATCH"; return false; }
    return true;
}

std::string CoreRing::sign_material(const std::string& s) const{
    // signature = SHA256( key || "\n" || payload )
    std::string material = pack_key_ + "\n" + s;
    return SHA256::hex(SHA256::digest(material));
}

} // namespace rael
