add_library(bzlib STATIC)

target_include_directories(bzlib PUBLIC ${CMAKE_CURRENT_LIST_DIR})

target_sources(bzlib
PRIVATE
    blocksort.c
    bzlib.c
    bzlib.h
    bzlib_private.h
    compress.c
    crctable.c
    decompress.c
    huffman.c
    randtable.c
)