cmake_minimum_required(VERSION 3.24)

file(GLOB_RECURSE srcs 
	src/*.h
	src/*.hpp
	src/*.c
	src/*.cpp
	)

add_library(
	luath STATIC
	${srcs}
)

target_compile_options(
	luath 
	PRIVATE	$<$<CXX_COMPILER_ID:MSVC>: /W3>
)

target_include_directories(
    luath
    PUBLIC src
)

target_link_libraries(
	luath
	PRIVATE
		libsynth++
)