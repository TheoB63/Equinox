#include "equinox/core/App.h"
#include "equinox/core/Log.h"

namespace Equinox
{
	class EquinoxApp : public App
	{
	public:
		EquinoxApp() {}

		~EquinoxApp() override = default;

	protected:
		void OnInit() override {}

		void OnUpdate() override {}

		void OnShutdown() override {}
	};

	App* createApp() 
	{
		return new EquinoxApp();
	}
}

int main()
{
	Equinox::Log::Init();
	Equinox::App* app = Equinox::createApp();
	app->Run();
	delete app;
	return 0;
}