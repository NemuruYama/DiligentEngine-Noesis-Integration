#pragma once

#include "DiligentNoesisBackendBase.hpp"

#include <d3d12.h>

#include <RefCntAutoPtr.hpp>
#include <DeviceContextD3D12.h>
#include <RenderDeviceD3D12.h>
#include <SwapChain.h>
#include <Fence.h>

#include <cstdint>

namespace NoesisDiligent
{
    class D3D12Backend final : public DiligentNoesisBackendBase
    {
    public:
        std::uint64_t GetWindowFlags() const override;

        void RegisterNoesisPackages() override;
        void ShutdownNoesisPackages() override;
        Noesis::Ptr<Noesis::RenderDevice> CreateRenderDevice() override;

    private:
        bool InitDiligent(PlatformWindow& window) override;
        void RenderFrameImpl(Noesis::IView *view, double timeSeconds, Diligent::ITexture *backBufferTexture, Diligent::ITexture *depthBufferTexture) override;
        void ReleaseBackendResources() override;
        void SyncTextureStates(Diligent::ITexture *backBufferTexture, Diligent::ITexture *depthBufferTexture);

        Diligent::RefCntAutoPtr<Diligent::IRenderDeviceD3D12> mDeviceD3D12;
        Diligent::RefCntAutoPtr<Diligent::IDeviceContextD3D12> mImmediateContextD3D12;
        Diligent::RefCntAutoPtr<Diligent::IFence> mFrameFence;

        ID3D12Fence *mD3D12Fence = nullptr;
    };

    int RunD3D12App(const AppStartupOptions &startupOptions = {});
}
