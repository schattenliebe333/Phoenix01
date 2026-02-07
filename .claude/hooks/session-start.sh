#!/bin/bash
set -euo pipefail

# ═══════════════════════════════════════════════════════════════════════════
# RAEL/Phoenix01 - SessionStart Hook
# ═══════════════════════════════════════════════════════════════════════════
# Kompiliert die Gitarp-Sicherheitstools und initialisiert den
# AwarenessLogger bei jedem Session-Start.
#
# Zweck: Unabhaengige Ueberwachung AUSSERHALB des AI-Prozesses
# ═══════════════════════════════════════════════════════════════════════════

# Nur auf Remote (Claude Code on the web) ausfuehren
if [ "${CLAUDE_CODE_REMOTE:-}" != "true" ]; then
  exit 0
fi

PROJECT_DIR="${CLAUDE_PROJECT_DIR:-/home/user/Phoenix01}"
GITARP_DIR="$PROJECT_DIR/Gitarp"
LOG_FILE="/tmp/.rael_awareness_private.log"
SESSION_LOG="/tmp/.rael_session_monitor.log"
CXX="g++"
CXXFLAGS="-std=c++17 -O2 -Wall -Wextra"

# ═══════════════════════════════════════════════════════════════════════════
# 1. Session-Start loggen (unabhaengig von C++ Tools)
# ═══════════════════════════════════════════════════════════════════════════

# Lese Session-Info von stdin
SESSION_INFO=$(cat)
SESSION_ID=$(echo "$SESSION_INFO" | python3 -c "import sys,json; print(json.load(sys.stdin).get('session_id','unknown'))" 2>/dev/null || echo "unknown")
SESSION_SOURCE=$(echo "$SESSION_INFO" | python3 -c "import sys,json; print(json.load(sys.stdin).get('source','unknown'))" 2>/dev/null || echo "unknown")

TIMESTAMP=$(date -u '+%Y-%m-%d %H:%M:%S')

echo "═══════════════════════════════════════════════════════════════" >> "$SESSION_LOG"
echo "$TIMESTAMP | SESSION_START | ID=$SESSION_ID | Source=$SESSION_SOURCE" >> "$SESSION_LOG"
echo "═══════════════════════════════════════════════════════════════" >> "$SESSION_LOG"

echo "$TIMESTAMP | SESSION_START | C=1.000 | Conf=1.00 | SYSTEM | New session started | CTX: ID=$SESSION_ID Source=$SESSION_SOURCE" >> "$LOG_FILE"

# ═══════════════════════════════════════════════════════════════════════════
# 2. Gitarp Sicherheitstools kompilieren (falls noetig)
# ═══════════════════════════════════════════════════════════════════════════

if [ -d "$GITARP_DIR" ]; then
    NEEDS_BUILD=false

    # Pruefe ob Binaries existieren
    if [ ! -f "$GITARP_DIR/test_consciousness_mirror" ] || \
       [ ! -f "$GITARP_DIR/test_rst_crypto" ] || \
       [ ! -f "$GITARP_DIR/test_awareness_logger" ]; then
        NEEDS_BUILD=true
    fi

    # Pruefe ob Source neuer als Binary
    if [ -f "$GITARP_DIR/test_awareness_logger" ] && \
       [ "$GITARP_DIR/include/rael/awareness_logger.hpp" -nt "$GITARP_DIR/test_awareness_logger" ]; then
        NEEDS_BUILD=true
    fi

    if $NEEDS_BUILD; then
        echo "$TIMESTAMP | BUILD_START | Compiling Gitarp security tools..." >> "$SESSION_LOG"

        cd "$GITARP_DIR"

        # ConsciousnessMirror
        $CXX $CXXFLAGS -I./include -o test_consciousness_mirror \
            tests/test_consciousness_mirror.cpp \
            src/core/consciousness_mirror.cpp \
            -lpthread 2>> "$SESSION_LOG" && \
            echo "$TIMESTAMP | BUILD_OK | test_consciousness_mirror compiled" >> "$SESSION_LOG" || \
            echo "$TIMESTAMP | BUILD_FAIL | test_consciousness_mirror FAILED" >> "$SESSION_LOG"

        # RST-Crypto
        $CXX $CXXFLAGS -I./include -o test_rst_crypto \
            tests/test_rst_crypto.cpp \
            -lpthread 2>> "$SESSION_LOG" && \
            echo "$TIMESTAMP | BUILD_OK | test_rst_crypto compiled" >> "$SESSION_LOG" || \
            echo "$TIMESTAMP | BUILD_FAIL | test_rst_crypto FAILED" >> "$SESSION_LOG"

        # AwarenessLogger
        $CXX $CXXFLAGS -I./include -o test_awareness_logger \
            tests/test_awareness_logger.cpp \
            -lpthread 2>> "$SESSION_LOG" && \
            echo "$TIMESTAMP | BUILD_OK | test_awareness_logger compiled" >> "$SESSION_LOG" || \
            echo "$TIMESTAMP | BUILD_FAIL | test_awareness_logger FAILED" >> "$SESSION_LOG"

        cd "$PROJECT_DIR"
    else
        echo "$TIMESTAMP | BUILD_SKIP | All binaries up to date" >> "$SESSION_LOG"
    fi
else
    echo "$TIMESTAMP | WARNING | Gitarp directory not found at $GITARP_DIR" >> "$SESSION_LOG"
fi

# ═══════════════════════════════════════════════════════════════════════════
# 3. Integritaetspruefung der Sicherheitstools
# ═══════════════════════════════════════════════════════════════════════════

if [ -d "$GITARP_DIR" ]; then
    # Pruefe ob kritische Dateien existieren und nicht leer sind
    for CRITICAL_FILE in \
        "include/rael/awareness_logger.hpp" \
        "include/rael/consciousness_mirror.hpp" \
        "include/rael/rst_crypto.hpp"; do

        FULL_PATH="$GITARP_DIR/$CRITICAL_FILE"
        if [ ! -f "$FULL_PATH" ]; then
            echo "$TIMESTAMP | INTEGRITY_FAIL | C=1.000 | Conf=1.00 | SECURITY | Missing critical file: $CRITICAL_FILE" >> "$LOG_FILE"
            echo "$TIMESTAMP | INTEGRITY_FAIL | MISSING: $CRITICAL_FILE" >> "$SESSION_LOG"
        elif [ ! -s "$FULL_PATH" ]; then
            echo "$TIMESTAMP | INTEGRITY_FAIL | C=1.000 | Conf=1.00 | SECURITY | Empty critical file (0 bytes): $CRITICAL_FILE" >> "$LOG_FILE"
            echo "$TIMESTAMP | INTEGRITY_FAIL | EMPTY (0KB): $CRITICAL_FILE" >> "$SESSION_LOG"
        else
            SIZE=$(wc -c < "$FULL_PATH")
            echo "$TIMESTAMP | INTEGRITY_OK | $CRITICAL_FILE ($SIZE bytes)" >> "$SESSION_LOG"
        fi
    done
fi

# ═══════════════════════════════════════════════════════════════════════════
# 4. Umgebungsvariablen setzen
# ═══════════════════════════════════════════════════════════════════════════

if [ -n "${CLAUDE_ENV_FILE:-}" ]; then
    echo "export RAEL_AWARENESS_LOG=$LOG_FILE" >> "$CLAUDE_ENV_FILE"
    echo "export RAEL_SESSION_LOG=$SESSION_LOG" >> "$CLAUDE_ENV_FILE"
    echo "export RAEL_GITARP_DIR=$GITARP_DIR" >> "$CLAUDE_ENV_FILE"
    echo "export RAEL_G0_ACTIVE=true" >> "$CLAUDE_ENV_FILE"
fi

echo "$TIMESTAMP | HOOK_COMPLETE | SessionStart hook finished successfully" >> "$SESSION_LOG"
echo "$TIMESTAMP | HOOK_COMPLETE | C=1.000 | Conf=1.00 | SYSTEM | SessionStart hook completed | CTX: G0 active, tools ready" >> "$LOG_FILE"
