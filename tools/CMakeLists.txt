add_executable(arcdec)

target_include_directories(arcdec PRIVATE ../)

target_compile_features(arcdec PUBLIC cxx_std_17)
target_sources(arcdec 
PRIVATE 
    arcdec.cpp
    ../NsaReader.cpp
    ../NsaReader.h
    ../DirPaths.cpp
    ../DirPaths.h
    ../DirectReader.cpp
    ../DirectReader.h
    ../SarReader.cpp
    ../SarReader.h
)

target_link_libraries(arcdec
PRIVATE
    BZip2::BZip2
)

target_compile_definitions(arcdec PUBLIC -DTOOLS_BUILD -DSAR)

set_target_properties(arcdec
PROPERTIES
    MSVC_RUNTIME_LIBRARY
        "MultiThreaded$<$<CONFIG:Debug>:Debug>"
)
