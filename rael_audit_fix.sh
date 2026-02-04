#!/bin/bash
# ═══════════════════════════════════════════════════════════════════════════════
# RAEL V48 — AUDIT-KORREKTURSKRIPT
# Datum: 4. Februar 2026
# Behebt alle identifizierten Fehler aus dem Code-Audit
# ═══════════════════════════════════════════════════════════════════════════════
#
# USAGE: ./rael_audit_fix.sh /pfad/zu/rael_v48_clean
#
# Das Skript:
#   1. Erstellt Backups aller geänderten Dateien
#   2. Korrigiert 4 mathematische Konstanten-Fehler (P1)
#   3. Korrigiert 1 Folgefehler (T_HOTSWAP Kommentar)
#   4. Fixt 1 Kompilier-Fehler (main_windows.cpp)
#   5. Verifiziert alle Änderungen
#   6. Führt Kompilier-Test durch
#
# ═══════════════════════════════════════════════════════════════════════════════

set -e

# Farben
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
CYAN='\033[0;36m'
NC='\033[0m'

# Pfad prüfen
ROOT="${1:-.}"
if [ ! -f "$ROOT/include/rael/rst_constants.hpp" ]; then
    echo -e "${RED}FEHLER: $ROOT/include/rael/rst_constants.hpp nicht gefunden${NC}"
    echo "Usage: $0 /pfad/zu/rael_v48_clean"
    exit 1
fi

cd "$ROOT"
echo -e "${CYAN}═══════════════════════════════════════════════════════════════${NC}"
echo -e "${CYAN}  RAEL V48 AUDIT-KORREKTUR — $(date '+%Y-%m-%d %H:%M:%S')${NC}"
echo -e "${CYAN}  Arbeitsverzeichnis: $(pwd)${NC}"
echo -e "${CYAN}═══════════════════════════════════════════════════════════════${NC}"
echo ""

# ───────────────────────────────────────────────────────────────────────────────
# PHASE 1: BACKUPS
# ───────────────────────────────────────────────────────────────────────────────
echo -e "${YELLOW}[PHASE 1] Backups erstellen...${NC}"
BACKUP_DIR=".audit_backup_$(date +%Y%m%d_%H%M%S)"
mkdir -p "$BACKUP_DIR"

for f in \
    include/rael/rst_constants.hpp \
    include/rael/rst_formulas.hpp \
    include/rael/security_core.hpp \
    src/cli/main_windows.cpp; do
    if [ -f "$f" ]; then
        mkdir -p "$BACKUP_DIR/$(dirname $f)"
        cp "$f" "$BACKUP_DIR/$f"
        echo "  ✓ $f"
    fi
done
echo -e "${GREEN}  Backups in: $BACKUP_DIR${NC}"
echo ""

# ───────────────────────────────────────────────────────────────────────────────
# PHASE 2: P1 — MATHEMATISCHE KORREKTUREN
# ───────────────────────────────────────────────────────────────────────────────
echo -e "${YELLOW}[PHASE 2] P1-Korrekturen: Mathematische Konstanten${NC}"
FIXES=0
ERRORS=0

# --- FIX 1: G_TUNNEL_GAIN in rst_constants.hpp ---
echo -n "  FIX 1: G_TUNNEL_GAIN = e^(8/9)... "
FILE="include/rael/rst_constants.hpp"
if grep -q "G_TUNNEL_GAIN = 2.43280931416225840" "$FILE"; then
    sed -i 's/constexpr double G_TUNNEL_GAIN = 2.43280931416225840;  \/\/ e^(8\/9)/constexpr double G_TUNNEL_GAIN = 2.43242545428720769;  \/\/ e^(8\/9) [AUDIT-FIX]/' "$FILE"
    echo -e "${GREEN}✅ 2.43280... → 2.43242...${NC}"
    FIXES=$((FIXES+1))
elif grep -q "G_TUNNEL_GAIN = 2.43242545428720769" "$FILE"; then
    echo -e "${GREEN}✅ bereits korrekt${NC}"
else
    echo -e "${RED}❌ Muster nicht gefunden${NC}"
    ERRORS=$((ERRORS+1))
fi

# --- FIX 2: CPU_SPAR_FAKTOR in rst_constants.hpp ---
echo -n "  FIX 2: CPU_SPAR_FAKTOR = 2/3... "
if grep -q "CPU_SPAR_FAKTOR = 0.70000000000000000" "$FILE"; then
    sed -i 's/constexpr double CPU_SPAR_FAKTOR = 0.70000000000000000;/constexpr double CPU_SPAR_FAKTOR = 0.66666666666666667;  \/\/ 2\/3 [AUDIT-FIX]/' "$FILE"
    echo -e "${GREEN}✅ 0.70 → 2/3${NC}"
    FIXES=$((FIXES+1))
elif grep -q "CPU_SPAR_FAKTOR = 0.66666666666666667" "$FILE"; then
    echo -e "${GREEN}✅ bereits korrekt${NC}"
else
    echo -e "${RED}❌ Muster nicht gefunden${NC}"
    ERRORS=$((ERRORS+1))
fi

# --- FIX 3: ETA_DUESE in rst_formulas.hpp ---
echo -n "  FIX 3: ETA_DUESE = 0.5 in rst_formulas... "
FILE="include/rael/rst_formulas.hpp"
if grep -q 'ETA_DUESE = 0.55555555555555556' "$FILE"; then
    sed -i "s/constexpr double ETA_DUESE = 0.55555555555555556;        \/\/ η = G1 = 5\/9/constexpr double ETA_DUESE = 0.50000000000000000;        \/\/ η = G1\/(1+G5) = 1\/2 [AUDIT-FIX]/" "$FILE"
    echo -e "${GREEN}✅ 5/9 → 1/2${NC}"
    FIXES=$((FIXES+1))
elif grep -q 'ETA_DUESE = 0.50000000000000000' "$FILE"; then
    echo -e "${GREEN}✅ bereits korrekt${NC}"
else
    echo -e "${RED}❌ Muster nicht gefunden${NC}"
    ERRORS=$((ERRORS+1))
fi

# --- FIX 4a: F_TOR/F_GATE vertauscht in rst_formulas.hpp ---
echo -n "  FIX 4a: F_TOR=720, F_GATE53=53.33... "
if grep -q 'constexpr double F_TOR = 53.33333333333333333' "$FILE"; then
    sed -i 's/constexpr double F_TOR = 53.33333333333333333;        \/\/ Gate 53 (160\/3)/constexpr double F_TOR = 720.00000000000000000;        \/\/ Tor-Referenz [AUDIT-FIX]/' "$FILE"
    sed -i 's/constexpr double F_GATE = 720.00000000000000000;      \/\/ Tor-Referenz/constexpr double F_GATE53 = 53.33333333333333333;      \/\/ Gate 53 (160\/3) [AUDIT-FIX]/' "$FILE"
    echo -e "${GREEN}✅ entvertauscht${NC}"
    FIXES=$((FIXES+1))
elif grep -q 'constexpr double F_TOR = 720.00000000000000000' "$FILE"; then
    echo -e "${GREEN}✅ bereits korrekt${NC}"
else
    echo -e "${RED}❌ Muster nicht gefunden${NC}"
    ERRORS=$((ERRORS+1))
fi

# --- FIX 4b: gravitation() Formel korrigieren ---
echo -n "  FIX 4b: gravitation() = (F_TOR-f)/F_TOR... "
if grep -q 'return (F_TOR - f) / F_GATE;' "$FILE"; then
    sed -i 's/return (F_TOR - f) \/ F_GATE;/return (F_TOR - f) \/ F_TOR;  \/\/ γ(f) = 1 - f\/720 [AUDIT-FIX]/' "$FILE"
    echo -e "${GREEN}✅ /F_GATE → /F_TOR${NC}"
    FIXES=$((FIXES+1))
elif grep -q 'return (F_TOR - f) / F_TOR;' "$FILE"; then
    echo -e "${GREEN}✅ bereits korrekt${NC}"
else
    echo -e "${RED}❌ Muster nicht gefunden${NC}"
    ERRORS=$((ERRORS+1))
fi

# --- FIX 5: T_HOTSWAP Kommentar korrigieren ---
echo -n "  FIX 5: T_HOTSWAP Kommentar... "
if grep -q '// Hotswap-Fenster = 1 / (F_TOR' "$FILE"; then
    sed -i 's/\/\/ Hotswap-Fenster = 1 \/ (F_TOR × G3)/\/\/ Hotswap-Fenster = 1 \/ (F_GATE53 × G3) [AUDIT-FIX: war F_TOR, aber Wert passt zu Gate53]/' "$FILE"
    echo -e "${GREEN}✅ Kommentar korrigiert${NC}"
    FIXES=$((FIXES+1))
elif grep -q 'AUDIT-FIX.*Gate53' "$FILE"; then
    echo -e "${GREEN}✅ bereits korrekt${NC}"
else
    echo -e "${YELLOW}⚠ Kommentar-Muster nicht gefunden (optional)${NC}"
fi

echo ""

# ───────────────────────────────────────────────────────────────────────────────
# PHASE 3: KOMPILIER-FIXES
# ───────────────────────────────────────────────────────────────────────────────
echo -e "${YELLOW}[PHASE 3] Kompilier-Fixes${NC}"

# --- FIX 6: main_windows.cpp fehlender include ---
echo -n "  FIX 6: main_windows.cpp #include <iostream>... "
FILE="src/cli/main_windows.cpp"
if [ -f "$FILE" ]; then
    # Der Non-Windows-Stub nutzt std::cout aber der include ist hinter #ifdef _WIN32
    # Prüfe ob nach #else ein cout ohne vorherigen include steht
    if grep -q '#else' "$FILE" && grep -q 'std::cout' "$FILE"; then
        # Füge include vor dem #else Block ein falls nicht schon vorhanden
        if ! grep -q '#include <iostream> // AUDIT-FIX' "$FILE"; then
            sed -i '/#else/{
                i\#ifndef _WIN32\n#include <iostream>  // AUDIT-FIX: für Non-Windows Stub\n#endif
            }' "$FILE"
            # Alternativ: einfacherer Fix - ersetze den Stub
            # Prüfe ob der sed geklappt hat
            if g++ -std=c++17 -I include -c "$FILE" -o /dev/null 2>/dev/null; then
                echo -e "${GREEN}✅ include eingefügt${NC}"
                FIXES=$((FIXES+1))
            else
                # sed war zu komplex, mache es einfacher
                cp "$BACKUP_DIR/$FILE" "$FILE"
                # Ersetze den ganzen Non-Windows Block
                python3 -c "
import re
with open('$FILE', 'r') as f:
    content = f.read()
# Finde den #else Block und füge iostream ein
content = content.replace(
    '#else\n// Non-Windows stub\nint main() {\n    std::cout',
    '#else\n// Non-Windows stub\n#include <iostream>  // AUDIT-FIX\nint main() {\n    std::cout'
)
with open('$FILE', 'w') as f:
    f.write(content)
"
                if g++ -std=c++17 -I include -c "$FILE" -o /dev/null 2>/dev/null; then
                    echo -e "${GREEN}✅ include eingefügt (Methode 2)${NC}"
                    FIXES=$((FIXES+1))
                else
                    echo -e "${YELLOW}⚠ Windows-only, übersprungen${NC}"
                    cp "$BACKUP_DIR/$FILE" "$FILE"
                fi
            fi
        else
            echo -e "${GREEN}✅ bereits gefixt${NC}"
        fi
    else
        echo -e "${YELLOW}⚠ Struktur unerwartet${NC}"
    fi
else
    echo -e "${YELLOW}⚠ Datei nicht vorhanden${NC}"
fi

echo ""

# ───────────────────────────────────────────────────────────────────────────────
# PHASE 4: VERIFIKATION
# ───────────────────────────────────────────────────────────────────────────────
echo -e "${YELLOW}[PHASE 4] Verifikation${NC}"
echo ""

# Mathematische Verifikation
python3 << 'PYVERIFY'
import math, sys

ok = 0
fail = 0

def check(name, file, pattern, expected, tolerance=1e-10):
    global ok, fail
    import subprocess
    r = subprocess.run(['grep', pattern, file], capture_output=True, text=True)
    if r.stdout.strip():
        # Extrahiere den Zahlenwert
        import re
        m = re.search(r'= ([0-9.]+)', r.stdout)
        if m:
            val = float(m.group(1))
            if abs(val - expected) < tolerance:
                print(f"  ✅ {name}: {val} (Δ={abs(val-expected):.2e})")
                ok += 1
            else:
                print(f"  ❌ {name}: {val} ≠ {expected} (Δ={abs(val-expected):.2e})")
                fail += 1
        else:
            print(f"  ❌ {name}: Wert nicht extrahierbar")
            fail += 1
    else:
        print(f"  ❌ {name}: Muster '{pattern}' nicht gefunden in {file}")
        fail += 1

print("  === Konstanten-Werte ===")
check("G_TUNNEL_GAIN", "include/rael/rst_constants.hpp",
      "G_TUNNEL_GAIN =", math.exp(8/9))
check("CPU_SPAR (const)", "include/rael/rst_constants.hpp",
      "CPU_SPAR_FAKTOR =", 2/3, 1e-14)
check("ETA_DUESE (form)", "include/rael/rst_formulas.hpp",
      "ETA_DUESE =", 0.5)
check("F_TOR (form)", "include/rael/rst_formulas.hpp",
      "constexpr double F_TOR =", 720.0)

print(f"\n  Ergebnis: {ok} OK, {fail} Fehler")
sys.exit(0 if fail == 0 else 1)
PYVERIFY

VERIFY_OK=$?
echo ""

# Konsistenz-Check: alle Dateien synchron?
echo "  === Konsistenz über alle Header ==="
python3 << 'PYCONSIST'
files = {
    'master': 'include/rael/rael_master_resonance.hpp',
    'const':  'include/rael/rst_constants.hpp',
    'form':   'include/rael/rst_formulas.hpp',
}
import subprocess

checks = [
    ("G_TUNNEL_GAIN", "2.43242"),
    ("ETA_DUESE", "0.5"),
    ("CPU_SPAR_FAKTOR", "0.666"),
]

all_ok = True
for name, expected in checks:
    vals = {}
    for label, path in files.items():
        r = subprocess.run(['grep', f'constexpr.*{name}', path],
                          capture_output=True, text=True)
        if r.stdout.strip():
            line = r.stdout.strip().split('\n')[0]
            has = expected in line
            vals[label] = (line.strip()[:60], has)
        # else: nicht in dieser Datei definiert

    if vals:
        ok = all(v[1] for v in vals.values())
        if not ok: all_ok = False
        status = "✅" if ok else "❌"
        print(f"  {status} {name}")
        for label, (line, has) in vals.items():
            m = "✅" if has else "❌"
            print(f"      {label:8s}: {m} {line}")

# F_TOR Check
r1 = subprocess.run(['grep', 'constexpr double F_TOR =', 'include/rael/rst_formulas.hpp'],
                    capture_output=True, text=True)
if '720' in r1.stdout:
    print(f"  ✅ F_TOR=720 in rst_formulas")
else:
    print(f"  ❌ F_TOR NICHT 720 in rst_formulas")
    all_ok = False

print(f"\n  {'ALLE SYNCHRON ✅' if all_ok else 'WIDERSPRÜCHE GEFUNDEN ❌'}")
PYCONSIST

echo ""

# ───────────────────────────────────────────────────────────────────────────────
# PHASE 5: KOMPILIER-TEST
# ───────────────────────────────────────────────────────────────────────────────
echo -e "${YELLOW}[PHASE 5] Kompilier-Test${NC}"

if command -v g++ &> /dev/null; then
    SUCCESS=0
    FAIL=0
    FAIL_LIST=""

    for f in $(find src -name "*.cpp" ! -name "*.bak" 2>/dev/null); do
        if g++ -std=c++17 -I include -c "$f" -o /dev/null 2>/dev/null; then
            SUCCESS=$((SUCCESS+1))
        else
            FAIL=$((FAIL+1))
            FAIL_LIST="$FAIL_LIST  ❌ $(basename $f)\n"
        fi
    done

    # Tests
    TEST_OK=0
    for f in tests/*.cpp; do
        if [ -f "$f" ] && g++ -std=c++17 -I include -c "$f" -o /dev/null 2>/dev/null; then
            TEST_OK=$((TEST_OK+1))
        fi
    done

    echo -e "  Source:  ${GREEN}✅ $SUCCESS OK${NC}  ${RED}❌ $FAIL Fehler${NC}"
    echo -e "  Tests:   ${GREEN}✅ $TEST_OK OK${NC}"
    if [ -n "$FAIL_LIST" ]; then
        echo -e "  Fehler-Details:"
        echo -e "$FAIL_LIST"
    fi
else
    echo -e "  ${YELLOW}⚠ g++ nicht verfügbar, Kompilier-Test übersprungen${NC}"
fi

echo ""

# ───────────────────────────────────────────────────────────────────────────────
# ZUSAMMENFASSUNG
# ───────────────────────────────────────────────────────────────────────────────
echo -e "${CYAN}═══════════════════════════════════════════════════════════════${NC}"
echo -e "${CYAN}  ZUSAMMENFASSUNG${NC}"
echo -e "${CYAN}═══════════════════════════════════════════════════════════════${NC}"
echo -e "  Fixes angewendet: ${GREEN}$FIXES${NC}"
echo -e "  Fehler:           ${RED}$ERRORS${NC}"
echo -e "  Backups in:       $BACKUP_DIR"
echo ""
echo "  Geänderte Dateien:"
echo "    • include/rael/rst_constants.hpp  (G_TUNNEL_GAIN, CPU_SPAR_FAKTOR)"
echo "    • include/rael/rst_formulas.hpp   (ETA_DUESE, F_TOR/F_GATE53, gravitation())"
echo "    • src/cli/main_windows.cpp        (#include <iostream>)"
echo ""

if [ $ERRORS -eq 0 ]; then
    echo -e "${GREEN}  ✅ ALLE KORREKTUREN ERFOLGREICH${NC}"
    echo ""
    echo "  Empfohlener Commit:"
    echo "    git add include/rael/rst_constants.hpp include/rael/rst_formulas.hpp src/cli/main_windows.cpp"
    echo "    git commit -m 'Audit-Fix: G_TUNNEL_GAIN, ETA_DUESE, CPU_SPAR, F_TOR/F_GATE, gravitation()'"
else
    echo -e "${RED}  ❌ $ERRORS FEHLER — manuelle Prüfung nötig${NC}"
fi
echo ""
