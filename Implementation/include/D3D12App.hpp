#pragma once

#include "NoesisAppHost.hpp"

#include <d3d12.h>

#include <RefCntAutoPtr.hpp>
#include <DeviceContextD3D12.h>
#include <RenderDeviceD3D12.h>
#include <SwapChain.h>
#include <Fence.h>

#include <cstdint>

namespace NoesisDiligent
{
    class D3D12Backend final : public NoesisAppBackend
    {
    public:
        std::uint64_t GetSDLWindowFlags() const override;

        bool Initialize(SDL_Window &window, std::uint32_t width, std::uint32_t height) override;
        void UpdateSize(std::uint32_t width, std::uint32_t height) override;
        void RegisterNoesisPackages() override;
        void ShutdownNoesisPackages() override;
        Noesis::Ptr<Noesis::RenderDevice> CreateRenderDevice() override;
        void RenderFrame(Noesis::IView *view, double timeSeconds) override;
        void PrepareForNoesisShutdown() override;
        void Shutdown() override;

    private:
        bool InitDiligent(SDL_Window &window);
        bool ResizeSwapChain();
        bool QueryWindowPixelSize(std::uint32_t &width, std::uint32_t &height) const;
        bool CanResizeSwapChain(std::uint32_t &width, std::uint32_t &height) const;
        void SyncTextureStates(Diligent::ITexture *backBufferTexture, Diligent::ITexture *depthBufferTexture);

        bool mPendingResize = false;
        std::uint32_t mWindowWidth = 0;
        std::uint32_t mWindowHeight = 0;
        std::uint64_t mFrameNumber = 1;
        SDL_Window *mWindow = nullptr;

        Noesis::Ptr<Noesis::RenderDevice> mNoesisDevice;

        Diligent::RefCntAutoPtr<Diligent::IRenderDevice> mDevice;
        Diligent::RefCntAutoPtr<Diligent::IDeviceContext> mImmediateContext;
        Diligent::RefCntAutoPtr<Diligent::ISwapChain> mSwapChain;
        Diligent::RefCntAutoPtr<Diligent::IRenderDeviceD3D12> mDeviceD3D12;
        Diligent::RefCntAutoPtr<Diligent::IDeviceContextD3D12> mImmediateContextD3D12;
        Diligent::RefCntAutoPtr<Diligent::IFence> mFrameFence;

        ID3D12Fence *mD3D12Fence = nullptr;
    };

    int RunD3D12App(const AppStartupOptions &startupOptions = {});
}
