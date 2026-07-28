#pragma once

#include "equinox/editor/Editor.h"

namespace Equinox
{
	class InspectorPanel : public Panel
	{
	public : 
        void OnRender() override
        {
            ImGui::Begin("Inspector");
            // Add stuff here
            ImGui::Text("Such empty, very wow.");
            ImGui::End();
        }
	};
}