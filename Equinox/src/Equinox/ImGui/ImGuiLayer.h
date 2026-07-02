#pragma once

#include "Equinox/Layer.h"

#include "Equinox/Events/ApplicationEvent.h"
#include "Equinox/Events/KeyEvent.h"
#include "Equinox/Events/MouseEvent.h"

namespace Equinox {

	class EQUINOX_API ImGuiLayer : public Layer
	{
	public:
		ImGuiLayer();
		~ImGuiLayer();

		virtual void OnAttach() override;
		virtual void OnDetach() override;
		virtual void OnImGuiRender() override;

		void Begin();
		void End();
	private:
		float m_Time = 0.0f;
	};

}