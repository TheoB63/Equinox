#include "eqnpch.h"
#include "equinox/core/App.h"

namespace Equinox
{
	void App::Run()
	{
		OnInit();

		while (m_Running)
		{
			OnUpdate();
		}
		OnShutdown();
	}

	void App::Close()
	{
		m_Running = false;
	}
}