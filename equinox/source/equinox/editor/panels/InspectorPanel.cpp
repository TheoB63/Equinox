#include "eqnpch.h"
#include "equinox/editor/panels/InspectorPanel.h"
#include "equinox/scene/Components.h"


namespace Equinox
{
    void InspectorPanel::OnInit() {}

    void InspectorPanel::OnRender()
    {
        ImGui::Begin("Inspector");
        ImGui::Text("Such empty, very wow.");
        ImGui::End();
    }
}