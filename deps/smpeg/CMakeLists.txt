add_library(smpeg STATIC)

target_sources(smpeg
PRIVATE
    MPEG.cpp
    MPEG.h
    MPEGaction.h
    MPEGaudio.h
    MPEGerror.h
    MPEGframe.h
    MPEGlist.cpp
    MPEGlist.h
    MPEGring.cpp
    MPEGring.h
    MPEGstream.cpp
    MPEGstream.h
    MPEGsystem.cpp
    MPEGsystem.h
    MPEGvideo.h
    smpeg.cpp
    smpeg.h
    audio/bitwindow.cpp
    audio/filter.cpp
    audio/filter_2.cpp
    audio/hufftable.cpp
    audio/MPEGaudio.cpp
    audio/mpeglayer1.cpp
    audio/mpeglayer2.cpp
    audio/mpeglayer3.cpp
    audio/mpegtable.cpp
    audio/mpegtoraw.cpp
    video/decoders.cpp
    video/decoders.h
    video/dither.h
    video/floatdct.cpp
    video/gdith.cpp
    video/jrevdct.cpp
    video/mmxflags_asm.S
    video/mmxidct_asm.S
    video/motionvec.cpp
    video/MPEGvideo.cpp
    video/parseblock.cpp
    video/proto.h
    video/readfile.cpp
    video/README
    video/util.cpp
    video/util.h
    video/video.cpp
    video/video.h
)

target_include_directories(smpeg PUBLIC ${CMAKE_CURRENT_LIST_DIR})
target_link_libraries(smpeg PRIVATE SDL2)