cmake_minimum_required(VERSION 3.24)

file(GLOB_RECURSE srcs 
	src/*.h
	src/*.hpp
	src/*.c
	src/*.cpp
	)
	
add_executable(
	luath-gui WIN32 
	${srcs}
	)
target_sources(
	luath-gui 
	PRIVATE src/luath_gui/luath-gui.exe.manifest
)
		
target_include_directories(
	luath-gui
	PUBLIC  src
)

target_compile_options(
	luath-gui 
	PRIVATE	$<$<CXX_COMPILER_ID:MSVC>: /W3>
)

target_link_libraries(
	luath-gui 
	PRIVATE 
		libsynthpp
		SDL2::SDL2
		SDL2_ttf::SDL2_ttf
)

get_property(LUATH_BINARY_DIR TARGET luath-gui PROPERTY BINARY_DIR )
get_property(LUATH_SOURCEY_DIR TARGET luath-gui PROPERTY SOURCE_DIR )

add_custom_command(TARGET luath-gui POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E make_directory ${LUATH_BINARY_DIR}/assets
	COMMAND ${CMAKE_COMMAND} -E make_directory ${LUATH_BINARY_DIR}/assets/font
	COMMAND ${CMAKE_COMMAND} -E make_directory ${LUATH_BINARY_DIR}/assets/sample_midi

	COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_LIST_DIR}/../third_party/UmeFont/ume-tgo4.ttf ${LUATH_BINARY_DIR}/assets/font
	COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_LIST_DIR}/../third_party/UmeFont/license.html ${LUATH_BINARY_DIR}/assets/font

	COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_LIST_DIR}/assets ${LUATH_BINARY_DIR}/assets
)
 
if(WIN32)
	get_target_property(SDL2_DLL_PATH SDL2::SDL2 LOCATION)
	get_target_property(SDL2_TTF_DLL_PATH SDL2_ttf::SDL2_ttf LOCATION)

	add_custom_command(TARGET luath-gui POST_BUILD
		COMMAND ${CMAKE_COMMAND} -E copy ${SDL2_DLL_PATH}		${LUATH_BINARY_DIR}
		COMMAND ${CMAKE_COMMAND} -E copy ${SDL2_TTF_DLL_PATH}	${LUATH_BINARY_DIR}
	) 
endif()
