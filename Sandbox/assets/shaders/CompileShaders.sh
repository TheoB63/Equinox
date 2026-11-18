#!/usr/bin/env bash
# Compile shared shaders for Vulkan + OpenGL, and vk_* shaders for Vulkan only.
set -euo pipefail
cd "$(dirname "$0")"

GLSLC="${GLSLC:-glslc}"

mkdir -p spv
shopt -s nullglob

for file in *.vert *.frag; do
    echo "[Vulkan] $file"
    "$GLSLC" --target-env=vulkan1.3 -O "$file" -o "spv/$file.spv"
done
echo "SPIR-V generated in spv/"