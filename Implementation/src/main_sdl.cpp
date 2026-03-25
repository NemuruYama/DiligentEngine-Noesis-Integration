#include <NsApp/ThemeProviders.h>
#include <NsGui/Grid.h>
#include <NsGui/IntegrationAPI.h>
#include <NsGui/IRenderer.h>
#include <NsGui/IView.h>
#include <NsGui/Uri.h>
#include <NsCore/Package.h>
#include <NsRender/RenderContext.h>

#include "PlatformNoesisInput.hpp"
#include "SDLPlatform.hpp"

#include <cstdio>

using namespace NoesisDiligent;

extern "C" void NsRegisterReflectionAppProviders();
extern "C" void NsInitPackageAppProviders();
extern "C" void NsShutdownPackageAppProviders();
extern "C" void NsRegisterReflectionAppTheme();
extern "C" void NsInitPackageAppTheme();
extern "C" void NsShutdownPackageAppTheme();
extern "C" void NsRegisterReflectionRenderRenderContext();
extern "C" void NsInitPackageRenderRenderContext();
extern "C" void NsShutdownPackageRenderRenderContext();
extern "C" void NsRegisterReflectionRenderVKRenderDevice();
extern "C" void NsInitPackageRenderVKRenderDevice();
extern "C" void NsShutdownPackageRenderVKRenderDevice();
extern "C" void NsRegisterReflectionRenderVKRenderContext();
extern "C" void NsInitPackageRenderVKRenderContext();
extern "C" void NsShutdownPackageRenderVKRenderContext();

namespace
{
    Noesis::Ptr<Noesis::IView> gView;
    Noesis::Ptr<NoesisApp::RenderContext> gRenderContext;

    void InitNoesisPackages()
    {
        NsRegisterReflectionAppProviders();
        NsInitPackageAppProviders();
        NsRegisterReflectionAppTheme();
        NsInitPackageAppTheme();
        NsRegisterReflectionRenderRenderContext();
        NsInitPackageRenderRenderContext();
        NsRegisterReflectionRenderVKRenderDevice();
        NsInitPackageRenderVKRenderDevice();
        NsRegisterReflectionRenderVKRenderContext();
        NsInitPackageRenderVKRenderContext();
    }

    void ShutdownNoesisPackages()
    {
        NsShutdownPackageRenderVKRenderContext();
        NsShutdownPackageRenderVKRenderDevice();
        NsShutdownPackageRenderRenderContext();
        NsShutdownPackageAppTheme();
        NsShutdownPackageAppProviders();
    }

    void UpdateViewSize(PlatformWindow &window)
    {
        int width = 0;
        int height = 0;
        if (window.GetSizeInPixels(width, height) && gView != nullptr)
        {
            gView->SetSize(static_cast<uint32_t>(width), static_cast<uint32_t>(height));
        }
    }

    bool NoesisInit(PlatformWindow &window)
    {
        InitNoesisPackages();

        Noesis::GUI::SetLogHandler([](const char *, uint32_t, uint32_t level, const char *, const char *msg)
                                   {
        const char* prefixes[] = {"T", "D", "I", "W", "E"};
        printf("[NOESIS/%s] %s\n", prefixes[level], msg); });

        Noesis::GUI::SetLicense(NS_LICENSE_NAME, NS_LICENSE_KEY);
        Noesis::GUI::Init();

        NoesisApp::SetThemeProviders();
        Noesis::GUI::LoadApplicationResources(NoesisApp::Theme::DarkBlue());

        Noesis::Ptr<Noesis::Grid> xaml(Noesis::GUI::ParseXaml<Noesis::Grid>(
            R"(
        <Grid xmlns="http://schemas.microsoft.com/winfx/2006/xaml/presentation">
            <Grid.Background>
                <LinearGradientBrush StartPoint="0,0" EndPoint="0,1">
                    <GradientStop Offset="0" Color="#FF123F61"/>
                    <GradientStop Offset="0.6" Color="#FF0E4B79"/>
                    <GradientStop Offset="0.7" Color="#FF106097"/>
                </LinearGradientBrush>
            </Grid.Background>
            <Viewbox>
                <StackPanel Margin="50">
                    <Button Content="Hello World!" Margin="0,30,0,0"/>
                    <ComboBox Width="220" Margin="0,16,0,0" SelectedIndex="0">
                        <ComboBoxItem Content="Option 1"/>
                        <ComboBoxItem Content="Option 2"/>
                        <ComboBoxItem Content="Option 3"/>
                    </ComboBox>
                </StackPanel>
            </Viewbox>
        </Grid>
    )"));

        gView = Noesis::GUI::CreateView(xaml);
        gView->SetFlags(Noesis::RenderFlags_PPAA | Noesis::RenderFlags_LCD);

        void *nativeWindow = window.GetNativeWindowHandle();
        if (nativeWindow == nullptr)
        {
            std::fprintf(stderr, "Failed to get native window handle: %s\n", PlatformGetError());
            return false;
        }

        uint32_t samples = 1;
        gRenderContext = NoesisApp::RenderContext::Create("VK");
        gRenderContext->Init(nativeWindow, samples, true, false);

        gView->GetRenderer()->Init(gRenderContext->GetDevice());
        UpdateViewSize(window);
        return true;
    }

    void RenderFrame(PlatformWindow &window, double timeSeconds)
    {
        int width = 0;
        int height = 0;
        if (!window.GetSizeInPixels(width, height))
        {
            return;
        }

        gView->Update(timeSeconds);

        gRenderContext->BeginRender();
        gView->GetRenderer()->UpdateRenderTree();
        gView->GetRenderer()->RenderOffscreen();
        gRenderContext->SetDefaultRenderTarget(static_cast<uint32_t>(width), static_cast<uint32_t>(height), true);
        gView->GetRenderer()->Render();
        gRenderContext->EndRender();
        gRenderContext->Swap();
    }
}

int main(int argc, char **argv)
{
    (void)argc;
    (void)argv;

    if (!PlatformInit())
    {
        std::fprintf(stderr, "PlatformInit failed: %s\n", PlatformGetError());
        return 1;
    }

    PlatformWindow window{"NoesisDiligent", 1280, 720, PlatformWindowFlag::Vulkan | PlatformWindowFlag::Resizable};
    if (!window.IsValid())
    {
        std::fprintf(stderr, "PlatformWindow creation failed: %s\n", PlatformGetError());
        PlatformQuit();
        return 1;
    }

    if (!NoesisInit(window))
    {
        PlatformQuit();
        return 1;
    }

    bool textInputStarted = false;
    if (!window.StartTextInput())
    {
        std::fprintf(stderr, "StartTextInput failed: %s\n", PlatformGetError());
    }
    else
    {
        textInputStarted = true;
    }

    bool running = true;
    while (running)
    {
        PlatformEvent event;
        while (PlatformPollEvent(event))
        {
            switch (event.type)
            {
            case PlatformEventType::Quit:
                running = false;
                break;
            case PlatformEventType::WindowResized:
            case PlatformEventType::WindowPixelSizeChanged:
                UpdateViewSize(window);
                gRenderContext->Resize();
                break;
            case PlatformEventType::MouseMotion:
                gView->MouseMove(static_cast<int>(event.motion.x), static_cast<int>(event.motion.y));
                break;
            case PlatformEventType::MouseButtonDown:
                if (event.button.clicks > 1)
                {
                    gView->MouseDoubleClick(
                        static_cast<int>(event.button.x),
                        static_cast<int>(event.button.y),
                        PlatformMouseButtonToNoesisButton(event.button.button));
                }
                else
                {
                    gView->MouseButtonDown(
                        static_cast<int>(event.button.x),
                        static_cast<int>(event.button.y),
                        PlatformMouseButtonToNoesisButton(event.button.button));
                }
                break;
            case PlatformEventType::MouseButtonUp:
                gView->MouseButtonUp(
                    static_cast<int>(event.button.x),
                    static_cast<int>(event.button.y),
                    PlatformMouseButtonToNoesisButton(event.button.button));
                break;
            case PlatformEventType::MouseWheel:
                gView->MouseWheel(
                    static_cast<int>(event.wheel.mouseX),
                    static_cast<int>(event.wheel.mouseY),
                    event.wheel.integerY);
                break;
            case PlatformEventType::KeyDown:
            {
                const Noesis::Key key = PlatformKeyCodeToNoesisKey(event.key.key);
                if (key != Noesis::Key_None)
                {
                    gView->KeyDown(key);
                }
                break;
            }
            case PlatformEventType::KeyUp:
            {
                const Noesis::Key key = PlatformKeyCodeToNoesisKey(event.key.key);
                if (key != Noesis::Key_None)
                {
                    gView->KeyUp(key);
                }
                break;
            }
            case PlatformEventType::TextInput:
                if (event.text[0] != '\0')
                {
                    SendUtf8TextToNoesis(*gView, event.text);
                }
                break;
            default:
                break;
            }
        }

        RenderFrame(window, static_cast<double>(PlatformGetTicks()) / 1000.0);
    }

    if (gRenderContext != nullptr)
    {
        gRenderContext->Shutdown();
        gRenderContext.Reset();
    }

    gView.Reset();
    Noesis::GUI::Shutdown();
    ShutdownNoesisPackages();

    if (textInputStarted)
    {
        window.StopTextInput();
    }

    PlatformQuit();
    return 0;
}