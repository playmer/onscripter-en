cmake_minimum_required(VERSION 3.20.0 FATAL_ERROR) 
project(onscripter_en CXX)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

add_definitions(-DMUSIC_OGG)

add_subdirectory(deps)

add_executable(onscripter_en)

target_compile_definitions(onscripter_en PUBLIC -DUSE_OGG_VORBIS -DINTEGER_OGG_VORBIS)

target_compile_features(onscripter_en PUBLIC cxx_std_17)

target_link_libraries(onscripter_en
PRIVATE
    bzlib
    SDL2_mixer
    smpeg
    SDL2
    SDL2main
    SDL2::image
    SDL2_ttf
    soloud
)

target_sources(onscripter_en
PRIVATE
    AnimationInfo.cpp
    AnimationInfo.h
    #AVIWrapper.cpp
    #AVIWrapper.h
    BaseReader.h
    CMakeLists.txt
    DirectReader.cpp
    DirectReader.h
    DirPaths.cpp
    DirPaths.h
    DirtyRect.cpp
    DirtyRect.h
    #embed.cpp
    FontInfo.cpp
    FontInfo.h
    graphics_altivec.cpp
    graphics_altivec.h
    graphics_blend.h
    graphics_common.h
    graphics_cpu.h
    graphics_mmx.cpp
    graphics_mmx.h
    graphics_resize.h
    graphics_routines.cpp
    graphics_sse2.cpp
    graphics_sse2.h
    graphics_sum.h
    Layer.cpp
    Layer.h
    #LUAHandler.cpp
    #LUAHandler.h
    #MadWrapper.cpp
    #MadWrapper.h
    NsaReader.cpp
    NsaReader.h
    #nscriptdecode.cpp
    onscripter.cpp
    ONScripterLabel.cpp
    ONScripterLabel.h
    ONScripterLabel_animation.cpp
    ONScripterLabel_command.cpp
    ONScripterLabel_effect.cpp
    ONScripterLabel_effect_breakup.cpp
    ONScripterLabel_effect_cascade.cpp
    ONScripterLabel_effect_trig.cpp
    ONScripterLabel_event.cpp
    ONScripterLabel_file.cpp
    ONScripterLabel_file2.cpp
    ONScripterLabel_image.cpp
    ONScripterLabel_rmenu.cpp
    ONScripterLabel_sound.cpp
    ONScripterLabel_text.cpp
    resize_image.cpp
    resize_image.h
    resources.h
    SarReader.cpp
    SarReader.h
    ScriptHandler.cpp
    ScriptHandler.h
    ScriptParser.cpp
    ScriptParser.h
    ScriptParser_command.cpp
    #SDL_win32_main.c
    sjis2utf16.cpp
    version.h
    winres.h
)

set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT onscripter_en)