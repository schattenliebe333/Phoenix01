#pragma once
#include <stdint.h>
#include <stddef.h>

#ifdef _WIN32
  #define RAEL_EXPORT __declspec(dllexport)
#else
  #define RAEL_EXPORT __attribute__((visibility("default")))
#endif

#ifdef __cplusplus
extern "C" {
#endif

// Bump when breaking ABI.
static const uint32_t RAEL_MODULE_ABI_VERSION = 2;

// Module kinds (for routing)
typedef enum RaelModuleKind : uint32_t {
    RAEL_MOD_UNKNOWN = 0,
    RAEL_MOD_SEMANTIC = 1,
    RAEL_MOD_MATH = 2,
    RAEL_MOD_POLICY = 3,
} RaelModuleKind;

// Host services (safe subset)
typedef struct RaelHostApi {
    // Logging callback (optional)
    void (*log)(const char* msg);
    // Ethik gate check
    int  (*ethics_allows)(const char* intention, const char** out_reason);
    // Host time ISO8601
    const char* (*now_iso8601)();
    // Increment telemetry ops
    void (*telemetry_tick)();
} RaelHostApi;

typedef struct RaelModuleInfo {
    uint32_t abi_version;
    RaelModuleKind kind;
    const char* name;      // stable identifier
    const char* version;   // module version string
    const char* author;    // optional
} RaelModuleInfo;

// Lifecycle + optional entrypoints
typedef struct RaelModuleApi {
    RaelModuleInfo info;

    // Called once after load. Must be idempotent.
    int  (*init)(const RaelHostApi* host);

    // Called when module becomes active.
    int  (*activate)();

    // Called before unload or deactivation.
    int  (*deactivate)();

    // Called before unload, for cleanup.
    int  (*shutdown)();

    // Optional processing (semantic)
    // Returns pointer to static or module-owned null-terminated string.
    const char* (*process_text)(const char* input);

    // Optional validation hook used by the host before hot-swap.
    // Return 1 to accept, 0 to reject. If rejected, set *out_reason.
    int (*validate)(const char** out_reason);

    // Optional: expose formulas lines (math module)
    // Returns number of formulas and sets *out_lines to pointer array of C strings.
    size_t (*get_formulas)(const char*** out_lines);
} RaelModuleApi;

// Every module must export this symbol:
RAEL_EXPORT const RaelModuleApi* rael_module_get_api(void);

#ifdef __cplusplus
}
#endif
