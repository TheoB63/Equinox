#pragma once

#include "equinox/editor/Editor.h"
#include "equinox/core/UUID.h"
#include "equinox/ECS/systems/RenderingSystem.h"

#include <map>
#include <string>
#include <functional>

namespace Equinox
{
    class RenderPanel : public Panel
    {
    public:
        RenderPanel();
        void OnInit() override;
        void OnRender() override;

        u32 GetSelectedAttachment() const { return m_SelectedAttachment; }

    private:
        
        std::shared_ptr<RenderingSystem> m_RS;
        std::string m_SelectedMode;
        u32 m_SelectedAttachment = 0;

        u32 m_SelectedTab = 0; // 0 for Model Viewer, 1 for Post Processing
    };
}
