cmake_minimum_required(VERSION 3.25)
project(NoesisDiligent VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Set default build type if not specified
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "Choose the type of build (Debug, Release, RelWithDebInfo, MinSizeRel)" FORCE)
endif()
# Print the build type for confirmation
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")

include(SetPlatform.cmake)
include(Dependencies.cmake)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

option(NoesisDiligent_BUILD_TESTS "Build NoesisDiligent native test harnesses" ON)

if(NoesisDiligent_BUILD_TESTS)
    include(CTest)
else()
    set(BUILD_TESTING OFF CACHE BOOL "Enable testing" FORCE)
endif()

add_subdirectory(ThirdParty)
add_subdirectory(Implementation)
