#include "SDLPlatform.hpp"

#include <SDL3/SDL.h>
#include <SDL3/SDL_keyboard.h>
#include <SDL3/SDL_properties.h>
#include <SDL3/SDL_vulkan.h>

#include <cstdio>
#include <cstring>

namespace NoesisDiligent
{
    namespace
    {
        SDL_Window *ToSDL(void *window)
        {
            return static_cast<SDL_Window *>(window);
        }

        std::uint64_t TranslatePlatformFlagsToSDL(std::uint64_t flags)
        {
            std::uint64_t sdlFlags = 0;
            if (flags & PlatformWindowFlag::Resizable)        sdlFlags |= SDL_WINDOW_RESIZABLE;
            if (flags & PlatformWindowFlag::HighPixelDensity)  sdlFlags |= SDL_WINDOW_HIGH_PIXEL_DENSITY;
            if (flags & PlatformWindowFlag::Vulkan)            sdlFlags |= SDL_WINDOW_VULKAN;
            if (flags & PlatformWindowFlag::Fullscreen)        sdlFlags |= SDL_WINDOW_FULLSCREEN;
            if (flags & PlatformWindowFlag::Borderless)        sdlFlags |= SDL_WINDOW_BORDERLESS;
            return sdlFlags;
        }

        std::uint64_t TranslateSDLFlagsToPlatform(SDL_WindowFlags sdlFlags)
        {
            std::uint64_t flags = 0;
            if (sdlFlags & SDL_WINDOW_RESIZABLE)          flags |= PlatformWindowFlag::Resizable;
            if (sdlFlags & SDL_WINDOW_HIGH_PIXEL_DENSITY)  flags |= PlatformWindowFlag::HighPixelDensity;
            if (sdlFlags & SDL_WINDOW_VULKAN)              flags |= PlatformWindowFlag::Vulkan;
            if (sdlFlags & SDL_WINDOW_FULLSCREEN)          flags |= PlatformWindowFlag::Fullscreen;
            if (sdlFlags & SDL_WINDOW_BORDERLESS)          flags |= PlatformWindowFlag::Borderless;
            if (sdlFlags & SDL_WINDOW_MINIMIZED)           flags |= PlatformWindowFlag::Minimized;
            return flags;
        }

        PlatformKeyCode TranslateSDLScancode(SDL_Scancode scancode)
        {
            const int value = static_cast<int>(scancode);
            if (value < static_cast<int>(PlatformKeyCode::Unknown) ||
                value > static_cast<int>(PlatformKeyCode::Last))
            {
                return PlatformKeyCode::Unknown;
            }

            return static_cast<PlatformKeyCode>(value);
        }

        PlatformMouseButton TranslateSDLMouseButton(Uint8 sdlButton)
        {
            switch (sdlButton)
            {
            case SDL_BUTTON_LEFT:   return PlatformMouseButton::Left;
            case SDL_BUTTON_MIDDLE: return PlatformMouseButton::Middle;
            case SDL_BUTTON_RIGHT:  return PlatformMouseButton::Right;
            case SDL_BUTTON_X1:     return PlatformMouseButton::X1;
            case SDL_BUTTON_X2:     return PlatformMouseButton::X2;
            default:                return PlatformMouseButton::Left;
            }
        }
    }

    // ── System functions ────────────────────────────────────────────────

    bool PlatformInit()
    {
        return SDL_Init(SDL_INIT_VIDEO);
    }

    void PlatformQuit()
    {
        SDL_Quit();
    }

    const char *PlatformGetError()
    {
        return SDL_GetError();
    }

    std::string PlatformGetBasePath()
    {
        const char *basePath = SDL_GetBasePath();
        return basePath ? std::string(basePath) : std::string{};
    }

    std::uint64_t PlatformGetTicks()
    {
        return SDL_GetTicks();
    }

    // ── Event system ────────────────────────────────────────────────────

    bool PlatformPollEvent(PlatformEvent &platformEvent)
    {
        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            platformEvent = PlatformEvent{};
            switch (event.type)
            {
            case SDL_EVENT_QUIT:
                platformEvent.type = PlatformEventType::Quit;
                return true;
            case SDL_EVENT_WINDOW_RESIZED:
                platformEvent.type = PlatformEventType::WindowResized;
                return true;
            case SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED:
                platformEvent.type = PlatformEventType::WindowPixelSizeChanged;
                return true;
            case SDL_EVENT_WINDOW_FOCUS_GAINED:
                platformEvent.type = PlatformEventType::WindowFocusGained;
                return true;
            case SDL_EVENT_WINDOW_FOCUS_LOST:
                platformEvent.type = PlatformEventType::WindowFocusLost;
                return true;
            case SDL_EVENT_MOUSE_MOTION:
                platformEvent.type = PlatformEventType::MouseMotion;
                platformEvent.motion.x = event.motion.x;
                platformEvent.motion.y = event.motion.y;
                return true;
            case SDL_EVENT_MOUSE_BUTTON_DOWN:
                platformEvent.type = PlatformEventType::MouseButtonDown;
                platformEvent.button.x = event.button.x;
                platformEvent.button.y = event.button.y;
                platformEvent.button.button = TranslateSDLMouseButton(event.button.button);
                platformEvent.button.clicks = event.button.clicks;
                return true;
            case SDL_EVENT_MOUSE_BUTTON_UP:
                platformEvent.type = PlatformEventType::MouseButtonUp;
                platformEvent.button.x = event.button.x;
                platformEvent.button.y = event.button.y;
                platformEvent.button.button = TranslateSDLMouseButton(event.button.button);
                platformEvent.button.clicks = event.button.clicks;
                return true;
            case SDL_EVENT_MOUSE_WHEEL:
                platformEvent.type = PlatformEventType::MouseWheel;
                platformEvent.wheel.mouseX = event.wheel.mouse_x;
                platformEvent.wheel.mouseY = event.wheel.mouse_y;
                platformEvent.wheel.integerX = event.wheel.integer_x;
                platformEvent.wheel.integerY = event.wheel.integer_y;
                return true;
            case SDL_EVENT_KEY_DOWN:
                platformEvent.type = PlatformEventType::KeyDown;
                platformEvent.key.key = TranslateSDLScancode(event.key.scancode);
                platformEvent.key.repeat = event.key.repeat;
                return true;
            case SDL_EVENT_KEY_UP:
                platformEvent.type = PlatformEventType::KeyUp;
                platformEvent.key.key = TranslateSDLScancode(event.key.scancode);
                platformEvent.key.repeat = false;
                return true;
            case SDL_EVENT_TEXT_INPUT:
                if (event.text.text != nullptr)
                {
                    platformEvent.type = PlatformEventType::TextInput;
                    std::strncpy(platformEvent.text, event.text.text, sizeof(platformEvent.text) - 1);
                    platformEvent.text[sizeof(platformEvent.text) - 1] = '\0';
                    return true;
                }
                continue;
            default:
                continue;
            }
        }
        return false;
    }

    void PlatformPushQuitEvent()
    {
        SDL_Event event{};
        event.type = SDL_EVENT_QUIT;
        SDL_PushEvent(&event);
    }

    // ── Vulkan support ──────────────────────────────────────────────────

    void *PlatformGetVulkanInstanceProcAddr()
    {
        return reinterpret_cast<void *>(SDL_Vulkan_GetVkGetInstanceProcAddr());
    }

    // ── PlatformWindow ──────────────────────────────────────────────────

    PlatformWindow::PlatformWindow(const char *title, int width, int height, std::uint64_t flags)
    {
        const std::uint64_t sdlFlags = TranslatePlatformFlagsToSDL(flags);
        mWindow = SDL_CreateWindow(title, width, height, static_cast<SDL_WindowFlags>(sdlFlags));
    }

    PlatformWindow::~PlatformWindow()
    {
        if (mWindow != nullptr)
        {
            SDL_DestroyWindow(ToSDL(mWindow));
            mWindow = nullptr;
        }
    }

    PlatformWindow::PlatformWindow(PlatformWindow &&other) noexcept
        : mWindow(other.mWindow)
    {
        other.mWindow = nullptr;
    }

    PlatformWindow &PlatformWindow::operator=(PlatformWindow &&other) noexcept
    {
        if (this != &other)
        {
            if (mWindow != nullptr)
            {
                SDL_DestroyWindow(ToSDL(mWindow));
            }
            mWindow = other.mWindow;
            other.mWindow = nullptr;
        }
        return *this;
    }

    bool PlatformWindow::IsValid() const
    {
        return mWindow != nullptr;
    }

    bool PlatformWindow::GetSizeInPixels(int &width, int &height) const
    {
        if (mWindow == nullptr)
        {
            return false;
        }
        return SDL_GetWindowSizeInPixels(ToSDL(mWindow), &width, &height);
    }

    std::uint64_t PlatformWindow::GetFlags() const
    {
        if (mWindow == nullptr)
        {
            return 0;
        }
        return TranslateSDLFlagsToPlatform(SDL_GetWindowFlags(ToSDL(mWindow)));
    }

    void *PlatformWindow::GetNativeWindowHandle() const
    {
        if (mWindow == nullptr)
        {
            return nullptr;
        }
        SDL_PropertiesID properties = SDL_GetWindowProperties(ToSDL(mWindow));
        return SDL_GetPointerProperty(properties, SDL_PROP_WINDOW_WIN32_HWND_POINTER, nullptr);
    }

    bool PlatformWindow::SetFullscreen(bool fullscreen)
    {
        if (mWindow == nullptr)
        {
            return false;
        }
        return SDL_SetWindowFullscreen(ToSDL(mWindow), fullscreen);
    }

    bool PlatformWindow::SetBordered(bool bordered)
    {
        if (mWindow == nullptr)
        {
            return false;
        }
        return SDL_SetWindowBordered(ToSDL(mWindow), bordered);
    }

    bool PlatformWindow::SetSize(int width, int height)
    {
        if (mWindow == nullptr)
        {
            return false;
        }
        return SDL_SetWindowSize(ToSDL(mWindow), width, height);
    }

    bool PlatformWindow::SetPosition(int x, int y)
    {
        if (mWindow == nullptr)
        {
            return false;
        }
        return SDL_SetWindowPosition(ToSDL(mWindow), x, y);
    }

    bool PlatformWindow::SetMouseGrab(bool grabbed)
    {
        if (mWindow == nullptr)
        {
            return false;
        }
        return SDL_SetWindowMouseGrab(ToSDL(mWindow), grabbed);
    }

    void PlatformWindow::Sync()
    {
        if (mWindow != nullptr)
        {
            SDL_SyncWindow(ToSDL(mWindow));
        }
    }

    bool PlatformWindow::HasExclusiveFullscreenMode() const
    {
        if (mWindow == nullptr)
        {
            return false;
        }
        return SDL_GetWindowFullscreenMode(ToSDL(mWindow)) != nullptr;
    }

    bool PlatformWindow::SetExclusiveFullscreenMode(int width, int height)
    {
        if (mWindow == nullptr)
        {
            return false;
        }
        SDL_Window *sdlWindow = ToSDL(mWindow);
        const SDL_DisplayID display = SDL_GetDisplayForWindow(sdlWindow);
        if (display == 0)
        {
            return false;
        }
        SDL_DisplayMode closestMode{};
        if (!SDL_GetClosestFullscreenDisplayMode(display, width, height, 0.0f, true, &closestMode))
        {
            return false;
        }
        return SDL_SetWindowFullscreenMode(sdlWindow, &closestMode);
    }

    bool PlatformWindow::ClearFullscreenMode()
    {
        if (mWindow == nullptr)
        {
            return false;
        }
        return SDL_SetWindowFullscreenMode(ToSDL(mWindow), nullptr);
    }

    bool PlatformWindow::GetDesktopDisplayMode(int &width, int &height) const
    {
        if (mWindow == nullptr)
        {
            return false;
        }
        const SDL_DisplayID display = SDL_GetDisplayForWindow(ToSDL(mWindow));
        if (display == 0)
        {
            return false;
        }
        const SDL_DisplayMode *mode = SDL_GetDesktopDisplayMode(display);
        if (mode == nullptr || mode->w <= 0 || mode->h <= 0)
        {
            return false;
        }
        width = mode->w;
        height = mode->h;
        return true;
    }

    bool PlatformWindow::GetDisplayBounds(int &x, int &y, int &w, int &h) const
    {
        if (mWindow == nullptr)
        {
            return false;
        }
        const SDL_DisplayID display = SDL_GetDisplayForWindow(ToSDL(mWindow));
        if (display == 0)
        {
            return false;
        }
        SDL_Rect bounds{};
        if (!SDL_GetDisplayBounds(display, &bounds))
        {
            return false;
        }
        x = bounds.x;
        y = bounds.y;
        w = bounds.w;
        h = bounds.h;
        return true;
    }

    bool PlatformWindow::StartTextInput()
    {
        if (mWindow == nullptr)
        {
            return false;
        }
        return SDL_StartTextInput(ToSDL(mWindow));
    }

    bool PlatformWindow::StopTextInput()
    {
        if (mWindow == nullptr)
        {
            return false;
        }
        return SDL_StopTextInput(ToSDL(mWindow));
    }
}
