#pragma once

#include <NsApp/DelegateCommand.h>
#include <NsApp/NotifyPropertyChangedBase.h>
#include <NsCore/Noesis.h>
#include <NsCore/Ptr.h>
#include <NsCore/ReflectionDeclare.h>
#include <NsGui/ObservableCollection.h>

#include <functional>

namespace NoesisDiligent::Visuals
{
    class KeybindRowViewModel;

    struct SettingsCallbacks
    {
        std::function<void()> onClose;
        std::function<void(bool)> onDevModeChanged;
        std::function<bool(int)> onResolutionChanged;
        std::function<bool(int)> onWindowModeChanged;
    };

    class SettingsViewModel final : public NoesisApp::NotifyPropertyChangedBase
    {
    public:
        explicit SettingsViewModel(
            SettingsCallbacks callbacks = {},
            bool isDevMode = true,
            int resolutionIndex = 0,
            int fullscreenModeIndex = 0);

        bool GetIsDevMode() const;
        void SetIsDevMode(bool value);

        float GetMasterVolume() const;
        void SetMasterVolume(float value);

        int GetResolutionIndex() const;
        void SetResolutionIndex(int value);

        int GetFullscreenModeIndex() const;
        void SetFullscreenModeIndex(int value);

        bool GetShowPerformanceStats() const;
        void SetShowPerformanceStats(bool value);

        NoesisApp::DelegateCommand* GetCloseCommand() const;
        Noesis::ObservableCollection<Noesis::BaseComponent>* GetKeybinds() const;

    private:
        void ExecuteClose(Noesis::BaseComponent* parameter);
        void BuildKeybindRows();

    private:
        SettingsCallbacks callbacks_;
        bool isDevMode_ = true;
        float masterVolume_ = 0.8f;
        int resolutionIndex_ = 0;
        int fullscreenModeIndex_ = 0;
        bool showPerformanceStats_ = true;
        Noesis::Ptr<Noesis::ObservableCollection<Noesis::BaseComponent>> keybinds_;
        Noesis::Ptr<NoesisApp::DelegateCommand> closeCommand_;

        NS_DECLARE_REFLECTION(SettingsViewModel, NotifyPropertyChangedBase)
    };
}