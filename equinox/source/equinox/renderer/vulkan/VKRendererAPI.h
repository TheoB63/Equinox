#pragma once

#include "equinox/core/Log.h"
#include "equinox/window/Window.h"
#include "equinox/renderer/Renderer.h"
#include "equinox/renderer/vulkan/VKDevice.h"
#include "equinox/renderer/vulkan/VKSwapChain.h"

#include <vulkan/vulkan.h>
#include <vector>
#include <memory>

namespace Equinox
{
	class VKRendererAPI : public RendererAPI
	{
	public:
		virtual void Init() override;
		virtual void Shutdown() override;

        virtual void SetViewport(u32 x, u32 y, u32 width, u32 height) override;
        virtual void SetClearColor(const glm::vec4& color) override;
        virtual void Clear() override;

        virtual void DrawIndexed(u32 count) override;

		VkInstance GetInstance() const { return m_Instance; }

	private:
        void CreateInstance();
        void CreateSurface();
        void CreateDevice();
        void CreateSwapChain();

        void SetupDebugMessenger();
        void DestroyDebugMessenger();
        bool CheckValidationLayerSupport() const;
		void PrintExtensions() const;
		void PrintLayers() const;

        // Core Vulkan objects
        VkInstance m_Instance = VK_NULL_HANDLE;
        VkSurfaceKHR m_Surface = VK_NULL_HANDLE;
        VkDebugUtilsMessengerEXT m_DebugMessenger = VK_NULL_HANDLE;

        std::unique_ptr<VKPhysicalDevice> m_PhysicalDevice;
        std::unique_ptr<VKLogicalDevice> m_LogicalDevice;
        std::unique_ptr<VKSwapchain> m_Swapchain;

        // Configuration
        const std::vector<const char*> m_ValidationLayers = {
            "VK_LAYER_KHRONOS_validation"
        };
        const std::vector<const char*> m_InstanceExtensions = {
            VK_EXT_DEBUG_UTILS_EXTENSION_NAME
        };

#ifdef NDEBUG
        const bool m_EnableValidationLayers = false;
#else
        const bool m_EnableValidationLayers = true;
#endif
    };
}