#pragma once

#include "equinox/editor/Editor.h"

#include "equinox/core/UUID.h"

#include "equinox/ECS/Entity.h"

namespace Equinox
{
	class InspectorPanel : public Panel
	{
	public:
		InspectorPanel();

		void OnInit() override;
		void OnRender() override;

		void SetSelectedEntity(Entity entity) 
		{
			m_SelectedEntity = entity;
			m_SelectedResource = {};
		}
		void SetSelectedResource(UUID resource)
		{
			m_SelectedResource = resource;
			m_SelectedEntity = {};
		}

	private:
		void DrawEntityComponents();
		void DrawResourceProperties();

		template<typename T, typename UIFunction>
		void DrawComponent(const std::string& name, Entity entity, UIFunction uiFunction);

	private:
		Entity m_SelectedEntity;
		UUID m_SelectedResource;

	};
}