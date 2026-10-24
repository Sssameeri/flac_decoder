cmake_minimum_required(VERSION 4.3)
project(flac)

set(CMAKE_CXX_STANDARD 20)

add_executable(flac main.cpp
        streaminfo.h
        streaminfo.cpp
        Endianness.h
        vorbis.h
        vorbis.cpp
)
target_compile_options(flac PRIVATE -O0 -pedantic-errors -Wall -Wextra -Werror)
