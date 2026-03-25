#include "NoesisAppHost.hpp"

#include "MainMenuVisuals.hpp"

#include <NsApp/LocalFontProvider.h>
#include <NsApp/LocalTextureProvider.h>
#include <NsApp/LocalXamlProvider.h>
#include <NsApp/ThemeProviders.h>
#include <NsGui/Uri.h>
#include <NsGui/IntegrationAPI.h>
#include <NsGui/IRenderer.h>
#include <NsGui/FrameworkElement.h>

#include "PlatformNoesisInput.hpp"
#include "SDLPlatform.hpp"

#include <chrono>
#include <cstdint>
#include <cstdio>
#include <functional>
#include <memory>
#include <string>
#include <utility>

#ifndef NOESIS_DILIGENT_EXCLUSIVE_FULLSCREEN_CAPTURES_MOUSE
#define NOESIS_DILIGENT_EXCLUSIVE_FULLSCREEN_CAPTURES_MOUSE 1
#endif

extern "C" void NsRegisterReflectionAppProviders();
extern "C" void NsInitPackageAppProviders();
extern "C" void NsShutdownPackageAppProviders();
extern "C" void NsRegisterReflectionAppTheme();
extern "C" void NsInitPackageAppTheme();
extern "C" void NsShutdownPackageAppTheme();

namespace NoesisDiligent
{
    namespace
    {
        using Clock = std::chrono::steady_clock;

        constexpr std::uint32_t kInitialWidth = 1280;
        constexpr std::uint32_t kInitialHeight = 720;

        struct ResolutionPreset
        {
            int width;
            int height;
        };

        constexpr ResolutionPreset kResolutionPresets[] = {
            {1920, 1080},
            {1280, 720},
            {2560, 1440}
        };
        constexpr int kResolutionPresetCount = sizeof(kResolutionPresets) / sizeof(kResolutionPresets[0]);
        constexpr int kDefaultResolutionPresetIndex = 1;

        struct WindowSettingsState
        {
            int resolutionIndex = kDefaultResolutionPresetIndex;
            int windowModeIndex = static_cast<int>(WindowMode::Borderless);
        };

        void ShutdownCommonNoesisPackages();

        struct SDLSystemGuard
        {
            ~SDLSystemGuard()
            {
                if (initialized)
                {
                    PlatformQuit();
                }
            }

            bool initialized = false;
        };

        struct TextInputGuard
        {
            explicit TextInputGuard(PlatformWindow& windowHandle) : window(windowHandle)
            {
            }

            ~TextInputGuard()
            {
                if (started)
                {
                    window.get().StopTextInput();
                }
            }

            std::reference_wrapper<PlatformWindow> window;
            bool started = false;
        };

        struct AppRuntimeGuard
        {
            AppRuntimeGuard(NoesisAppBackend& appBackend,
                Noesis::Ptr<Noesis::IView>& appView,
                Noesis::Ptr<Noesis::RenderDevice>& appRenderDevice) :
                backend(appBackend),
                view(appView),
                renderDevice(appRenderDevice)
            {
            }

            ~AppRuntimeGuard()
            {
                if (view != nullptr)
                {
                    view->GetRenderer()->Shutdown();
                }

                view.Reset();
                renderDevice.Reset();

                if (backendInitialized)
                {
                    backend.PrepareForNoesisShutdown();
                }

                if (noesisInitialized)
                {
                    Noesis::GUI::Shutdown();
                }

                if (backendPackagesInitialized)
                {
                    backend.ShutdownNoesisPackages();
                }

                if (commonPackagesInitialized)
                {
                    ShutdownCommonNoesisPackages();
                }

                if (backendInitialized)
                {
                    backend.Shutdown();
                }
            }

            NoesisAppBackend& backend;
            Noesis::Ptr<Noesis::IView>& view;
            Noesis::Ptr<Noesis::RenderDevice>& renderDevice;
            bool backendInitialized = false;
            bool commonPackagesInitialized = false;
            bool backendPackagesInitialized = false;
            bool noesisInitialized = false;
        };

        void InitCommonNoesisPackages()
        {
            NsRegisterReflectionAppProviders();
            NsInitPackageAppProviders();
            NsRegisterReflectionAppTheme();
            NsInitPackageAppTheme();
        }

        void ShutdownCommonNoesisPackages()
        {
            NsShutdownPackageAppTheme();
            NsShutdownPackageAppProviders();
        }

        void PollWindowSize(PlatformWindow& window, std::uint32_t &width, std::uint32_t &height)
        {
            int pixelWidth = 0;
            int pixelHeight = 0;
            if (window.GetSizeInPixels(pixelWidth, pixelHeight))
            {
                width = static_cast<std::uint32_t>(pixelWidth);
                height = static_cast<std::uint32_t>(pixelHeight);
            }
        }

        void UpdateViewSize(const Noesis::Ptr<Noesis::IView> &view, std::uint32_t width, std::uint32_t height)
        {
            if (view != nullptr && width > 0 && height > 0)
            {
                view->SetSize(width, height);
            }
        }

        bool QueryLaunchDisplayResolution(PlatformWindow& window, std::uint32_t& width, std::uint32_t& height)
        {
            int displayWidth = 0;
            int displayHeight = 0;
            if (!window.GetDesktopDisplayMode(displayWidth, displayHeight))
            {
                return false;
            }

            width = static_cast<std::uint32_t>(displayWidth);
            height = static_cast<std::uint32_t>(displayHeight);
            return true;
        }

        bool QueryWindowDisplayBounds(PlatformWindow& window, int& bx, int& by, int& bw, int& bh)
        {
            return window.GetDisplayBounds(bx, by, bw, bh);
        }

        void InstallLocalAssetProviders()
        {
            std::string basePath = PlatformGetBasePath();
            std::string assetsPath = basePath.empty() ? "assets" : basePath + "assets";

            Noesis::GUI::SetXamlProvider(Noesis::MakePtr<NoesisApp::LocalXamlProvider>(assetsPath.c_str()));
            Noesis::GUI::SetTextureProvider(Noesis::MakePtr<NoesisApp::LocalTextureProvider>(assetsPath.c_str()));
            Noesis::GUI::SetFontProvider(Noesis::MakePtr<NoesisApp::LocalFontProvider>(assetsPath.c_str()));
        }

        const ResolutionPreset& GetResolutionPreset(int index)
        {
            if (index < 0 || index >= kResolutionPresetCount)
            {
                return kResolutionPresets[kDefaultResolutionPresetIndex];
            }

            return kResolutionPresets[index];
        }

        int FindResolutionPresetIndex(std::uint32_t width, std::uint32_t height)
        {
            int bestIndex = 0;
            std::uint32_t bestDistance = UINT32_MAX;
            for (int index = 0; index < kResolutionPresetCount; ++index)
            {
                const ResolutionPreset& preset = kResolutionPresets[index];
                const int widthDelta = static_cast<int>(width) - preset.width;
                const int heightDelta = static_cast<int>(height) - preset.height;
                const std::uint32_t distance =
                    static_cast<std::uint32_t>((widthDelta < 0 ? -widthDelta : widthDelta) +
                                               (heightDelta < 0 ? -heightDelta : heightDelta));
                if (distance < bestDistance)
                {
                    bestDistance = distance;
                    bestIndex = index;
                }
            }

            return bestIndex;
        }

        int GetWindowResolutionPresetIndex(PlatformWindow& window)
        {
            std::uint32_t width = 0;
            std::uint32_t height = 0;
            PollWindowSize(window, width, height);
            return FindResolutionPresetIndex(width, height);
        }

        int GetLaunchResolutionPresetIndex(PlatformWindow& window, bool useDisplayResolution)
        {
            std::uint32_t width = 0;
            std::uint32_t height = 0;
            if (useDisplayResolution && QueryLaunchDisplayResolution(window, width, height))
            {
                return FindResolutionPresetIndex(width, height);
            }

            return GetWindowResolutionPresetIndex(window);
        }

        int GetCurrentWindowModeIndex(PlatformWindow& window)
        {
            const std::uint64_t flags = window.GetFlags();
            if ((flags & PlatformWindowFlag::Fullscreen) != 0)
            {
                return window.HasExclusiveFullscreenMode() ?
                    static_cast<int>(WindowMode::Fullscreen) :
                    static_cast<int>(WindowMode::Borderless);
            }

            if ((flags & PlatformWindowFlag::Borderless) != 0)
            {
                return static_cast<int>(WindowMode::Borderless);
            }

            return static_cast<int>(WindowMode::Windowed);
        }

        bool LeaveFullscreen(PlatformWindow& window)
        {
            if (!window.SetFullscreen(false))
            {
                std::fprintf(stderr, "SetFullscreen(false) failed: %s\n", PlatformGetError());
                return false;
            }

            if (!window.SetMouseGrab(false))
            {
                std::fprintf(stderr, "SetMouseGrab(false) failed: %s\n", PlatformGetError());
                return false;
            }

            window.Sync();
            return true;
        }

        bool ApplyExclusiveFullscreenMouseCapture(PlatformWindow& window)
        {
#if NOESIS_DILIGENT_EXCLUSIVE_FULLSCREEN_CAPTURES_MOUSE
            if (!window.SetMouseGrab(true))
            {
                std::fprintf(stderr, "SetMouseGrab(true) failed: %s\n", PlatformGetError());
                return false;
            }
#else
            if (!window.SetMouseGrab(false))
            {
                std::fprintf(stderr, "SetMouseGrab(false) failed: %s\n", PlatformGetError());
                return false;
            }
#endif

            return true;
        }

        bool CenterWindowOnDisplay(PlatformWindow& window, int width, int height)
        {
            int bx = 0, by = 0, bw = 0, bh = 0;
            if (!QueryWindowDisplayBounds(window, bx, by, bw, bh))
            {
                std::fprintf(stderr, "GetDisplayBounds failed: %s\n", PlatformGetError());
                return false;
            }

            const int x = bx + ((bw - width) / 2);
            const int y = by + ((bh - height) / 2);
            if (!window.SetPosition(x, y))
            {
                std::fprintf(stderr, "SetPosition failed: %s\n", PlatformGetError());
                return false;
            }

            return true;
        }

        bool IsBorderlessFullscreenResolution(PlatformWindow& window, int resolutionIndex)
        {
            const ResolutionPreset& preset = GetResolutionPreset(resolutionIndex);
            std::uint32_t displayWidth = 0;
            std::uint32_t displayHeight = 0;
            if (!QueryLaunchDisplayResolution(window, displayWidth, displayHeight))
            {
                return false;
            }

            return preset.width == static_cast<int>(displayWidth) &&
                   preset.height == static_cast<int>(displayHeight);
        }

        bool ApplyWindowedResolution(PlatformWindow& window, int resolutionIndex)
        {
            const ResolutionPreset& preset = GetResolutionPreset(resolutionIndex);
            if (!LeaveFullscreen(window))
            {
                return false;
            }

            if (!window.SetBordered(true))
            {
                std::fprintf(stderr, "SetBordered(true) failed: %s\n", PlatformGetError());
                return false;
            }

            if (!window.SetSize(preset.width, preset.height))
            {
                std::fprintf(stderr, "SetSize failed: %s\n", PlatformGetError());
                return false;
            }

            CenterWindowOnDisplay(window, preset.width, preset.height);
            window.Sync();
            return true;
        }

        bool ApplyBorderlessWindowed(PlatformWindow& window, int resolutionIndex)
        {
            const ResolutionPreset& preset = GetResolutionPreset(resolutionIndex);
            if (!LeaveFullscreen(window))
            {
                return false;
            }

            if (!window.SetBordered(false))
            {
                std::fprintf(stderr, "SetBordered(false) failed: %s\n", PlatformGetError());
                return false;
            }

            if (!window.SetSize(preset.width, preset.height))
            {
                std::fprintf(stderr, "SetSize failed: %s\n", PlatformGetError());
                return false;
            }

            if (!CenterWindowOnDisplay(window, preset.width, preset.height))
            {
                return false;
            }

            window.Sync();
            return true;
        }

        bool ApplyBorderlessFullscreen(PlatformWindow& window)
        {
            if (!LeaveFullscreen(window))
            {
                return false;
            }

            if (!window.SetBordered(false))
            {
                std::fprintf(stderr, "SetBordered(false) failed: %s\n", PlatformGetError());
                return false;
            }

            if (!window.ClearFullscreenMode())
            {
                std::fprintf(stderr, "ClearFullscreenMode failed: %s\n", PlatformGetError());
                return false;
            }

            if (!window.SetFullscreen(true))
            {
                std::fprintf(stderr, "SetFullscreen(true) failed: %s\n", PlatformGetError());
                return false;
            }

            window.Sync();
            return true;
        }

        bool ApplyBorderlessMode(PlatformWindow& window, int resolutionIndex)
        {
            if (IsBorderlessFullscreenResolution(window, resolutionIndex))
            {
                return ApplyBorderlessFullscreen(window);
            }

            return ApplyBorderlessWindowed(window, resolutionIndex);
        }

        bool ApplyExclusiveFullscreen(PlatformWindow& window, int resolutionIndex)
        {
            const ResolutionPreset& preset = GetResolutionPreset(resolutionIndex);
            if (!LeaveFullscreen(window))
            {
                return false;
            }

            if (!window.SetExclusiveFullscreenMode(preset.width, preset.height))
            {
                std::fprintf(stderr, "SetExclusiveFullscreenMode failed: %s\n", PlatformGetError());
                return false;
            }

            if (!window.SetFullscreen(true))
            {
                std::fprintf(stderr, "SetFullscreen(true) failed: %s\n", PlatformGetError());
                return false;
            }

            if (!ApplyExclusiveFullscreenMouseCapture(window))
            {
                return false;
            }

            window.Sync();
            return true;
        }

        bool ApplyWindowMode(PlatformWindow& window, int windowModeIndex, int resolutionIndex)
        {
            switch (windowModeIndex)
            {
            case static_cast<int>(WindowMode::Windowed):
                return ApplyWindowedResolution(window, resolutionIndex);
            case static_cast<int>(WindowMode::Borderless):
                return ApplyBorderlessMode(window, resolutionIndex);
            case static_cast<int>(WindowMode::Fullscreen):
                return ApplyExclusiveFullscreen(window, resolutionIndex);
            default:
                return false;
            }
        }

        bool ApplyResolution(PlatformWindow& window, int resolutionIndex)
        {
            const int windowModeIndex = GetCurrentWindowModeIndex(window);
            switch (windowModeIndex)
            {
            case static_cast<int>(WindowMode::Windowed):
                return ApplyWindowedResolution(window, resolutionIndex);
            case static_cast<int>(WindowMode::Borderless):
                return ApplyBorderlessMode(window, resolutionIndex);
            case static_cast<int>(WindowMode::Fullscreen):
                return ApplyExclusiveFullscreen(window, resolutionIndex);
            default:
                return false;
            }
        }

        Noesis::Ptr<Noesis::IView> CreateMainMenuView(PlatformWindow& window)
        {
            Visuals::MainMenuCallbacks callbacks;
            auto settingsState = std::make_shared<WindowSettingsState>();
            callbacks.onQuit = []()
            {
                PlatformPushQuitEvent();
            };
            callbacks.initialResolutionIndex = GetLaunchResolutionPresetIndex(
                window,
                GetCurrentWindowModeIndex(window) == static_cast<int>(WindowMode::Fullscreen)
            );
            settingsState->resolutionIndex = callbacks.initialResolutionIndex;
            callbacks.initialWindowModeIndex = GetCurrentWindowModeIndex(window);
            settingsState->windowModeIndex = callbacks.initialWindowModeIndex;
            callbacks.onResolutionChanged = [&window, settingsState](int resolutionIndex)
            {
                if (!ApplyResolution(window, resolutionIndex))
                {
                    return false;
                }

                settingsState->resolutionIndex = resolutionIndex;
                return true;
            };
            callbacks.onWindowModeChanged = [&window, settingsState](int windowModeIndex)
            {
                if (!ApplyWindowMode(window, windowModeIndex, settingsState->resolutionIndex))
                {
                    return false;
                }

                settingsState->windowModeIndex = windowModeIndex;
                return true;
            };

            Noesis::Ptr<Noesis::FrameworkElement> root = Visuals::CreateMainMenuRoot(std::move(callbacks));
            if (root == nullptr)
            {
                return nullptr;
            }

            return Noesis::GUI::CreateView(root);
        }
    }

    int RunNoesisApp(NoesisAppBackend &backend, const AppStartupOptions& startupOptions)
    {
        SDLSystemGuard sdlSystem;
        if (!PlatformInit())
        {
            std::fprintf(stderr, "PlatformInit failed: %s\n", PlatformGetError());
            return 1;
        }
        sdlSystem.initialized = true;

        bool success = false;
        Clock::time_point startTime{};
        bool running = false;

        std::uint32_t windowWidth = kInitialWidth;
        std::uint32_t windowHeight = kInitialHeight;
        Noesis::Ptr<Noesis::IView> view;
        Noesis::Ptr<Noesis::RenderDevice> renderDevice;
        AppRuntimeGuard runtimeGuard{backend, view, renderDevice};

        const std::uint64_t windowFlags = PlatformWindowFlag::Resizable | PlatformWindowFlag::HighPixelDensity |
                                          backend.GetWindowFlags();
        PlatformWindow window{"NoesisDiligent", static_cast<int>(kInitialWidth), static_cast<int>(kInitialHeight), windowFlags};
        if (!window.IsValid())
        {
            std::fprintf(stderr, "PlatformWindow creation failed: %s\n", PlatformGetError());
            return 1;
        }

        const bool useLaunchDisplayResolution = startupOptions.useDisplayResolution;
        const int initialResolutionIndex = useLaunchDisplayResolution ?
            GetLaunchResolutionPresetIndex(window, true) :
            kDefaultResolutionPresetIndex;
            
        if (!ApplyWindowMode(window, static_cast<int>(startupOptions.windowMode), initialResolutionIndex))
        {
            std::fprintf(stderr, "Failed to apply startup window mode\n");
        }

        TextInputGuard textInput{window};

        PollWindowSize(window, windowWidth, windowHeight);

        if (!backend.Initialize(window, windowWidth, windowHeight))
        {
            return 1;
        }
        runtimeGuard.backendInitialized = true;

        InitCommonNoesisPackages();
        runtimeGuard.commonPackagesInitialized = true;
        backend.RegisterNoesisPackages();
        runtimeGuard.backendPackagesInitialized = true;

        Noesis::GUI::SetLogHandler([](const char*, std::uint32_t, std::uint32_t level, const char*, const char* msg)
        {
            const char* prefixes[] = {"T", "D", "I", "W", "E"};
            std::printf("[NOESIS/%s] %s\n", prefixes[level], msg);
            std::fflush(stdout);
        });

        Noesis::GUI::SetLicense(NS_LICENSE_NAME, NS_LICENSE_KEY);
        Noesis::GUI::Init();
        runtimeGuard.noesisInitialized = true;

        InstallLocalAssetProviders();

        renderDevice = backend.CreateRenderDevice();
        if (renderDevice == nullptr)
        {
            std::fprintf(stderr, "Failed to create Noesis render device\n");
            return 1;
        }

        NoesisApp::SetThemeProviders();
        Noesis::GUI::LoadApplicationResources(NoesisApp::Theme::DarkBlue());

        view = CreateMainMenuView(window);
        if (view == nullptr)
        {
            std::fprintf(stderr, "Failed to create Noesis view\n");
            return 1;
        }

        view->SetFlags(Noesis::RenderFlags_PPAA | Noesis::RenderFlags_LCD);
        view->GetRenderer()->Init(renderDevice);
        UpdateViewSize(view, windowWidth, windowHeight);
        view->Activate();
        success = true;

        if (!window.StartTextInput())
        {
            std::fprintf(stderr, "StartTextInput failed: %s\n", PlatformGetError());
        }
        else
        {
            textInput.started = true;
        }

        startTime = Clock::now();
        running = true;
        while (running)
        {
            PlatformEvent event;
            while (PlatformPollEvent(event))
            {
                switch (event.type)
                {
                case PlatformEventType::Quit:
                    running = false;
                    break;
                case PlatformEventType::WindowResized:
                case PlatformEventType::WindowPixelSizeChanged:
                    PollWindowSize(window, windowWidth, windowHeight);
                    backend.UpdateSize(windowWidth, windowHeight);
                    UpdateViewSize(view, windowWidth, windowHeight);
                    break;
                case PlatformEventType::WindowFocusGained:
                    if (view != nullptr)
                    {
                        view->Activate();
                    }
                    break;
                case PlatformEventType::WindowFocusLost:
                    if (view != nullptr)
                    {
                        view->Deactivate();
                    }
                    break;
                case PlatformEventType::MouseMotion:
                    if (view != nullptr)
                    {
                        view->MouseMove(static_cast<int>(event.motion.x), static_cast<int>(event.motion.y));
                    }
                    break;
                case PlatformEventType::MouseButtonDown:
                    if (view != nullptr)
                    {
                        if (event.button.clicks > 1)
                        {
                            view->MouseDoubleClick(static_cast<int>(event.button.x), static_cast<int>(event.button.y), PlatformMouseButtonToNoesisButton(event.button.button));
                        }
                        else
                        {
                            view->MouseButtonDown(static_cast<int>(event.button.x), static_cast<int>(event.button.y), PlatformMouseButtonToNoesisButton(event.button.button));
                        }
                    }
                    break;
                case PlatformEventType::MouseButtonUp:
                    if (view != nullptr)
                    {
                        view->MouseButtonUp(static_cast<int>(event.button.x), static_cast<int>(event.button.y), PlatformMouseButtonToNoesisButton(event.button.button));
                    }
                    break;
                case PlatformEventType::MouseWheel:
                    if (view != nullptr)
                    {
                        if (event.wheel.integerY != 0)
                        {
                            view->MouseWheel(static_cast<int>(event.wheel.mouseX), static_cast<int>(event.wheel.mouseY), event.wheel.integerY * 120);
                        }

                        if (event.wheel.integerX != 0)
                        {
                            view->MouseHWheel(static_cast<int>(event.wheel.mouseX), static_cast<int>(event.wheel.mouseY), event.wheel.integerX * 120);
                        }
                    }
                    break;
                case PlatformEventType::KeyDown:
                    if (view != nullptr)
                    {
                        const Noesis::Key key = PlatformKeyCodeToNoesisKey(event.key.key);
                        if (key != Noesis::Key_None)
                        {
                            view->KeyDown(key);
                        }
                    }
                    break;
                case PlatformEventType::KeyUp:
                    if (view != nullptr)
                    {
                        const Noesis::Key key = PlatformKeyCodeToNoesisKey(event.key.key);
                        if (key != Noesis::Key_None)
                        {
                            view->KeyUp(key);
                        }
                    }
                    break;
                case PlatformEventType::TextInput:
                    if (view != nullptr && event.text[0] != '\0')
                    {
                        SendUtf8TextToNoesis(*view, event.text);
                    }
                    break;
                default:
                    break;
                }
            }

            const std::chrono::duration<double> elapsed = Clock::now() - startTime;
            backend.RenderFrame(view, elapsed.count());
        }
        return success ? 0 : 1;
    }
}