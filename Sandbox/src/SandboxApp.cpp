#include <Equinox.h>

#include "imgui/imgui.h"

class ExampleLayer : public Equinox::Layer
{
public:
	ExampleLayer()
		: Layer("Example")
	{
	}

	void OnUpdate() override
	{
		if (Equinox::Input::IsKeyPressed(EQN_KEY_TAB))
			EQN_TRACE("Tab key is pressed (poll)!");
	}

	virtual void OnImGuiRender() override
	{
		ImGui::Begin("Test");
		ImGui::Text("Hello World");
		ImGui::End();
	}

	void OnEvent(Equinox::Event& event) override
	{
		if (event.GetEventType() == Equinox::EventType::KeyPressed)
		{
			Equinox::KeyPressedEvent& e = (Equinox::KeyPressedEvent&)event;
			if (e.GetKeyCode() == EQN_KEY_TAB)
				EQN_TRACE("Tab key is pressed (event)!");
			EQN_TRACE("{0}", (char)e.GetKeyCode());
		}
	}

};

class Sandbox : public Equinox::Application
{
public:
	Sandbox()
	{
		PushLayer(new ExampleLayer());
	}

	~Sandbox()
	{

	}

};

Equinox::Application* Equinox::CreateApplication()
{
	return new Sandbox();
}