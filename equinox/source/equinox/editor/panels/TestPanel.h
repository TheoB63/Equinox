#pragma once

#include "equinox/editor/Editor.h"

namespace Equinox
{
    // TestPanel inherits from the base Panel class
    class TestPanel : public Panel
    {
    public:
        TestPanel();
        virtual ~TestPanel() = default;

        // Virtual overrides required by the Panel class
        void OnInit() override;
        void OnRender() override;

    private:
        // State variables for our test interface widgets
        float m_SliderValue = 0.5f;             // Value for the float slider
        int m_Counter = 0;                      // Click counter for the button
        char m_TextBuffer[128] = "Drag me!";    // Editable text / Drag and drop payload
        bool m_CheckboxState = false;           // Checkbox state
    };
}