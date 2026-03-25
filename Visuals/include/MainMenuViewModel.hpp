#pragma once

#include "SettingsViewModel.hpp"

#include <NsApp/DelegateCommand.h>
#include <NsApp/NotifyPropertyChangedBase.h>
#include <NsCore/Noesis.h>
#include <NsCore/Ptr.h>
#include <NsCore/ReflectionDeclare.h>
#include <NsCore/String.h>

#include <functional>

namespace NoesisDiligent::Visuals
{
    struct MainMenuCallbacks
    {
        std::function<void()> onStartGame;
        std::function<void()> onQuickstart;
        std::function<void()> onLoadGame;
        std::function<void()> onJoinGame;
        std::function<void()> onHostGame;
        std::function<void()> onMods;
        std::function<void()> onSettings;
        std::function<void()> onQuit;
        std::function<bool(int)> onResolutionChanged;
        std::function<bool(int)> onWindowModeChanged;
        int initialResolutionIndex = 0;
        int initialWindowModeIndex = 0;
    };

    class MainMenuViewModel final : public NoesisApp::NotifyPropertyChangedBase
    {
    public:
        explicit MainMenuViewModel(MainMenuCallbacks callbacks = {});

        const char* GetTitle() const;
        const char* GetSubtitle() const;
        const char* GetStatusText() const;
        const char* GetVersionText() const;
        bool GetIsDevMode() const;
        bool GetIsSettingsOpen() const;
        SettingsViewModel* GetSettings() const;

        void SetStatusText(const char* text);
        void SetIsDevMode(bool enabled);

        NoesisApp::DelegateCommand* GetStartGameCommand() const;
        NoesisApp::DelegateCommand* GetQuickstartCommand() const;
        NoesisApp::DelegateCommand* GetLoadGameCommand() const;
        NoesisApp::DelegateCommand* GetJoinGameCommand() const;
        NoesisApp::DelegateCommand* GetHostGameCommand() const;
        NoesisApp::DelegateCommand* GetModsCommand() const;
        NoesisApp::DelegateCommand* GetSettingsCommand() const;
        NoesisApp::DelegateCommand* GetQuitCommand() const;

    private:
        void ExecuteStartGame(Noesis::BaseComponent* parameter);
        void ExecuteQuickstart(Noesis::BaseComponent* parameter);
        void ExecuteLoadGame(Noesis::BaseComponent* parameter);
        void ExecuteJoinGame(Noesis::BaseComponent* parameter);
        void ExecuteHostGame(Noesis::BaseComponent* parameter);
        void ExecuteMods(Noesis::BaseComponent* parameter);
        void ExecuteSettings(Noesis::BaseComponent* parameter);
        void ExecuteQuit(Noesis::BaseComponent* parameter);

        void ExecuteMenuAction(
            Noesis::BaseComponent* parameter,
            const char* statusText,
            const std::function<void()>& callback);
        void SetIsSettingsOpen(bool open);
        void HandleSettingsClosed();
        void HandleSettingsDevModeChanged(bool enabled);

    private:
        MainMenuCallbacks callbacks_;
        Noesis::String title_;
        Noesis::String subtitle_;
        Noesis::String statusText_;
        Noesis::String versionText_;
        bool isDevMode_ = true;
        bool isSettingsOpen_ = false;
        Noesis::Ptr<SettingsViewModel> settings_;

        Noesis::Ptr<NoesisApp::DelegateCommand> startGameCommand_;
        Noesis::Ptr<NoesisApp::DelegateCommand> quickstartCommand_;
        Noesis::Ptr<NoesisApp::DelegateCommand> loadGameCommand_;
        Noesis::Ptr<NoesisApp::DelegateCommand> joinGameCommand_;
        Noesis::Ptr<NoesisApp::DelegateCommand> hostGameCommand_;
        Noesis::Ptr<NoesisApp::DelegateCommand> modsCommand_;
        Noesis::Ptr<NoesisApp::DelegateCommand> settingsCommand_;
        Noesis::Ptr<NoesisApp::DelegateCommand> quitCommand_;

        NS_DECLARE_REFLECTION(MainMenuViewModel, NotifyPropertyChangedBase)
    };
}
