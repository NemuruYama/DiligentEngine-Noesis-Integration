include(${CMAKE_CURRENT_LIST_DIR}/CPM.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/SetPlatform.cmake)

if (NOT CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    add_compile_options(-Wno-unsafe-buffer-usage)
    if (NOT PLATFORM_LINUX)
        link_libraries(-Wl,--undefined-version)
    endif()
endif()