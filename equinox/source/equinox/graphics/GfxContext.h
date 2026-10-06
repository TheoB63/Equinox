#pragma once

#include "equinox/core/EquinoxTypes.h"
#include <vulkan/vulkan.h>
#include <functional>
#include <mutex>
#include <string>
#include <vector>

VK_DEFINE_HANDLE(VmaAllocator)
VK_DEFINE_HANDLE(VmaAllocation)

namespace Equinox::Gfx
{
    struct GfxQueue
    {
        VkQueue handle = VK_NULL_HANDLE;
        u32 familyIndex = 0;
    };

    class GfxContext
    {
    public:
        static void Init(void* windowHandle);
        static void Shutdown();
        static GfxContext& Get();

        VkInstance GetInstance() const { return m_Instance; }
        VkDevice GetDevice() const { return m_Device; }
        VkPhysicalDevice GetPhysicalDevice() const { return m_PhysicalDevice; }
        VkSurfaceKHR GetSurface() const { return m_Surface; }
        VmaAllocator GetAllocator() const { return m_Allocator; }

        const GfxQueue& GetGraphicsQueue() const { return m_GraphicsQueue; }
        const GfxQueue& GetPresentQueue() const { return m_PresentQueue; }

        const std::string& GetDeviceName() const { return m_DeviceName; }
        float GetMaxAnisotropy() const { return m_MaxAnisotropy; } // 0 = unsupported
        VkFormat GetDepthFormat() const { return m_DepthFormat; }
        float GetTimestampPeriod() const { return m_TimestampPeriod; }      // nanoseconds per GPU "tick"
        bool SupportsTimestamps() const { return m_TimestampValidBits > 0; }

        std::mutex& GetQueueMutex() { return m_QueueMutex; }

        void ImmediateSubmit(const std::function<void(VkCommandBuffer)>& record);

    private:
        GfxContext() = default;
        ~GfxContext() = default;

        void CreateInstance();
        void SetupDebugMessenger();
        void CreateSurface(void* windowHandle);
        void SelectPhysicalDevice();
        void CreateLogicalDevice();
        void CreateAllocator();
        void CreateImmediateContext();
        void FindDepthFormat();

        VkInstance m_Instance = VK_NULL_HANDLE;
        VkDebugUtilsMessengerEXT m_DebugMessenger = VK_NULL_HANDLE;
        VkSurfaceKHR m_Surface = VK_NULL_HANDLE;

        VkPhysicalDevice m_PhysicalDevice = VK_NULL_HANDLE;
        VkDevice m_Device = VK_NULL_HANDLE;
        VmaAllocator m_Allocator = nullptr;

        GfxQueue m_GraphicsQueue;
        GfxQueue m_PresentQueue;

        std::string m_DeviceName;
        float m_MaxAnisotropy = 0.0f;
        VkFormat m_DepthFormat = VK_FORMAT_UNDEFINED;
        float m_TimestampPeriod = 1.0f;
        u32 m_TimestampValidBits = 0;

        // Immediate submit context (used for loading)
        VkCommandPool m_ImmediatePool = VK_NULL_HANDLE;
        VkFence m_ImmediateFence = VK_NULL_HANDLE;
        std::mutex m_QueueMutex;

        static GfxContext* s_Instance;
    };
}

