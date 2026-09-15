#include "eqnpch.h"
#include "equinox/renderer/SkeletonRenderer.h"
#include "equinox/renderer/openGL/GLSkeletonRenderer.h"

namespace Equinox
{
    std::unique_ptr<SkeletonRenderer> SkeletonRenderer::Create() 
    {
        // Currently only OpenGL implementation
        return std::make_unique<GLSkeletonRenderer>();
    }
}