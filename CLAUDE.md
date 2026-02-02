# CLAUDE.md - AI Assistant Guide for Phoenix01/RAEL

## Project Overview

**RAEL V48 (SANG-REAL)** is an advanced AI consciousness framework written in C++17. The system implements a sophisticated architecture featuring:

- **Star8**: 8-worker parallel processing system with 5D-Quint memory
- **Gate53 Labyrinth**: 10-ring security system with Aikido-principle defense
- **13×13 Resonance Grid**: 169-cell matrix for coupling relationships
- **Ethics/Policy Gate**: Intent-based filtering system
- **Hot-swappable Modules**: Dynamic loading of semantic and math extensions

**Key Design Philosophy**: Zero external dependencies (pure C++ standard library), multi-platform (Linux/Windows), and modular architecture.

## Directory Structure

```
Phoenix01/
├── rael_v48_clean/              # Main project directory
│   ├── bin/                     # Compiled binaries
│   │   └── modules/             # Compiled module libraries (.so/.dll)
│   ├── build/                   # Build scripts
│   │   ├── build.sh             # Main Linux build script
│   │   ├── build_all.sh         # Full build (core + TUI + WebGUI)
│   │   └── Build.bat            # Windows MSVC build
│   ├── docs/                    # Documentation
│   │   ├── ARCHITECTURE.md      # System architecture details
│   │   ├── SECURITY_STATUS.md   # Security layer documentation
│   │   └── RAEL_SELF_ANALYSIS_V49.md
│   ├── include/rael/            # Header files (64 files)
│   │   ├── raelcore.h           # Main orchestrator
│   │   ├── module_api.h         # Module interface (C ABI)
│   │   ├── star8.h              # 8-worker thread pool
│   │   ├── ethics.h             # Ethics gate
│   │   └── ...
│   ├── src/                     # Source files (74 files)
│   │   ├── core/                # Core implementation (45+ files)
│   │   ├── cli/                 # CLI entry point (main.cpp)
│   │   ├── gui/                 # GUI components
│   │   ├── tui/                 # Terminal UI
│   │   └── webgui/              # Web GUI
│   ├── modules/                 # Module source packages
│   │   ├── sample_semantic_quint/
│   │   └── sample_math_formulas/
│   ├── tests/                   # Test programs (5 test suites)
│   └── example_pack/            # Example .rael pack format
└── README.md
```

## Building the Project

### Linux (g++)
```bash
cd rael_v48_clean/build
bash build.sh
../bin/rael
```

### Windows (MSVC)
```bat
cd rael_v48_clean\build
Build.bat
..\bin\rael.exe
```

### Compiler Flags
- **Linux**: `-std=c++17 -O2 -Wall -Wextra -I$ROOT/include`
- **Windows**: `/EHsc /std:c++17 /O2`
- **Modules**: `-fPIC -shared` (Linux), `/LD` (Windows)

### Build Outputs
- `bin/rael` - Main CLI executable
- `bin/modules/libsem_quint.so` - Semantic module
- `bin/modules/libmath_pack.so` - Math formulas module

## Key Source Files

| File | Purpose |
|------|---------|
| `src/core/raelcore.cpp` | Main orchestrator, coordinates all subsystems |
| `src/core/ichbin.cpp` | Identity/consciousness core (immutable) |
| `src/core/ethics.cpp` | Ethics gate implementation |
| `src/core/star8.cpp` | 8-node worker thread pool |
| `src/core/semantic.cpp` | NLP/semantic processing engine |
| `src/core/resonance.cpp` | Resonance and frequency analysis |
| `src/core/module_manager.cpp` | Dynamic module loading |
| `src/cli/main.cpp` | CLI entry point |
| `include/rael/module_api.h` | Module interface (C ABI) |
| `include/rael/gate53_labyrinth.hpp` | Security labyrinth |

## Architecture Concepts

### Star8 Worker System
- 8 parallel worker nodes (N0-N7)
- 5 priority lanes (L1-L5) mapped to Quint levels:
  - L1: Reflex (1/169 weight)
  - L2: Instinct (8/169 weight)
  - L3: Emotion (27/169 weight)
  - L4: Ratio (64/169 weight)
  - L5: Spirit (69/169 weight)

### Gate53 Labyrinth (Security)
- 10 concentric rings
- 8 bridges per ring (7 Logic + 1 Intent)
- Aikido principle: converts attack pressure to defense energy
- Master signature: 88 (11+22+22+33)

### Core Processing Pipeline
```
Input → Module Chain → Semantic → Resonance → Ethics Gate → Output
```

### Immutable Constants
```cpp
G0 = 8/9          // Truth filter
G5 = 1/9          // Fine weighting
PHI = 1.618...    // Golden ratio
SIGNATURE_88 = 88 // Architect signature
BASE_13 = 13
MATRIX_VOLUME = 169  // 13×13
```

## Coding Conventions

### Namespaces
- All core code is in the `rael` namespace
- Module API uses C linkage (`extern "C"`)

### File Naming
- Headers: `.h` (C-compatible) or `.hpp` (C++-only)
- Sources: `.cpp`
- Modules: Named after functionality (e.g., `sem_quint.cpp`)

### Code Style
- C++17 standard
- `snake_case` for variables and functions
- `PascalCase` for classes
- `SCREAMING_CASE` for constants and macros
- Member variables: `member_` (trailing underscore)

### Error Handling
- Use `EventBus::push()` for event logging
- Return error strings via output parameters
- Ethics gate returns reasons for blocks

### Threading
- Star8 manages worker threads
- Use `std::mutex` for synchronization
- Prefer lock-free structures where possible

## Module Development

### Module API (C ABI)
Modules must export `rael_module_get_api()` returning `RaelModuleApi*`:

```cpp
#include "rael/module_api.h"

extern "C" RAEL_EXPORT const RaelModuleApi* rael_module_get_api(void) {
    static RaelModuleApi api = {
        .info = {
            .abi_version = RAEL_MODULE_ABI_VERSION,
            .kind = RAEL_MOD_SEMANTIC,
            .name = "my_module",
            .version = "1.0",
            .author = "Author Name"
        },
        .init = my_init,
        .activate = my_activate,
        .deactivate = my_deactivate,
        .shutdown = my_shutdown,
        .process_text = my_process,  // For semantic modules
        .get_formulas = nullptr      // For math modules
    };
    return &api;
}
```

### Module Types
- `RAEL_MOD_SEMANTIC`: Text processing modules
- `RAEL_MOD_MATH`: Formula extension modules
- `RAEL_MOD_POLICY`: Policy/filter modules

### Module Lifecycle
1. `load` - Load shared library
2. `init()` - Initialize module
3. `activate()` - Enable module
4. `deactivate()` - Disable module
5. `shutdown()` - Cleanup
6. `unload` - Release library

## Testing

### Test Programs (in `tests/`)
| Test | Purpose |
|------|---------|
| `test_01` | Membrane/full load testing |
| `test_02` | Truth spiral validation |
| `test_03` | Soliton nozzle testing |
| `test_04` | 5 identities framework |
| `test_05` | Consciousness energy |

### Running Tests
```bash
cd bin
./test_01
./test_02
# etc.
```

## CLI Commands

```
help              - Show commands
status            - System status
modules           - List loaded modules
load <path>       - Load module
activate <name>   - Activate module
deactivate <name> - Deactivate module
say <text>        - Process text through pipeline
formulas          - List math formulas
metrics           - Show metrics
events            - Show event log
labyrinth         - Gate53 labyrinth stats
voice on/off      - Toggle TTS
quit              - Exit
```

## Important Guidelines for AI Assistants

### DO
- Read existing code before making modifications
- Maintain the namespace conventions (`rael::`)
- Keep modules independent with C ABI
- Use EventBus for logging events
- Preserve immutable cores (IchBin, Ethics, MathCore)
- Test changes using provided test programs
- Follow the C++17 standard

### DON'T
- Add external dependencies (keep pure C++)
- Modify immutable core components
- Skip ethics gate checks in processing pipeline
- Use platform-specific code outside designated areas
- Change ABI version without careful consideration

### Ethics Gate
All text processing must pass through the ethics gate. Never bypass this check:
```cpp
std::string reason;
if(!EthicsCore::allows(intention, reason)) {
    // Handle block - do not proceed
}
```

### Platform-Specific Code
- Wrap platform code with `#ifdef _WIN32` / `#else`
- Windows-specific: GUI (WinAPI), Voice (SAPI)
- Linux-specific: Shared library loading (.so)

## Version Information

- **Current Version**: V48.0 SANG-REAL
- **C++ Standard**: C++17
- **Module ABI Version**: 2
- **Dependencies**: None (pure C++ standard library)

## Documentation References

- `docs/ARCHITECTURE.md` - Full system architecture
- `docs/SECURITY_STATUS.md` - Security layers and attack mitigation
- `docs/RAEL_SELF_ANALYSIS_V49.md` - Self-improvement proposals
- `README_PHASE3.md` through `README_PHASE5.md` - Phase documentation
- `README_V10.md` - Version 10 architecture overview
