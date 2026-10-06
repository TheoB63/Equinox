#include "eqnpch.h"
#include "equinox/graphics/GfxResources.h"
#include "equinox/core/Log.h"

#define VMA_STATIC_VULKAN_FUNCTIONS 1
#define VMA_DYNAMIC_VULKAN_FUNCTIONS 0
#define VMA_IMPLEMENTATION
#include "vma/vk_mem_alloc.h"

#include <algorithm>
#include <cstring>

namespace Equinox::Gfx
{
    GfxBuffer CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage, bool hostVisible)
    {
        GfxBuffer result;
        result.size = size;

        VkBufferCreateInfo bufferInfo{};
        bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferInfo.size = size;
        bufferInfo.usage = usage;
        bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        VmaAllocationCreateInfo allocInfo{};
        allocInfo.usage = VMA_MEMORY_USAGE_AUTO;
        if (hostVisible)
            allocInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT;

        VmaAllocationInfo info{};
        VkResult res = vmaCreateBuffer(GfxContext::Get().GetAllocator(), &bufferInfo, &allocInfo,
                                       &result.buffer, &result.allocation, &info);
        if (res != VK_SUCCESS)
        {
            EQN_CORE_ERROR("vmaCreateBuffer failed ({0})", (int)res);
            return result;
        }
        result.mapped = hostVisible ? info.pMappedData : nullptr;
        return result;
    }

    GfxBuffer CreateBufferWithData(const void* data, VkDeviceSize size, VkBufferUsageFlags usage)
    {
        // 1) transfer buffer visible by the CPU
        GfxBuffer staging = CreateBuffer(size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, true);
        std::memcpy(staging.mapped, data, (size_t)size);
        FlushBuffer(staging);

        // 2) final buffer in GPU memory
        GfxBuffer result = CreateBuffer(size, usage | VK_BUFFER_USAGE_TRANSFER_DST_BIT, false);

        // 3) GPU copy, wait for it to finish
        GfxContext::Get().ImmediateSubmit([&](VkCommandBuffer cmd)
        {
            VkBufferCopy region{};
            region.size = size;
            vkCmdCopyBuffer(cmd, staging.buffer, result.buffer, 1, &region);
        });

        DestroyBuffer(staging);
        return result;
    }

    void FlushBuffer(GfxBuffer& buffer)
    {
        if (buffer.allocation)
            vmaFlushAllocation(GfxContext::Get().GetAllocator(), buffer.allocation, 0, VK_WHOLE_SIZE);
    }

    void DestroyBuffer(GfxBuffer& buffer)
    {
        if (buffer.buffer)
            vmaDestroyBuffer(GfxContext::Get().GetAllocator(), buffer.buffer, buffer.allocation);
        buffer = GfxBuffer{};
    }

    GfxImage CreateImage2D(u32 width, u32 height, VkFormat format, VkImageUsageFlags usage,
                           VkImageAspectFlags aspect, u32 mipLevels)
    {
        GfxImage result;
        result.format = format;
        result.extent = { width, height };
        result.aspect = aspect;
        result.mipLevels = mipLevels;

        VkImageCreateInfo imageInfo{};
        imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageInfo.imageType = VK_IMAGE_TYPE_2D;
        imageInfo.format = format;
        imageInfo.extent = { width, height, 1 };
        imageInfo.mipLevels = mipLevels;
        imageInfo.arrayLayers = 1;
        imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
        imageInfo.usage = usage;
        imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

        VmaAllocationCreateInfo allocInfo{};
        allocInfo.usage = VMA_MEMORY_USAGE_AUTO;
        allocInfo.flags = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT; // render images: dedicated memory

        VkResult res = vmaCreateImage(GfxContext::Get().GetAllocator(), &imageInfo, &allocInfo,
                                      &result.image, &result.allocation, nullptr);
        if (res != VK_SUCCESS)
        {
            EQN_CORE_ERROR("vmaCreateImage failed ({0})", (int)res);
            return result;
        }

        VkImageViewCreateInfo viewInfo{};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image = result.image;
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format = format;
        viewInfo.subresourceRange.aspectMask = aspect;
        viewInfo.subresourceRange.baseMipLevel = 0;
        viewInfo.subresourceRange.levelCount = mipLevels;
        viewInfo.subresourceRange.baseArrayLayer = 0;
        viewInfo.subresourceRange.layerCount = 1;
        vkCreateImageView(GfxContext::Get().GetDevice(), &viewInfo, nullptr, &result.view);
        return result;
    }

    void DestroyImage(GfxImage& image)
    {
        if (image.view)
            vkDestroyImageView(GfxContext::Get().GetDevice(), image.view, nullptr);
        if (image.image)
            vmaDestroyImage(GfxContext::Get().GetAllocator(), image.image, image.allocation);
        image = GfxImage{};
    }

    void TransitionRawImage(VkCommandBuffer cmd, VkImage image, VkImageAspectFlags aspect,
                            VkImageLayout oldLayout, VkImageLayout newLayout, u32 mipLevels)
    {
        VkImageMemoryBarrier2 barrier{};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
        barrier.srcStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
        barrier.srcAccessMask = VK_ACCESS_2_MEMORY_WRITE_BIT;
        barrier.dstStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
        barrier.dstAccessMask = VK_ACCESS_2_MEMORY_READ_BIT | VK_ACCESS_2_MEMORY_WRITE_BIT;
        barrier.oldLayout = oldLayout;
        barrier.newLayout = newLayout;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.image = image;
        barrier.subresourceRange.aspectMask = aspect;
        barrier.subresourceRange.baseMipLevel = 0;
        barrier.subresourceRange.levelCount = mipLevels;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = 1;

        VkDependencyInfo dep{};
        dep.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
        dep.imageMemoryBarrierCount = 1;
        dep.pImageMemoryBarriers = &barrier;
        vkCmdPipelineBarrier2(cmd, &dep);
    }

    void TransitionImage(VkCommandBuffer cmd, GfxImage& image, VkImageLayout newLayout)
    {
        if (image.layout == newLayout) return;
        TransitionRawImage(cmd, image.image, image.aspect, image.layout, newLayout, image.mipLevels);
        image.layout = newLayout;
    }

    GfxImage CreateTextureRGBA8(const u8* pixels, u32 width, u32 height, bool generateMips)
    {
        u32 mips = 1;
        if (generateMips)
            mips = (u32)std::floor(std::log2((double)std::max(width, height))) + 1;

        VkDeviceSize dataSize = (VkDeviceSize)width * height * 4;
        GfxBuffer staging = CreateBuffer(dataSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, true);
        std::memcpy(staging.mapped, pixels, (size_t)dataSize);
        FlushBuffer(staging);

        GfxImage image = CreateImage2D(width, height, VK_FORMAT_R8G8B8A8_UNORM,
            VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
            VK_IMAGE_ASPECT_COLOR_BIT, mips);

        GfxContext::Get().ImmediateSubmit([&](VkCommandBuffer cmd)
        {
            // Everything in TRANSFER_DST to receive the pixels
            TransitionImage(cmd, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

            VkBufferImageCopy region{};
            region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            region.imageSubresource.layerCount = 1;
            region.imageExtent = { width, height, 1 };
            vkCmdCopyBufferToImage(cmd, staging.buffer, image.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

            // Mipmaps: each level is made from the previous one (blit 2x smaller)
            i32 mipW = (i32)width, mipH = (i32)height;
            for (u32 level = 1; level < mips; level++)
            {
                // level (level-1) becomes a read source
                VkImageMemoryBarrier2 toSrc{};
                toSrc.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
                toSrc.srcStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
                toSrc.srcAccessMask = VK_ACCESS_2_MEMORY_WRITE_BIT;
                toSrc.dstStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
                toSrc.dstAccessMask = VK_ACCESS_2_MEMORY_READ_BIT;
                toSrc.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
                toSrc.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
                toSrc.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                toSrc.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                toSrc.image = image.image;
                toSrc.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, level - 1, 1, 0, 1 };
                VkDependencyInfo dep{};
                dep.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
                dep.imageMemoryBarrierCount = 1;
                dep.pImageMemoryBarriers = &toSrc;
                vkCmdPipelineBarrier2(cmd, &dep);

                VkImageBlit blit{};
                blit.srcSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, level - 1, 0, 1 };
                blit.srcOffsets[1] = { mipW, mipH, 1 };
                blit.dstSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, level, 0, 1 };
                blit.dstOffsets[1] = { std::max(mipW / 2, 1), std::max(mipH / 2, 1), 1 };
                vkCmdBlitImage(cmd, image.image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                               image.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &blit, VK_FILTER_LINEAR);

                // level (level-1) is finished: read by the shaders
                VkImageMemoryBarrier2 toRead = toSrc;
                toRead.srcAccessMask = VK_ACCESS_2_MEMORY_READ_BIT;
                toRead.dstAccessMask = VK_ACCESS_2_MEMORY_READ_BIT;
                toRead.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
                toRead.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                dep.pImageMemoryBarriers = &toRead;
                vkCmdPipelineBarrier2(cmd, &dep);

                mipW = std::max(mipW / 2, 1);
                mipH = std::max(mipH / 2, 1);
            }

            // Last level (the one that was never a "source") -> shader read
            VkImageMemoryBarrier2 last{};
            last.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
            last.srcStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
            last.srcAccessMask = VK_ACCESS_2_MEMORY_WRITE_BIT;
            last.dstStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
            last.dstAccessMask = VK_ACCESS_2_MEMORY_READ_BIT;
            last.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            last.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            last.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            last.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            last.image = image.image;
            last.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, mips - 1, 1, 0, 1 };
            VkDependencyInfo dep{};
            dep.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
            dep.imageMemoryBarrierCount = 1;
            dep.pImageMemoryBarriers = &last;
            vkCmdPipelineBarrier2(cmd, &dep);
        });

        image.layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        DestroyBuffer(staging);
        return image;
    }
}