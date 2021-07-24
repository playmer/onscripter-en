option(SUPPORT_OGG "Support loading OGG Vorbis music via Tremor" ON)

add_subdirectory(bzip2-1.0.4)
add_subdirectory(freetype)
add_subdirectory(SDL2)
add_subdirectory(SDL_mixer)

# Hack to try to fix an error from Github Actions when compiling libpng I can't repro locally.
include_directories(SDL_Image/zlib-1.2.11/)
add_subdirectory(SDL_image)

add_subdirectory(SDL_ttf)
add_subdirectory(soloud)
add_subdirectory(smpeg)