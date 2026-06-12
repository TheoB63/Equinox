#include "eqnpch.h"
#include "Application.h"

#include "Equinox/Events/ApplicationEvent.h"
#include "Equinox/Log.h"

namespace Equinox {

	Application::Application()
	{
	}


	Application::~Application()
	{
	}

	void Application::Run()
	{
		WindowResizeEvent e(1280, 720);
		if (e.IsInCategory(EventCategoryApplication))
		{
			EQN_TRACE(e);
		}
		if (e.IsInCategory(EventCategoryInput))
		{
			EQN_TRACE(e);
		}

		while (true);
	}

}