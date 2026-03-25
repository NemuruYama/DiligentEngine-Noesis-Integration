#include "DiligentNoesisBackendBase.hpp"

#include "SDLPlatform.hpp"

namespace NoesisDiligent
{
    bool DiligentNoesisBackendBase::Initialize(PlatformWindow& window, std::uint32_t width, std::uint32_t height)
    {
        mWindow = &window;
        mWindowWidth = width;
        mWindowHeight = height;
        mPendingResize = false;
        return InitDiligent(window);
    }

    void DiligentNoesisBackendBase::UpdateSize(std::uint32_t width, std::uint32_t height)
    {
        mWindowWidth = width;
        mWindowHeight = height;
        mPendingResize = true;
    }

    bool DiligentNoesisBackendBase::QueryWindowPixelSize(std::uint32_t& width, std::uint32_t& height) const
    {
        if (mWindow == nullptr)
        {
            return false;
        }

        int pixelWidth = 0;
        int pixelHeight = 0;
        if (!mWindow->GetSizeInPixels(pixelWidth, pixelHeight))
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

    bool DiligentNoesisBackendBase::CanResizeSwapChain(std::uint32_t& width, std::uint32_t& height) const
    {
        if (mWindow == nullptr)
        {
            return false;
        }

        const std::uint64_t flags = mWindow->GetFlags();
        if ((flags & PlatformWindowFlag::Minimized) != 0)
        {
            return false;
        }

        return QueryWindowPixelSize(width, height);
    }

    bool DiligentNoesisBackendBase::OnSwapChainResized()
    {
        return true;
    }

    bool DiligentNoesisBackendBase::ResizeSwapChain()
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
        if (!OnSwapChainResized())
        {
            return false;
        }

        mPendingResize = false;
        return true;
    }

    DiligentNoesisBackendBase::RenderTargetExtent DiligentNoesisBackendBase::GetRenderTargetExtent(
        Diligent::ITexture *backBufferTexture,
        Diligent::ITexture *depthBufferTexture) const
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

    void DiligentNoesisBackendBase::RenderFrame(Noesis::IView *view, double timeSeconds)
    {
        if (view == nullptr || mSwapChain == nullptr)
        {
            return;
        }

        if (mPendingResize && !ResizeSwapChain())
        {
            return;
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

        Diligent::ITextureView* backBufferView = mSwapChain->GetCurrentBackBufferRTV();
        Diligent::ITextureView* depthBufferView = mSwapChain->GetDepthBufferDSV();
        if (backBufferView == nullptr || depthBufferView == nullptr)
        {
            return;
        }

        Diligent::ITexture *backBufferTexture = backBufferView->GetTexture();
        Diligent::ITexture *depthBufferTexture = depthBufferView->GetTexture();
        if (backBufferTexture == nullptr || depthBufferTexture == nullptr)
        {
            return;
        }

        const RenderTargetExtent extent = GetRenderTargetExtent(backBufferTexture, depthBufferTexture);
        if (extent.width == 0 || extent.height == 0)
        {
            return;
        }

        RenderFrameImpl(view, timeSeconds, backBufferTexture, depthBufferTexture);
    }

    void DiligentNoesisBackendBase::PrepareForNoesisShutdown()
    {
        if (mImmediateContext != nullptr)
        {
            mImmediateContext->Flush();
            mImmediateContext->WaitForIdle();
        }

        mNoesisDevice.Reset();
    }

    void DiligentNoesisBackendBase::Shutdown()
    {
        if (mImmediateContext != nullptr)
        {
            mImmediateContext->WaitForIdle();
        }

        ReleaseBackendResources();
        mSwapChain.Release();
        mImmediateContext.Release();
        mDevice.Release();
        mNoesisDevice.Reset();
        mWindow = nullptr;
        mPendingResize = false;
        mWindowWidth = 0;
        mWindowHeight = 0;
        mFrameNumber = 1;
    }
}
