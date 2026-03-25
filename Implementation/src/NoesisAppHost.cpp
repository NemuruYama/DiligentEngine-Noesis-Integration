#include "NoesisAppHost.hpp"

#include "MainMenuVisuals.hpp"

#include <NsApp/LocalFontProvider.h>
#include <NsApp/LocalTextureProvider.h>
#include <NsApp/LocalXamlProvider.h>
#include <NsApp/ThemeProviders.h>
#include <NsGui/InputEnums.h>
#include <NsGui/Uri.h>
#include <NsGui/IntegrationAPI.h>
#include <NsGui/IRenderer.h>
#include <NsGui/FrameworkElement.h>

#include <SDL3/SDL.h>
#include <SDL3/SDL_keyboard.h>

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
        using WindowPtr = std::unique_ptr<SDL_Window, void (*)(SDL_Window*)>;

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
                    SDL_Quit();
                }
            }

            bool initialized = false;
        };

        struct TextInputGuard
        {
            explicit TextInputGuard(SDL_Window& windowHandle) : window(windowHandle)
            {
            }

            ~TextInputGuard()
            {
                if (started)
                {
                    SDL_StopTextInput(&window.get());
                }
            }

            std::reference_wrapper<SDL_Window> window;
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

        Noesis::Key TranslateKey(SDL_Keycode key)
        {
            if (key >= SDLK_0 && key <= SDLK_9)
            {
                return static_cast<Noesis::Key>(Noesis::Key_D0 + (key - SDLK_0));
            }

            if (key >= SDLK_A && key <= SDLK_Z)
            {
                return static_cast<Noesis::Key>(Noesis::Key_A + (key - SDLK_A));
            }

            if (key >= SDLK_F1 && key <= SDLK_F12)
            {
                return static_cast<Noesis::Key>(Noesis::Key_F1 + (key - SDLK_F1));
            }

            switch (key)
            {
            case SDLK_BACKSPACE:
                return Noesis::Key_Back;
            case SDLK_TAB:
                return Noesis::Key_Tab;
            case SDLK_CLEAR:
                return Noesis::Key_Clear;
            case SDLK_RETURN:
            case SDLK_KP_ENTER:
            case SDLK_RETURN2:
                return Noesis::Key_Return;
            case SDLK_PAUSE:
                return Noesis::Key_Pause;
            case SDLK_CAPSLOCK:
                return Noesis::Key_CapsLock;
            case SDLK_ESCAPE:
                return Noesis::Key_Escape;
            case SDLK_SPACE:
                return Noesis::Key_Space;
            case SDLK_PAGEUP:
                return Noesis::Key_PageUp;
            case SDLK_PAGEDOWN:
                return Noesis::Key_PageDown;
            case SDLK_END:
                return Noesis::Key_End;
            case SDLK_HOME:
                return Noesis::Key_Home;
            case SDLK_LEFT:
                return Noesis::Key_Left;
            case SDLK_UP:
                return Noesis::Key_Up;
            case SDLK_RIGHT:
                return Noesis::Key_Right;
            case SDLK_DOWN:
                return Noesis::Key_Down;
            case SDLK_INSERT:
                return Noesis::Key_Insert;
            case SDLK_DELETE:
                return Noesis::Key_Delete;
            case SDLK_NUMLOCKCLEAR:
                return Noesis::Key_NumLock;
            case SDLK_SCROLLLOCK:
                return Noesis::Key_Scroll;
            case SDLK_LSHIFT:
                return Noesis::Key_LeftShift;
            case SDLK_RSHIFT:
                return Noesis::Key_RightShift;
            case SDLK_LCTRL:
                return Noesis::Key_LeftCtrl;
            case SDLK_RCTRL:
                return Noesis::Key_RightCtrl;
            case SDLK_LALT:
                return Noesis::Key_LeftAlt;
            case SDLK_RALT:
                return Noesis::Key_RightAlt;
            case SDLK_LGUI:
                return Noesis::Key_LWin;
            case SDLK_RGUI:
                return Noesis::Key_RWin;
            case SDLK_APPLICATION:
                return Noesis::Key_Apps;
            case SDLK_KP_0:
                return Noesis::Key_NumPad0;
            case SDLK_KP_1:
                return Noesis::Key_NumPad1;
            case SDLK_KP_2:
                return Noesis::Key_NumPad2;
            case SDLK_KP_3:
                return Noesis::Key_NumPad3;
            case SDLK_KP_4:
                return Noesis::Key_NumPad4;
            case SDLK_KP_5:
                return Noesis::Key_NumPad5;
            case SDLK_KP_6:
                return Noesis::Key_NumPad6;
            case SDLK_KP_7:
                return Noesis::Key_NumPad7;
            case SDLK_KP_8:
                return Noesis::Key_NumPad8;
            case SDLK_KP_9:
                return Noesis::Key_NumPad9;
            case SDLK_KP_MULTIPLY:
                return Noesis::Key_Multiply;
            case SDLK_KP_PLUS:
                return Noesis::Key_Add;
            case SDLK_KP_MINUS:
                return Noesis::Key_Subtract;
            case SDLK_KP_DECIMAL:
                return Noesis::Key_Decimal;
            case SDLK_KP_DIVIDE:
                return Noesis::Key_Divide;
            case SDLK_F10:
                return Noesis::Key_F10;
            default:
                return Noesis::Key_None;
            }
        }

        Noesis::MouseButton ToNoesisMouseButton(Uint8 button)
        {
            switch (button)
            {
            case SDL_BUTTON_LEFT:
                return Noesis::MouseButton_Left;
            case SDL_BUTTON_RIGHT:
                return Noesis::MouseButton_Right;
            case SDL_BUTTON_MIDDLE:
                return Noesis::MouseButton_Middle;
            case SDL_BUTTON_X1:
                return Noesis::MouseButton_XButton1;
            case SDL_BUTTON_X2:
                return Noesis::MouseButton_XButton2;
            default:
                return Noesis::MouseButton_Left;
            }
        }

        std::uint32_t DecodeNextUtf8Codepoint(const char *&text)
        {
            const unsigned char lead = static_cast<unsigned char>(*text++);
            if ((lead & 0x80u) == 0)
            {
                return lead;
            }

            if ((lead & 0xE0u) == 0xC0u)
            {
                const unsigned char trail0 = static_cast<unsigned char>(*text++);
                return ((lead & 0x1Fu) << 6) | (trail0 & 0x3Fu);
            }

            if ((lead & 0xF0u) == 0xE0u)
            {
                const unsigned char trail0 = static_cast<unsigned char>(*text++);
                const unsigned char trail1 = static_cast<unsigned char>(*text++);
                return ((lead & 0x0Fu) << 12) | ((trail0 & 0x3Fu) << 6) | (trail1 & 0x3Fu);
            }

            const unsigned char trail0 = static_cast<unsigned char>(*text++);
            const unsigned char trail1 = static_cast<unsigned char>(*text++);
            const unsigned char trail2 = static_cast<unsigned char>(*text++);
            return ((lead & 0x07u) << 18) | ((trail0 & 0x3Fu) << 12) | ((trail1 & 0x3Fu) << 6) | (trail2 & 0x3Fu);
        }

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

        void PollWindowSize(SDL_Window& window, std::uint32_t &width, std::uint32_t &height)
        {
            int pixelWidth = 0;
            int pixelHeight = 0;
            if (SDL_GetWindowSizeInPixels(&window, &pixelWidth, &pixelHeight))
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

        bool QueryLaunchDisplayResolution(SDL_Window& window, std::uint32_t& width, std::uint32_t& height)
        {
            const SDL_DisplayID display = SDL_GetDisplayForWindow(&window);
            if (display == 0)
            {
                return false;
            }

            const SDL_DisplayMode* displayMode = SDL_GetDesktopDisplayMode(display);
            if (displayMode == nullptr || displayMode->w <= 0 || displayMode->h <= 0)
            {
                return false;
            }

            width = static_cast<std::uint32_t>(displayMode->w);
            height = static_cast<std::uint32_t>(displayMode->h);
            return true;
        }

        bool QueryWindowDisplayBounds(SDL_Window& window, SDL_Rect& bounds)
        {
            const SDL_DisplayID display = SDL_GetDisplayForWindow(&window);
            if (display == 0)
            {
                return false;
            }

            return SDL_GetDisplayBounds(display, &bounds);
        }

        void InstallLocalAssetProviders()
        {
            const char* basePath = SDL_GetBasePath();
            std::string assetsPath = basePath ? std::string(basePath) + "assets" : "assets";

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

        int GetWindowResolutionPresetIndex(SDL_Window& window)
        {
            std::uint32_t width = 0;
            std::uint32_t height = 0;
            PollWindowSize(window, width, height);
            return FindResolutionPresetIndex(width, height);
        }

        int GetLaunchResolutionPresetIndex(SDL_Window& window, bool useDisplayResolution)
        {
            std::uint32_t width = 0;
            std::uint32_t height = 0;
            if (useDisplayResolution && QueryLaunchDisplayResolution(window, width, height))
            {
                return FindResolutionPresetIndex(width, height);
            }

            return GetWindowResolutionPresetIndex(window);
        }

        int GetCurrentWindowModeIndex(SDL_Window& window)
        {
            const SDL_WindowFlags flags = SDL_GetWindowFlags(&window);
            if ((flags & SDL_WINDOW_FULLSCREEN) != 0)
            {
                return SDL_GetWindowFullscreenMode(&window) == nullptr ?
                    static_cast<int>(WindowMode::Borderless) :
                    static_cast<int>(WindowMode::Fullscreen);
            }

            if ((flags & SDL_WINDOW_BORDERLESS) != 0)
            {
                return static_cast<int>(WindowMode::Borderless);
            }

            return static_cast<int>(WindowMode::Windowed);
        }

        bool LeaveFullscreen(SDL_Window& window)
        {
            if (!SDL_SetWindowFullscreen(&window, false))
            {
                std::fprintf(stderr, "SDL_SetWindowFullscreen(false) failed: %s\n", SDL_GetError());
                return false;
            }

            if (!SDL_SetWindowMouseGrab(&window, false))
            {
                std::fprintf(stderr, "SDL_SetWindowMouseGrab(false) failed: %s\n", SDL_GetError());
                return false;
            }

            SDL_SyncWindow(&window);
            return true;
        }

        bool ApplyExclusiveFullscreenMouseCapture(SDL_Window& window)
        {
#if NOESIS_DILIGENT_EXCLUSIVE_FULLSCREEN_CAPTURES_MOUSE
            if (!SDL_SetWindowMouseGrab(&window, true))
            {
                std::fprintf(stderr, "SDL_SetWindowMouseGrab(true) failed: %s\n", SDL_GetError());
                return false;
            }
#else
            if (!SDL_SetWindowMouseGrab(&window, false))
            {
                std::fprintf(stderr, "SDL_SetWindowMouseGrab(false) failed: %s\n", SDL_GetError());
                return false;
            }
#endif

            return true;
        }

        bool CenterWindowOnDisplay(SDL_Window& window, int width, int height)
        {
            SDL_Rect bounds{};
            if (!QueryWindowDisplayBounds(window, bounds))
            {
                std::fprintf(stderr, "SDL_GetDisplayBounds failed: %s\n", SDL_GetError());
                return false;
            }

            const int x = bounds.x + ((bounds.w - width) / 2);
            const int y = bounds.y + ((bounds.h - height) / 2);
            if (!SDL_SetWindowPosition(&window, x, y))
            {
                std::fprintf(stderr, "SDL_SetWindowPosition failed: %s\n", SDL_GetError());
                return false;
            }

            return true;
        }

        bool IsBorderlessFullscreenResolution(SDL_Window& window, int resolutionIndex)
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

        bool ApplyWindowedResolution(SDL_Window& window, int resolutionIndex)
        {
            const ResolutionPreset& preset = GetResolutionPreset(resolutionIndex);
            if (!LeaveFullscreen(window))
            {
                return false;
            }

            if (!SDL_SetWindowBordered(&window, true))
            {
                std::fprintf(stderr, "SDL_SetWindowBordered(true) failed: %s\n", SDL_GetError());
                return false;
            }

            if (!SDL_SetWindowSize(&window, preset.width, preset.height))
            {
                std::fprintf(stderr, "SDL_SetWindowSize failed: %s\n", SDL_GetError());
                return false;
            }

            CenterWindowOnDisplay(window, preset.width, preset.height);
            SDL_SyncWindow(&window);
            return true;
        }

        bool ApplyBorderlessWindowed(SDL_Window& window, int resolutionIndex)
        {
            const ResolutionPreset& preset = GetResolutionPreset(resolutionIndex);
            if (!LeaveFullscreen(window))
            {
                return false;
            }

            if (!SDL_SetWindowBordered(&window, false))
            {
                std::fprintf(stderr, "SDL_SetWindowBordered(false) failed: %s\n", SDL_GetError());
                return false;
            }

            if (!SDL_SetWindowSize(&window, preset.width, preset.height))
            {
                std::fprintf(stderr, "SDL_SetWindowSize failed: %s\n", SDL_GetError());
                return false;
            }

            if (!CenterWindowOnDisplay(window, preset.width, preset.height))
            {
                return false;
            }

            SDL_SyncWindow(&window);
            return true;
        }

        bool ApplyBorderlessFullscreen(SDL_Window& window)
        {
            if (!LeaveFullscreen(window))
            {
                return false;
            }

            if (!SDL_SetWindowBordered(&window, false))
            {
                std::fprintf(stderr, "SDL_SetWindowBordered(false) failed: %s\n", SDL_GetError());
                return false;
            }

            if (!SDL_SetWindowFullscreenMode(&window, nullptr))
            {
                std::fprintf(stderr, "SDL_SetWindowFullscreenMode(nullptr) failed: %s\n", SDL_GetError());
                return false;
            }

            if (!SDL_SetWindowFullscreen(&window, true))
            {
                std::fprintf(stderr, "SDL_SetWindowFullscreen(true) failed: %s\n", SDL_GetError());
                return false;
            }

            SDL_SyncWindow(&window);
            return true;
        }

        bool ApplyBorderlessMode(SDL_Window& window, int resolutionIndex)
        {
            if (IsBorderlessFullscreenResolution(window, resolutionIndex))
            {
                return ApplyBorderlessFullscreen(window);
            }

            return ApplyBorderlessWindowed(window, resolutionIndex);
        }

        bool ApplyExclusiveFullscreen(SDL_Window& window, int resolutionIndex)
        {
            const ResolutionPreset& preset = GetResolutionPreset(resolutionIndex);
            if (!LeaveFullscreen(window))
            {
                return false;
            }

            const SDL_DisplayID display = SDL_GetDisplayForWindow(&window);
            if (display == 0)
            {
                std::fprintf(stderr, "SDL_GetDisplayForWindow failed: %s\n", SDL_GetError());
                return false;
            }

            SDL_DisplayMode closestMode{};
            if (!SDL_GetClosestFullscreenDisplayMode(display, preset.width, preset.height, 0.0f, true, &closestMode))
            {
                std::fprintf(stderr, "SDL_GetClosestFullscreenDisplayMode failed: %s\n", SDL_GetError());
                return false;
            }

            if (!SDL_SetWindowFullscreenMode(&window, &closestMode))
            {
                std::fprintf(stderr, "SDL_SetWindowFullscreenMode(exclusive) failed: %s\n", SDL_GetError());
                return false;
            }

            if (!SDL_SetWindowFullscreen(&window, true))
            {
                std::fprintf(stderr, "SDL_SetWindowFullscreen(true) failed: %s\n", SDL_GetError());
                return false;
            }

            if (!ApplyExclusiveFullscreenMouseCapture(window))
            {
                return false;
            }

            SDL_SyncWindow(&window);
            return true;
        }

        bool ApplyWindowMode(SDL_Window& window, int windowModeIndex, int resolutionIndex)
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

        bool ApplyResolution(SDL_Window& window, int resolutionIndex)
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

        Noesis::Ptr<Noesis::IView> CreateMainMenuView(SDL_Window& window)
        {
            Visuals::MainMenuCallbacks callbacks;
            auto settingsState = std::make_shared<WindowSettingsState>();
            callbacks.onQuit = []()
            {
                SDL_Event quitEvent{};
                quitEvent.type = SDL_EVENT_QUIT;
                SDL_PushEvent(&quitEvent);
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
        if (!SDL_Init(SDL_INIT_VIDEO))
        {
            std::fprintf(stderr, "SDL_Init failed: %s\n", SDL_GetError());
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

        const std::uint64_t windowFlags = static_cast<std::uint64_t>(SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY) |
                                          backend.GetSDLWindowFlags();
        WindowPtr window{SDL_CreateWindow("NoesisDiligent", static_cast<int>(kInitialWidth), static_cast<int>(kInitialHeight), windowFlags), SDL_DestroyWindow};
        if (window == nullptr)
        {
            std::fprintf(stderr, "SDL_CreateWindow failed: %s\n", SDL_GetError());
            return 1;
        }

        const bool useLaunchDisplayResolution = startupOptions.useDisplayResolution;
        const int initialResolutionIndex = useLaunchDisplayResolution ?
            GetLaunchResolutionPresetIndex(*window, true) :
            kDefaultResolutionPresetIndex;
            
        if (!ApplyWindowMode(*window, static_cast<int>(startupOptions.windowMode), initialResolutionIndex))
        {
            std::fprintf(stderr, "Failed to apply startup window mode\n");
        }

        TextInputGuard textInput{*window};

        PollWindowSize(*window, windowWidth, windowHeight);

        if (!backend.Initialize(*window, windowWidth, windowHeight))
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

        view = CreateMainMenuView(*window);
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

        if (!SDL_StartTextInput(window.get()))
        {
            std::fprintf(stderr, "SDL_StartTextInput failed: %s\n", SDL_GetError());
        }
        else
        {
            textInput.started = true;
        }

        startTime = Clock::now();
        running = true;
        while (running)
        {
            SDL_Event event;
            while (SDL_PollEvent(&event))
            {
                switch (event.type)
                {
                case SDL_EVENT_QUIT:
                    running = false;
                    break;
                case SDL_EVENT_WINDOW_RESIZED:
                case SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED:
                    PollWindowSize(*window, windowWidth, windowHeight);
                    backend.UpdateSize(windowWidth, windowHeight);
                    UpdateViewSize(view, windowWidth, windowHeight);
                    break;
                case SDL_EVENT_WINDOW_FOCUS_GAINED:
                    if (view != nullptr)
                    {
                        view->Activate();
                    }
                    break;
                case SDL_EVENT_WINDOW_FOCUS_LOST:
                    if (view != nullptr)
                    {
                        view->Deactivate();
                    }
                    break;
                case SDL_EVENT_MOUSE_MOTION:
                    if (view != nullptr)
                    {
                        view->MouseMove(static_cast<int>(event.motion.x), static_cast<int>(event.motion.y));
                    }
                    break;
                case SDL_EVENT_MOUSE_BUTTON_DOWN:
                    if (view != nullptr)
                    {
                        if (event.button.clicks > 1)
                        {
                            view->MouseDoubleClick(static_cast<int>(event.button.x), static_cast<int>(event.button.y), ToNoesisMouseButton(event.button.button));
                        }
                        else
                        {
                            view->MouseButtonDown(static_cast<int>(event.button.x), static_cast<int>(event.button.y), ToNoesisMouseButton(event.button.button));
                        }
                    }
                    break;
                case SDL_EVENT_MOUSE_BUTTON_UP:
                    if (view != nullptr)
                    {
                        view->MouseButtonUp(static_cast<int>(event.button.x), static_cast<int>(event.button.y), ToNoesisMouseButton(event.button.button));
                    }
                    break;
                case SDL_EVENT_MOUSE_WHEEL:
                    if (view != nullptr)
                    {
                        if (event.wheel.integer_y != 0)
                        {
                            view->MouseWheel(static_cast<int>(event.wheel.mouse_x), static_cast<int>(event.wheel.mouse_y), event.wheel.integer_y * 120);
                        }

                        if (event.wheel.integer_x != 0)
                        {
                            view->MouseHWheel(static_cast<int>(event.wheel.mouse_x), static_cast<int>(event.wheel.mouse_y), event.wheel.integer_x * 120);
                        }
                    }
                    break;
                case SDL_EVENT_KEY_DOWN:
                    if (view != nullptr)
                    {
                        const Noesis::Key key = TranslateKey(event.key.key);
                        if (key != Noesis::Key_None)
                        {
                            view->KeyDown(key);
                        }
                    }
                    break;
                case SDL_EVENT_KEY_UP:
                    if (view != nullptr)
                    {
                        const Noesis::Key key = TranslateKey(event.key.key);
                        if (key != Noesis::Key_None)
                        {
                            view->KeyUp(key);
                        }
                    }
                    break;
                case SDL_EVENT_TEXT_INPUT:
                    if (view != nullptr && event.text.text != nullptr)
                    {
                        const char *text = event.text.text;
                        while (*text != '\0')
                        {
                            view->Char(DecodeNextUtf8Codepoint(text));
                        }
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