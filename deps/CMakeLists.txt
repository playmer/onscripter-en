#Image Libraries

#jpgd used under the public domain license.
add_library(jpgd)

target_sources(jpgd 
PUBLIC 
    basis_universal/encoder/jpgd.cpp
    basis_universal/encoder/jpgd.h
)

target_include_directories(jpgd PUBLIC basis_universal/encoder/)

#everything else
option(SUPPORT_OGG "Support loading OGG Vorbis music via Tremor" ON)

#if (NOT UNIX OR APPLE)
#    add_subdirectory(SDL2)
#else()
#    find_package(SDL2 REQUIRED)
#endif()

add_subdirectory(soloud)


add_library(SDL2 ALIAS SDL2::SDL2-static)
add_subdirectory(smpeg)

set_target_properties(smpeg
PROPERTIES
    MSVC_RUNTIME_LIBRARY
        "MultiThreaded$<$<CONFIG:Debug>:Debug>"
)
