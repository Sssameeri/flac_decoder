cmake_minimum_required(VERSION 4.3)
project(flac)

set(CMAKE_CXX_STANDARD 20)

add_executable(flac main.cpp
        streaminfo.h
        streaminfo.cpp
        endianness.h
        vorbis.h
        vorbis.cpp
        bitsreader.h
        bitsreader.cpp
        blocksize.h
        blocksize.cpp
        samplerate.h
        samplerate.cpp
        channels.h
        channels.cpp
        bitdepth.cpp
        bitdepth.h
        subframetype.cpp
        subframetype.h
        residual.h
        residual.cpp
        subframe.h
        subframe.cpp
        frame.h
        frame.cpp
        player.h
        player.cpp
)
target_compile_options(flac PRIVATE -O0 -pedantic-errors -Wall -Wextra -Werror)
target_link_libraries(flac PRIVATE winmm)
