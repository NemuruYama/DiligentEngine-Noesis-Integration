#pragma once

#include "NoesisAppHost.hpp"

#include <DeviceContext.h>
#include <RefCntAutoPtr.hpp>
#include <RenderDevice.h>
#include <SwapChain.h>
#include <Texture.h>

namespace NoesisDiligent
{
    class DiligentNoesisBackendBase : public NoesisAppBackend
    {
    public:
        bool Initialize(SDL_Window& window, std::uint32_t width, std::uint32_t height) final override;
        void UpdateSize(std::uint32_t width, std::uint32_t height) final override;
        void RenderFrame(Noesis::IView *view, double timeSeconds) final override;
        void PrepareForNoesisShutdown() final override;
        void Shutdown() final override;

    protected:
        struct RenderTargetExtent
        {
            std::uint32_t width = 0;
            std::uint32_t height = 0;
        };

        virtual bool InitDiligent(SDL_Window& window) = 0;
        virtual bool OnSwapChainResized();
        virtual void RenderFrameImpl(Noesis::IView *view, double timeSeconds, Diligent::ITexture *backBufferTexture, Diligent::ITexture *depthBufferTexture) = 0;
        virtual void ReleaseBackendResources() = 0;

        bool QueryWindowPixelSize(std::uint32_t& width, std::uint32_t& height) const;
        bool CanResizeSwapChain(std::uint32_t& width, std::uint32_t& height) const;
        bool ResizeSwapChain();
        RenderTargetExtent GetRenderTargetExtent(Diligent::ITexture *backBufferTexture, Diligent::ITexture *depthBufferTexture) const;

        bool mPendingResize = false;
        std::uint32_t mWindowWidth = 0;
        std::uint32_t mWindowHeight = 0;
        std::uint64_t mFrameNumber = 1;
        SDL_Window *mWindow = nullptr;

        Noesis::Ptr<Noesis::RenderDevice> mNoesisDevice;

        Diligent::RefCntAutoPtr<Diligent::IRenderDevice> mDevice;
        Diligent::RefCntAutoPtr<Diligent::IDeviceContext> mImmediateContext;
        Diligent::RefCntAutoPtr<Diligent::ISwapChain> mSwapChain;
    };

    template <typename Backend>
    int RunBackendApp(const AppStartupOptions& startupOptions = {})
    {
        Backend backend;
        return RunNoesisApp(backend, startupOptions);
    }
}
