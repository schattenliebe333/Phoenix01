#!/bin/bash
cd "$(dirname "$0")/.."

echo "Building RAEL V4..."
# Security hardening flags (F-01 audit fix)
SECURITY_FLAGS="-fstack-protector-strong -D_FORTIFY_SOURCE=2 -fPIE -Wformat -Wformat-security"
LDFLAGS="-pie -Wl,-z,relro,-z,now"
g++ -std=c++17 -O2 $SECURITY_FLAGS -I./include -o bin/rael src/core/*.cpp src/cli/main.cpp $LDFLAGS -ldl -lpthread
echo "  bin/rael"

echo "Building modules..."
g++ -std=c++17 -O2 -shared -fPIC -I./include -o bin/modules/libsem_quint.so modules/sample_semantic_quint/sem_quint.cpp
g++ -std=c++17 -O2 -shared -fPIC -I./include -o bin/modules/libmath_pack.so modules/sample_math_formulas/math_pack.cpp
echo "  bin/modules/*.so"

echo "Building TUI..."
g++ -std=c++17 -O2 -o bin/rael_tui src/tui/rael_tui.cpp -lpthread
echo "  bin/rael_tui"

echo "Building WebGUI..."
g++ -std=c++17 -O2 -o bin/rael_webgui src/webgui/rael_webgui_standalone.cpp -lpthread
echo "  bin/rael_webgui"

echo "Building tests..."
# RST-Crypto Test (RAEL-native Kryptografie)
g++ -std=c++17 -O2 $SECURITY_FLAGS -I./include -o bin/test_rst_crypto tests/test_rst_crypto.cpp $LDFLAGS
echo "  bin/test_rst_crypto"

# ConsciousnessMirror Test
if [ -f "tests/test_consciousness_mirror.cpp" ]; then
    g++ -std=c++17 -O2 $SECURITY_FLAGS -I./include -o bin/test_consciousness_mirror tests/test_consciousness_mirror.cpp src/core/consciousness_mirror.cpp $LDFLAGS
    echo "  bin/test_consciousness_mirror"
fi

echo "Done!"
