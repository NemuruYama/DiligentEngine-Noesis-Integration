#include "SettingsViewModel.hpp"

#include "KeybindRowViewModel.hpp"

#include <NsCore/ReflectionImplement.h>

#include <algorithm>
#include <utility>

namespace NoesisDiligent::Visuals
{
    namespace
    {
        struct KeybindDefinition
        {
            const char* displayName;
            const char* primaryBinding;
            const char* secondaryBinding;
        };

        constexpr KeybindDefinition kDefaultKeybinds[] = {
            {"Move Forward", "W", "Up Arrow"},
            {"Move Backward", "S", "Down Arrow"},
            {"Move Left", "A", "Left Arrow"},
            {"Move Right", "D", "Right Arrow"},
            {"Rotate Camera", "Q", "E"},
            {"Pan Camera", "Middle Mouse", "Shift + Middle Mouse"},
            {"Zoom", "Mouse Wheel", "Page Up / Page Down"},
            {"Pause Simulation", "Space", "P"},
            {"Toggle Build Mode", "B", ""},
            {"Open Colony Overview", "Tab", "F1"}
        };
    }

    SettingsViewModel::SettingsViewModel(
        SettingsCallbacks callbacks,
        bool isDevMode,
        int resolutionIndex,
        int fullscreenModeIndex) :
        callbacks_(std::move(callbacks)),
        isDevMode_(isDevMode),
        resolutionIndex_(resolutionIndex),
        fullscreenModeIndex_(fullscreenModeIndex),
        keybinds_(Noesis::MakePtr<Noesis::ObservableCollection<Noesis::BaseComponent>>())
    {
        closeCommand_ = Noesis::MakePtr<NoesisApp::DelegateCommand>(Noesis::MakeDelegate(this, &SettingsViewModel::ExecuteClose));
        BuildKeybindRows();
    }

    bool SettingsViewModel::GetIsDevMode() const
    {
        return isDevMode_;
    }

    void SettingsViewModel::SetIsDevMode(bool value)
    {
        if (isDevMode_ != value)
        {
            isDevMode_ = value;
            OnPropertyChanged("IsDevMode");
            if (callbacks_.onDevModeChanged)
            {
                callbacks_.onDevModeChanged(value);
            }
        }
    }

    float SettingsViewModel::GetMasterVolume() const
    {
        return masterVolume_;
    }

    void SettingsViewModel::SetMasterVolume(float value)
    {
        value = std::clamp(value, 0.0f, 1.0f);
        if (masterVolume_ != value)
        {
            masterVolume_ = value;
            OnPropertyChanged("MasterVolume");
        }
    }

    int SettingsViewModel::GetResolutionIndex() const
    {
        return resolutionIndex_;
    }

    void SettingsViewModel::SetResolutionIndex(int value)
    {
        if (resolutionIndex_ != value)
        {
            if (callbacks_.onResolutionChanged && !callbacks_.onResolutionChanged(value))
            {
                return;
            }

            resolutionIndex_ = value;
            OnPropertyChanged("ResolutionIndex");
        }
    }

    int SettingsViewModel::GetFullscreenModeIndex() const
    {
        return fullscreenModeIndex_;
    }

    void SettingsViewModel::SetFullscreenModeIndex(int value)
    {
        if (fullscreenModeIndex_ != value)
        {
            if (callbacks_.onWindowModeChanged && !callbacks_.onWindowModeChanged(value))
            {
                return;
            }

            fullscreenModeIndex_ = value;
            OnPropertyChanged("FullscreenModeIndex");
        }
    }

    bool SettingsViewModel::GetShowPerformanceStats() const
    {
        return showPerformanceStats_;
    }

    void SettingsViewModel::SetShowPerformanceStats(bool value)
    {
        if (showPerformanceStats_ != value)
        {
            showPerformanceStats_ = value;
            OnPropertyChanged("ShowPerformanceStats");
        }
    }

    NoesisApp::DelegateCommand* SettingsViewModel::GetCloseCommand() const
    {
        return closeCommand_;
    }

    Noesis::ObservableCollection<Noesis::BaseComponent>* SettingsViewModel::GetKeybinds() const
    {
        return keybinds_;
    }

    void SettingsViewModel::ExecuteClose(Noesis::BaseComponent* parameter)
    {
        (void)parameter;
        if (callbacks_.onClose)
        {
            callbacks_.onClose();
        }
    }

    void SettingsViewModel::BuildKeybindRows()
    {
        keybinds_->Clear();
        for (const KeybindDefinition& keybind : kDefaultKeybinds)
        {
            keybinds_->Add(Noesis::MakePtr<KeybindRowViewModel>(
                keybind.displayName,
                keybind.primaryBinding,
                keybind.secondaryBinding));
        }
    }

    NS_IMPLEMENT_REFLECTION(SettingsViewModel, "NoesisDiligent.Visuals.SettingsViewModel")
    {
        NsProp("IsDevMode", &SettingsViewModel::GetIsDevMode, &SettingsViewModel::SetIsDevMode);
        NsProp("MasterVolume", &SettingsViewModel::GetMasterVolume, &SettingsViewModel::SetMasterVolume);
        NsProp("ResolutionIndex", &SettingsViewModel::GetResolutionIndex, &SettingsViewModel::SetResolutionIndex);
        NsProp("FullscreenModeIndex", &SettingsViewModel::GetFullscreenModeIndex, &SettingsViewModel::SetFullscreenModeIndex);
        NsProp("ShowPerformanceStats", &SettingsViewModel::GetShowPerformanceStats, &SettingsViewModel::SetShowPerformanceStats);
        NsProp("CloseCommand", &SettingsViewModel::GetCloseCommand);
        NsProp("Keybinds", &SettingsViewModel::GetKeybinds);
    }
}