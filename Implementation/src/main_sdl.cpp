#include <NsApp/ThemeProviders.h>
#include <NsGui/Grid.h>
#include <NsGui/IntegrationAPI.h>
#include <NsGui/IRenderer.h>
#include <NsGui/IView.h>
#include <NsGui/InputEnums.h>
#include <NsGui/Uri.h>
#include <NsCore/Package.h>
#include <NsRender/RenderContext.h>
#include <SDL3/SDL.h>
#include <SDL3/SDL_properties.h>

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

    Noesis::MouseButton ToNoesisMouseButton(Uint8 button)
    {
        switch (button)
        {
        case SDL_BUTTON_LEFT:
            return Noesis::MouseButton_Left;
        case SDL_BUTTON_RIGHT:
            return Noesis::MouseButton_Right;
        case SDL_BUTTON_MIDDLE:
            return Noesis::MouseButton_Middle;
        case SDL_BUTTON_X1:
            return Noesis::MouseButton_XButton1;
        case SDL_BUTTON_X2:
            return Noesis::MouseButton_XButton2;
        default:
            return Noesis::MouseButton_Left;
        }
    }

    void UpdateViewSize(SDL_Window *window)
    {
        int width = 0;
        int height = 0;
        if (SDL_GetWindowSizeInPixels(window, &width, &height) && gView != nullptr)
        {
            gView->SetSize(static_cast<uint32_t>(width), static_cast<uint32_t>(height));
        }
    }

    bool NoesisInit(SDL_Window *window)
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

        SDL_PropertiesID properties = SDL_GetWindowProperties(window);
        void *nativeWindow = SDL_GetPointerProperty(properties, SDL_PROP_WINDOW_WIN32_HWND_POINTER, nullptr);
        if (nativeWindow == nullptr)
        {
            SDL_Log("Failed to get native Win32 window handle from SDL: %s", SDL_GetError());
            return false;
        }

        uint32_t samples = 1;
        gRenderContext = NoesisApp::RenderContext::Create("VK");
        gRenderContext->Init(nativeWindow, samples, true, false);

        gView->GetRenderer()->Init(gRenderContext->GetDevice());
        UpdateViewSize(window);
        return true;
    }

    void RenderFrame(SDL_Window *window, double timeSeconds)
    {
        int width = 0;
        int height = 0;
        if (!SDL_GetWindowSizeInPixels(window, &width, &height))
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

    if (!SDL_Init(SDL_INIT_VIDEO))
    {
        SDL_Log("SDL_Init failed: %s", SDL_GetError());
        return 1;
    }

    SDL_Window *window = SDL_CreateWindow("NoesisDiligent", 1280, 720, SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE);
    if (window == nullptr)
    {
        SDL_Log("SDL_CreateWindow failed: %s", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    if (!NoesisInit(window))
    {
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    bool running = true;
    while (running)
    {
        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            switch (event.type)
            {
            case SDL_EVENT_QUIT:
                running = false;
                break;
            case SDL_EVENT_WINDOW_RESIZED:
            case SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED:
                UpdateViewSize(window);
                gRenderContext->Resize();
                break;
            case SDL_EVENT_MOUSE_MOTION:
                gView->MouseMove(static_cast<int>(event.motion.x), static_cast<int>(event.motion.y));
                break;
            case SDL_EVENT_MOUSE_BUTTON_DOWN:
                if (event.button.clicks > 1)
                {
                    gView->MouseDoubleClick(
                        static_cast<int>(event.button.x),
                        static_cast<int>(event.button.y),
                        ToNoesisMouseButton(event.button.button));
                }
                else
                {
                    gView->MouseButtonDown(
                        static_cast<int>(event.button.x),
                        static_cast<int>(event.button.y),
                        ToNoesisMouseButton(event.button.button));
                }
                break;
            case SDL_EVENT_MOUSE_BUTTON_UP:
                gView->MouseButtonUp(
                    static_cast<int>(event.button.x),
                    static_cast<int>(event.button.y),
                    ToNoesisMouseButton(event.button.button));
                break;
            case SDL_EVENT_MOUSE_WHEEL:
                gView->MouseWheel(
                    static_cast<int>(event.wheel.mouse_x),
                    static_cast<int>(event.wheel.mouse_y),
                    event.wheel.integer_y);
                break;
            default:
                break;
            }
        }

        RenderFrame(window, static_cast<double>(SDL_GetTicks()) / 1000.0);
    }

    if (gRenderContext != nullptr)
    {
        gRenderContext->Shutdown();
        gRenderContext.Reset();
    }

    gView.Reset();
    Noesis::GUI::Shutdown();
    ShutdownNoesisPackages();

    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}