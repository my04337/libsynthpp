cmake_minimum_required(VERSION 3.24)

file(GLOB_RECURSE srcs 
	src/*.h
	src/*.hpp
	src/*.c
	src/*.cpp
	)
	
add_executable(
	luath WIN32 
	${srcs}
	)
target_sources(
	luath 
	PRIVATE src/luath/luath.exe.manifest
)
		
target_include_directories(
	luath
	PUBLIC  src
)

target_compile_options(
	luath 
	PRIVATE	$<$<CXX_COMPILER_ID:MSVC>: /W3>
)

target_link_libraries(
	luath 
	PRIVATE 
		libsynth++
		D2d1.lib
		Dwrite.lib
		Shell32.lib
)

get_property(LUATH_BINARY_DIR TARGET luath PROPERTY BINARY_DIR )
get_property(LUATH_SOURCEY_DIR TARGET luath PROPERTY SOURCE_DIR )

add_custom_command(TARGET luath POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E make_directory ${LUATH_BINARY_DIR}/assets
	COMMAND ${CMAKE_COMMAND} -E make_directory ${LUATH_BINARY_DIR}/assets/font
	COMMAND ${CMAKE_COMMAND} -E make_directory ${LUATH_BINARY_DIR}/assets/sample_midi

	COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_LIST_DIR}/../third_party/UmeFont/ume-tgo4.ttf ${LUATH_BINARY_DIR}/assets/font
	COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_LIST_DIR}/../third_party/UmeFont/license.html ${LUATH_BINARY_DIR}/assets/font

	COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_LIST_DIR}/assets ${LUATH_BINARY_DIR}/assets
)