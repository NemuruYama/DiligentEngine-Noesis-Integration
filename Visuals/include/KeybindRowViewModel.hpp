#pragma once

#include <NsApp/NotifyPropertyChangedBase.h>
#include <NsCore/ReflectionDeclare.h>

#include <string>

namespace NoesisDiligent::Visuals
{
    class KeybindRowViewModel final : public NoesisApp::NotifyPropertyChangedBase
    {
    public:
        KeybindRowViewModel(
            std::string displayName = {},
            std::string primaryBindingText = {},
            std::string secondaryBindingText = {});

        const char* GetDisplayName() const;
        const char* GetPrimaryBindingText() const;
        const char* GetSecondaryBindingText() const;
        bool GetHasSecondaryBinding() const;

    private:
        std::string displayName_;
        std::string primaryBindingText_;
        std::string secondaryBindingText_;
        bool hasSecondaryBinding_ = false;

        NS_DECLARE_REFLECTION(KeybindRowViewModel, NotifyPropertyChangedBase)
    };
}