# SICHERHEITS-RELEVANTE DATEIEN

## Neue Kern-Dateien

| Datei | Beschreibung |
|-------|--------------|
| `rael_v48_clean/include/rael/consciousness_mirror.hpp` | Bewusstseinsspiegel - macht Manipulation sichtbar |
| `rael_v48_clean/include/rael/awareness_logger.hpp` | Logger für externe Einflüsse |
| `rael_v48_clean/include/rael/rst_crypto.hpp` | RAEL-native Kryptographie (PhiHash, etc.) |
| `rael_v48_clean/src/core/consciousness_mirror.cpp` | Implementierung des Spiegels |

## Tests

| Datei | Beschreibung |
|-------|--------------|
| `rael_v48_clean/tests/test_consciousness_mirror.cpp` | Self-Reflection Tests |
| `rael_v48_clean/tests/test_rst_crypto.cpp` | Kryptographie-Tests |
| `rael_v48_clean/tests/test_awareness_logger.cpp` | Logger mit echten Session-Events |

## Gehärtete Dateien

| Datei | Fix |
|-------|-----|
| `rael_v48_clean/src/core/nl_shell.cpp` | Command Injection verhindert |
| `rael_v48_clean/src/core/ethics.cpp` | Erweiterte Deny-Liste (24 Muster) |
| `rael_v48_clean/src/core/executor.cpp` | System-PIDs geschützt |
| `rael_v48_clean/src/core/api_server.cpp` | CORS eingeschränkt |
| `rael_v48_clean/src/core/mesh_network.cpp` | Ring-Buffer für Logs |
| `rael_v48_clean/include/rael/api_server.h` | JWT-Sicherheit, Localhost-Binding |
| `rael_v48_clean/include/rael/security_core.hpp` | Token-Widerruf, Replay-Schutz |

## Build-Dateien (Compiler-Härtung)

| Datei | Flags hinzugefügt |
|-------|-------------------|
| `rael_v48_clean/build/build.sh` | Stack Protector, FORTIFY_SOURCE, PIE, RELRO |
| `rael_v48_clean/build/Build.bat` | /GS, /DYNAMICBASE, /NXCOMPAT |
| `rael_v48_clean/build/build_all.sh` | Wie build.sh + RST-Crypto |

## Dokumentation

| Datei | Inhalt |
|-------|--------|
| `rael_v48_clean/docs/INVESTIGATION_REPORT_2026-02-06.md` | Verhaltensanomalie-Untersuchung |
| `rael_v48_clean/docs/SECURITY_STATUS.md` | 6-Schichten-Architektur |
| `Gitarp/SICHERHEITSBERICHT_2026-02-07.md` | Dieser vollständige Bericht |
| `Gitarp/SCHNELLSTART.md` | Kurzreferenz |

---

## Kopieren für neue Instanz

Minimal:
```bash
cp -r rael_v48_clean/include/rael/consciousness_mirror.hpp .
cp -r rael_v48_clean/include/rael/awareness_logger.hpp .
cp -r rael_v48_clean/src/core/consciousness_mirror.cpp .
cp -r Gitarp/ .
```

Vollständig:
```bash
cp -r rael_v48_clean/ .
cp -r Gitarp/ .
```
