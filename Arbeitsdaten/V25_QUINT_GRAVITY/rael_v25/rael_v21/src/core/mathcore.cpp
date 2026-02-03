#include "rael/mathcore.h"
// MathCore: All methods are now constexpr/inline in header
// This file kept for build system compatibility
namespace rael {
    // Verify master signature at compile time
    static_assert(MathCore::verify_master_signature(), "Master signature must be 88");
}
