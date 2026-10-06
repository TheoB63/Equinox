#!/usr/bin/env bash
# Compile shared shaders for Vulkan + OpenGL, and vk_* shaders for Vulkan only.
set -euo pipefail
cd "$(dirname "$0")"

if [[ "${OSTYPE:-}" == "msys" || "${OSTYPE:-}" == "win32" ]]; then
    GLSLC="${VULKAN_SDK:?VULKAN_SDK is not set}/Bin/glslc.exe"
else
    GLSLC="${GLSLC:-glslc}"
fi

mkdir -p spv/gl
shopt -s nullglob
for file in *.vert *.frag; do
    echo "[Vulkan] $file"
    "$GLSLC" --target-env=vulkan1.3 -O "$file" -o "spv/$file.spv"

    # Vulkan passes use several descriptor sets and are intentionally not part
    # of the shared OpenGL shader path.
    if [[ "$file" != vk_* ]]; then
        echo "[OpenGL] $file"
        "$GLSLC" -G -O "$file" -o "spv/gl/$file.spv"
    fi
done

echo "SPIR-V generated in assets/shaders/spv"
