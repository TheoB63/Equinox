#pragma once

#include "equinox/core/EquinoxTypes.h"
#include "equinox/graphics/GfxContext.h"
#include <vulkan/vulkan.h>

namespace Equinox::Gfx
{
	struct GfxBuffer
	{
		VkBuffer buffer = VK_NULL_HANDLE;
		VmaAllocation allocation = nullptr;
		VkDeviceSize size = 0;
		void* mapped = nullptr;
	};

	struct GfxImage
	{
		VkImage image = VK_NULL_HANDLE;
		VkImageView view = VK_NULL_HANDLE;
		VmaAllocation allocation = nullptr;
		VkFormat format = VK_FORMAT_UNDEFINED;
		VkExtent2D extent = { 0, 0 };
		VkImageAspectFlags aspect = VK_IMAGE_ASPECT_COLOR_BIT;
		VkImageLayout layout = VK_IMAGE_LAYOUT_UNDEFINED;
		u32 mipLevels = 1;
	};

	// ---- Buffers ----
	// hostVisible = true : the CPU can write into it (buffer.mapped). For data that changes every frame.
	// hostVisible = false: fast GPU memory. For meshes (filled via CreateBufferWithData).
	GfxBuffer CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage, bool hostVisible);
	GfxBuffer CreateBufferWithData(const void* data, VkDeviceSize size, VkBufferUsageFlags usage); // through a transfer buffer
	void FlushBuffer(GfxBuffer& buffer);   // call after writing into buffer.mapped
	void DestroyBuffer(GfxBuffer& buffer);

	// ---- Images ----
	GfxImage CreateImage2D(u32 width, u32 height, VkFormat format, VkImageUsageFlags usage,
		VkImageAspectFlags aspect, u32 mipLevels = 1);
	void DestroyImage(GfxImage& image);

	// Changes the layout of an image (with the required barrier) and updates image.layout
	void TransitionImage(VkCommandBuffer cmd, GfxImage& image, VkImageLayout newLayout);
	// Same thing for an image we do not own (e.g. a swapchain image)
	void TransitionRawImage(VkCommandBuffer cmd, VkImage image, VkImageAspectFlags aspect,
		VkImageLayout oldLayout, VkImageLayout newLayout, u32 mipLevels = 1);

	// Creates an RGBA8 image from CPU pixels, with mipmaps. Returns the image ready to be sampled.
	GfxImage CreateTextureRGBA8(const u8* pixels, u32 width, u32 height, bool generateMips);
}