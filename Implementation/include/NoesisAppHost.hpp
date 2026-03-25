#pragma once

#include <cstdint>

#include <NsGui/IView.h>
#include <NsRender/RenderDevice.h>

struct SDL_Window;

namespace NoesisDiligent
{
    enum class WindowMode : int
    {
        Windowed = 0,
        Borderless = 1,
        Fullscreen = 2
    };

    struct AppStartupOptions
    {
        WindowMode windowMode = WindowMode::Borderless;
        bool useDisplayResolution = false;
    };

    class NoesisAppBackend
    {
    public:
        virtual ~NoesisAppBackend() = default;

        virtual std::uint64_t GetSDLWindowFlags() const = 0;
        virtual bool Initialize(SDL_Window& window, std::uint32_t width, std::uint32_t height) = 0;
        virtual void UpdateSize(std::uint32_t width, std::uint32_t height) = 0;
        virtual void RegisterNoesisPackages() = 0;
        virtual void ShutdownNoesisPackages() = 0;
        virtual Noesis::Ptr<Noesis::RenderDevice> CreateRenderDevice() = 0;
        virtual void RenderFrame(Noesis::IView *view, double timeSeconds) = 0;
        virtual void PrepareForNoesisShutdown() = 0;
        virtual void Shutdown() = 0;
    };

    int RunNoesisApp(NoesisAppBackend &backend, const AppStartupOptions& startupOptions = {});
}