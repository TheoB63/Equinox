#pragma once

#ifdef EQN_PLATFORM_WINDOWS

extern Equinox::Application* Equinox::CreateApplication();

int main(int argc, char** argv)
{
	Equinox::Log::Init();
	EQN_CORE_WARN("Initialized Log!");
	int a = 5;
	EQN_INFO("Hello! Var={0}", a);

	auto app = Equinox::CreateApplication();
	app->Run();
	delete app;
}

#endif