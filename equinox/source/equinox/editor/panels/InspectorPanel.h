#pragma once

#include "equinox/editor/Editor.h"

#include "equinox/scene/Entity.h"

namespace Equinox
{
	class InspectorPanel : public Panel
	{
	public:
		InspectorPanel();

		void OnInit() override;
		void OnRender() override;

		void SetSelectedEntity(Entity entity) { m_SelectedEntity = entity; }

	private:
		template<typename T, typename UIFunction>
		void DrawComponent(const std::string& name, Entity entity, UIFunction uiFunction);

	private:
		Entity m_SelectedEntity;

	};
}