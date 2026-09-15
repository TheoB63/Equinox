#pragma once

#include "equinox/core/EquinoxTypes.h"

#include <memory>

namespace Equinox
{
	class SkinnedModel;

	class SkeletonRenderer
	{
	public : 
		virtual ~SkeletonRenderer() = default;
		virtual void Update(const SkinnedModel& model) = 0;
		virtual void Draw() = 0;

		static std::unique_ptr<SkeletonRenderer> Create();
	};
}