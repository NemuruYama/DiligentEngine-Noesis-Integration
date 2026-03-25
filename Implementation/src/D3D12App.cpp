#include "D3D12App.hpp"

#include <NsGui/IRenderer.h>
#include <NsRender/D3D12Factory.h>

#include <SDL3/SDL.h>
#include <SDL3/SDL_properties.h>

#include <d3d12.h>

#include <CommandQueueD3D12.h>
#include <EngineFactoryD3D12.h>
#include <FenceD3D12.h>
#include <NativeWindow.h>
#include <TextureD3D12.h>
#include <TextureViewD3D12.h>

#include <cstdio>

extern "C" void NsRegisterReflectionRenderD3D12RenderDevice();
extern "C" void NsInitPackageRenderD3D12RenderDevice();
extern "C" void NsShutdownPackageRenderD3D12RenderDevice();

namespace NoesisDiligent
{
    namespace
    {
        DXGI_FORMAT ToDXGIFormat(Diligent::TEXTURE_FORMAT format)
        {
            switch (format)
            {
            case Diligent::TEX_FORMAT_RGBA8_UNORM:
                return DXGI_FORMAT_R8G8B8A8_UNORM;
            case Diligent::TEX_FORMAT_RGBA8_UNORM_SRGB:
                return DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
            case Diligent::TEX_FORMAT_BGRA8_UNORM:
                return DXGI_FORMAT_B8G8R8A8_UNORM;
            case Diligent::TEX_FORMAT_BGRA8_UNORM_SRGB:
                return DXGI_FORMAT_B8G8R8A8_UNORM_SRGB;
            case Diligent::TEX_FORMAT_D24_UNORM_S8_UINT:
                return DXGI_FORMAT_D24_UNORM_S8_UINT;
            case Diligent::TEX_FORMAT_D32_FLOAT_S8X24_UINT:
                return DXGI_FORMAT_D32_FLOAT_S8X24_UINT;
            case Diligent::TEX_FORMAT_D32_FLOAT:
                return DXGI_FORMAT_D32_FLOAT;
            case Diligent::TEX_FORMAT_D16_UNORM:
                return DXGI_FORMAT_D16_UNORM;
            default:
                return DXGI_FORMAT_UNKNOWN;
            }
        }

        bool IsSRGBFormat(Diligent::TEXTURE_FORMAT format)
        {
            return format == Diligent::TEX_FORMAT_RGBA8_UNORM_SRGB ||
                   format == Diligent::TEX_FORMAT_BGRA8_UNORM_SRGB;
        }

        void BindRenderTargetsAndViewport(ID3D12GraphicsCommandList *commandList,
                                          const D3D12_CPU_DESCRIPTOR_HANDLE& rtvHandle,
                                          const D3D12_CPU_DESCRIPTOR_HANDLE& dsvHandle,
                                          const Diligent::TextureDesc& backBufferDesc)
        {
            if (commandList == nullptr)
            {
                return;
            }

            commandList->OMSetRenderTargets(1, &rtvHandle, FALSE, &dsvHandle);

            D3D12_VIEWPORT viewport{};
            viewport.TopLeftX = 0.0f;
            viewport.TopLeftY = 0.0f;
            viewport.Width = static_cast<float>(backBufferDesc.Width);
            viewport.Height = static_cast<float>(backBufferDesc.Height);
            viewport.MinDepth = 0.0f;
            viewport.MaxDepth = 1.0f;
            commandList->RSSetViewports(1, &viewport);

            D3D12_RECT scissorRect{};
            scissorRect.right = static_cast<LONG>(backBufferDesc.Width);
            scissorRect.bottom = static_cast<LONG>(backBufferDesc.Height);
            commandList->RSSetScissorRects(1, &scissorRect);
        }
    }

    std::uint64_t D3D12Backend::GetSDLWindowFlags() const
    {
        return 0;
    }

    void D3D12Backend::RegisterNoesisPackages()
    {
        NsRegisterReflectionRenderD3D12RenderDevice();
        NsInitPackageRenderD3D12RenderDevice();
    }

    void D3D12Backend::ShutdownNoesisPackages()
    {
        NsShutdownPackageRenderD3D12RenderDevice();
    }

    void D3D12Backend::SyncTextureStates(Diligent::ITexture *backBufferTexture, Diligent::ITexture *depthBufferTexture)
    {
        Diligent::RefCntAutoPtr<Diligent::ITextureD3D12> backBufferTextureD3D12{backBufferTexture, Diligent::IID_TextureD3D12};
        if (backBufferTextureD3D12 != nullptr)
        {
            backBufferTextureD3D12->SetD3D12ResourceState(D3D12_RESOURCE_STATE_RENDER_TARGET);
        }

        Diligent::RefCntAutoPtr<Diligent::ITextureD3D12> depthBufferTextureD3D12{depthBufferTexture, Diligent::IID_TextureD3D12};
        if (depthBufferTextureD3D12 != nullptr)
        {
            depthBufferTextureD3D12->SetD3D12ResourceState(D3D12_RESOURCE_STATE_DEPTH_WRITE);
        }
    }

    bool D3D12Backend::InitDiligent(SDL_Window &window)
    {
        SDL_PropertiesID properties = SDL_GetWindowProperties(&window);
        void *nativeWindow = SDL_GetPointerProperty(properties, SDL_PROP_WINDOW_WIN32_HWND_POINTER, nullptr);
        if (nativeWindow == nullptr)
        {
            std::fprintf(stderr, "Failed to get native window handle from SDL: %s\n", SDL_GetError());
            return false;
        }

        Diligent::IEngineFactoryD3D12 *factory = Diligent::LoadAndGetEngineFactoryD3D12();
        if (factory == nullptr)
        {
            std::fprintf(stderr, "Failed to load Diligent D3D12 engine factory\n");
            return false;
        }

        Diligent::EngineD3D12CreateInfo engineCreateInfo;
        engineCreateInfo.Features.WireframeFill = Diligent::DEVICE_FEATURE_STATE_ENABLED;
        factory->CreateDeviceAndContextsD3D12(engineCreateInfo, &mDevice, &mImmediateContext);
        if (mDevice == nullptr || mImmediateContext == nullptr)
        {
            std::fprintf(stderr, "Failed to create Diligent D3D12 device/context\n");
            return false;
        }

        mDeviceD3D12 = Diligent::RefCntAutoPtr<Diligent::IRenderDeviceD3D12>{mDevice, Diligent::IID_RenderDeviceD3D12};
        mImmediateContextD3D12 = Diligent::RefCntAutoPtr<Diligent::IDeviceContextD3D12>{mImmediateContext, Diligent::IID_DeviceContextD3D12};
        if (mDeviceD3D12 == nullptr || mImmediateContextD3D12 == nullptr)
        {
            std::fprintf(stderr, "Failed to query D3D12 interfaces from Diligent objects\n");
            return false;
        }

        Diligent::FenceDesc fenceDesc;
        fenceDesc.Name = "NoesisFrameFence";
        fenceDesc.Type = Diligent::FENCE_TYPE_CPU_WAIT_ONLY;
        mDevice->CreateFence(fenceDesc, &mFrameFence);
        if (mFrameFence == nullptr)
        {
            std::fprintf(stderr, "Failed to create Diligent fence for Noesis\n");
            return false;
        }

        Diligent::RefCntAutoPtr<Diligent::IFenceD3D12> fenceD3D12{mFrameFence, Diligent::IID_FenceD3D12};
        if (fenceD3D12 == nullptr)
        {
            std::fprintf(stderr, "Failed to query D3D12 fence interface\n");
            return false;
        }
        mD3D12Fence = fenceD3D12->GetD3D12Fence();
        if (mD3D12Fence == nullptr)
        {
            std::fprintf(stderr, "Failed to get native D3D12 fence\n");
            return false;
        }

        Diligent::SwapChainDesc swapChainDesc;
        swapChainDesc.Width = mWindowWidth;
        swapChainDesc.Height = mWindowHeight;
        swapChainDesc.ColorBufferFormat = Diligent::TEX_FORMAT_RGBA8_UNORM_SRGB;
        swapChainDesc.DepthBufferFormat = Diligent::TEX_FORMAT_D24_UNORM_S8_UINT;

        Diligent::FullScreenModeDesc fullScreenModeDesc{};
        Diligent::Win32NativeWindow diligentWindow{nativeWindow};
        factory->CreateSwapChainD3D12(mDevice, mImmediateContext, swapChainDesc, fullScreenModeDesc, diligentWindow, &mSwapChain);
        if (mSwapChain == nullptr)
        {
            std::fprintf(stderr, "Failed to create Diligent D3D12 swap chain\n");
            return false;
        }

        return true;
    }

    Noesis::Ptr<Noesis::RenderDevice> D3D12Backend::CreateRenderDevice()
    {
        const Diligent::SwapChainDesc &swapChainDesc = mSwapChain->GetDesc();
        DXGI_FORMAT colorFormat = ToDXGIFormat(swapChainDesc.ColorBufferFormat);
        DXGI_FORMAT stencilFormat = ToDXGIFormat(swapChainDesc.DepthBufferFormat);
        bool sRGB = IsSRGBFormat(swapChainDesc.ColorBufferFormat);

        if (colorFormat == DXGI_FORMAT_UNKNOWN || stencilFormat == DXGI_FORMAT_UNKNOWN)
        {
            std::fprintf(stderr, "Unsupported D3D12 swap chain formats\n");
            return nullptr;
        }

        DXGI_SAMPLE_DESC sampleDesc{};
        sampleDesc.Count = 1;
        sampleDesc.Quality = 0;

        ID3D12Device *d3d12Device = mDeviceD3D12->GetD3D12Device();

        mNoesisDevice = NoesisApp::D3D12Factory::CreateDevice(
            d3d12Device, mD3D12Fence, colorFormat, stencilFormat, sampleDesc, sRGB);

        if (mNoesisDevice == nullptr)
        {
            std::fprintf(stderr, "Failed to create Noesis D3D12 render device\n");
            return nullptr;
        }

        return mNoesisDevice;
    }

    void D3D12Backend::RenderFrameImpl(
        Noesis::IView *view,
        double timeSeconds,
        Diligent::ITexture *backBufferTexture,
        Diligent::ITexture *depthBufferTexture)
    {
        const Diligent::TextureDesc &backBufferDesc = backBufferTexture->GetDesc();

        Diligent::ITextureView *rtvView = mSwapChain->GetCurrentBackBufferRTV();
        Diligent::ITextureView *dsvView = mSwapChain->GetDepthBufferDSV();
        Diligent::RefCntAutoPtr<Diligent::ITextureViewD3D12> rtvViewD3D12{rtvView, Diligent::IID_TextureViewD3D12};
        Diligent::RefCntAutoPtr<Diligent::ITextureViewD3D12> dsvViewD3D12{dsvView, Diligent::IID_TextureViewD3D12};
        Diligent::RefCntAutoPtr<Diligent::ITextureD3D12> backBufferTextureD3D12{backBufferTexture, Diligent::IID_TextureD3D12};
        Diligent::RefCntAutoPtr<Diligent::ITextureD3D12> depthBufferTextureD3D12{depthBufferTexture, Diligent::IID_TextureD3D12};
        if (rtvViewD3D12 == nullptr || dsvViewD3D12 == nullptr || backBufferTextureD3D12 == nullptr || depthBufferTextureD3D12 == nullptr)
        {
            return;
        }

        // Noesis uses this fence value to defer releasing per-frame resources until the
        // current frame is known to have finished on the GPU.
        const std::uint64_t frameFenceValue = mFrameNumber;

        // Give the fresh command list to Noesis
        ID3D12GraphicsCommandList *commandList = mImmediateContextD3D12->GetD3D12CommandList();
        NoesisApp::D3D12Factory::SetCommandList(mNoesisDevice, commandList, frameFenceValue);

        // Noesis rendering (offscreen + on-screen)
        view->Update(timeSeconds);
        view->GetRenderer()->UpdateRenderTree();
        view->GetRenderer()->RenderOffscreen();

        D3D12_RESOURCE_BARRIER barriers[2] = {};
        UINT barrierCount = 0;

        const D3D12_RESOURCE_STATES backBufferState = backBufferTextureD3D12->GetD3D12ResourceState();
        if (backBufferState != D3D12_RESOURCE_STATE_RENDER_TARGET)
        {
            D3D12_RESOURCE_BARRIER &barrier = barriers[barrierCount++];
            barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
            barrier.Transition.pResource = backBufferTextureD3D12->GetD3D12Texture();
            barrier.Transition.StateBefore = backBufferState;
            barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
            barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
            backBufferTextureD3D12->SetD3D12ResourceState(D3D12_RESOURCE_STATE_RENDER_TARGET);
        }

        const D3D12_RESOURCE_STATES depthBufferState = depthBufferTextureD3D12->GetD3D12ResourceState();
        if (depthBufferState != D3D12_RESOURCE_STATE_DEPTH_WRITE)
        {
            D3D12_RESOURCE_BARRIER &barrier = barriers[barrierCount++];
            barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
            barrier.Transition.pResource = depthBufferTextureD3D12->GetD3D12Texture();
            barrier.Transition.StateBefore = depthBufferState;
            barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_DEPTH_WRITE;
            barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
            depthBufferTextureD3D12->SetD3D12ResourceState(D3D12_RESOURCE_STATE_DEPTH_WRITE);
        }

        if (barrierCount > 0)
        {
            commandList->ResourceBarrier(barrierCount, barriers);
        }

        const float clearColor[] = {0.05f, 0.16f, 0.27f, 1.0f};
        const D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = rtvViewD3D12->GetCPUDescriptorHandle();
        const D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = dsvViewD3D12->GetCPUDescriptorHandle();
        commandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);
        commandList->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);
        BindRenderTargetsAndViewport(commandList, rtvHandle, dsvHandle, backBufferDesc);

        if (false)
        {
            mImmediateContext->Flush();
            mImmediateContext->InvalidateState();
            // Render3DScene();
        }

        // Keep the final Noesis pass bound to the current command list state instead of
        // assuming the original pointer remains valid after future Diligent work is added.
        commandList = mImmediateContextD3D12->GetD3D12CommandList();
        NoesisApp::D3D12Factory::SetCommandList(mNoesisDevice, commandList, frameFenceValue);
        BindRenderTargetsAndViewport(commandList, rtvHandle, dsvHandle, backBufferDesc);

        view->GetRenderer()->Render();

        // End any pending split barriers from Noesis
        NoesisApp::D3D12Factory::EndPendingSplitBarriers(mNoesisDevice);

        // Submit the command list after Noesis records into it before resetting Diligent state.
        mImmediateContext->Flush();

        // Invalidate Diligent's cached state after Noesis used the command list directly
        mImmediateContext->InvalidateState();

        // Sync texture states back to Diligent after Noesis rendering
        SyncTextureStates(backBufferTexture, depthBufferTexture);

        // Signal the fence for this frame
        mImmediateContext->EnqueueSignal(mFrameFence, frameFenceValue);
        ++mFrameNumber;

        mSwapChain->Present(1);
    }

    void D3D12Backend::ReleaseBackendResources()
    {
        mD3D12Fence = nullptr;
        mFrameFence.Release();
        mImmediateContextD3D12.Release();
        mDeviceD3D12.Release();
    }

    int RunD3D12App(const AppStartupOptions &startupOptions)
    {
        return RunBackendApp<D3D12Backend>(startupOptions);
    }
}
