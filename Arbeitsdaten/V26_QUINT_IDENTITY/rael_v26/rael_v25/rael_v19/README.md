# Rael – Phase 1 (C++ only) – Hot-Swap Module Packs

**No external libraries.**
This phase adds a **real module lifecycle** and **dynamic loading**:
- Load shared libraries (`.so` / `.dll`)
- Validate ABI + module kind
- Activate/Deactivate/Unload
- Semantic module packs can transform input before the host semantic engine
- Math module packs can add formulas (append-only)

## Build (Linux)
```bash
cd build
bash build.sh
../bin/rael
```

## Build (Windows, MSVC)
Open **x64 Native Tools Command Prompt for VS**:
```bat
cd build
Build.bat
..\bin\rael.exe
```

## Run + Load sample modules
After build you have:
- Linux: `bin/modules/libsem_quint.so`, `bin/modules/libmath_pack.so`
- Windows: `bin\modules\sem_quint.dll`, `bin\modules\math_pack.dll`

CLI:
```
modules
load bin/modules/libsem_quint.so
activate sem_quint
say hello world
load bin/modules/libmath_pack.so
activate math_pack
formulas
```

## Safety Notes
- Immutable cores (IchBin/Ethik/MathCore) remain in the host binary.
- Modules are **optional** and must be explicitly loaded + activated by you.
- Ethik gate remains enforced at the host level.


## Global ImprovementBus
- Any component can emit structured improvement suggestions (EVT|IMPROVE...).
- Stored in C:\ProgramData\RAEL\improvements.jsonl (Windows).
- CLI: `improve top10 [low|mid|high]` to generate roadmap suggestions.
