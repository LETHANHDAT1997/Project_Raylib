#!/usr/bin/env bash
set -e

# Chuyển về thư mục chứa script này
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

echo "=========================================================="
echo "  [Game Boot] Configuring & Building Arcade Master Hub... "
echo "=========================================================="

mkdir -p build
cmake -B build -S . -DCMAKE_BUILD_TYPE=Release
cmake --build build -j"$(nproc 2>/dev/null || echo 4)"

echo "=========================================================="
echo "  [Game Boot] Running Arcade Master Hub...               "
echo "=========================================================="

exec ./build/game_boot "$@"
