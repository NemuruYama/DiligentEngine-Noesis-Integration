#pragma once

#include "NoesisAppHost.hpp"

#include <vulkan/vulkan.h>

#include <RefCntAutoPtr.hpp>
#include <DeviceContextVk.h>
#include <RenderDeviceVk.h>
#include <SwapChain.h>

#include <cstdint>
#include <vector>

namespace NoesisDiligent
{
    class VulkanBackend final : public NoesisAppBackend
    {
    public:
        std::uint64_t GetSDLWindowFlags() const override;

        bool Initialize(SDL_Window& window, std::uint32_t width, std::uint32_t height) override;
        void UpdateSize(std::uint32_t width, std::uint32_t height) override;
        void RegisterNoesisPackages() override;
        void ShutdownNoesisPackages() override;
        Noesis::Ptr<Noesis::RenderDevice> CreateRenderDevice() override;
        void RenderFrame(Noesis::IView *view, double timeSeconds) override;
        void PrepareForNoesisShutdown() override;
        void Shutdown() override;

    private:
        struct BackBufferTarget
        {
            VkImage image = VK_NULL_HANDLE;
            VkImageView view = VK_NULL_HANDLE;
            VkFramebuffer framebuffer = VK_NULL_HANDLE;
        };

        struct RenderTargetExtent
        {
            std::uint32_t width = 0;
            std::uint32_t height = 0;
        };

        bool LoadAppVulkanFunctions();
        void SyncTextureLayouts(Diligent::ITexture *backBufferTexture, Diligent::ITexture *depthBufferTexture);
        void DestroyBackBufferTargets();
        void DestroyVulkanTargets();
        bool CreateRenderPass();
        bool CreateDepthView();
        bool RecreateVulkanTargets();
        bool QueryWindowPixelSize(std::uint32_t& width, std::uint32_t& height) const;
        bool CanResizeSwapChain(std::uint32_t& width, std::uint32_t& height) const;
        RenderTargetExtent GetRenderTargetExtent(Diligent::ITexture* backBufferTexture, Diligent::ITexture* depthBufferTexture) const;
        BackBufferTarget *GetBackBufferTarget(Diligent::ITexture *backBufferTexture);
        bool InitDiligent(SDL_Window& window);
        bool ResizeSwapChain();

        bool mPendingResize = false;
        std::uint32_t mWindowWidth = 0;
        std::uint32_t mWindowHeight = 0;
        std::uint64_t mFrameNumber = 1;
        std::uint32_t mQueueFamilyIndex = 0xFFFFFFFF;
        SDL_Window* mWindow = nullptr;

        Noesis::Ptr<Noesis::RenderDevice> mNoesisDevice;

        Diligent::RefCntAutoPtr<Diligent::IRenderDevice> mDevice;
        Diligent::RefCntAutoPtr<Diligent::IDeviceContext> mImmediateContext;
        Diligent::RefCntAutoPtr<Diligent::ISwapChain> mSwapChain;
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