#include "MainMenuVisuals.hpp"

#include <NsApp/LocalFontProvider.h>
#include <NsApp/LocalTextureProvider.h>
#include <NsApp/LocalXamlProvider.h>
#include <NsApp/ThemeProviders.h>
#include <NsGui/FrameworkElement.h>
#include <NsGui/IRenderer.h>
#include <NsGui/IView.h>
#include <NsGui/InputEnums.h>
#include <NsGui/IntegrationAPI.h>
#include <NsGui/Uri.h>
#include <NsRender/RenderContext.h>

#include <SDL3/SDL.h>
#include <SDL3/SDL_keyboard.h>
#include <SDL3/SDL_properties.h>

#include <cstdint>
#include <cstdio>
#include <memory>
#include <string>

extern "C" void NsRegisterReflectionAppProviders();
extern "C" void NsInitPackageAppProviders();
extern "C" void NsShutdownPackageAppProviders();
extern "C" void NsRegisterReflectionAppTheme();
extern "C" void NsInitPackageAppTheme();
extern "C" void NsShutdownPackageAppTheme();
extern "C" void NsRegisterReflectionRenderRenderContext();
extern "C" void NsInitPackageRenderRenderContext();
extern "C" void NsShutdownPackageRenderRenderContext();
extern "C" void NsRegisterReflectionRenderVKRenderDevice();
extern "C" void NsInitPackageRenderVKRenderDevice();
extern "C" void NsShutdownPackageRenderVKRenderDevice();
extern "C" void NsRegisterReflectionRenderVKRenderContext();
extern "C" void NsInitPackageRenderVKRenderContext();
extern "C" void NsShutdownPackageRenderVKRenderContext();

namespace
{
    using NoesisDiligent::Visuals::CreateMainMenuRoot;
    using NoesisDiligent::Visuals::MainMenuCallbacks;

    Noesis::Ptr<Noesis::IView> gView;
    Noesis::Ptr<NoesisApp::RenderContext> gRenderContext;

    constexpr int kDefaultWidth = 1280;
    constexpr int kDefaultHeight = 720;

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

    enum class WindowMode : int
    {
        Windowed = 0,
        Borderless = 1,
        Fullscreen = 2
    };

    void InitNoesisPackages()
    {
        NsRegisterReflectionAppProviders();
        NsInitPackageAppProviders();
        NsRegisterReflectionAppTheme();
        NsInitPackageAppTheme();
        NsRegisterReflectionRenderRenderContext();
        NsInitPackageRenderRenderContext();
        NsRegisterReflectionRenderVKRenderDevice();
        NsInitPackageRenderVKRenderDevice();
        NsRegisterReflectionRenderVKRenderContext();
        NsInitPackageRenderVKRenderContext();
    }

    void ShutdownNoesisPackages()
    {
        NsShutdownPackageRenderVKRenderContext();
        NsShutdownPackageRenderVKRenderDevice();
        NsShutdownPackageRenderRenderContext();
        NsShutdownPackageAppTheme();
        NsShutdownPackageAppProviders();
    }

    void InstallLocalAssetProviders()
    {
        const char* basePath = SDL_GetBasePath();
        std::string assetsPath = basePath ? std::string(basePath) + "assets" : "assets";

        Noesis::GUI::SetXamlProvider(Noesis::MakePtr<NoesisApp::LocalXamlProvider>(assetsPath.c_str()));
        Noesis::GUI::SetTextureProvider(Noesis::MakePtr<NoesisApp::LocalTextureProvider>(assetsPath.c_str()));
        Noesis::GUI::SetFontProvider(Noesis::MakePtr<NoesisApp::LocalFontProvider>(assetsPath.c_str()));
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
        default:
            return Noesis::Key_None;
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

    void UpdateViewSize(SDL_Window *window)
    {
        int width = 0;
        int height = 0;
        if (SDL_GetWindowSizeInPixels(window, &width, &height) && gView != nullptr)
        {
            gView->SetSize(static_cast<std::uint32_t>(width), static_cast<std::uint32_t>(height));
        }
    }

    int FindResolutionPresetIndex(int width, int height)
    {
        int bestIndex = 0;
        int bestDistance = 0x7fffffff;
        for (int index = 0; index < kResolutionPresetCount; ++index)
        {
            const int widthDelta = width - kResolutionPresets[index].width;
            const int heightDelta = height - kResolutionPresets[index].height;
            const int distance = (widthDelta < 0 ? -widthDelta : widthDelta) +
                (heightDelta < 0 ? -heightDelta : heightDelta);
            if (distance < bestDistance)
            {
                bestDistance = distance;
                bestIndex = index;
            }
        }

        return bestIndex;
    }

    int GetWindowResolutionPresetIndex(SDL_Window *window)
    {
        int width = 0;
        int height = 0;
        if (!SDL_GetWindowSizeInPixels(window, &width, &height))
        {
            return kDefaultResolutionPresetIndex;
        }

        return FindResolutionPresetIndex(width, height);
    }

    int GetCurrentWindowModeIndex(SDL_Window *window)
    {
        const SDL_WindowFlags flags = SDL_GetWindowFlags(window);
        if ((flags & SDL_WINDOW_FULLSCREEN) != 0)
        {
            return SDL_GetWindowFullscreenMode(window) == nullptr ?
                static_cast<int>(WindowMode::Borderless) :
                static_cast<int>(WindowMode::Fullscreen);
        }

        if ((flags & SDL_WINDOW_BORDERLESS) != 0)
        {
            return static_cast<int>(WindowMode::Borderless);
        }

        return static_cast<int>(WindowMode::Windowed);
    }

    bool LeaveFullscreen(SDL_Window *window)
    {
        if (!SDL_SetWindowFullscreen(window, false))
        {
            std::fprintf(stderr, "SDL_SetWindowFullscreen(false) failed: %s\n", SDL_GetError());
            return false;
        }

        if (!SDL_SetWindowMouseGrab(window, false))
        {
            std::fprintf(stderr, "SDL_SetWindowMouseGrab(false) failed: %s\n", SDL_GetError());
            return false;
        }

        SDL_SyncWindow(window);
        return true;
    }

    bool CenterWindowOnDisplay(SDL_Window *window, int width, int height)
    {
        const SDL_DisplayID display = SDL_GetDisplayForWindow(window);
        if (display == 0)
        {
            std::fprintf(stderr, "SDL_GetDisplayForWindow failed: %s\n", SDL_GetError());
            return false;
        }

        SDL_Rect bounds{};
        if (!SDL_GetDisplayBounds(display, &bounds))
        {
            std::fprintf(stderr, "SDL_GetDisplayBounds failed: %s\n", SDL_GetError());
            return false;
        }

        const int x = bounds.x + ((bounds.w - width) / 2);
        const int y = bounds.y + ((bounds.h - height) / 2);
        if (!SDL_SetWindowPosition(window, x, y))
        {
            std::fprintf(stderr, "SDL_SetWindowPosition failed: %s\n", SDL_GetError());
            return false;
        }

        return true;
    }

    bool ApplyWindowedResolution(SDL_Window *window, int resolutionIndex)
    {
        if (resolutionIndex < 0 || resolutionIndex >= kResolutionPresetCount)
        {
            return false;
        }

        const ResolutionPreset& preset = kResolutionPresets[resolutionIndex];
        if (!LeaveFullscreen(window))
        {
            return false;
        }

        if (!SDL_SetWindowBordered(window, true))
        {
            std::fprintf(stderr, "SDL_SetWindowBordered(true) failed: %s\n", SDL_GetError());
            return false;
        }

        if (!SDL_SetWindowSize(window, preset.width, preset.height))
        {
            std::fprintf(stderr, "SDL_SetWindowSize failed: %s\n", SDL_GetError());
            return false;
        }

        CenterWindowOnDisplay(window, preset.width, preset.height);
        SDL_SyncWindow(window);
        return true;
    }

    bool ApplyBorderlessMode(SDL_Window *window, int resolutionIndex)
    {
        if (resolutionIndex < 0 || resolutionIndex >= kResolutionPresetCount)
        {
            return false;
        }

        const ResolutionPreset& preset = kResolutionPresets[resolutionIndex];
        if (!LeaveFullscreen(window))
        {
            return false;
        }

        if (!SDL_SetWindowBordered(window, false))
        {
            std::fprintf(stderr, "SDL_SetWindowBordered(false) failed: %s\n", SDL_GetError());
            return false;
        }

        if (!SDL_SetWindowSize(window, preset.width, preset.height))
        {
            std::fprintf(stderr, "SDL_SetWindowSize failed: %s\n", SDL_GetError());
            return false;
        }

        CenterWindowOnDisplay(window, preset.width, preset.height);
        SDL_SyncWindow(window);
        return true;
    }

    bool ApplyExclusiveFullscreen(SDL_Window *window, int resolutionIndex)
    {
        if (resolutionIndex < 0 || resolutionIndex >= kResolutionPresetCount)
        {
            return false;
        }

        const ResolutionPreset& preset = kResolutionPresets[resolutionIndex];
        if (!LeaveFullscreen(window))
        {
            return false;
        }

        const SDL_DisplayID display = SDL_GetDisplayForWindow(window);
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

        if (!SDL_SetWindowFullscreenMode(window, &closestMode))
        {
            std::fprintf(stderr, "SDL_SetWindowFullscreenMode failed: %s\n", SDL_GetError());
            return false;
        }

        if (!SDL_SetWindowFullscreen(window, true))
        {
            std::fprintf(stderr, "SDL_SetWindowFullscreen(true) failed: %s\n", SDL_GetError());
            return false;
        }

        if (!SDL_SetWindowMouseGrab(window, true))
        {
            std::fprintf(stderr, "SDL_SetWindowMouseGrab(true) failed: %s\n", SDL_GetError());
            return false;
        }

        SDL_SyncWindow(window);
        return true;
    }

    bool ApplyWindowMode(SDL_Window *window, int windowModeIndex, int resolutionIndex)
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

    bool ApplyResolution(SDL_Window *window, int resolutionIndex)
    {
        return ApplyWindowMode(window, GetCurrentWindowModeIndex(window), resolutionIndex);
    }

    Noesis::Ptr<Noesis::IView> CreateMainMenuView(SDL_Window *window)
    {
        MainMenuCallbacks callbacks;
        const auto activeResolutionIndex = std::make_shared<int>(GetWindowResolutionPresetIndex(window));

        callbacks.onQuit = []()
        {
            SDL_Event quitEvent{};
            quitEvent.type = SDL_EVENT_QUIT;
            SDL_PushEvent(&quitEvent);
        };
        callbacks.initialResolutionIndex = *activeResolutionIndex;
        callbacks.initialWindowModeIndex = GetCurrentWindowModeIndex(window);
        callbacks.onResolutionChanged = [window, activeResolutionIndex](int resolutionIndex)
        {
            if (!ApplyResolution(window, resolutionIndex))
            {
                return false;
            }

            *activeResolutionIndex = resolutionIndex;
            return true;
        };
        callbacks.onWindowModeChanged = [window, activeResolutionIndex](int windowModeIndex)
        {
            return ApplyWindowMode(window, windowModeIndex, *activeResolutionIndex);
        };

        Noesis::Ptr<Noesis::FrameworkElement> root = CreateMainMenuRoot(std::move(callbacks));
        if (root == nullptr)
        {
            return nullptr;
        }

        return Noesis::GUI::CreateView(root);
    }

    bool NoesisInit(SDL_Window *window)
    {
        InitNoesisPackages();

        Noesis::GUI::SetLogHandler([](const char *, std::uint32_t, std::uint32_t level, const char *, const char *msg)
        {
            const char* prefixes[] = {"T", "D", "I", "W", "E"};
            std::printf("[NOESIS/%s] %s\n", prefixes[level], msg);
            std::fflush(stdout);
        });

        Noesis::GUI::SetLicense(NS_LICENSE_NAME, NS_LICENSE_KEY);
        Noesis::GUI::Init();

        InstallLocalAssetProviders();
        NoesisApp::SetThemeProviders();
        Noesis::GUI::LoadApplicationResources(NoesisApp::Theme::DarkBlue());

        gView = CreateMainMenuView(window);
        if (gView == nullptr)
        {
            return false;
        }

        gView->SetFlags(Noesis::RenderFlags_PPAA | Noesis::RenderFlags_LCD);

        SDL_PropertiesID properties = SDL_GetWindowProperties(window);
        void *nativeWindow = SDL_GetPointerProperty(properties, SDL_PROP_WINDOW_WIN32_HWND_POINTER, nullptr);
        if (nativeWindow == nullptr)
        {
            SDL_Log("Failed to get native Win32 window handle from SDL: %s", SDL_GetError());
            return false;
        }

        std::uint32_t sampleCount = 1;
        gRenderContext = NoesisApp::RenderContext::Create("VK");
        gRenderContext->Init(nativeWindow, sampleCount, true, false);

        gView->GetRenderer()->Init(gRenderContext->GetDevice());
        UpdateViewSize(window);
        gView->Activate();
        return true;
    }

    void RenderFrame(SDL_Window *window, double timeSeconds)
    {
        if (gView == nullptr || gRenderContext == nullptr)
        {
            return;
        }

        int width = 0;
        int height = 0;
        if (!SDL_GetWindowSizeInPixels(window, &width, &height))
        {
            return;
        }

        gView->Update(timeSeconds);

        gRenderContext->BeginRender();
        gView->GetRenderer()->UpdateRenderTree();
        gView->GetRenderer()->RenderOffscreen();
        gRenderContext->SetDefaultRenderTarget(static_cast<std::uint32_t>(width), static_cast<std::uint32_t>(height), true);
        gView->GetRenderer()->Render();
        gRenderContext->EndRender();
        gRenderContext->Swap();
    }
}

int main(int argc, char **argv)
{
    (void)argc;
    (void)argv;

    if (!SDL_Init(SDL_INIT_VIDEO))
    {
        SDL_Log("SDL_Init failed: %s", SDL_GetError());
        return 1;
    }

    SDL_Window *window = SDL_CreateWindow("NoesisSDL", kDefaultWidth, kDefaultHeight, SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY);
    if (window == nullptr)
    {
        SDL_Log("SDL_CreateWindow failed: %s", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    if (!NoesisInit(window))
    {
        if (gRenderContext != nullptr)
        {
            gRenderContext->Shutdown();
            gRenderContext.Reset();
        }

        gView.Reset();
        Noesis::GUI::Shutdown();
        ShutdownNoesisPackages();
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    bool textInputStarted = false;
    if (SDL_StartTextInput(window))
    {
        textInputStarted = true;
    }

    bool running = true;
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
                UpdateViewSize(window);
                if (gRenderContext != nullptr)
                {
                    gRenderContext->Resize();
                }
                break;
            case SDL_EVENT_WINDOW_FOCUS_GAINED:
                if (gView != nullptr)
                {
                    gView->Activate();
                }
                break;
            case SDL_EVENT_WINDOW_FOCUS_LOST:
                if (gView != nullptr)
                {
                    gView->Deactivate();
                }
                break;
            case SDL_EVENT_MOUSE_MOTION:
                if (gView != nullptr)
                {
                    gView->MouseMove(static_cast<int>(event.motion.x), static_cast<int>(event.motion.y));
                }
                break;
            case SDL_EVENT_MOUSE_BUTTON_DOWN:
                if (gView != nullptr)
                {
                    if (event.button.clicks > 1)
                    {
                        gView->MouseDoubleClick(
                            static_cast<int>(event.button.x),
                            static_cast<int>(event.button.y),
                            ToNoesisMouseButton(event.button.button));
                    }
                    else
                    {
                        gView->MouseButtonDown(
                            static_cast<int>(event.button.x),
                            static_cast<int>(event.button.y),
                            ToNoesisMouseButton(event.button.button));
                    }
                }
                break;
            case SDL_EVENT_MOUSE_BUTTON_UP:
                if (gView != nullptr)
                {
                    gView->MouseButtonUp(
                        static_cast<int>(event.button.x),
                        static_cast<int>(event.button.y),
                        ToNoesisMouseButton(event.button.button));
                }
                break;
            case SDL_EVENT_MOUSE_WHEEL:
                if (gView != nullptr)
                {
                    if (event.wheel.integer_y != 0)
                    {
                        gView->MouseWheel(
                            static_cast<int>(event.wheel.mouse_x),
                            static_cast<int>(event.wheel.mouse_y),
                            event.wheel.integer_y * 120);
                    }

                    if (event.wheel.integer_x != 0)
                    {
                        gView->MouseHWheel(
                            static_cast<int>(event.wheel.mouse_x),
                            static_cast<int>(event.wheel.mouse_y),
                            event.wheel.integer_x * 120);
                    }
                }
                break;
            case SDL_EVENT_KEY_DOWN:
                if (gView != nullptr)
                {
                    const Noesis::Key key = TranslateKey(event.key.key);
                    if (key != Noesis::Key_None)
                    {
                        gView->KeyDown(key);
                    }
                }
                break;
            case SDL_EVENT_KEY_UP:
                if (gView != nullptr)
                {
                    const Noesis::Key key = TranslateKey(event.key.key);
                    if (key != Noesis::Key_None)
                    {
                        gView->KeyUp(key);
                    }
                }
                break;
            case SDL_EVENT_TEXT_INPUT:
                if (gView != nullptr && event.text.text != nullptr)
                {
                    const char *text = event.text.text;
                    while (*text != '\0')
                    {
                        gView->Char(DecodeNextUtf8Codepoint(text));
                    }
                }
                break;
            default:
                break;
            }
        }

        RenderFrame(window, static_cast<double>(SDL_GetTicks()) / 1000.0);
    }

    if (textInputStarted)
    {
        SDL_StopTextInput(window);
    }

    if (gView != nullptr)
    {
        gView->GetRenderer()->Shutdown();
    }

    if (gRenderContext != nullptr)
    {
        gRenderContext->Shutdown();
        gRenderContext.Reset();
    }

    gView.Reset();
    Noesis::GUI::Shutdown();
    ShutdownNoesisPackages();

    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
