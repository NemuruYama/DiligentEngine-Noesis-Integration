#include <cstdio>
#include <cstring>

#include "D3D12App.hpp"
#include "VulkanApp.hpp"

namespace
{
    using NoesisDiligent::AppStartupOptions;
    using NoesisDiligent::WindowMode;

    enum class RenderBackend
    {
        Vulkan,
        D3D12
    };

    struct LaunchOptions
    {
        RenderBackend backend = RenderBackend::Vulkan;
        AppStartupOptions startupOptions;
    };

    bool ParseBackendValue(const char *value, RenderBackend &backend)
    {
        if (std::strcmp(value, "vulkan") == 0)
        {
            backend = RenderBackend::Vulkan;
            return true;
        }

        if (std::strcmp(value, "dx12") == 0)
        {
            backend = RenderBackend::D3D12;
            return true;
        }

        return false;
    }

    bool TryParseWindowMode(const char* argument, AppStartupOptions& startupOptions)
    {
        if (std::strcmp(argument, "--windowed") == 0)
        {
            startupOptions.windowMode = WindowMode::Windowed;
            return true;
        }

        if (std::strcmp(argument, "--borderless") == 0)
        {
            startupOptions.windowMode = WindowMode::Borderless;
            return true;
        }

        if (std::strcmp(argument, "--fullscreen") == 0)
        {
            startupOptions.windowMode = WindowMode::Fullscreen;
            return true;
        }

        return false;
    }

    bool TryParseLaunchOptions(int argc, char **argv, LaunchOptions &options)
    {
        options = {};

        for (int argumentIndex = 1; argumentIndex < argc; ++argumentIndex)
        {
            const char *argument = argv[argumentIndex];

            if (TryParseWindowMode(argument, options.startupOptions))
            {
                continue;
            }

            if (std::strcmp(argument, "--vulkan") == 0)
            {
                options.backend = RenderBackend::Vulkan;
                continue;
            }

            if (std::strcmp(argument, "--dx12") == 0)
            {
                options.backend = RenderBackend::D3D12;
                continue;
            }

            if (std::strcmp(argument, "--renderer") == 0)
            {
                if (argumentIndex + 1 >= argc)
                {
                    std::fprintf(stderr, "Missing value for --renderer\n");
                    return false;
                }

                ++argumentIndex;
                if (!ParseBackendValue(argv[argumentIndex], options.backend))
                {
                    std::fprintf(stderr, "Unknown renderer '%s'\n", argv[argumentIndex]);
                    return false;
                }

                continue;
            }

            constexpr char RendererPrefix[] = "--renderer=";
            constexpr std::size_t RendererPrefixLength = sizeof(RendererPrefix) - 1;
            if (std::strncmp(argument, RendererPrefix, RendererPrefixLength) == 0)
            {
                if (!ParseBackendValue(argument + RendererPrefixLength, options.backend))
                {
                    std::fprintf(stderr, "Unknown renderer '%s'\n", argument + RendererPrefixLength);
                    return false;
                }

                continue;
            }

            std::fprintf(stderr, "Unknown argument '%s'\n", argument);
            return false;
        }

        return true;
    }
}

int main(int argc, char **argv)
{
    LaunchOptions options;
    if (!TryParseLaunchOptions(argc, argv, options))
    {
        return 1;
    }

    if (options.backend == RenderBackend::D3D12)
    {
        return NoesisDiligent::RunD3D12App(options.startupOptions);
    }

    return NoesisDiligent::RunVulkanApp(options.startupOptions);
}