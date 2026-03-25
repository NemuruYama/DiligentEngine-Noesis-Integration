project(Implementation LANGUAGES C CXX)

set(ENGINE_BASE_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
include(${CMAKE_CURRENT_SOURCE_DIR}/NoesisGUI.cmake)

set(CLIENT_SOURCES
    src/DiligentNoesisBackendBase.cpp
    src/NoesisAppHost.cpp
    src/VulkanApp.cpp
    src/D3D12App.cpp
    ${CMAKE_SOURCE_DIR}/Visuals/src/KeybindRowViewModel.cpp
    ${CMAKE_SOURCE_DIR}/Visuals/src/MainMenuViewModel.cpp
    ${CMAKE_SOURCE_DIR}/Visuals/src/MainMenuVisuals.cpp
    ${CMAKE_SOURCE_DIR}/Visuals/src/SettingsViewModel.cpp
    ${NOESISGUI_APP_SOURCES}
)

set(CLIENT_HEADERS
    include/DiligentNoesisBackendBase.hpp
    include/NoesisAppHost.hpp
    include/VulkanApp.hpp
    include/D3D12App.hpp
    ${CMAKE_SOURCE_DIR}/Visuals/include/KeybindRowViewModel.hpp
    ${CMAKE_SOURCE_DIR}/Visuals/include/MainMenuViewModel.hpp
    ${CMAKE_SOURCE_DIR}/Visuals/include/MainMenuVisuals.hpp
    ${CMAKE_SOURCE_DIR}/Visuals/include/SettingsViewModel.hpp
)

add_library(ImplementationLib STATIC ${CLIENT_SOURCES} ${CLIENT_HEADERS})

if(NOT DEFINED NS_LICENSE_NAME)
    set(NS_LICENSE_NAME "" CACHE STRING "Noesis license name")
endif()

if(NOT DEFINED NS_LICENSE_KEY)
    set(NS_LICENSE_KEY "" CACHE STRING "Noesis license key")
endif()

target_compile_definitions(ImplementationLib
    PUBLIC
        NS_APP_APPLICATIONLAUNCHER_API=
        NS_APP_PROVIDERS_API=
        NS_APP_THEME_API=
        NS_RENDER_RENDERCONTEXT_API=
        NS_RENDER_D3D12RENDERDEVICE_API=
        NS_RENDER_VKRENDERDEVICE_API=
        $<$<PLATFORM_ID:Windows>:NOMINMAX>
    PRIVATE
        NS_LICENSE_NAME="${NS_LICENSE_NAME}"
        NS_LICENSE_KEY="${NS_LICENSE_KEY}"
        $<$<CXX_COMPILER_ID:MSVC>:_HAS_EXCEPTIONS=0>
)

target_compile_options(ImplementationLib PRIVATE
    $<$<CXX_COMPILER_ID:MSVC>:/EHs-c->
)

target_include_directories(ImplementationLib
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/Visuals/include
        ${NOESISGUI_INCLUDE_DIR}
        ${NOESISGUI_APP_INCLUDE_DIR}
        ${NOESISGUI_APP_THEME_INCLUDE_DIR}
        ${NOESISGUI_RENDERCONTEXT_INCLUDE_DIR}
        ${NOESISGUI_RENDER_D3D12_INCLUDE_DIR}
        ${NOESISGUI_RENDER_VK_INCLUDE_DIR}
        ${CMAKE_SOURCE_DIR}/ThirdParty/DiligentEngine/DiligentCore/ThirdParty/Vulkan-Headers/include
    PRIVATE
        ${NOESISGUI_THEME_GENERATED_DIR}
        ${NOESISGUI_RENDER_D3D12_SRC_DIR}
        ${NOESISGUI_RENDER_VK_SRC_DIR}
        ${NOESISGUI_RENDER_VKCONTEXT_SRC_DIR}
)

target_sources(ImplementationLib
    PRIVATE
        ${NOESISGUI_THEME_GENERATED_HEADERS}
)

target_link_libraries(ImplementationLib
    PUBLIC
        DiligentCore::Common
        DiligentCore::GraphicsEngine
        DiligentCore::GraphicsTools
        DiligentCore::BasicPlatform
        NoesisSDK
        SDL3::SDL3
        $<$<PLATFORM_ID:Windows>:$<TARGET_NAME_IF_EXISTS:DiligentCore::GraphicsEngineD3D12>>
        $<$<PLATFORM_ID:Windows>:$<TARGET_NAME_IF_EXISTS:DiligentCore::GraphicsEngineVk>>
        $<$<PLATFORM_ID:Linux>:$<TARGET_NAME_IF_EXISTS:DiligentCore::GraphicsEngineVk>>
)

if(PLATFORM_WIN32)
    set_source_files_properties(
        ${_NS_D3D12_SRC}/D3D12RenderDevice.cpp
        ${_NS_D3D12_SRC}/Render.D3D12RenderDevice.cpp
        PROPERTIES COMPILE_DEFINITIONS "UNICODE;_UNICODE"
    )
endif()

set(IMPLEMENTATION_APP_SOURCES
    src/main.cpp
)

set(IMPLEMENTATION_SDL_APP_SOURCES
    src/main_sdl.cpp
)

add_executable(ImplementationApp ${IMPLEMENTATION_APP_SOURCES})
add_executable(ImplementationSDLApp ${IMPLEMENTATION_SDL_APP_SOURCES})

target_link_libraries(ImplementationApp
    PRIVATE
        ImplementationLib
    SDL3::SDL3
)

target_link_libraries(ImplementationSDLApp
    PRIVATE
        ImplementationLib
    SDL3::SDL3
)

# ── Post-build: copy runtime DLLs to output directory ──────────────────
set(ASSETS_DIR "${CMAKE_SOURCE_DIR}/Implementation/assets")
set(VISUALS_ASSETS_DIR "${CMAKE_SOURCE_DIR}/Visuals/assets")

if(PLATFORM_WIN32)
    set(NOESIS_NATIVE_SDK_PLATFORM_SUFFIX "win")
    set(NOESIS_RUNTIME_LIB "${NOESISGUI_BINARY_DIR}/Noesis.dll")
endif()

function(configure_implementation_runtime target_name)
    if(DEFINED NOESIS_RUNTIME_LIB)
        add_custom_command(TARGET ${target_name} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${NOESIS_RUNTIME_LIB}"
                "$<TARGET_FILE_DIR:${target_name}>/Noesis.dll"
            COMMENT "Copying Noesis runtime library to output directory"
        )
    endif()

    add_custom_command(TARGET ${target_name} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${ASSETS_DIR}"
            "$<TARGET_FILE_DIR:${target_name}>/assets"
        COMMENT "Copying game assets to output directory"
    )

    add_custom_command(TARGET ${target_name} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${VISUALS_ASSETS_DIR}"
            "$<TARGET_FILE_DIR:${target_name}>/assets"
        COMMENT "Copying UI assets to output directory"
    )

    if(DEFINED NOESIS_NATIVE_SDK_PLATFORM_SUFFIX)
        set(NOESIS_NOCTURNAL_THEME "${NOESISGUI_DATA_DIR}/Styles/Nocturnal.xaml")
        if(EXISTS "${NOESIS_NOCTURNAL_THEME}")
            add_custom_command(TARGET ${target_name} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    "${NOESIS_NOCTURNAL_THEME}"
                    "$<TARGET_FILE_DIR:${target_name}>/assets/UI/Nocturnal.xaml"
                COMMENT "Copying Noesis Nocturnal theme to output directory"
            )
        endif()
    endif()
endfunction()

configure_implementation_runtime(ImplementationApp)
configure_implementation_runtime(ImplementationSDLApp)