cmake_minimum_required(VERSION 3.24)

file(GLOB_RECURSE srcs 
	src/*.h
	src/*.hpp
	src/*.c
	src/*.cpp
	)

add_library(
	libsynth++ STATIC
	${srcs}
)

target_compile_options(
	libsynth++ 
	PRIVATE	$<$<CXX_COMPILER_ID:MSVC>: /W3>
)

target_include_directories(
    libsynth++
    PUBLIC src
)