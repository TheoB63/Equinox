#include "eqnpch.h"
#include "equinox/renderer/pipeline/RenderPipeline.h"

namespace Equinox
{
    void RenderPipeline::InitAll(u32 w, u32 h) 
    {
        m_Width = w; m_Height = h;
        for (auto& p : m_Passes) p->Init(w, h);
    }

    void RenderPipeline::ResizeAll(u32 w, u32 h) 
    {
        m_Width = w; m_Height = h;
        for (auto& p : m_Passes) p->Resize(w, h);
    }

    void RenderPipeline::RenderAll(const RenderContext& ctx) 
    {
        for (auto& p : m_Passes)
            p->Execute(ctx);
    }
}