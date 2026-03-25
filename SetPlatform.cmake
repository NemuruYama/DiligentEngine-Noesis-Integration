# Credit: https://github.com/kuncarous/nextmu.git

set(PLATFORM_WIN32 FALSE)
set(PLATFORM_LINUX FALSE)
set(PLATFORM_MACOS FALSE)

if("${CMAKE_SIZEOF_VOID_P}" EQUAL "8")
    set(ARCH 64 CACHE INTERNAL "64-bit architecture")
else()
    set(ARCH 32 CACHE INTERNAL "32-bit architecture")
endif()

if(${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
    set(PLATFORM_LINUX TRUE)
    message("Target platform: Linux " ${ARCH})
elseif(${CMAKE_SYSTEM_NAME} STREQUAL "Darwin")
    set(PLATFORM_MACOS TRUE)
    message("Target platform: MacOS " ${ARCH})
elseif(WIN32)
    set(PLATFORM_WIN32 TRUE) #WIN32 is a variable, so we cannot use string "WIN32"
    message("Target platform: Win32 " ${ARCH} ". SDK Version: " ${CMAKE_SYSTEM_VERSION})
else()
    message(FATAL_ERROR "Unsupported platform")
endif()