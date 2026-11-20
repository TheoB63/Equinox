#pragma once

#include "equinox/editor/Editor.h"
#include "equinox/editor/EditorSelection.h"
#include "equinox/editor/inspectors/ModelViewer.h"
#include "equinox/editor/inspectors/MaterialEditor.h"
#include "equinox/editor/inspectors/TextureEditor.h"

namespace Equinox
{
	class InspectorPanel : public Panel
	{
	public:
		InspectorPanel();

		void OnInit() override;
		void OnRender() override;

	private:

		void DrawEntityComponents(Entity entity);

		template<typename T, typename UIFunction>
		void DrawComponent(const std::string& name, Entity entity, UIFunction uiFunction);

		void DrawResourceProperties(UUID resource);


	private:
		ModelViewer    m_ModelViewer;
		MaterialEditor m_MaterialEditor;
		TextureEditor  m_TextureEditor;
	};
}
