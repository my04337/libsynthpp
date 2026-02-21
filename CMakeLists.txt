cmake_minimum_required(VERSION 3.26)

project(libsynthpp CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# C++ Modules有効化 (experimental)
#   まだ不安定なAPIのため、どのCMakeバージョンについてのドキュメントか十分注意すること
#   see https://www.kitware.com/import-cmake-c20-modules/
#set(CMAKE_EXPERIMENTAL_CXX_MODULE_CMAKE_API "2182bf5c-ef0d-489a-91da-49dbc3090d2a") # for CMake 3.26
#set(CMAKE_EXPERIMENTAL_CXX_MODULE_DYNDEP 1) # for MSVC

if(MSVC)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /Zc:rvalueCast /Zc:strictStrings /Zc:inline")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /EHsc")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /Gy /Gw")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DUNICODE -D_UNICODE")
endif()

find_program(CPPLINT_EXE cpplint)
if(NOT CPPLINT_EXE STREQUAL "CPPLINT_EXE-NOTFOUND") 
	#set(CMAKE_CXX_CPPLINT ${CPPLINT_EXE})
endif()

set(LSP_SAMPLE_MIDI_PATH "${CMAKE_CURRENT_LIST_DIR}/test/files")

# ---

include(depends.cmake)

# ---

add_subdirectory(libsynthpp)
add_subdirectory(luath)
