#include "eqnpch.h"
#include "equinox/graphics/GfxImGui.h"

#include "equinox/graphics/GfxContext.h"
#include "equinox/graphics/GfxRenderer.h"
#include "equinox/graphics/GfxResources.h"
#include "equinox/core/Log.h"

#include <imgui.h>
#include <backends/imgui_impl_vulkan.h>

namespace Equinox::Gfx::GfxImGui
{
    static VkDescriptorPool s_Pool = VK_NULL_HANDLE;
    static bool s_Initialized = false;

    // Grey 2x2 texture drawn in place of textures that have no GPU image yet (see SanitizeTextureIds)
    static GfxImage s_PlaceholderImage;
    static VkSampler s_PlaceholderSampler = VK_NULL_HANDLE;
    static u64 s_PlaceholderId = 0;

    static void CheckVkResult(VkResult err)
    {
        if (err != VK_SUCCESS)
            EQN_CORE_ERROR("ImGui Vulkan error: VkResult = {0}", (int)err);
    }

    void Init()
    {
        auto& ctx = GfxContext::Get();

        // Descriptor pool reserved for ImGui (the font + every ImGui::Image registered).
        // FREE_DESCRIPTOR_SET_BIT is mandatory: ImGui frees and recreates its descriptor sets.
        VkDescriptorPoolSize poolSize{};
        poolSize.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        poolSize.descriptorCount = 64;

        VkDescriptorPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
        poolInfo.maxSets = 64;
        poolInfo.poolSizeCount = 1;
        poolInfo.pPoolSizes = &poolSize;
        vkCreateDescriptorPool(ctx.GetDevice(), &poolInfo, nullptr, &s_Pool);

        // Format of the image ImGui draws into = the swapchain format
        VkFormat colorFormat = (VkFormat)GfxRenderer::GetSwapchainFormat();   // the facade returns a plain integer (no API type)

        VkPipelineRenderingCreateInfoKHR rendering{};
        rendering.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO_KHR;
        rendering.colorAttachmentCount = 1;
        rendering.pColorAttachmentFormats = &colorFormat;

        ImGui_ImplVulkan_InitInfo info{};
        info.Instance = ctx.GetInstance();
        info.PhysicalDevice = ctx.GetPhysicalDevice();
        info.Device = ctx.GetDevice();
        info.QueueFamily = ctx.GetGraphicsQueue().familyIndex;
        info.Queue = ctx.GetGraphicsQueue().handle;
        info.DescriptorPool = s_Pool;
        info.MinImageCount = 2;
        info.ImageCount = GfxRenderer::GetSwapchainImageCount();
        info.CheckVkResultFn = CheckVkResult;

        // The backend API changed in recent ImGui versions (late 2025):
        // the pipeline settings now live in "PipelineInfoMain".
#if IMGUI_VERSION_NUM >= 19231
        info.ApiVersion = VK_API_VERSION_1_3;
        info.UseDynamicRendering = true;
        info.PipelineInfoMain.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
        info.PipelineInfoMain.PipelineRenderingCreateInfo = rendering;
#else
        info.UseDynamicRendering = true;
        info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
        info.PipelineRenderingCreateInfo = rendering;
#endif

        if (!ImGui_ImplVulkan_Init(&info))
        {
            EQN_CORE_CRITICAL("ImGui_ImplVulkan_Init failed!");
            return;
        }
        s_Initialized = true;

        // Placeholder texture (must be created after s_Initialized = true: AddTexture needs the backend)
        const u8 grey[16] = { 90, 90, 90, 255,  90, 90, 90, 255,  90, 90, 90, 255,  90, 90, 90, 255 };
        s_PlaceholderImage = CreateTextureRGBA8(grey, 2, 2, false);

        VkSamplerCreateInfo samplerInfo{};
        samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        samplerInfo.magFilter = VK_FILTER_NEAREST;
        samplerInfo.minFilter = VK_FILTER_NEAREST;
        samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        vkCreateSampler(ctx.GetDevice(), &samplerInfo, nullptr, &s_PlaceholderSampler);
        s_PlaceholderId = AddTexture(s_PlaceholderSampler, s_PlaceholderImage.view);

        EQN_CORE_INFO(" - Initialized ImGui Vulkan backend");
    }

    void Shutdown()
    {
        if (!s_Initialized) return;
        auto device = GfxContext::Get().GetDevice();
        vkDeviceWaitIdle(device);

        RemoveTexture(s_PlaceholderId);
        s_PlaceholderId = 0;
        if (s_PlaceholderSampler) vkDestroySampler(device, s_PlaceholderSampler, nullptr);
        s_PlaceholderSampler = VK_NULL_HANDLE;
        DestroyImage(s_PlaceholderImage);

        ImGui_ImplVulkan_Shutdown();
        vkDestroyDescriptorPool(device, s_Pool, nullptr);
        s_Pool = VK_NULL_HANDLE;
        s_Initialized = false;
    }

    bool IsReady() { return s_Initialized; }

    void NewFrame()
    {
        if (s_Initialized) ImGui_ImplVulkan_NewFrame();
    }

    // The editor panels were written for OpenGL: they pass "texture ids" that are small integers
    // (an OpenGL texture name, or 0 when a resource has no GPU texture, e.g. a NullTexture).
    // A real Vulkan id is a VkDescriptorSet handle, which is a large pointer-like value.
    // Binding a null/invalid descriptor set crashes the driver, so before drawing we replace every
    // small id by the grey placeholder. Real Vulkan textures (the scene image, the font) are untouched.
    static void SanitizeTextureIds(ImDrawData* drawData)
    {
        constexpr u64 kSmallestVulkanId = 0x10000;
        if (!s_PlaceholderId) return;

        for (int n = 0; n < drawData->CmdListsCount; n++)
        {
            ImDrawList* list = drawData->CmdLists[n];
            for (int c = 0; c < list->CmdBuffer.Size; c++)
            {
                ImDrawCmd& cmd = list->CmdBuffer[c];
#if IMGUI_VERSION_NUM >= 19200
                // Recent ImGui: textures are ImTextureRef. Managed textures (the font) have _TexData set.
                if (cmd.TexRef._TexData == nullptr && (u64)cmd.TexRef._TexID < kSmallestVulkanId)
                    cmd.TexRef._TexID = (ImTextureID)s_PlaceholderId;
#else
                if ((u64)cmd.TextureId < kSmallestVulkanId)
                    cmd.TextureId = (ImTextureID)s_PlaceholderId;
#endif
            }
        }
    }

    void Record(VkCommandBuffer cmd)
    {
        if (!s_Initialized) return;
        ImDrawData* drawData = ImGui::GetDrawData();
        if (!drawData) return;
        SanitizeTextureIds(drawData);
        ImGui_ImplVulkan_RenderDrawData(drawData, cmd);
    }

    u64 AddTexture(VkSampler sampler, VkImageView view)
    {
        if (!s_Initialized) return 0;   // backend not initialized yet (calling it would crash)
        VkDescriptorSet set = ImGui_ImplVulkan_AddTexture(sampler, view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
        return (u64)set;
    }

    void RemoveTexture(u64 id)
    {
        if (!s_Initialized || !id) return;   // backend already shut down: its descriptor pool is destroyed
        ImGui_ImplVulkan_RemoveTexture((VkDescriptorSet)id);
    }
}