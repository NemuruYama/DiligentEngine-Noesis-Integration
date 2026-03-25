set(SDL_UNIX_CONSOLE_BUILD ON CACHE BOOL "" FORCE)
set(SDL_TEST_LIBRARY OFF CACHE BOOL "" FORCE)
add_subdirectory(SDL)

set(DILIGENT_TREAT_WARNINGS_AS_ERRORS OFF CACHE BOOL "" FORCE)
set(SPIRV_WERROR OFF CACHE BOOL "" FORCE)
set(DILIGENT_BUILD_SAMPLES OFF CACHE BOOL "" FORCE)
set(DILIGENT_BUILD_TUTORIALS OFF CACHE BOOL "" FORCE)
set(DILIGENT_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(DILIGENT_NO_FORMAT_VALIDATION ON CACHE BOOL "" FORCE)
set(DILIGENT_NO_OPENGL ON CACHE BOOL "" FORCE)

add_subdirectory(DiligentEngine/DiligentCore)

if(NOT TARGET DiligentCore::Common)
    add_library(DiligentCore::Common ALIAS Diligent-Common)
endif()

if(NOT TARGET DiligentCore::GraphicsEngine AND TARGET Diligent-GraphicsEngine)
    add_library(DiligentCore::GraphicsEngine ALIAS Diligent-GraphicsEngine)
endif()

if(NOT TARGET DiligentCore::GraphicsTools AND TARGET Diligent-GraphicsTools)
    add_library(DiligentCore::GraphicsTools ALIAS Diligent-GraphicsTools)
endif()

if(NOT TARGET DiligentCore::BasicPlatform AND TARGET Diligent-BasicPlatform)
    add_library(DiligentCore::BasicPlatform ALIAS Diligent-BasicPlatform)
endif()

if(WIN32)
    if(TARGET Diligent-GraphicsEngineD3D11-static)
        add_library(DiligentCore::GraphicsEngineD3D11 ALIAS Diligent-GraphicsEngineD3D11-static)
    endif()
    if(TARGET Diligent-GraphicsEngineD3D12-static)
        add_library(DiligentCore::GraphicsEngineD3D12 ALIAS Diligent-GraphicsEngineD3D12-static)
    endif()
    if(TARGET Diligent-GraphicsEngineOpenGL-static)
        add_library(DiligentCore::GraphicsEngineGL ALIAS Diligent-GraphicsEngineOpenGL-static)
    endif()
    if(TARGET Diligent-GraphicsEngineVk-static)
        add_library(DiligentCore::GraphicsEngineVk ALIAS Diligent-GraphicsEngineVk-static)
    endif()
elseif(UNIX AND NOT APPLE)
    if(TARGET Diligent-GraphicsEngineVk-static)
        add_library(DiligentCore::GraphicsEngineVk ALIAS Diligent-GraphicsEngineVk-static)
    endif()
    if(TARGET Diligent-GraphicsEngineOpenGL-static)
        add_library(DiligentCore::GraphicsEngineGL ALIAS Diligent-GraphicsEngineOpenGL-static)
    endif()
endif()

if(MSVC)
    foreach(TARGET_NAME SPIRV-Tools-opt SPIRV-Tools-static SPIRV-Tools)
        if(TARGET ${TARGET_NAME})
            get_target_property(ALIASED_NAME ${TARGET_NAME} ALIASED_TARGET)
            if(ALIASED_NAME)
                target_compile_options(${ALIASED_NAME} PRIVATE /WX-)
            else()
                target_compile_options(${TARGET_NAME} PRIVATE /WX-)
            endif()
        endif()
    endforeach()
endif()

include(FetchContent)
FetchContent_Declare(
    freetype
    GIT_REPOSITORY https://github.com/freetype/freetype.git
    GIT_TAG VER-2-13-3
)

set(FT_DISABLE_ZLIB ON CACHE BOOL "" FORCE)
set(FT_DISABLE_BZIP2 ON CACHE BOOL "" FORCE)
set(FT_DISABLE_PNG ON CACHE BOOL "" FORCE)
set(FT_DISABLE_HARFBUZZ ON CACHE BOOL "" FORCE)
set(FT_DISABLE_BROTLI ON CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(freetype)

if(NOT TARGET Freetype::Freetype)
    add_library(Freetype::Freetype ALIAS freetype)
endif()