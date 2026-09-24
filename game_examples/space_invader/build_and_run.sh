#!/usr/bin/env bash
set -e

# Chuyển về thư mục chứa script này
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

echo "================================================"
echo "  [Space Invader] Configuring & Building...    "
echo "================================================"

mkdir -p build
cmake -B build -S . -DCMAKE_BUILD_TYPE=Release
cmake --build build -j"$(nproc 2>/dev/null || echo 4)"

echo "================================================"
echo "  [Space Invader] Running Game...              "
echo "================================================"

exec ./build/space_invader "$@"
