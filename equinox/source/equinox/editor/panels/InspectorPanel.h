#pragma once

#include "equinox/editor/Editor.h"

namespace Equinox
{
	class InspectorPanel : public Panel
	{
	public : 
		void OnInit() override;
		void OnRender() override;
	};
}