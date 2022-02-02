#Image Libraries

#jpgd used under the public domain license.
add_library(jpgd)

target_sources(jpgd 
PUBLIC 
    basis_universal/encoder/jpgd.cpp
    basis_universal/encoder/jpgd.h
)

target_include_directories(jpgd PUBLIC basis_universal/encoder/)

#stb_image
add_library(stb_image INTERFACE stb/stb_image.h)

target_include_directories(stb_image INTERFACE stb)

#lodepng
add_library(lodepng)

target_sources(lodepng 
PUBLIC 
    lodepng/lodepng.cpp
    lodepng/lodepng.h
)

target_include_directories(lodepng PUBLIC lodepng)

#everything else
option(SUPPORT_OGG "Support loading OGG Vorbis music via Tremor" ON)

add_subdirectory(bzip2-1.0.4)
add_subdirectory(freetype)
add_subdirectory(SDL2)
add_subdirectory(SDL_ttf)
add_subdirectory(soloud)
add_subdirectory(smpeg)
