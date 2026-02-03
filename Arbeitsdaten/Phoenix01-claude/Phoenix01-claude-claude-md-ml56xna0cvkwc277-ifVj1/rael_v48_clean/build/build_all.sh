#!/bin/bash
cd "$(dirname "$0")/.."

echo "Building RAEL V4..."
g++ -std=c++17 -O2 -I./include -o bin/rael src/core/*.cpp src/cli/main.cpp -ldl -lpthread
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

echo "Done!"
