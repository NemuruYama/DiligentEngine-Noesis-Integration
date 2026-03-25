# Diligent Engine - Noesis GUI Integration
This project demonstrates how to integrate Noesis GUI with Diligent Engine.
It now provides a sample application that can render the same Noesis UI using either Vulkan or DirectX 12.

## How to use
1. Clone this repository and open the workspace in Visual Studio Code.
2. Update the submodules to get the Diligent Engine and SDL3. (`git submodule update --init --recursive`)
3. Download Noesis GUI and place it in the `ThirdParty/Noesis` directory. The project also checks `ThirdParty/NoesisSDK`, and it also supports the versioned SDK folder layout directly if you prefer to keep the original folder name. You can get a free trial from the Noesis [website](https://www.noesisengine.com/developers/downloads.php).
4. Build the project using CMake Tools in Visual Studio Code. Make sure to configure the project first if it is not already configured.
5. Run the application to see the Noesis GUI rendered using Diligent Engine.

## Build and run
The main target is `ImplementationApp`.

If you are using VS Code, configure the project with CMake Tools first, then build `ImplementationApp`, and run that target.

There is also `ImplementationSDLApp` if you want to compare against the simpler SDL-focused sample path.

## Command line options
The app currently supports a few startup arguments.

- `--vulkan`
- `--dx12`
- `--renderer=vulkan`
- `--renderer=dx12`
- `--windowed`
- `--borderless`
- `--fullscreen`

Examples:

- `ImplementationApp.exe --vulkan --borderless`
- `ImplementationApp.exe --vulkan --windowed`
- `ImplementationApp.exe --vulkan --fullscreen`
- `ImplementationApp.exe --dx12 --borderless --fullscreen`
- `ImplementationApp.exe --dx12 --borderless --windowed`

## Window modes
The app now starts in borderless mode by default.

`Windowed` is a normal bordered window.

`Borderless` is a little special. If the selected resolution matches the display that the window is currently on, it will behave like a borderless fullscreen window. If the selected resolution does not match the display resolution, it will become a borderless window and center itself on that display.

At startup, `--borderless` acts like a modifier:

- `--borderless --fullscreen` starts as borderless fullscreen
- `--borderless --windowed` starts as borderless windowed
- if you do not pass `--windowed` or `--fullscreen`, the app uses the default startup mode

`Fullscreen` is exclusive fullscreen.

There is also a small compile-time define in the host code that controls whether exclusive fullscreen captures the mouse.

## Current state
At this point both rendering backends are up and running.

The project currently has:

- a shared app host for SDL, Noesis, and backend-independent app flow
- a shared `DiligentNoesisBackendBase` for the duplicated Diligent backend lifecycle
- a Vulkan backend
- a DirectX 12 backend
- a main menu UI
- a settings panel
- working resolution switching
- working window mode switching
- working startup window mode parsing for mixed borderless/fullscreen and borderless/windowed arguments
- a visual-only keybinds page

## Backends Status
- Vulkan: Working
- DirectX 12: Working

## Current limitations
There are still a few rough edges.

- The available resolutions are currently based on a small preset list
- Exclusive fullscreen behavior can vary a bit depending on the system and display setup
- I have validated the app flow mostly through the sample menu and backend bring-up work, not through a large gameplay-style test surface

## SDL
There is also a simple SDL3 implementation that I made purely as a starting point.
If you only need SDL3 support for Noesis, simply refer to the main_sdl.cpp file.

## Disclaimer
I made this because I was having trouble finding clear examples of how to set up Noesis with Diligent Engine, and I wanted to share my findings. It started as me trying to get Vulkan working cleanly, and for some reason the AI agent suddenly became possible of figuring out how to do DirectX 12 as well using both the Vulkan and Noesis' own D3D12 implementation as a reference.

Parts of this project were made using the help of agentic tools (GitHub Copilot) as I am not an expert in Graphics programming, and I am still learning how to use Diligent Engine and Noesis effectively. I have done my best to ensure the code is correct and functional, but there may be mistakes or suboptimal implementations.
If you find any issues or have suggestions for improvements, please feel free to open an issue or submit a pull request.

This is not an official Noesis or Diligent Engine project, and I am not affiliated with either company. Use this code at your own risk, and refer to the official documentation for Noesis and Diligent Engine for production use.