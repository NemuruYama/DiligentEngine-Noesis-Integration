#pragma once

#include <NsGui/InputEnums.h>

#include "SDLPlatform.hpp"

namespace Noesis
{
    class IView;
}

namespace NoesisDiligent
{
    Noesis::Key PlatformKeyCodeToNoesisKey(PlatformKeyCode key);
    Noesis::MouseButton PlatformMouseButtonToNoesisButton(PlatformMouseButton button);
    void SendUtf8TextToNoesis(Noesis::IView& view, const char* text);
}