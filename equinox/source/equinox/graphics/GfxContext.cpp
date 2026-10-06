#include "eqnpch.h"
#include "equinox/graphics/GfxContext.h"
#include "equinox/core/Log.h"

#include "vma/vk_mem_alloc.h"

#include <GLFW/glfw3.h>
#include <cstring>
#include <set>

namespace Equinox::Gfx
{
    GfxContext* GfxContext::s_Instance = nullptr;

    // Validation layers: only enabled in Debug
#if defined(DEBUG)
    static constexpr bool kEnableValidation = true;
#else
    static constexpr bool kEnableValidation = false;
#endif

    static VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(
        VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
        VkDebugUtilsMessageTypeFlagsEXT messageType,
        const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
        void* pUserData)
    {
        if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
            EQN_CORE_ERROR("Vulkan: {0}", pCallbackData->pMessage);
        else if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
            EQN_CORE_WARN("Vulkan: {0}", pCallbackData->pMessage);
        return VK_FALSE;
    }

    static bool HasInstanceLayer(const char* name)
    {
        u32 count = 0;
        vkEnumerateInstanceLayerProperties(&count, nullptr);
        std::vector<VkLayerProperties> layers(count);
        vkEnumerateInstanceLayerProperties(&count, layers.data());
        for (const auto& l : layers)
            if (std::strcmp(l.layerName, name) == 0) return true;
        return false;
    }

    static bool HasDeviceExtension(VkPhysicalDevice device, const char* name)
    {
        u32 count = 0;
        vkEnumerateDeviceExtensionProperties(device, nullptr, &count, nullptr);
        std::vector<VkExtensionProperties> exts(count);
        vkEnumerateDeviceExtensionProperties(device, nullptr, &count, exts.data());
        for (const auto& e : exts)
            if (std::strcmp(e.extensionName, name) == 0) return true;
        return false;
    }

    void GfxContext::Init(void* windowHandle)
    {
        EQN_CORE_ASSERT(!s_Instance, "GfxContext already initialized!");
        s_Instance = new GfxContext();

        s_Instance->CreateInstance();
        s_Instance->SetupDebugMessenger();
        s_Instance->CreateSurface(windowHandle);
        s_Instance->SelectPhysicalDevice();
        s_Instance->CreateLogicalDevice();
        s_Instance->CreateAllocator();
        s_Instance->CreateImmediateContext();
        s_Instance->FindDepthFormat();
    }

    void GfxContext::Shutdown()
    {
        if (!s_Instance) return;
        GfxContext& c = *s_Instance;

        vkDeviceWaitIdle(c.m_Device);

        vkDestroyFence(c.m_Device, c.m_ImmediateFence, nullptr);
        vkDestroyCommandPool(c.m_Device, c.m_ImmediatePool, nullptr);

        // The allocator must be destroyed BEFORE the device
        vmaDestroyAllocator(c.m_Allocator);

        vkDestroyDevice(c.m_Device, nullptr);
        vkDestroySurfaceKHR(c.m_Instance, c.m_Surface, nullptr);

        if (c.m_DebugMessenger)
        {
            auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(c.m_Instance, "vkDestroyDebugUtilsMessengerEXT");
            if (func) func(c.m_Instance, c.m_DebugMessenger, nullptr);
        }

        vkDestroyInstance(c.m_Instance, nullptr);

        delete s_Instance;
        s_Instance = nullptr;
    }

    GfxContext& GfxContext::Get()
    {
        EQN_CORE_ASSERT(s_Instance, "GfxContext not initialized!");
        return *s_Instance;
    }

    void GfxContext::CreateInstance()
    {
        VkApplicationInfo appInfo{};
        appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        appInfo.pApplicationName = "Equinox Engine";
        appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.pEngineName = "Equinox";
        appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.apiVersion = VK_API_VERSION_1_3;

        VkInstanceCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        createInfo.pApplicationInfo = &appInfo;

        u32 glfwExtensionCount = 0;
        const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
        std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

        const char* validationLayer = "VK_LAYER_KHRONOS_validation";
        bool useValidation = kEnableValidation && HasInstanceLayer(validationLayer);
        if (kEnableValidation && !useValidation)
            EQN_CORE_WARN("Validation layer not found (install the Vulkan SDK). Continuing without it.");

        if (useValidation)
            extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

        createInfo.enabledExtensionCount = (u32)extensions.size();
        createInfo.ppEnabledExtensionNames = extensions.data();

        if (useValidation)
        {
            createInfo.enabledLayerCount = 1;
            createInfo.ppEnabledLayerNames = &validationLayer;
        }

        if (vkCreateInstance(&createInfo, nullptr, &m_Instance) != VK_SUCCESS)
        {
            EQN_CORE_CRITICAL("Failed to create Vulkan Instance! (is a Vulkan 1.3 driver installed?)");
        }
    }

    void GfxContext::SetupDebugMessenger()
    {
        if (!kEnableValidation || !HasInstanceLayer("VK_LAYER_KHRONOS_validation")) return;

        VkDebugUtilsMessengerCreateInfoEXT createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
        createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
        createInfo.pfnUserCallback = DebugCallback;

        auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(m_Instance, "vkCreateDebugUtilsMessengerEXT");
        if (func) func(m_Instance, &createInfo, nullptr, &m_DebugMessenger);
    }

    void GfxContext::CreateSurface(void* windowHandle)
    {
        if (glfwCreateWindowSurface(m_Instance, (GLFWwindow*)windowHandle, nullptr, &m_Surface) != VK_SUCCESS)
        {
            EQN_CORE_CRITICAL("Failed to create Window Surface!");
        }
    }

    void GfxContext::SelectPhysicalDevice()
    {
        u32 deviceCount = 0;
        vkEnumeratePhysicalDevices(m_Instance, &deviceCount, nullptr);
        std::vector<VkPhysicalDevice> devices(deviceCount);
        vkEnumeratePhysicalDevices(m_Instance, &deviceCount, devices.data());

        int bestScore = -1;
        for (const auto& device : devices)
        {
            VkPhysicalDeviceProperties props;
            vkGetPhysicalDeviceProperties(device, &props);

            if (props.apiVersion < VK_API_VERSION_1_3)
            {
                EQN_CORE_WARN("GPU '{0}' skipped: needs Vulkan 1.3 (update the driver, or use --opengl)", props.deviceName);
                continue;
            }
            if (!HasDeviceExtension(device, VK_KHR_SWAPCHAIN_EXTENSION_NAME)) continue;

            VkPhysicalDeviceVulkan13Features f13{};
            f13.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;
            VkPhysicalDeviceFeatures2 f2{};
            f2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
            f2.pNext = &f13;
            vkGetPhysicalDeviceFeatures2(device, &f2);
            if (!f13.dynamicRendering || !f13.synchronization2) continue;

            // A queue that does graphics + presentation?
            u32 qCount = 0;
            vkGetPhysicalDeviceQueueFamilyProperties(device, &qCount, nullptr);
            std::vector<VkQueueFamilyProperties> families(qCount);
            vkGetPhysicalDeviceQueueFamilyProperties(device, &qCount, families.data());
            bool ok = false;
            for (u32 i = 0; i < qCount; i++)
            {
                VkBool32 present = VK_FALSE;
                vkGetPhysicalDeviceSurfaceSupportKHR(device, i, m_Surface, &present);
                if ((families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) && present) { ok = true; break; }
            }
            if (!ok) continue;

            int score = 1;
            if (props.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) score += 1000;
            else if (props.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU) score += 100;

            if (score > bestScore)
            {
                bestScore = score;
                m_PhysicalDevice = device;
                m_DeviceName = props.deviceName;
                m_MaxAnisotropy = props.limits.maxSamplerAnisotropy;
                m_TimestampPeriod = props.limits.timestampPeriod;
            }
        }

        if (m_PhysicalDevice == VK_NULL_HANDLE)
        {
            EQN_CORE_CRITICAL("No GPU with Vulkan 1.3 support found. Update your graphics driver, or run with --opengl.");
            return;
        }
        EQN_CORE_INFO("Selected GPU: {0}", m_DeviceName);
    }

    void GfxContext::CreateLogicalDevice()
    {
        // Queue family: first look for one that does graphics AND presentation
        u32 qCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(m_PhysicalDevice, &qCount, nullptr);
        std::vector<VkQueueFamilyProperties> families(qCount);
        vkGetPhysicalDeviceQueueFamilyProperties(m_PhysicalDevice, &qCount, families.data());

        int graphicsFamily = -1, presentFamily = -1;
        for (u32 i = 0; i < qCount; i++)
        {
            VkBool32 present = VK_FALSE;
            vkGetPhysicalDeviceSurfaceSupportKHR(m_PhysicalDevice, i, m_Surface, &present);
            bool graphics = (families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) != 0;
            if (graphics && present) { graphicsFamily = (int)i; presentFamily = (int)i; break; }
            if (graphics && graphicsFamily < 0) graphicsFamily = (int)i;
            if (present && presentFamily < 0) presentFamily = (int)i;
        }

        std::set<int> uniqueFamilies = { graphicsFamily, presentFamily };
        std::vector<VkDeviceQueueCreateInfo> queueInfos;
        float priority = 1.0f;
        for (int family : uniqueFamilies)
        {
            VkDeviceQueueCreateInfo q{};
            q.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            q.queueFamilyIndex = (u32)family;
            q.queueCount = 1;
            q.pQueuePriorities = &priority;
            queueInfos.push_back(q);
        }

        // Features: only enable what the GPU supports
        VkPhysicalDeviceFeatures supported{};
        vkGetPhysicalDeviceFeatures(m_PhysicalDevice, &supported);

        VkPhysicalDeviceVulkan13Features f13{};
        f13.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;
        f13.dynamicRendering = VK_TRUE;
        f13.synchronization2 = VK_TRUE;

        VkPhysicalDeviceFeatures2 features2{};
        features2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
        features2.pNext = &f13;
        features2.features.samplerAnisotropy = supported.samplerAnisotropy;
        features2.features.fillModeNonSolid = supported.fillModeNonSolid;
        if (!supported.samplerAnisotropy) m_MaxAnisotropy = 0.0f;

        std::vector<const char*> deviceExtensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };
        // ImGui loads vkCmdBeginRenderingKHR: we also enable the extension (harmless in 1.3)
        if (HasDeviceExtension(m_PhysicalDevice, VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME))
            deviceExtensions.push_back(VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME);

        VkDeviceCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        createInfo.pNext = &features2; // (pEnabledFeatures stays null when Features2 is used)
        createInfo.queueCreateInfoCount = (u32)queueInfos.size();
        createInfo.pQueueCreateInfos = queueInfos.data();
        createInfo.enabledExtensionCount = (u32)deviceExtensions.size();
        createInfo.ppEnabledExtensionNames = deviceExtensions.data();

        if (vkCreateDevice(m_PhysicalDevice, &createInfo, nullptr, &m_Device) != VK_SUCCESS)
        {
            EQN_CORE_CRITICAL("Failed to create Logical Device!");
        }

        vkGetDeviceQueue(m_Device, (u32)graphicsFamily, 0, &m_GraphicsQueue.handle);
        m_GraphicsQueue.familyIndex = (u32)graphicsFamily;
        m_TimestampValidBits = families[graphicsFamily].timestampValidBits;
        vkGetDeviceQueue(m_Device, (u32)presentFamily, 0, &m_PresentQueue.handle);
        m_PresentQueue.familyIndex = (u32)presentFamily;
    }

    void GfxContext::CreateAllocator()
    {
        VmaAllocatorCreateInfo info{};
        info.vulkanApiVersion = VK_API_VERSION_1_3;
        info.physicalDevice = m_PhysicalDevice;
        info.device = m_Device;
        info.instance = m_Instance;
        if (vmaCreateAllocator(&info, &m_Allocator) != VK_SUCCESS)
        {
            EQN_CORE_CRITICAL("Failed to create VMA allocator!");
        }
    }

    void GfxContext::CreateImmediateContext()
    {
        VkCommandPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        poolInfo.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT | VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        poolInfo.queueFamilyIndex = m_GraphicsQueue.familyIndex;
        vkCreateCommandPool(m_Device, &poolInfo, nullptr, &m_ImmediatePool);

        VkFenceCreateInfo fenceInfo{};
        fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        vkCreateFence(m_Device, &fenceInfo, nullptr, &m_ImmediateFence);
    }

    void GfxContext::FindDepthFormat()
    {
        const VkFormat candidates[] = {
            VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT, VK_FORMAT_D16_UNORM
        };
        for (VkFormat format : candidates)
        {
            VkFormatProperties props;
            vkGetPhysicalDeviceFormatProperties(m_PhysicalDevice, format, &props);
            if (props.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT)
            {
                m_DepthFormat = format;
                return;
            }
        }
        EQN_CORE_CRITICAL("No supported depth format!");
    }

    void GfxContext::ImmediateSubmit(const std::function<void(VkCommandBuffer)>& record)
    {
        std::lock_guard<std::mutex> lock(m_QueueMutex);

        VkCommandBufferAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.commandPool = m_ImmediatePool;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandBufferCount = 1;
        VkCommandBuffer cmd = VK_NULL_HANDLE;
        vkAllocateCommandBuffers(m_Device, &allocInfo, &cmd);

        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        vkBeginCommandBuffer(cmd, &beginInfo);

        record(cmd);

        vkEndCommandBuffer(cmd);

        VkSubmitInfo submit{};
        submit.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submit.commandBufferCount = 1;
        submit.pCommandBuffers = &cmd;
        vkQueueSubmit(m_GraphicsQueue.handle, 1, &submit, m_ImmediateFence);
        vkWaitForFences(m_Device, 1, &m_ImmediateFence, VK_TRUE, UINT64_MAX);
        vkResetFences(m_Device, 1, &m_ImmediateFence);

        vkFreeCommandBuffers(m_Device, m_ImmediatePool, 1, &cmd);
    }
}

