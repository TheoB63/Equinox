#include <Equinox.h>


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
		PushLayer(new Equinox::ImGuiLayer());
	}

	~Sandbox()
	{

	}

};

Equinox::Application* Equinox::CreateApplication()
{
	return new Sandbox();
}