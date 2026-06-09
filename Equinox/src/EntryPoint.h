#pragma once

#ifdef EQN_PLATFORM_WINDOWS

extern Equinox::Application* Equinox::CreateApplication();

int main(int argc,char** argv)
{
	auto app = Equinox::CreateApplication();
	app->Run();
	delete app;
}

#endif