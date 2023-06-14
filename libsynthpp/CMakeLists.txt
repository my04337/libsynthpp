cmake_minimum_required(VERSION 3.24)

file(GLOB_RECURSE srcs 
	src/*.h
	src/*.hpp
	src/*.c
	src/*.cpp
	)

add_library(
	libsynthpp STATIC
	${srcs}
)

target_compile_options(
	libsynthpp PRIVATE
	$<$<CXX_COMPILER_ID:MSVC>: /W3>
)


target_include_directories(
             libsynthpp
             PUBLIC  src
            )
			
target_link_libraries(libsynthpp SDL2.lib)