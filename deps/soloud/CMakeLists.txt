add_library(soloud STATIC)

target_link_libraries(soloud PUBLIC SDL2::SDL2-static)

target_include_directories(soloud PUBLIC repo/include)


set_target_properties(soloud
PROPERTIES
    MSVC_RUNTIME_LIBRARY
        "MultiThreaded$<$<CONFIG:Debug>:Debug>"
)


target_sources(soloud
PRIVATE
    #repo/demos/c_test/main.c
    #repo/demos/common/glew/GL/glew.c
    repo/include/soloud.h
    repo/include/soloud_audiosource.h
    repo/include/soloud_ay.h
    repo/include/soloud_bassboostfilter.h
    repo/include/soloud_biquadresonantfilter.h
    repo/include/soloud_bus.h
    repo/include/soloud_c.h
    repo/include/soloud_dcremovalfilter.h
    repo/include/soloud_echofilter.h
    repo/include/soloud_eqfilter.h
    repo/include/soloud_error.h
    repo/include/soloud_fader.h
    repo/include/soloud_fft.h
    repo/include/soloud_fftfilter.h
    repo/include/soloud_file.h
    repo/include/soloud_file_hack_off.h
    repo/include/soloud_file_hack_on.h
    repo/include/soloud_filter.h
    repo/include/soloud_flangerfilter.h
    repo/include/soloud_freeverbfilter.h
    repo/include/soloud_internal.h
    repo/include/soloud_lofifilter.h
    repo/include/soloud_misc.h
    repo/include/soloud_monotone.h
    repo/include/soloud_noise.h
    repo/include/soloud_openmpt.h
    repo/include/soloud_queue.h
    repo/include/soloud_robotizefilter.h
    repo/include/soloud_sfxr.h
    repo/include/soloud_speech.h
    repo/include/soloud_tedsid.h
    repo/include/soloud_thread.h
    repo/include/soloud_vic.h
    repo/include/soloud_vizsn.h
    repo/include/soloud_wav.h
    repo/include/soloud_waveshaperfilter.h
    repo/include/soloud_wavstream.h
    repo/include/zx7decompress.h
    repo/src/audiosource/ay/chipplayer.cpp
    repo/src/audiosource/ay/chipplayer.h
    repo/src/audiosource/ay/sndbuffer.cpp
    repo/src/audiosource/ay/sndbuffer.h
    repo/src/audiosource/ay/sndchip.cpp
    repo/src/audiosource/ay/sndchip.h
    repo/src/audiosource/ay/sndrender.cpp
    repo/src/audiosource/ay/sndrender.h
    repo/src/audiosource/ay/soloud_ay.cpp
    repo/src/audiosource/monotone/soloud_monotone.cpp
    repo/src/audiosource/noise/soloud_noise.cpp
    repo/src/audiosource/openmpt/soloud_openmpt.cpp
    repo/src/audiosource/openmpt/soloud_openmpt_dll.c
    repo/src/audiosource/sfxr/soloud_sfxr.cpp
    repo/src/audiosource/speech/darray.cpp
    repo/src/audiosource/speech/darray.h
    repo/src/audiosource/speech/klatt.cpp
    repo/src/audiosource/speech/klatt.h
    repo/src/audiosource/speech/resonator.cpp
    repo/src/audiosource/speech/resonator.h
    repo/src/audiosource/speech/soloud_speech.cpp
    repo/src/audiosource/speech/tts.cpp
    repo/src/audiosource/speech/tts.h
    repo/src/audiosource/tedsid/sid.cpp
    repo/src/audiosource/tedsid/sid.h
    repo/src/audiosource/tedsid/soloud_tedsid.cpp
    repo/src/audiosource/tedsid/ted.cpp
    repo/src/audiosource/tedsid/ted.h
    repo/src/audiosource/vic/soloud_vic.cpp
    repo/src/audiosource/vizsn/soloud_vizsn.cpp
    repo/src/audiosource/wav/dr_flac.h
    repo/src/audiosource/wav/dr_impl.cpp
    repo/src/audiosource/wav/dr_mp3.h
    repo/src/audiosource/wav/dr_wav.h
    repo/src/audiosource/wav/soloud_wav.cpp
    repo/src/audiosource/wav/soloud_wavstream.cpp
    repo/src/audiosource/wav/stb_vorbis.c
    repo/src/audiosource/wav/stb_vorbis.h
    #repo/src/backend/alsa/soloud_alsa.cpp
    #repo/src/backend/coreaudio/soloud_coreaudio.cpp
    #repo/src/backend/jack/soloud_jack.cpp
    #repo/src/backend/miniaudio/miniaudio.h
    #repo/src/backend/miniaudio/soloud_miniaudio.cpp
    #repo/src/backend/nosound/soloud_nosound.cpp
    #repo/src/backend/null/soloud_null.cpp
    #repo/src/backend/openal/soloud_openal.cpp
    #repo/src/backend/openal/soloud_openal_dll.c
    #repo/src/backend/opensles/soloud_opensles.cpp
    #repo/src/backend/oss/soloud_oss.cpp
    #repo/src/backend/portaudio/soloud_portaudio.cpp
    #repo/src/backend/portaudio/soloud_portaudio_dll.c
    #repo/src/backend/sdl/soloud_sdl1.cpp
    #repo/src/backend/sdl/soloud_sdl1_dll.c
    #repo/src/backend/sdl/soloud_sdl2.cpp
    #repo/src/backend/sdl/soloud_sdl2_dll.c
    #repo/src/backend/sdl2_static/soloud_sdl2_static.cpp
    #repo/src/backend/sdl_static/soloud_sdl_static.cpp
    #repo/src/backend/vita_homebrew/soloud_vita_homebrew.cpp
    repo/src/c_api/soloud_c.cpp
    repo/src/core/soloud.cpp
    repo/src/core/soloud_audiosource.cpp
    repo/src/core/soloud_bus.cpp
    repo/src/core/soloud_core_3d.cpp
    repo/src/core/soloud_core_basicops.cpp
    repo/src/core/soloud_core_faderops.cpp
    repo/src/core/soloud_core_filterops.cpp
    repo/src/core/soloud_core_getters.cpp
    repo/src/core/soloud_core_setters.cpp
    repo/src/core/soloud_core_voicegroup.cpp
    repo/src/core/soloud_core_voiceops.cpp
    repo/src/core/soloud_fader.cpp
    repo/src/core/soloud_fft.cpp
    repo/src/core/soloud_fft_lut.cpp
    repo/src/core/soloud_file.cpp
    repo/src/core/soloud_filter.cpp
    repo/src/core/soloud_misc.cpp
    repo/src/core/soloud_queue.cpp
    repo/src/core/soloud_thread.cpp
    repo/src/filter/soloud_bassboostfilter.cpp
    repo/src/filter/soloud_biquadresonantfilter.cpp
    repo/src/filter/soloud_dcremovalfilter.cpp
    repo/src/filter/soloud_echofilter.cpp
    repo/src/filter/soloud_eqfilter.cpp
    repo/src/filter/soloud_fftfilter.cpp
    repo/src/filter/soloud_flangerfilter.cpp
    repo/src/filter/soloud_freeverbfilter.cpp
    repo/src/filter/soloud_lofifilter.cpp
    repo/src/filter/soloud_robotizefilter.cpp
    repo/src/filter/soloud_waveshaperfilter.cpp
    #repo/src/tools/codegen/main.cpp
    #repo/src/tools/lutgen/main.cpp
    #repo/src/tools/resamplerlab/main.cpp
    #repo/src/tools/resamplerlab/stb_image_write.c
    #repo/src/tools/resamplerlab/stb_image_write.h
    #repo/src/tools/sanity/sanity.cpp
)


if (${CMAKE_SYSTEM_NAME} STREQUAL Windows)
    target_compile_definitions(soloud PUBLIC -DWITH_WASAPI -DWITH_WINMM -DWITH_XAUDIO2)

    target_sources(soloud PUBLIC
        repo/src/backend/wasapi/soloud_wasapi.cpp
        repo/src/backend/winmm/soloud_winmm.cpp
        repo/src/backend/xaudio2/soloud_xaudio2.cpp
    )
elseif(${CMAKE_SYSTEM_NAME} STREQUAL Darwin)
    find_library(COREAUDIO_LIBRARY CoreAudio)
    find_library(COREFOUNDATION_LIBRARY CoreFoundation)
    find_library(AUDIOTOOLBOX_LIBRARY AudioToolbox)

    target_link_libraries(soloud PUBLIC ${AUDIOTOOLBOX_LIBRARY} ${COREAUDIO_LIBRARY} ${COREFOUNDATION_LIBRARY})
    target_compile_definitions(soloud PUBLIC -DWITH_COREAUDIO)

    target_sources(soloud PUBLIC
        repo/src/backend/coreaudio/soloud_coreaudio.cpp
    )
else()
    set(CMAKE_THREAD_PREFER_PTHREAD TRUE)
    set(THREADS_PREFER_PTHREAD_FLAG TRUE)
    find_package(Threads REQUIRED)

    target_compile_definitions(soloud PUBLIC -DWITH_MINIAUDIO)

    target_link_libraries(soloud PUBLIC dl Threads::Threads)

    target_sources(soloud PUBLIC
        repo/src/backend/miniaudio/soloud_miniaudio.cpp
    )
endif()