#include <Equinox.h>

class Sandbox : public Equinox::Application
{
public:
	Sandbox()
	{

	}

	~Sandbox()
	{

	}
};

Equinox::Application* Equinox::CreateApplication()
{
	return new Sandbox();
}