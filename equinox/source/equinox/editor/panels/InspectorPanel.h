#pragma once

#include "equinox/editor/Editor.h"

#include "equinox/core/UUID.h"

#include "equinox/ECS/Entity.h"

namespace Equinox
{
	class Model;
	class Material;
	class Texture;

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
		void SetSelectedResourceNone() 
		{
			m_SelectedResource = {};
		}

	private:
		void DrawEntityComponents();

		template<typename T, typename UIFunction>
		void DrawComponent(const std::string& name, Entity entity, UIFunction uiFunction);

		void DrawResourceProperties();
		void DrawModel(Model& model);
		void DrawMaterial(Material& material);
		void DrawTexture(Texture& texture);

	private:
		Entity m_SelectedEntity;
		UUID m_SelectedResource;

	};
}