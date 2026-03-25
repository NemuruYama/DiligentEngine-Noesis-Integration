#include "VulkanApp.hpp"

#include <NsGui/IRenderer.h>
#include <NsRender/VKFactory.h>

#include <SDL3/SDL.h>
#include <SDL3/SDL_properties.h>
#include <SDL3/SDL_vulkan.h>

#include <vulkan/vulkan.h>

#include <CommandQueueVk.h>
#include <EngineFactoryVk.h>
#include <NativeWindow.h>
#include <TextureVk.h>

#include <algorithm>
#include <cstdio>

extern "C" void NsRegisterReflectionRenderVKRenderDevice();
extern "C" void NsInitPackageRenderVKRenderDevice();
extern "C" void NsShutdownPackageRenderVKRenderDevice();

namespace NoesisDiligent
{
    namespace
    {
        VkFormat ToVkFormat(Diligent::TEXTURE_FORMAT format)
        {
            switch (format)
            {
            case Diligent::TEX_FORMAT_RGBA8_UNORM:
                return VK_FORMAT_R8G8B8A8_UNORM;
            case Diligent::TEX_FORMAT_RGBA8_UNORM_SRGB:
                return VK_FORMAT_R8G8B8A8_SRGB;
            case Diligent::TEX_FORMAT_BGRA8_UNORM:
                return VK_FORMAT_B8G8R8A8_UNORM;
            case Diligent::TEX_FORMAT_BGRA8_UNORM_SRGB:
                return VK_FORMAT_B8G8R8A8_SRGB;
            case Diligent::TEX_FORMAT_D24_UNORM_S8_UINT:
                return VK_FORMAT_D24_UNORM_S8_UINT;
            case Diligent::TEX_FORMAT_D32_FLOAT_S8X24_UINT:
                return VK_FORMAT_D32_SFLOAT_S8_UINT;
            case Diligent::TEX_FORMAT_D32_FLOAT:
                return VK_FORMAT_D32_SFLOAT;
            case Diligent::TEX_FORMAT_D16_UNORM:
                return VK_FORMAT_D16_UNORM;
            default:
                return VK_FORMAT_UNDEFINED;
            }
        }

        VkImageAspectFlags GetDepthAspectMask(Diligent::TEXTURE_FORMAT format)
        {
            switch (format)
            {
            case Diligent::TEX_FORMAT_D24_UNORM_S8_UINT:
            case Diligent::TEX_FORMAT_D32_FLOAT_S8X24_UINT:
                return VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
            case Diligent::TEX_FORMAT_D32_FLOAT:
            case Diligent::TEX_FORMAT_D16_UNORM:
                return VK_IMAGE_ASPECT_DEPTH_BIT;
            default:
                return VK_IMAGE_ASPECT_STENCIL_BIT;
            }
        }
    }

    bool VulkanBackend::Initialize(SDL_Window& window, std::uint32_t width, std::uint32_t height)
    {
        mWindow = &window;
        mWindowWidth = width;
        mWindowHeight = height;
        mPendingResize = false;
        return InitDiligent(window);
    }

    std::uint64_t VulkanBackend::GetSDLWindowFlags() const
    {
        return SDL_WINDOW_VULKAN;
    }

    void VulkanBackend::UpdateSize(std::uint32_t width, std::uint32_t height)
    {
        mWindowWidth = width;
        mWindowHeight = height;
        mPendingResize = true;
    }

    void VulkanBackend::RegisterNoesisPackages()
    {
        NsRegisterReflectionRenderVKRenderDevice();
        NsInitPackageRenderVKRenderDevice();
    }

    void VulkanBackend::ShutdownNoesisPackages()
    {
        NsShutdownPackageRenderVKRenderDevice();
    }

    bool VulkanBackend::LoadAppVulkanFunctions()
    {
        mVkGetInstanceProcAddr = reinterpret_cast<PFN_vkGetInstanceProcAddr>(
            SDL_Vulkan_GetVkGetInstanceProcAddr());
        if (mVkGetInstanceProcAddr == VK_NULL_HANDLE)
        {
            std::fprintf(stderr, "Failed to get vkGetInstanceProcAddr from SDL: %s\n", SDL_GetError());
            return false;
        }

        mVkGetDeviceProcAddr = reinterpret_cast<PFN_vkGetDeviceProcAddr>(
            mVkGetInstanceProcAddr(mDeviceVk->GetVkInstance(), "vkGetDeviceProcAddr"));
        if (mVkGetDeviceProcAddr == VK_NULL_HANDLE)
        {
            std::fprintf(stderr, "Failed to load vkGetDeviceProcAddr\n");
            return false;
        }

        VkDevice device = mDeviceVk->GetVkDevice();
        mVkCreateRenderPass = reinterpret_cast<PFN_vkCreateRenderPass>(mVkGetDeviceProcAddr(device, "vkCreateRenderPass"));
        mVkDestroyRenderPass = reinterpret_cast<PFN_vkDestroyRenderPass>(mVkGetDeviceProcAddr(device, "vkDestroyRenderPass"));
        mVkCreateImageView = reinterpret_cast<PFN_vkCreateImageView>(mVkGetDeviceProcAddr(device, "vkCreateImageView"));
        mVkDestroyImageView = reinterpret_cast<PFN_vkDestroyImageView>(mVkGetDeviceProcAddr(device, "vkDestroyImageView"));
        mVkCreateFramebuffer = reinterpret_cast<PFN_vkCreateFramebuffer>(mVkGetDeviceProcAddr(device, "vkCreateFramebuffer"));
        mVkDestroyFramebuffer = reinterpret_cast<PFN_vkDestroyFramebuffer>(mVkGetDeviceProcAddr(device, "vkDestroyFramebuffer"));
        mVkCmdBeginRenderPass = reinterpret_cast<PFN_vkCmdBeginRenderPass>(mVkGetDeviceProcAddr(device, "vkCmdBeginRenderPass"));
        mVkCmdEndRenderPass = reinterpret_cast<PFN_vkCmdEndRenderPass>(mVkGetDeviceProcAddr(device, "vkCmdEndRenderPass"));
        mVkCmdSetViewport = reinterpret_cast<PFN_vkCmdSetViewport>(mVkGetDeviceProcAddr(device, "vkCmdSetViewport"));
        mVkCmdSetScissor = reinterpret_cast<PFN_vkCmdSetScissor>(mVkGetDeviceProcAddr(device, "vkCmdSetScissor"));

        const bool loaded =
            mVkCreateRenderPass != VK_NULL_HANDLE &&
            mVkDestroyRenderPass != VK_NULL_HANDLE &&
            mVkCreateImageView != VK_NULL_HANDLE &&
            mVkDestroyImageView != VK_NULL_HANDLE &&
            mVkCreateFramebuffer != VK_NULL_HANDLE &&
            mVkDestroyFramebuffer != VK_NULL_HANDLE &&
            mVkCmdBeginRenderPass != VK_NULL_HANDLE &&
            mVkCmdEndRenderPass != VK_NULL_HANDLE &&
            mVkCmdSetViewport != VK_NULL_HANDLE &&
            mVkCmdSetScissor != VK_NULL_HANDLE;

        if (!loaded)
        {
            std::fprintf(stderr, "Failed to load required Vulkan device functions for app rendering\n");
            return false;
        }

        return true;
    }

    void VulkanBackend::SyncTextureLayouts(Diligent::ITexture *backBufferTexture, Diligent::ITexture *depthBufferTexture)
    {
        Diligent::RefCntAutoPtr<Diligent::ITextureVk> backBufferTextureVk{backBufferTexture, Diligent::IID_TextureVk};
        if (backBufferTextureVk != nullptr)
        {
            backBufferTextureVk->SetLayout(VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
        }

        Diligent::RefCntAutoPtr<Diligent::ITextureVk> depthBufferTextureVk{depthBufferTexture, Diligent::IID_TextureVk};
        if (depthBufferTextureVk != nullptr)
        {
            depthBufferTextureVk->SetLayout(VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
        }
    }

    void VulkanBackend::DestroyBackBufferTargets()
    {
        if (mDeviceVk == nullptr)
        {
            mBackBufferTargets.clear();
            return;
        }

        VkDevice device = mDeviceVk->GetVkDevice();
        for (BackBufferTarget &target : mBackBufferTargets)
        {
            if (target.framebuffer != VK_NULL_HANDLE)
            {
                mVkDestroyFramebuffer(device, target.framebuffer, nullptr);
                target.framebuffer = VK_NULL_HANDLE;
            }

            if (target.view != VK_NULL_HANDLE)
            {
                mVkDestroyImageView(device, target.view, nullptr);
                target.view = VK_NULL_HANDLE;
            }
        }

        mBackBufferTargets.clear();
    }

    void VulkanBackend::DestroyVulkanTargets()
    {
        DestroyBackBufferTargets();

        if (mDeviceVk == nullptr)
        {
            mRenderPass = VK_NULL_HANDLE;
            mDepthView = VK_NULL_HANDLE;
            return;
        }

        VkDevice device = mDeviceVk->GetVkDevice();
        if (mDepthView != VK_NULL_HANDLE)
        {
            mVkDestroyImageView(device, mDepthView, nullptr);
            mDepthView = VK_NULL_HANDLE;
        }

        if (mRenderPass != VK_NULL_HANDLE)
        {
            mVkDestroyRenderPass(device, mRenderPass, nullptr);
            mRenderPass = VK_NULL_HANDLE;
        }
    }

    bool VulkanBackend::CreateRenderPass()
    {
        const Diligent::SwapChainDesc &swapChainDesc = mSwapChain->GetDesc();
        mColorFormatVk = ToVkFormat(swapChainDesc.ColorBufferFormat);
        mDepthFormatVk = ToVkFormat(swapChainDesc.DepthBufferFormat);

        if (mColorFormatVk == VK_FORMAT_UNDEFINED || mDepthFormatVk == VK_FORMAT_UNDEFINED)
        {
            std::fprintf(stderr, "Unsupported Vulkan swap chain formats\n");
            return false;
        }

        VkAttachmentReference depthRef{0, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL};
        VkAttachmentReference colorRef{1, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};

        VkSubpassDescription subpass{};
        subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.colorAttachmentCount = 1;
        subpass.pColorAttachments = &colorRef;
        subpass.pDepthStencilAttachment = &depthRef;

        VkAttachmentDescription attachments[2] = {};

        attachments[0].format = mDepthFormatVk;
        attachments[0].samples = VK_SAMPLE_COUNT_1_BIT;
        attachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        attachments[0].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attachments[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        attachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attachments[0].initialLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        attachments[0].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        attachments[1].format = mColorFormatVk;
        attachments[1].samples = VK_SAMPLE_COUNT_1_BIT;
        attachments[1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        attachments[1].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        attachments[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        attachments[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attachments[1].initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        attachments[1].finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        VkSubpassDependency dependency{};
        dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
        dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT |
                                  VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
        dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT |
                                   VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT |
                                   VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT |
                                   VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

        VkRenderPassCreateInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        renderPassInfo.attachmentCount = 2;
        renderPassInfo.pAttachments = attachments;
        renderPassInfo.subpassCount = 1;
        renderPassInfo.pSubpasses = &subpass;
        renderPassInfo.dependencyCount = 1;
        renderPassInfo.pDependencies = &dependency;

        if (mVkCreateRenderPass(mDeviceVk->GetVkDevice(), &renderPassInfo, nullptr, &mRenderPass) != VK_SUCCESS)
        {
            std::fprintf(stderr, "Failed to create Vulkan render pass\n");
            return false;
        }

        NoesisApp::VKFactory::WarmUpRenderPass(mNoesisDevice, mRenderPass, 1);

        return true;
    }

    bool VulkanBackend::CreateDepthView()
    {
        Diligent::RefCntAutoPtr<Diligent::ITextureVk> depthTextureVk{mSwapChain->GetDepthBufferDSV()->GetTexture(), Diligent::IID_TextureVk};
        if (depthTextureVk == nullptr)
        {
            std::fprintf(stderr, "Failed to query Vulkan depth texture\n");
            return false;
        }

        VkImageViewCreateInfo viewInfo{};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image = depthTextureVk->GetVkImage();
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format = mDepthFormatVk;
        viewInfo.subresourceRange.aspectMask = GetDepthAspectMask(mSwapChain->GetDesc().DepthBufferFormat);
        viewInfo.subresourceRange.baseMipLevel = 0;
        viewInfo.subresourceRange.levelCount = 1;
        viewInfo.subresourceRange.baseArrayLayer = 0;
        viewInfo.subresourceRange.layerCount = 1;

        if (mVkCreateImageView(mDeviceVk->GetVkDevice(), &viewInfo, nullptr, &mDepthView) != VK_SUCCESS)
        {
            std::fprintf(stderr, "Failed to create Vulkan depth image view\n");
            return false;
        }

        return true;
    }

    bool VulkanBackend::RecreateVulkanTargets()
    {
        DestroyVulkanTargets();
        const bool renderPassCreated = CreateRenderPass();
        if (!renderPassCreated)
        {
            return false;
        }

        const bool depthViewCreated = CreateDepthView();
        if (!depthViewCreated)
        {
            return false;
        }

        return true;
    }

    bool VulkanBackend::QueryWindowPixelSize(std::uint32_t& width, std::uint32_t& height) const
    {
        if (mWindow == nullptr)
        {
            return false;
        }

        int pixelWidth = 0;
        int pixelHeight = 0;
        if (!SDL_GetWindowSizeInPixels(mWindow, &pixelWidth, &pixelHeight))
        {
            return false;
        }

        if (pixelWidth <= 0 || pixelHeight <= 0)
        {
            return false;
        }

        width = static_cast<std::uint32_t>(pixelWidth);
        height = static_cast<std::uint32_t>(pixelHeight);
        return true;
    }

    bool VulkanBackend::CanResizeSwapChain(std::uint32_t& width, std::uint32_t& height) const
    {
        if (mWindow == nullptr)
        {
            return false;
        }

        const SDL_WindowFlags flags = SDL_GetWindowFlags(mWindow);
        if ((flags & SDL_WINDOW_MINIMIZED) != 0)
        {
            return false;
        }

        return QueryWindowPixelSize(width, height);
    }

    VulkanBackend::RenderTargetExtent VulkanBackend::GetRenderTargetExtent(
        Diligent::ITexture* backBufferTexture,
        Diligent::ITexture* depthBufferTexture) const
    {
        RenderTargetExtent extent{};

        if (backBufferTexture != nullptr)
        {
            const Diligent::TextureDesc& backBufferDesc = backBufferTexture->GetDesc();
            extent.width = backBufferDesc.Width;
            extent.height = backBufferDesc.Height;
        }

        if ((extent.width == 0 || extent.height == 0) && depthBufferTexture != nullptr)
        {
            const Diligent::TextureDesc& depthDesc = depthBufferTexture->GetDesc();
            extent.width = depthDesc.Width;
            extent.height = depthDesc.Height;
        }

        return extent;
    }

    VulkanBackend::BackBufferTarget *VulkanBackend::GetBackBufferTarget(Diligent::ITexture *backBufferTexture)
    {
        Diligent::RefCntAutoPtr<Diligent::ITextureVk> backBufferTextureVk{backBufferTexture, Diligent::IID_TextureVk};
        if (backBufferTextureVk == nullptr)
        {
            return nullptr;
        }

        VkImage image = backBufferTextureVk->GetVkImage();
        auto existing = std::find_if(mBackBufferTargets.begin(), mBackBufferTargets.end(),
                                     [image](const BackBufferTarget &target)
                                     {
                                         return target.image == image;
                                     });

        if (existing != mBackBufferTargets.end())
        {
            return &(*existing);
        }

        BackBufferTarget target{};
        target.image = image;

        const RenderTargetExtent extent = GetRenderTargetExtent(backBufferTexture, mSwapChain->GetDepthBufferDSV()->GetTexture());
        if (extent.width == 0 || extent.height == 0)
        {
            return nullptr;
        }

        VkImageViewCreateInfo viewInfo{};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image = image;
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format = mColorFormatVk;
        viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        viewInfo.subresourceRange.baseMipLevel = 0;
        viewInfo.subresourceRange.levelCount = 1;
        viewInfo.subresourceRange.baseArrayLayer = 0;
        viewInfo.subresourceRange.layerCount = 1;

        VkDevice device = mDeviceVk->GetVkDevice();
        if (mVkCreateImageView(device, &viewInfo, nullptr, &target.view) != VK_SUCCESS)
        {
            std::fprintf(stderr, "Failed to create Vulkan back buffer image view\n");
            return nullptr;
        }

        VkImageView attachments[] = {mDepthView, target.view};
        VkFramebufferCreateInfo framebufferInfo{};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = mRenderPass;
        framebufferInfo.attachmentCount = 2;
        framebufferInfo.pAttachments = attachments;
        framebufferInfo.width = extent.width;
        framebufferInfo.height = extent.height;
        framebufferInfo.layers = 1;

        if (mVkCreateFramebuffer(device, &framebufferInfo, nullptr, &target.framebuffer) != VK_SUCCESS)
        {
            mVkDestroyImageView(device, target.view, nullptr);
            target.view = VK_NULL_HANDLE;
            std::fprintf(stderr, "Failed to create Vulkan framebuffer\n");
            return nullptr;
        }

        mBackBufferTargets.push_back(target);
        return &mBackBufferTargets.back();
    }

    bool VulkanBackend::InitDiligent(SDL_Window& window)
    {
        SDL_PropertiesID properties = SDL_GetWindowProperties(&window);
        void *nativeWindow = SDL_GetPointerProperty(properties, SDL_PROP_WINDOW_WIN32_HWND_POINTER, nullptr);
        if (nativeWindow == nullptr)
        {
            std::fprintf(stderr, "Failed to get native window handle from SDL: %s\n", SDL_GetError());
            return false;
        }

        Diligent::IEngineFactoryVk *factory = Diligent::LoadAndGetEngineFactoryVk();
        if (factory == nullptr)
        {
            std::fprintf(stderr, "Failed to load Diligent Vulkan engine factory\n");
            return false;
        }

        Diligent::EngineVkCreateInfo engineCreateInfo;
        engineCreateInfo.Features.WireframeFill = Diligent::DEVICE_FEATURE_STATE_ENABLED;
        factory->CreateDeviceAndContextsVk(engineCreateInfo, &mDevice, &mImmediateContext);
        if (mDevice == nullptr || mImmediateContext == nullptr)
        {
            std::fprintf(stderr, "Failed to create Diligent Vulkan device/context\n");
            return false;
        }

        mDeviceVk = Diligent::RefCntAutoPtr<Diligent::IRenderDeviceVk>{mDevice, Diligent::IID_RenderDeviceVk};
        mImmediateContextVk = Diligent::RefCntAutoPtr<Diligent::IDeviceContextVk>{mImmediateContext, Diligent::IID_DeviceContextVk};
        if (mDeviceVk == nullptr || mImmediateContextVk == nullptr)
        {
            std::fprintf(stderr, "Failed to query Vulkan interfaces from Diligent objects\n");
            return false;
        }

        if (!LoadAppVulkanFunctions())
        {
            return false;
        }

        Diligent::SwapChainDesc swapChainDesc;
        swapChainDesc.Width = mWindowWidth;
        swapChainDesc.Height = mWindowHeight;
        swapChainDesc.ColorBufferFormat = Diligent::TEX_FORMAT_RGBA8_UNORM_SRGB;
        swapChainDesc.DepthBufferFormat = Diligent::TEX_FORMAT_D24_UNORM_S8_UINT;

        Diligent::Win32NativeWindow diligentWindow{nativeWindow};
        factory->CreateSwapChainVk(mDevice, mImmediateContext, swapChainDesc, diligentWindow, &mSwapChain);
        if (mSwapChain == nullptr)
        {
            std::fprintf(stderr, "Failed to create Diligent Vulkan swap chain\n");
            return false;
        }

        Diligent::RefCntAutoPtr<Diligent::ICommandQueueVk> commandQueueVk{mImmediateContext->LockCommandQueue(), Diligent::IID_CommandQueueVk};
        if (commandQueueVk == nullptr)
        {
            mImmediateContext->UnlockCommandQueue();
            std::fprintf(stderr, "Failed to query Vulkan command queue\n");
            return false;
        }

        mQueueFamilyIndex = commandQueueVk->GetQueueFamilyIndex();
        mImmediateContext->UnlockCommandQueue();
        return true;
    }

    Noesis::Ptr<Noesis::RenderDevice> VulkanBackend::CreateRenderDevice()
    {
        NoesisApp::VKFactory::InstanceInfo instanceInfo{};
        instanceInfo.instance = mDeviceVk->GetVkInstance();
        instanceInfo.physicalDevice = mDeviceVk->GetVkPhysicalDevice();
        instanceInfo.device = mDeviceVk->GetVkDevice();
        instanceInfo.pipelineCache = VK_NULL_HANDLE;
        instanceInfo.queueFamilyIndex = mQueueFamilyIndex;
        instanceInfo.vkGetInstanceProcAddr = mVkGetInstanceProcAddr;

        if (instanceInfo.vkGetInstanceProcAddr == VK_NULL_HANDLE)
        {
            std::fprintf(stderr, "Failed to get vkGetInstanceProcAddr from SDL: %s\n", SDL_GetError());
            return nullptr;
        }

        mNoesisDevice = NoesisApp::VKFactory::CreateDevice(true, instanceInfo);

        if (mNoesisDevice == nullptr)
        {
            std::fprintf(stderr, "Failed to create Noesis Vulkan render device\n");
            return nullptr;
        }

        if (!RecreateVulkanTargets())
        {
            return nullptr;
        }

        return mNoesisDevice;
    }

    bool VulkanBackend::ResizeSwapChain()
    {
        if (mSwapChain == nullptr)
        {
            return false;
        }

        std::uint32_t width = 0;
        std::uint32_t height = 0;
        if (!CanResizeSwapChain(width, height))
        {
            return false;
        }

        mWindowWidth = width;
        mWindowHeight = height;

        mImmediateContext->WaitForIdle();
        mSwapChain->Resize(mWindowWidth, mWindowHeight, Diligent::SURFACE_TRANSFORM_OPTIMAL);
        if (!RecreateVulkanTargets())
        {
            std::fprintf(stderr, "Failed to recreate Vulkan render targets after resize\n");
            return false;
        }
        mPendingResize = false;
        return true;
    }

    void VulkanBackend::RenderFrame(Noesis::IView *view, double timeSeconds)
    {
        if (view == nullptr || mSwapChain == nullptr)
        {
            return;
        }

        if (mPendingResize)
        {
            if (!ResizeSwapChain())
            {
                return;
            }
        }

        std::uint32_t drawableWidth = 0;
        std::uint32_t drawableHeight = 0;
        if (!CanResizeSwapChain(drawableWidth, drawableHeight))
        {
            return;
        }

        if (drawableWidth != mWindowWidth || drawableHeight != mWindowHeight)
        {
            mWindowWidth = drawableWidth;
            mWindowHeight = drawableHeight;
            mPendingResize = true;
            if (!ResizeSwapChain())
            {
                return;
            }
        }

        Diligent::ITexture *backBufferTexture = mSwapChain->GetCurrentBackBufferRTV()->GetTexture();
        Diligent::ITexture *depthBufferTexture = mSwapChain->GetDepthBufferDSV()->GetTexture();
        const RenderTargetExtent extent = GetRenderTargetExtent(backBufferTexture, depthBufferTexture);
        if (extent.width == 0 || extent.height == 0)
        {
            return;
        }

        BackBufferTarget *backBufferTarget = GetBackBufferTarget(backBufferTexture);
        if (backBufferTarget == nullptr || mRenderPass == VK_NULL_HANDLE || mDepthView == VK_NULL_HANDLE)
        {
            return;
        }

        Diligent::RefCntAutoPtr<Diligent::ICommandQueueVk> commandQueueVk{mImmediateContext->LockCommandQueue(), Diligent::IID_CommandQueueVk};
        const std::uint64_t safeFrameNumber = commandQueueVk != nullptr ? commandQueueVk->GetCompletedFenceValue() : mFrameNumber;
        mImmediateContext->UnlockCommandQueue();

        mImmediateContextVk->TransitionImageLayout(backBufferTexture, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
        mImmediateContextVk->TransitionImageLayout(depthBufferTexture, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);

        NoesisApp::VKFactory::RecordingInfo recordingInfo{};
        recordingInfo.commandBuffer = mImmediateContextVk->GetVkCommandBuffer();
        recordingInfo.frameNumber = mFrameNumber++;
        recordingInfo.safeFrameNumber = safeFrameNumber;
        NoesisApp::VKFactory::SetCommandBuffer(mNoesisDevice, recordingInfo);

        view->Update(timeSeconds);
        view->GetRenderer()->UpdateRenderTree();
        view->GetRenderer()->RenderOffscreen();

        VkClearValue clearValues[2] = {};
        clearValues[0].depthStencil.depth = 1.0f;
        clearValues[0].depthStencil.stencil = 0;
        clearValues[1].color.float32[0] = 0.05f;
        clearValues[1].color.float32[1] = 0.16f;
        clearValues[1].color.float32[2] = 0.27f;
        clearValues[1].color.float32[3] = 1.0f;

        VkRenderPassBeginInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassInfo.renderPass = mRenderPass;
        renderPassInfo.framebuffer = backBufferTarget->framebuffer;
        renderPassInfo.renderArea.offset = {0, 0};
        renderPassInfo.renderArea.extent = {extent.width, extent.height};
        renderPassInfo.clearValueCount = 2;
        renderPassInfo.pClearValues = clearValues;

        VkCommandBuffer commandBuffer = mImmediateContextVk->GetVkCommandBuffer();
        mVkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

        VkViewport viewport{};
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width = static_cast<float>(extent.width);
        viewport.height = static_cast<float>(extent.height);
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;
        mVkCmdSetViewport(commandBuffer, 0, 1, &viewport);

        VkRect2D scissor{};
        scissor.extent = {extent.width, extent.height};
        mVkCmdSetScissor(commandBuffer, 0, 1, &scissor);

        NoesisApp::VKFactory::SetRenderPass(mNoesisDevice, mRenderPass, 1);
        view->GetRenderer()->Render();
        mVkCmdEndRenderPass(commandBuffer);

        SyncTextureLayouts(backBufferTexture, depthBufferTexture);
        mSwapChain->Present(1);
    }

    void VulkanBackend::PrepareForNoesisShutdown()
    {
        if (mImmediateContext != nullptr)
        {
            mImmediateContext->Flush();
            mImmediateContext->WaitForIdle();
        }

        mNoesisDevice.Reset();
    }

    void VulkanBackend::Shutdown()
    {
        if (mImmediateContext != nullptr)
        {
            mImmediateContext->WaitForIdle();
        }

        DestroyVulkanTargets();
        mImmediateContextVk.Release();
        mDeviceVk.Release();
        mSwapChain.Release();
        mImmediateContext.Release();
        mDevice.Release();
        mNoesisDevice.Reset();
        mWindow = nullptr;
    }

    int RunVulkanApp(const AppStartupOptions& startupOptions)
    {
        VulkanBackend backend;
        return RunNoesisApp(backend, startupOptions);
    }
}