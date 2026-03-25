# ══════════════════════════════════════════════════════════════════════════
#  NoesisGUI.cmake — platform-aware Noesis SDK setup
#
#  Expects ENGINE_BASE_DIR to be set (Engine/).
#  Provides:
#    • NoesisSDK           – IMPORTED library target (Noesis core)
#    • NOESISGUI_BINARY_DIR – runtime DLL/SO directory for post-build copies
#    • NOESISGUI_APP_SOURCES – App Framework .cpp files to compile in
#    • NOESISGUI_APP_INCLUDE_DIR – App Framework header search path
# ══════════════════════════════════════════════════════════════════════════

set(NOESISGUI_VERSION "3.2.12" CACHE INTERNAL "NoesisGUI SDK version")

if (PLATFORM_WIN32)
    set(_NS_SDK_ROOT ${ENGINE_BASE_DIR}/../ThirdParty/NoesisSDK)

    if(NOT EXISTS "${_NS_SDK_ROOT}")
        message(FATAL_ERROR "Noesis SDK root not found at ${_NS_SDK_ROOT}")
    endif()

    if(NOT CMAKE_SIZEOF_VOID_P EQUAL 8)
        set(_NS_ARCH windows_x86)
    elseif (CMAKE_SYSTEM_PROCESSOR STREQUAL "ARM64")
        set(_NS_ARCH uwp_arm)
    else()
        set(_NS_ARCH windows_x86_64)
    endif()
# elseif (PLATFORM_LINUX)
#     set(_NS_SDK_ROOT ${ENGINE_BASE_DIR}/../ThirdParty/NoesisSDK)
#     if (CMAKE_SYSTEM_PROCESSOR STREQUAL x86_64 OR CMAKE_SYSTEM_PROCESSOR STREQUAL amd64 OR CMAKE_SYSTEM_PROCESSOR STREQUAL i686)
#         set(_NS_ARCH linux_x86_64)
#     elseif (CMAKE_SYSTEM_PROCESSOR STREQUAL armv7-a OR CMAKE_SYSTEM_PROCESSOR STREQUAL arm71 OR CMAKE_SYSTEM_PROCESSOR STREQUAL arm)
#         set(_NS_ARCH linux_arm)
#     else()
#         set(_NS_ARCH linux_arm64)
#     endif()
endif()

set(NOESISGUI_BINARY_DIR  ${_NS_SDK_ROOT}/Bin/${_NS_ARCH} CACHE INTERNAL "Noesis runtime DLL/SO directory")
set(NOESISGUI_LIBRARY_DIR ${_NS_SDK_ROOT}/Lib/${_NS_ARCH} CACHE INTERNAL "Noesis import library directory")
set(NOESISGUI_INCLUDE_DIR ${_NS_SDK_ROOT}/Include CACHE INTERNAL "Noesis header directory")
set(NOESISGUI_DATA_DIR ${_NS_SDK_ROOT}/Data CACHE INTERNAL "Noesis data directory")
set(NOESISGUI_TOOLS_DIR ${_NS_SDK_ROOT}/Src/Tools CACHE INTERNAL "Noesis SDK tools directory")

# On Linux the shared library sits next to the binaries.
if (PLATFORM_LINUX)
    set(NOESISGUI_LIBRARY_DIR ${NOESISGUI_BINARY_DIR})
endif()

# ── Imported target ──────────────────────────────────────────────────────
if(NOT TARGET NoesisSDK)
    add_library(NoesisSDK SHARED IMPORTED GLOBAL)

    if(PLATFORM_WIN32)
        set_target_properties(NoesisSDK PROPERTIES
            IMPORTED_IMPLIB "${NOESISGUI_LIBRARY_DIR}/Noesis.lib"
            IMPORTED_LOCATION "${NOESISGUI_BINARY_DIR}/Noesis.dll"
        )
    elseif(PLATFORM_LINUX)
        set_target_properties(NoesisSDK PROPERTIES
            IMPORTED_LOCATION "${NOESISGUI_LIBRARY_DIR}/libNoesis.so"
        )
    endif()

    set_target_properties(NoesisSDK PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES "${NOESISGUI_INCLUDE_DIR}"
    )
endif()

# ── App Framework source files ───────────────────────────────────────────
# These are compiled into the consuming target (not pre-built in the SDK).
set(_NS_APP_SRC       ${_NS_SDK_ROOT}/Src/Packages/App/ApplicationLauncher/Src)
set(_NS_APP_PROV_SRC  ${_NS_SDK_ROOT}/Src/Packages/App/Providers/Src)
set(_NS_APP_THEME_SRC ${_NS_SDK_ROOT}/Src/Packages/App/Theme/Src)
set(_NS_APP_THEME_DATA ${_NS_SDK_ROOT}/Src/Packages/App/Theme/Data/Theme)
set(_NS_RENDERCONTEXT_SRC ${_NS_SDK_ROOT}/Src/Packages/Render/RenderContext/Src)
set(_NS_D3D12_SRC     ${_NS_SDK_ROOT}/Src/Packages/Render/D3D12RenderDevice/Src)
set(_NS_VK_SRC        ${_NS_SDK_ROOT}/Src/Packages/Render/VKRenderDevice/Src)
set(_NS_VK_CONTEXT_SRC ${_NS_SDK_ROOT}/Src/Packages/Render/VKRenderContext/Src)

set(_NS_BIN2H ${NOESISGUI_TOOLS_DIR}/Bin2h/Bin2h.exe)
if(NOT EXISTS "${_NS_BIN2H}")
    message(FATAL_ERROR "Noesis Bin2h tool not found at ${_NS_BIN2H}")
endif()

set(NOESISGUI_THEME_GENERATED_DIR ${CMAKE_CURRENT_BINARY_DIR}/NoesisThemeGenerated
    CACHE INTERNAL "Generated Noesis App Theme headers"
)

file(MAKE_DIRECTORY "${NOESISGUI_THEME_GENERATED_DIR}")

set(_NS_BIN2H_SCRIPT ${CMAKE_CURRENT_BINARY_DIR}/RunNoesisBin2h.cmake)
file(WRITE "${_NS_BIN2H_SCRIPT}" [=[
execute_process(
    COMMAND "${BIN2H}" "${INPUT}"
    OUTPUT_FILE "${OUTPUT}"
    ERROR_VARIABLE error_output
    RESULT_VARIABLE result
)

if(NOT result EQUAL 0)
    message(FATAL_ERROR "Bin2h failed for ${INPUT}: ${error_output}")
endif()
]=])

set(_NS_THEME_DATA_FILES
    "${_NS_APP_THEME_DATA}/NoesisTheme.Fonts.xaml"
    "${_NS_APP_THEME_DATA}/NoesisTheme.Styles.xaml"
    "${_NS_APP_THEME_DATA}/NoesisTheme.Colors.Dark.xaml"
    "${_NS_APP_THEME_DATA}/NoesisTheme.Colors.Light.xaml"
    "${_NS_APP_THEME_DATA}/NoesisTheme.Brushes.DarkRed.xaml"
    "${_NS_APP_THEME_DATA}/NoesisTheme.Brushes.LightRed.xaml"
    "${_NS_APP_THEME_DATA}/NoesisTheme.Brushes.DarkGreen.xaml"
    "${_NS_APP_THEME_DATA}/NoesisTheme.Brushes.LightGreen.xaml"
    "${_NS_APP_THEME_DATA}/NoesisTheme.Brushes.DarkBlue.xaml"
    "${_NS_APP_THEME_DATA}/NoesisTheme.Brushes.LightBlue.xaml"
    "${_NS_APP_THEME_DATA}/NoesisTheme.Brushes.DarkOrange.xaml"
    "${_NS_APP_THEME_DATA}/NoesisTheme.Brushes.LightOrange.xaml"
    "${_NS_APP_THEME_DATA}/NoesisTheme.Brushes.DarkEmerald.xaml"
    "${_NS_APP_THEME_DATA}/NoesisTheme.Brushes.LightEmerald.xaml"
    "${_NS_APP_THEME_DATA}/NoesisTheme.Brushes.DarkPurple.xaml"
    "${_NS_APP_THEME_DATA}/NoesisTheme.Brushes.LightPurple.xaml"
    "${_NS_APP_THEME_DATA}/NoesisTheme.Brushes.DarkCrimson.xaml"
    "${_NS_APP_THEME_DATA}/NoesisTheme.Brushes.LightCrimson.xaml"
    "${_NS_APP_THEME_DATA}/NoesisTheme.Brushes.DarkLime.xaml"
    "${_NS_APP_THEME_DATA}/NoesisTheme.Brushes.LightLime.xaml"
    "${_NS_APP_THEME_DATA}/NoesisTheme.Brushes.DarkAqua.xaml"
    "${_NS_APP_THEME_DATA}/NoesisTheme.Brushes.LightAqua.xaml"
    "${_NS_APP_THEME_DATA}/NoesisTheme.DarkRed.xaml"
    "${_NS_APP_THEME_DATA}/NoesisTheme.LightRed.xaml"
    "${_NS_APP_THEME_DATA}/NoesisTheme.DarkGreen.xaml"
    "${_NS_APP_THEME_DATA}/NoesisTheme.LightGreen.xaml"
    "${_NS_APP_THEME_DATA}/NoesisTheme.DarkBlue.xaml"
    "${_NS_APP_THEME_DATA}/NoesisTheme.LightBlue.xaml"
    "${_NS_APP_THEME_DATA}/NoesisTheme.DarkOrange.xaml"
    "${_NS_APP_THEME_DATA}/NoesisTheme.LightOrange.xaml"
    "${_NS_APP_THEME_DATA}/NoesisTheme.DarkEmerald.xaml"
    "${_NS_APP_THEME_DATA}/NoesisTheme.LightEmerald.xaml"
    "${_NS_APP_THEME_DATA}/NoesisTheme.DarkPurple.xaml"
    "${_NS_APP_THEME_DATA}/NoesisTheme.LightPurple.xaml"
    "${_NS_APP_THEME_DATA}/NoesisTheme.DarkCrimson.xaml"
    "${_NS_APP_THEME_DATA}/NoesisTheme.LightCrimson.xaml"
    "${_NS_APP_THEME_DATA}/NoesisTheme.DarkLime.xaml"
    "${_NS_APP_THEME_DATA}/NoesisTheme.LightLime.xaml"
    "${_NS_APP_THEME_DATA}/NoesisTheme.DarkAqua.xaml"
    "${_NS_APP_THEME_DATA}/NoesisTheme.LightAqua.xaml"
    "${_NS_APP_THEME_DATA}/Fonts/PT Root UI_Regular.otf"
    "${_NS_APP_THEME_DATA}/Fonts/PT Root UI_Bold.otf"
)

unset(NOESISGUI_THEME_GENERATED_HEADERS)
foreach(_ns_theme_data_file IN LISTS _NS_THEME_DATA_FILES)
    get_filename_component(_ns_theme_generated_name "${_ns_theme_data_file}" NAME)
    set(_ns_theme_generated_file "${NOESISGUI_THEME_GENERATED_DIR}/${_ns_theme_generated_name}.bin.h")

    add_custom_command(
        OUTPUT "${_ns_theme_generated_file}"
        COMMAND "${CMAKE_COMMAND}"
            -DBIN2H=${_NS_BIN2H}
            -DINPUT=${_ns_theme_data_file}
            -DOUTPUT=${_ns_theme_generated_file}
            -P "${_NS_BIN2H_SCRIPT}"
        DEPENDS "${_ns_theme_data_file}" "${_NS_BIN2H}"
        VERBATIM
    )

    list(APPEND NOESISGUI_THEME_GENERATED_HEADERS "${_ns_theme_generated_file}")
endforeach()

set(NOESISGUI_APP_SOURCES
    ${_NS_APP_SRC}/NotifyPropertyChangedBase.cpp
    ${_NS_APP_SRC}/DelegateCommand.cpp
    ${_NS_APP_PROV_SRC}/LocalFontProvider.cpp
    ${_NS_APP_PROV_SRC}/EmbeddedFontProvider.cpp
    ${_NS_APP_PROV_SRC}/Find.cpp
    ${_NS_APP_PROV_SRC}/App.Providers.cpp
    # XAML provider — loads .xaml files from local directory
    ${_NS_APP_PROV_SRC}/EmbeddedXamlProvider.cpp
    ${_NS_APP_PROV_SRC}/LocalXamlProvider.cpp
    # Texture provider — loads textures from local directory (uses STB internally)
    ${_NS_APP_PROV_SRC}/FileTextureProvider.cpp
    ${_NS_APP_PROV_SRC}/LocalTextureProvider.cpp
    # FastLZ — used by render device shader decompression
    ${_NS_APP_PROV_SRC}/FastLZ.cpp
    # RichText - for BBCode support
    ${_NS_APP_SRC}/RichText.cpp
    ${_NS_APP_THEME_SRC}/App.Theme.cpp
    ${_NS_APP_THEME_SRC}/ThemeProviders.cpp
    ${_NS_RENDERCONTEXT_SRC}/Image.cpp
    ${_NS_RENDERCONTEXT_SRC}/Render.RenderContext.cpp
    ${_NS_RENDERCONTEXT_SRC}/RenderContext.cpp
)

# ── Backend render-device source files ───────────────────────────────────
# Each backend is provided as source so we compile it into our binary.
if(PLATFORM_WIN32)
    list(APPEND NOESISGUI_APP_SOURCES
        ${_NS_D3D12_SRC}/Render.D3D12RenderDevice.cpp
        ${_NS_D3D12_SRC}/D3D12RenderDevice.cpp
    )
endif()

list(APPEND NOESISGUI_APP_SOURCES
    ${_NS_VK_SRC}/Render.VKRenderDevice.cpp
    ${_NS_VK_SRC}/VKRenderDevice.cpp
    ${_NS_VK_CONTEXT_SRC}/Render.VKRenderContext.cpp
    ${_NS_VK_CONTEXT_SRC}/VKRenderContext.cpp
)

set(NOESISGUI_APP_INCLUDE_DIR
    ${_NS_SDK_ROOT}/Src/Packages/App/ApplicationLauncher/Include
    ${_NS_SDK_ROOT}/Src/Packages/App/Providers/Include
    CACHE INTERNAL "Noesis App Framework include directories"
)

set(NOESISGUI_APP_THEME_INCLUDE_DIR
    ${_NS_SDK_ROOT}/Src/Packages/App/Theme/Include
    CACHE INTERNAL "Noesis App Theme include directory"
)

set(NOESISGUI_RENDER_D3D12_INCLUDE_DIR
    ${_NS_SDK_ROOT}/Src/Packages/Render/D3D12RenderDevice/Include
    CACHE INTERNAL "Noesis D3D12 render device include directory"
)

set(NOESISGUI_RENDERCONTEXT_INCLUDE_DIR
    ${_NS_SDK_ROOT}/Src/Packages/Render/RenderContext/Include
    CACHE INTERNAL "Noesis render context include directory"
)

set(NOESISGUI_RENDER_D3D12_SRC_DIR
    ${_NS_SDK_ROOT}/Src/Packages/Render/D3D12RenderDevice/Src
    CACHE INTERNAL "Noesis D3D12 render device source directory (internal headers)"
)

set(NOESISGUI_RENDER_VK_INCLUDE_DIR
    ${_NS_SDK_ROOT}/Src/Packages/Render/VKRenderDevice/Include
    CACHE INTERNAL "Noesis Vulkan render device include directory"
)

set(NOESISGUI_RENDER_VK_SRC_DIR
    ${_NS_SDK_ROOT}/Src/Packages/Render/VKRenderDevice/Src
    CACHE INTERNAL "Noesis Vulkan render device source directory (internal headers)"
)

set(NOESISGUI_RENDER_VKCONTEXT_SRC_DIR
    ${_NS_SDK_ROOT}/Src/Packages/Render/VKRenderContext/Src
    CACHE INTERNAL "Noesis Vulkan render context source directory (internal headers)"
)