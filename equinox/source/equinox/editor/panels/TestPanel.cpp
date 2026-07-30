#include "eqnpch.h"
#include "TestPanel.h"
#include <imgui.h>

namespace Equinox
{
    TestPanel::TestPanel()
    {
        EQN_CORE_INFO("Created TestPanel");
    }

    void TestPanel::OnInit()
    {
        // Specific initialization if needed (loading, resetting, etc.)
    }

    void TestPanel::OnRender()
    {
        // Open an ImGui window titled "Test Panel"
        if (ImGui::Begin("Test Panel"))
        {
            // --- 1. TEXT AND BUTTONS SECTION ---
            ImGui::Text("Welcome to your ImGui learning panel!");
            ImGui::Separator();

            // A simple button that increments on each click
            if (ImGui::Button("Click me!"))
            {
                m_Counter++;
                EQN_CORE_INFO("Button clicked {0} times", m_Counter);
            }

            // Display the counter value on the same line
            ImGui::SameLine();
            ImGui::Text("Counter: %d", m_Counter);

            // A checkbox
            ImGui::Checkbox("Enable an option", &m_CheckboxState);
            if (m_CheckboxState)
            {
                ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "Option enabled!");
            }

            ImGui::Spacing();
            ImGui::Separator();

            // --- 2. SLIDER SECTION ---
            ImGui::Text("Value Control (Slider)");
            // A float slider ranging from 0.0f to 10.0f
            ImGui::SliderFloat("Float Value", &m_SliderValue, 0.0f, 10.0f, "Val: %.2f");

            ImGui::Spacing();
            ImGui::Separator();

            // --- 3. DRAG AND DROP SECTION ---
            ImGui::Text("Drag and Drop Zone");

            // Drag and Drop source: this text can be grabbed and dragged
            ImGui::InputText("Text to drag", m_TextBuffer, sizeof(m_TextBuffer));

            if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID))
            {
                // Send the text data via a payload named "TEST_PAYLOAD"
                ImGui::SetDragDropPayload("TEST_PAYLOAD", m_TextBuffer, sizeof(m_TextBuffer));
                ImGui::Text("Drag text: %s", m_TextBuffer);
                ImGui::EndDragDropSource();
            }

            // Drag and Drop target: a rectangular button that accepts the payload
            ImGui::Button("Target Zone (Drop here)");

            if (ImGui::BeginDragDropTarget())
            {
                // If a payload named "TEST_PAYLOAD" is dropped onto this button
                if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("TEST_PAYLOAD"))
                {
                    // Retrieve the sent data
                    const char* droppedText = (const char*)payload->Data;
                    EQN_CORE_INFO("Data received via drag and drop: {0}", droppedText);

                    // Update our local buffer with the received text
                    strcpy_s(m_TextBuffer, droppedText);
                }
                ImGui::EndDragDropTarget();
            }
        }
        // Always close the window with End() if Begin() returned true
        ImGui::End();
    }
}