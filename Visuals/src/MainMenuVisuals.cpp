#include "MainMenuVisuals.hpp"

#include <NsGui/FrameworkElement.h>
#include <NsGui/Uri.h>
#include <NsGui/IntegrationAPI.h>

#include <utility>

namespace NoesisDiligent::Visuals
{
    Noesis::Ptr<Noesis::FrameworkElement> CreateMainMenuRoot(MainMenuCallbacks callbacks)
    {
        const Noesis::Uri mainMenuUri{"UI/MainMenu.xaml"};
        Noesis::Ptr<Noesis::FrameworkElement> root =
            Noesis::GUI::LoadXaml<Noesis::FrameworkElement>(mainMenuUri);

        if (root != nullptr)
        {
            root->SetDataContext(Noesis::MakePtr<MainMenuViewModel>(std::move(callbacks)));
        }

        return root;
    }
}
