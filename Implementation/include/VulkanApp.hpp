#pragma once

#include "DiligentNoesisBackendBase.hpp"

#include <vulkan/vulkan.h>

#include <RefCntAutoPtr.hpp>
#include <DeviceContextVk.h>
#include <RenderDeviceVk.h>

#include <cstdint>
#include <vector>

namespace NoesisDiligent
{
    class VulkanBackend final : public DiligentNoesisBackendBase
    {
    public:
        std::uint64_t GetWindowFlags() const override;

        void RegisterNoesisPackages() override;
        void ShutdownNoesisPackages() override;
        Noesis::Ptr<Noesis::RenderDevice> CreateRenderDevice() override;

    private:
        struct BackBufferTarget
        {
            VkImage image = VK_NULL_HANDLE;
            VkImageView view = VK_NULL_HANDLE;
            VkFramebuffer framebuffer = VK_NULL_HANDLE;
        };

        bool LoadAppVulkanFunctions();
        void SyncTextureLayouts(Diligent::ITexture *backBufferTexture, Diligent::ITexture *depthBufferTexture);
        void DestroyBackBufferTargets();
        void DestroyVulkanTargets();
        bool CreateRenderPass();
        bool CreateDepthView();
        bool RecreateVulkanTargets();
        BackBufferTarget *GetBackBufferTarget(Diligent::ITexture *backBufferTexture);
        bool InitDiligent(PlatformWindow& window) override;
        bool OnSwapChainResized() override;
        void RenderFrameImpl(Noesis::IView *view, double timeSeconds, Diligent::ITexture *backBufferTexture, Diligent::ITexture *depthBufferTexture) override;
        void ReleaseBackendResources() override;

        std::uint32_t mQueueFamilyIndex = 0xFFFFFFFF;
        Diligent::RefCntAutoPtr<Diligent::IRenderDeviceVk> mDeviceVk;
        Diligent::RefCntAutoPtr<Diligent::IDeviceContextVk> mImmediateContextVk;

        PFN_vkGetInstanceProcAddr mVkGetInstanceProcAddr = VK_NULL_HANDLE;
        PFN_vkGetDeviceProcAddr mVkGetDeviceProcAddr = VK_NULL_HANDLE;
        PFN_vkCreateRenderPass mVkCreateRenderPass = VK_NULL_HANDLE;
        PFN_vkDestroyRenderPass mVkDestroyRenderPass = VK_NULL_HANDLE;
        PFN_vkCreateImageView mVkCreateImageView = VK_NULL_HANDLE;
        PFN_vkDestroyImageView mVkDestroyImageView = VK_NULL_HANDLE;
        PFN_vkCreateFramebuffer mVkCreateFramebuffer = VK_NULL_HANDLE;
        PFN_vkDestroyFramebuffer mVkDestroyFramebuffer = VK_NULL_HANDLE;
        PFN_vkCmdBeginRenderPass mVkCmdBeginRenderPass = VK_NULL_HANDLE;
        PFN_vkCmdEndRenderPass mVkCmdEndRenderPass = VK_NULL_HANDLE;
        PFN_vkCmdSetViewport mVkCmdSetViewport = VK_NULL_HANDLE;
        PFN_vkCmdSetScissor mVkCmdSetScissor = VK_NULL_HANDLE;

        VkRenderPass mRenderPass = VK_NULL_HANDLE;
        VkImageView mDepthView = VK_NULL_HANDLE;
        VkFormat mColorFormatVk = VK_FORMAT_UNDEFINED;
        VkFormat mDepthFormatVk = VK_FORMAT_UNDEFINED;
        std::vector<BackBufferTarget> mBackBufferTargets;
    };

    int RunVulkanApp(const AppStartupOptions& startupOptions = {});
}
