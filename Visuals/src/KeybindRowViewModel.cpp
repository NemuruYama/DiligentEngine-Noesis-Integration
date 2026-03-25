#include "KeybindRowViewModel.hpp"

#include <NsCore/ReflectionImplement.h>

#include <utility>

namespace NoesisDiligent::Visuals
{
    KeybindRowViewModel::KeybindRowViewModel(
        std::string displayName,
        std::string primaryBindingText,
        std::string secondaryBindingText) :
        displayName_(std::move(displayName)),
        primaryBindingText_(std::move(primaryBindingText)),
        secondaryBindingText_(std::move(secondaryBindingText)),
        hasSecondaryBinding_(!secondaryBindingText_.empty())
    {
    }

    const char* KeybindRowViewModel::GetDisplayName() const
    {
        return displayName_.c_str();
    }

    const char* KeybindRowViewModel::GetPrimaryBindingText() const
    {
        return primaryBindingText_.c_str();
    }

    const char* KeybindRowViewModel::GetSecondaryBindingText() const
    {
        return secondaryBindingText_.empty() ? "-" : secondaryBindingText_.c_str();
    }

    bool KeybindRowViewModel::GetHasSecondaryBinding() const
    {
        return hasSecondaryBinding_;
    }

    NS_IMPLEMENT_REFLECTION(KeybindRowViewModel, "NoesisDiligent.Visuals.KeybindRowViewModel")
    {
        NsProp("DisplayName", &KeybindRowViewModel::GetDisplayName);
        NsProp("PrimaryBindingText", &KeybindRowViewModel::GetPrimaryBindingText);
        NsProp("SecondaryBindingText", &KeybindRowViewModel::GetSecondaryBindingText);
        NsProp("HasSecondaryBinding", &KeybindRowViewModel::GetHasSecondaryBinding);
    }
}