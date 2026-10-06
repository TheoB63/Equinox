#pragma once

#include "equinox/core/EquinoxTypes.h"
#include <vulkan/vulkan.h>

// GfxImGui : connects the ImGui Vulkan backend to our renderer.
namespace Equinox::Gfx::GfxImGui
{
	void Init();
	void Shutdown();
	void NewFrame();
	void Record(VkCommandBuffer cmd);

	bool IsReady();

	u64  AddTexture(VkSampler sampler, VkImageView view);
	void RemoveTexture(u64 id);
}