#!/usr/bin/env bash
# ============================================================================
#  Compiles all the GLSL shaders (.vert / .frag) of this folder to SPIR-V,
#  TWICE, from the very same source file:
#
#    glslc -V  -> assets/shaders/spv/<name>.spv      Vulkan  (GfxVulkanBackend)
#    glslc -G  -> assets/shaders/spv/gl/<name>.spv   OpenGL  (GfxOpenGLBackend,
#                                                     ARB_gl_spirv)
#
#  That is how Vulkan and OpenGL run exactly the same shader.
#  Rules: no push constant, "set = 0", per-object data in a std140 block
#  (binding 2).
# ============================================================================
set -e
cd "$(dirname "$0")"

if [[ "$OSTYPE" == "msys" || "$OSTYPE" == "win32" ]]; then
    GLSLC="$VULKAN_SDK/Bin/glslc.exe"
else
    GLSLC="${GLSLC:-glslc}"
fi

mkdir -p spv/gl

for file in *.vert *.frag; do
    echo "[Vulkan] $file"
    "$GLSLC" -V "$file" -o "spv/$file.spv"

    echo "[OpenGL] $file"
    "$GLSLC" -G "$file" -o "spv/gl/$file.spv"
done

echo ""
echo "Done: spv/ (Vulkan) and spv/gl/ (OpenGL)"
