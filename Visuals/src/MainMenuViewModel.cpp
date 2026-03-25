#include "MainMenuViewModel.hpp"

#include <NsCore/ReflectionImplement.h>

#include <utility>

namespace NoesisDiligent::Visuals
{
    namespace
    {
        Noesis::Ptr<NoesisApp::DelegateCommand> CreateCommand(
            MainMenuViewModel* owner,
            void (MainMenuViewModel::*method)(Noesis::BaseComponent*))
        {
            return Noesis::MakePtr<NoesisApp::DelegateCommand>(Noesis::MakeDelegate(owner, method));
        }
    }

    MainMenuViewModel::MainMenuViewModel(MainMenuCallbacks callbacks) :
        callbacks_(std::move(callbacks)),
        title_("Diligent Engine"),
        subtitle_("Noesis"),
        statusText_("Ready"),
        versionText_("Main Menu Prototype")
    {
        SettingsCallbacks settingsCallbacks;
        settingsCallbacks.onClose = [this]() { HandleSettingsClosed(); };
        settingsCallbacks.onDevModeChanged = [this](bool enabled) { HandleSettingsDevModeChanged(enabled); };
        settingsCallbacks.onResolutionChanged = callbacks_.onResolutionChanged;
        settingsCallbacks.onWindowModeChanged = callbacks_.onWindowModeChanged;
        settings_ = Noesis::MakePtr<SettingsViewModel>(
            std::move(settingsCallbacks),
            isDevMode_,
            callbacks_.initialResolutionIndex,
            callbacks_.initialWindowModeIndex);

        startGameCommand_ = CreateCommand(this, &MainMenuViewModel::ExecuteStartGame);
        quickstartCommand_ = CreateCommand(this, &MainMenuViewModel::ExecuteQuickstart);
        loadGameCommand_ = CreateCommand(this, &MainMenuViewModel::ExecuteLoadGame);
        joinGameCommand_ = CreateCommand(this, &MainMenuViewModel::ExecuteJoinGame);
        hostGameCommand_ = CreateCommand(this, &MainMenuViewModel::ExecuteHostGame);
        modsCommand_ = CreateCommand(this, &MainMenuViewModel::ExecuteMods);
        settingsCommand_ = CreateCommand(this, &MainMenuViewModel::ExecuteSettings);
        quitCommand_ = CreateCommand(this, &MainMenuViewModel::ExecuteQuit);
    }

    const char* MainMenuViewModel::GetTitle() const
    {
        return title_.Str();
    }

    const char* MainMenuViewModel::GetSubtitle() const
    {
        return subtitle_.Str();
    }

    const char* MainMenuViewModel::GetStatusText() const
    {
        return statusText_.Str();
    }

    const char* MainMenuViewModel::GetVersionText() const
    {
        return versionText_.Str();
    }

    bool MainMenuViewModel::GetIsDevMode() const
    {
        return isDevMode_;
    }

    bool MainMenuViewModel::GetIsSettingsOpen() const
    {
        return isSettingsOpen_;
    }

    SettingsViewModel* MainMenuViewModel::GetSettings() const
    {
        return settings_;
    }

    void MainMenuViewModel::SetStatusText(const char* text)
    {
        const char* nextValue = text != nullptr ? text : "";
        if (statusText_ != nextValue)
        {
            statusText_ = nextValue;
            OnPropertyChanged("StatusText");
        }
    }

    void MainMenuViewModel::SetIsDevMode(bool enabled)
    {
        if (isDevMode_ != enabled)
        {
            isDevMode_ = enabled;
            OnPropertyChanged("IsDevMode");
        }
    }

    NoesisApp::DelegateCommand* MainMenuViewModel::GetStartGameCommand() const
    {
        return startGameCommand_;
    }

    NoesisApp::DelegateCommand* MainMenuViewModel::GetQuickstartCommand() const
    {
        return quickstartCommand_;
    }

    NoesisApp::DelegateCommand* MainMenuViewModel::GetLoadGameCommand() const
    {
        return loadGameCommand_;
    }

    NoesisApp::DelegateCommand* MainMenuViewModel::GetJoinGameCommand() const
    {
        return joinGameCommand_;
    }

    NoesisApp::DelegateCommand* MainMenuViewModel::GetHostGameCommand() const
    {
        return hostGameCommand_;
    }

    NoesisApp::DelegateCommand* MainMenuViewModel::GetModsCommand() const
    {
        return modsCommand_;
    }

    NoesisApp::DelegateCommand* MainMenuViewModel::GetSettingsCommand() const
    {
        return settingsCommand_;
    }

    NoesisApp::DelegateCommand* MainMenuViewModel::GetQuitCommand() const
    {
        return quitCommand_;
    }

    void MainMenuViewModel::ExecuteStartGame(Noesis::BaseComponent* parameter)
    {
        ExecuteMenuAction(parameter, "Preparing a local game session", callbacks_.onStartGame);
    }

    void MainMenuViewModel::ExecuteQuickstart(Noesis::BaseComponent* parameter)
    {
        ExecuteMenuAction(parameter, "Running developer quickstart", callbacks_.onQuickstart);
    }

    void MainMenuViewModel::ExecuteLoadGame(Noesis::BaseComponent* parameter)
    {
        ExecuteMenuAction(parameter, "Load game is not implemented yet", callbacks_.onLoadGame);
    }

    void MainMenuViewModel::ExecuteJoinGame(Noesis::BaseComponent* parameter)
    {
        ExecuteMenuAction(parameter, "Join game is not implemented yet", callbacks_.onJoinGame);
    }

    void MainMenuViewModel::ExecuteHostGame(Noesis::BaseComponent* parameter)
    {
        ExecuteMenuAction(parameter, "Host game is not implemented yet", callbacks_.onHostGame);
    }

    void MainMenuViewModel::ExecuteMods(Noesis::BaseComponent* parameter)
    {
        ExecuteMenuAction(parameter, "Mods browser is not implemented yet", callbacks_.onMods);
    }

    void MainMenuViewModel::ExecuteSettings(Noesis::BaseComponent* parameter)
    {
        (void)parameter;
        SetIsSettingsOpen(true);
        SetStatusText("Settings panel opened");
        if (callbacks_.onSettings)
        {
            callbacks_.onSettings();
        }
    }

    void MainMenuViewModel::ExecuteQuit(Noesis::BaseComponent* parameter)
    {
        ExecuteMenuAction(parameter, "Exiting application", callbacks_.onQuit);
    }

    void MainMenuViewModel::ExecuteMenuAction(
        Noesis::BaseComponent* parameter,
        const char* statusText,
        const std::function<void()>& callback)
    {
        (void)parameter;
        SetStatusText(statusText);
        if (callback)
        {
            callback();
        }
    }

    void MainMenuViewModel::SetIsSettingsOpen(bool open)
    {
        if (isSettingsOpen_ != open)
        {
            isSettingsOpen_ = open;
            OnPropertyChanged("IsSettingsOpen");
        }
    }

    void MainMenuViewModel::HandleSettingsClosed()
    {
        SetIsSettingsOpen(false);
        SetStatusText("Returned to main menu");
    }

    void MainMenuViewModel::HandleSettingsDevModeChanged(bool enabled)
    {
        SetIsDevMode(enabled);
    }

    NS_IMPLEMENT_REFLECTION(MainMenuViewModel, "NoesisDiligent.Visuals.MainMenuViewModel")
    {
        NsProp("Title", &MainMenuViewModel::GetTitle);
        NsProp("Subtitle", &MainMenuViewModel::GetSubtitle);
        NsProp("StatusText", &MainMenuViewModel::GetStatusText);
        NsProp("VersionText", &MainMenuViewModel::GetVersionText);
        NsProp("IsDevMode", &MainMenuViewModel::GetIsDevMode);
        NsProp("IsSettingsOpen", &MainMenuViewModel::GetIsSettingsOpen);
        NsProp("Settings", &MainMenuViewModel::GetSettings);

        NsProp("StartGameCommand", &MainMenuViewModel::GetStartGameCommand);
        NsProp("QuickstartCommand", &MainMenuViewModel::GetQuickstartCommand);
        NsProp("LoadGameCommand", &MainMenuViewModel::GetLoadGameCommand);
        NsProp("JoinGameCommand", &MainMenuViewModel::GetJoinGameCommand);
        NsProp("HostGameCommand", &MainMenuViewModel::GetHostGameCommand);
        NsProp("ModsCommand", &MainMenuViewModel::GetModsCommand);
        NsProp("SettingsCommand", &MainMenuViewModel::GetSettingsCommand);
        NsProp("QuitCommand", &MainMenuViewModel::GetQuitCommand);
    }
}
