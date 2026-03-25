#pragma once

#include "MainMenuViewModel.hpp"

#include <NsCore/Ptr.h>

namespace Noesis
{
    class FrameworkElement;
}

namespace NoesisDiligent::Visuals
{
    Noesis::Ptr<Noesis::FrameworkElement> CreateMainMenuRoot(MainMenuCallbacks callbacks = {});
}
