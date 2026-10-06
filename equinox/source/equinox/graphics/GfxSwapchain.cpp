#include "eqnpch.h"
#include "equinox/graphics/GfxSwapchain.h"
#include "equinox/core/Log.h"
#include <algorithm>

namespace Equinox::Gfx
{
    GfxSwapchain::GfxSwapchain(u32 width, u32 height, bool vsync)
        : m_Width(width), m_Height(height), m_VSync(vsync)
    {
        Create();
    }

    GfxSwapchain::~GfxSwapchain()
    {
        auto device = GfxContext::Get().GetDevice();
        DestroyViews();
        if (m_Swapchain)
            vkDestroySwapchainKHR(device, m_Swapchain, nullptr);
    }

    bool GfxSwapchain::Resize(u32 width, u32 height)
    {
        m_Width = width;
        m_Height = height;
        return Create();
    }

    void GfxSwapchain::SetVSync(bool vsync)
    {
        if (m_VSync == vsync) return;
        m_VSync = vsync;
        Create();
    }

    void GfxSwapchain::DestroyViews()
    {
        auto device = GfxContext::Get().GetDevice();
        for (auto view : m_ImageViews)
            vkDestroyImageView(device, view, nullptr);
        m_ImageViews.clear();
        m_Images.clear();
    }

    bool GfxSwapchain::Create()
    {
        auto& ctx = GfxContext::Get();

        VkSurfaceCapabilitiesKHR caps;
        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(ctx.GetPhysicalDevice(), ctx.GetSurface(), &caps);

        // Size: the surface often imposes its own (Windows). Otherwise we use the requested one.
        VkExtent2D extent = caps.currentExtent;
        if (extent.width == UINT32_MAX)
        {
            extent.width = std::clamp(m_Width, caps.minImageExtent.width, caps.maxImageExtent.width);
            extent.height = std::clamp(m_Height, caps.minImageExtent.height, caps.maxImageExtent.height);
        }

        // Minimized window (size 0): a swapchain cannot be created
        if (extent.width == 0 || extent.height == 0)
        {
            DestroyViews();
            if (m_Swapchain)
            {
                vkDestroySwapchainKHR(ctx.GetDevice(), m_Swapchain, nullptr);
                m_Swapchain = VK_NULL_HANDLE;
            }
            m_Extent = { 0, 0 };
            return false;
        }

        VkSurfaceFormatKHR surfaceFormat = ChooseSurfaceFormat();
        VkPresentModeKHR presentMode = ChoosePresentMode();

        u32 imageCount = caps.minImageCount + 1;
        if (caps.maxImageCount > 0 && imageCount > caps.maxImageCount)
            imageCount = caps.maxImageCount;

        VkSwapchainCreateInfoKHR createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        createInfo.surface = ctx.GetSurface();
        createInfo.minImageCount = imageCount;
        createInfo.imageFormat = surfaceFormat.format;
        createInfo.imageColorSpace = surfaceFormat.colorSpace;
        createInfo.imageExtent = extent;
        createInfo.imageArrayLayers = 1;
        // COLOR_ATTACHMENT: ImGui draws into it / TRANSFER_DST: clear and scene copy
        createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;

        u32 queueFamilyIndices[] = { ctx.GetGraphicsQueue().familyIndex, ctx.GetPresentQueue().familyIndex };
        if (queueFamilyIndices[0] != queueFamilyIndices[1])
        {
            createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
            createInfo.queueFamilyIndexCount = 2;
            createInfo.pQueueFamilyIndices = queueFamilyIndices;
        }
        else
        {
            createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        }

        createInfo.preTransform = caps.currentTransform;
        createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        createInfo.presentMode = presentMode;
        createInfo.clipped = VK_TRUE;

        // We pass the old swapchain for a smooth transition, then destroy it AFTER the creation
        VkSwapchainKHR oldSwapchain = m_Swapchain;
        createInfo.oldSwapchain = oldSwapchain;

        VkSwapchainKHR newSwapchain = VK_NULL_HANDLE;
        if (vkCreateSwapchainKHR(ctx.GetDevice(), &createInfo, nullptr, &newSwapchain) != VK_SUCCESS)
        {
            EQN_CORE_CRITICAL("Failed to create Swapchain!");
            return false;
        }

        DestroyViews();
        if (oldSwapchain)
            vkDestroySwapchainKHR(ctx.GetDevice(), oldSwapchain, nullptr);
        m_Swapchain = newSwapchain;

        vkGetSwapchainImagesKHR(ctx.GetDevice(), m_Swapchain, &imageCount, nullptr);
        m_Images.resize(imageCount);
        vkGetSwapchainImagesKHR(ctx.GetDevice(), m_Swapchain, &imageCount, m_Images.data());

        m_Format = surfaceFormat.format;
        m_Extent = extent;

        m_ImageViews.resize(imageCount);
        for (u32 i = 0; i < imageCount; i++)
        {
            VkImageViewCreateInfo viewInfo{};
            viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            viewInfo.image = m_Images[i];
            viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
            viewInfo.format = m_Format;
            viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            viewInfo.subresourceRange.levelCount = 1;
            viewInfo.subresourceRange.layerCount = 1;
            if (vkCreateImageView(ctx.GetDevice(), &viewInfo, nullptr, &m_ImageViews[i]) != VK_SUCCESS)
            {
                EQN_CORE_CRITICAL("Failed to create Swapchain Image View!");
            }
        }
        return true;
    }

    VkResult GfxSwapchain::AcquireNextImage(VkSemaphore signalSemaphore, u32& outImageIndex)
    {
        return vkAcquireNextImageKHR(GfxContext::Get().GetDevice(), m_Swapchain, UINT64_MAX,
                                     signalSemaphore, VK_NULL_HANDLE, &outImageIndex);
    }

    VkResult GfxSwapchain::Present(VkSemaphore waitSemaphore, u32 imageIndex)
    {
        VkPresentInfoKHR presentInfo{};
        presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        presentInfo.waitSemaphoreCount = 1;
        presentInfo.pWaitSemaphores = &waitSemaphore;
        presentInfo.swapchainCount = 1;
        presentInfo.pSwapchains = &m_Swapchain;
        presentInfo.pImageIndices = &imageIndex;

        auto& ctx = GfxContext::Get();
        std::lock_guard<std::mutex> lock(ctx.GetQueueMutex());
        return vkQueuePresentKHR(ctx.GetPresentQueue().handle, &presentInfo);
    }

    // UNORM (not SRGB): ImGui is drawn for a "non converted" display.
    // The scene shaders apply the gamma correction themselves.
    VkSurfaceFormatKHR GfxSwapchain::ChooseSurfaceFormat()
    {
        auto& ctx = GfxContext::Get();
        u32 count = 0;
        vkGetPhysicalDeviceSurfaceFormatsKHR(ctx.GetPhysicalDevice(), ctx.GetSurface(), &count, nullptr);
        std::vector<VkSurfaceFormatKHR> formats(count);
        vkGetPhysicalDeviceSurfaceFormatsKHR(ctx.GetPhysicalDevice(), ctx.GetSurface(), &count, formats.data());

        for (const auto& f : formats)
            if (f.format == VK_FORMAT_B8G8R8A8_UNORM && f.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) return f;
        for (const auto& f : formats)
            if (f.format == VK_FORMAT_R8G8B8A8_UNORM && f.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) return f;
        return formats[0];
    }

    // VSync on: FIFO (guaranteed everywhere). Otherwise: MAILBOX if available, then IMMEDIATE, then FIFO.
    VkPresentModeKHR GfxSwapchain::ChoosePresentMode()
    {
        if (m_VSync) return VK_PRESENT_MODE_FIFO_KHR;

        auto& ctx = GfxContext::Get();
        u32 count = 0;
        vkGetPhysicalDeviceSurfacePresentModesKHR(ctx.GetPhysicalDevice(), ctx.GetSurface(), &count, nullptr);
        std::vector<VkPresentModeKHR> modes(count);
        vkGetPhysicalDeviceSurfacePresentModesKHR(ctx.GetPhysicalDevice(), ctx.GetSurface(), &count, modes.data());

        for (auto m : modes) if (m == VK_PRESENT_MODE_MAILBOX_KHR) return m;
        for (auto m : modes) if (m == VK_PRESENT_MODE_IMMEDIATE_KHR) return m;
        return VK_PRESENT_MODE_FIFO_KHR;
    }
}