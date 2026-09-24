#!/usr/bin/env bash
set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

echo "=========================================================="
echo "  [Master Build] Building all games in game_examples...   "
echo "=========================================================="

mkdir -p build
cmake -B build -S . -DCMAKE_BUILD_TYPE=Release
cmake --build build -j"$(nproc 2>/dev/null || echo 4)"

echo "=========================================================="
echo "  [Master Build] Success! Binaries created:               "
echo "    - ./build/tetris/tetris"
echo "    - ./build/space_invader/space_invader"
echo "=========================================================="
