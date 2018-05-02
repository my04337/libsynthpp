cmake_minimum_required(VERSION 3.2)

project(libsynthpp CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /Zc:rvalueCast /Zc:strictStrings /Zc:inline")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /EHsc")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /Gy /Gw")

# ---

add_subdirectory(third_party/sdl)
set(SDL2_INCLUDE_DIRS "${SDL2_SOURCE_DIR}/include")

# ---

add_subdirectory(src)
add_subdirectory(test)
