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
		EQN_INFO("ExampleLayer::Update");
	}

	void OnEvent(Equinox::Event& event) override
	{
		EQN_TRACE("{0}", event);
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