cmake_minimum_required(VERSION 3.18)

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
		
target_include_directories(
             luath-gui
             PUBLIC  src
            )

target_link_libraries(luath-gui libsynthpp SDL2main SDL2_ttf.lib)
add_dependencies(luath-gui SDL2_ttf UmeFont)

get_property(LUATH_BINARY_DIR TARGET luath-gui PROPERTY BINARY_DIR )
get_property(LUATH_SOURCEY_DIR TARGET luath-gui PROPERTY SOURCE_DIR )
add_custom_command(TARGET luath-gui POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E make_directory ${LUATH_BINARY_DIR}/assets
	COMMAND ${CMAKE_COMMAND} -E make_directory ${LUATH_BINARY_DIR}/assets/font
	COMMAND ${CMAKE_COMMAND} -E make_directory ${LUATH_BINARY_DIR}/assets/sample_midi

	COMMAND ${CMAKE_COMMAND} -E copy ${SDL2_LIB_DIRS}/SDL2.dll ${LUATH_BINARY_DIR}
	COMMAND ${CMAKE_COMMAND} -E copy ${SDL2_ttf_LIB_DIRS}/SDL2_ttf.dll ${LUATH_BINARY_DIR}
	COMMAND ${CMAKE_COMMAND} -E copy ${SDL2_ttf_LIB_DIRS}/libfreetype-6.dll ${LUATH_BINARY_DIR}
	COMMAND ${CMAKE_COMMAND} -E copy ${SDL2_ttf_LIB_DIRS}/zlib1.dll ${LUATH_BINARY_DIR}

	COMMAND ${CMAKE_COMMAND} -E copy ${UmeFont_ROOT_PATH}/ume-tgo4.ttf ${LUATH_BINARY_DIR}/assets/font
	COMMAND ${CMAKE_COMMAND} -E copy ${UmeFont_ROOT_PATH}/license.html ${LUATH_BINARY_DIR}/assets/font

	COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_LIST_DIR}/assets ${LUATH_BINARY_DIR}/assets
)
 