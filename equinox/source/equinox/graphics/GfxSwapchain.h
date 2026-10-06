#pragma once

#include "equinox/core/EquinoxTypes.h"
#include "equinox/graphics/GfxContext.h"
#include <vulkan/vulkan.h>
#include <vector>

namespace Equinox::Gfx
{
    class GfxSwapchain
    {
    public:
        GfxSwapchain(u32 width, u32 height, bool vsync);
        ~GfxSwapchain();

        GfxSwapchain(const GfxSwapchain&) = delete;
        GfxSwapchain& operator=(const GfxSwapchain&) = delete;

        bool Resize(u32 width, u32 height);
        void SetVSync(bool vsync);
        bool IsValid() const { return m_Swapchain != VK_NULL_HANDLE; }

        VkResult AcquireNextImage(VkSemaphore signalSemaphore, u32& outImageIndex);
        VkResult Present(VkSemaphore waitSemaphore, u32 imageIndex);

        VkFormat GetFormat() const { return m_Format; }
        VkExtent2D GetExtent() const { return m_Extent; }
        u32 GetImageCount() const { return (u32)m_Images.size(); }
        VkImage GetImage(u32 index) const { return m_Images[index]; }
        VkImageView GetImageView(u32 index) const { return m_ImageViews[index]; }

    private:
        bool Create();
        void DestroyViews();

        VkSurfaceFormatKHR ChooseSurfaceFormat();
        VkPresentModeKHR ChoosePresentMode();

        VkSwapchainKHR m_Swapchain = VK_NULL_HANDLE;
        VkFormat m_Format = VK_FORMAT_UNDEFINED;
        VkExtent2D m_Extent = { 0, 0 };

        std::vector<VkImage> m_Images;
        std::vector<VkImageView> m_ImageViews;

        u32 m_Width = 0;
        u32 m_Height = 0;
        bool m_VSync = false;
    };
}