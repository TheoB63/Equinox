#pragma once

#include "equinox/core/Time.h"
#include "equinox/ECS/Systems.h"
#include "equinox/ECS/components.h"

#include "equinox/renderer/SkinnedModel.h"
#include "equinox/renderer/SkeletonRenderer.h"

#include "equinox/resources/FileSystem.h"
#include "equinox/resources/ResourceDB.h"
#include "equinox/resources/libraries/ModelLibrary.h"

#define MAX_BONES 512

namespace Equinox
{
	class AnimationSystem : public System
	{
	public:
		AnimationSystem()
		{
			m_SkeletonRenderer = SkeletonRenderer::Create();

			glGenBuffers(1, &m_BonesUBO);
			glBindBuffer(GL_UNIFORM_BUFFER, m_BonesUBO);
			glBufferData(GL_UNIFORM_BUFFER, MAX_BONES * sizeof(Mat4), nullptr, GL_DYNAMIC_DRAW);
			glBindBufferBase(GL_UNIFORM_BUFFER, 2, m_BonesUBO);
			glBindBuffer(GL_UNIFORM_BUFFER, 0);
		}

		void Update(entt::registry& registry) override
		{
			auto view = registry.view<Animation>();
			for (auto [entity, anim] : view.each())
			{
				std::shared_ptr<SkinnedModel> skinned =
					std::dynamic_pointer_cast<SkinnedModel>(ModelLibrary::Get(anim.ModelUUID));
				skinned->UpdateAnimation(Time::GetTime());

				std::vector<Mat4> boneTransforms = skinned->GetFinalTransforms();

				glBindBuffer(GL_UNIFORM_BUFFER, m_BonesUBO);
				glBufferSubData(GL_UNIFORM_BUFFER, 0, boneTransforms.size() * sizeof(Mat4), boneTransforms.data());
				glBindBuffer(GL_UNIFORM_BUFFER, 0);

				if (m_DrawSkeletons) {
					m_SkeletonRenderer->Update(*skinned);
					m_SkeletonRenderer->Draw();
				}
			}
		}

	private:
		u32 m_BonesUBO;
		bool m_DrawSkeletons = true;
		std::unique_ptr<SkeletonRenderer> m_SkeletonRenderer;
	};
}