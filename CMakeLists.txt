cmake_minimum_required(VERSION 3.26)

project(libsynthpp CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(MSVC)
	# default compile options
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
add_subdirectory(luath-app)
