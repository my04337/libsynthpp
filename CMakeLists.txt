cmake_minimum_required(VERSION 3.2)

project(libsynthpp CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /Zc:rvalueCast /Zc:strictStrings /Zc:inline")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /EHsc")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /Gy /Gw")

set(LSP_SAMPLE_MIDI_PATH "${CMAKE_CURRENT_LIST_DIR}/test/files")

# ---

include(CMakeLists-SDL.txt)
include(CMakeLists-SDL_ttf.txt)
include(CMakeLists-UmeFont.txt)

# ---

add_subdirectory(libsynthpp)
add_subdirectory(test)
add_subdirectory(luath)
