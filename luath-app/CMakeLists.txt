cmake_minimum_required(VERSION 3.24)

file(GLOB_RECURSE srcs 
	src/*.h
	src/*.hpp
	src/*.c
	src/*.cpp
	)
	
add_executable(
	luath-app WIN32 
	${srcs}
	)
target_sources(
	luath-app 
	PRIVATE src/luath-app/luath-app.exe.manifest
)
		
target_include_directories(
	luath-app
	PUBLIC  src
)

target_compile_options(
	luath-app 
	PRIVATE	$<$<CXX_COMPILER_ID:MSVC>: /W3>
)

target_link_libraries(
	luath-app 
	PRIVATE 
		libsynth++
)

# link JUCE modules
unset(juce_modules)
# - ISC License
list(
	APPEND juce_modules
	juce_core
	juce_audio_basics
	juce_audio_devices
)
# - GPLv3 or Commerical
list(
	APPEND juce_modules
	juce_data_structures
	juce_graphics
	juce_gui_basics
	juce_gui_extra
	juce_opengl
)

# JUCEのライブラリをそのままtarget_link_libraryすると、不要なwarningが表示されるので
# warningレベルを調整している。
# 
# どうもtarget_link_libraryする対象毎にライブラリのobjをビルドしてリンクしているようなので、
# リンクしている側での調整が必要
foreach(juce_module IN LISTS juce_modules)
	get_target_property(juce_module_sources ${juce_module} INTERFACE_SOURCES)
	target_link_libraries(luath-app PRIVATE ${juce_module})
	set_property(SOURCE ${juce_module_sources} APPEND PROPERTY COMPILE_OPTIONS /wd4996  /wd4819)
endforeach()

# assets
get_property(LUATH_APP_BINARY_DIR TARGET luath-app PROPERTY BINARY_DIR )
get_property(LUATH_APP_SOURCEY_DIR TARGET luath-app PROPERTY SOURCE_DIR )

add_custom_command(TARGET luath-app POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E make_directory ${LUATH_APP_BINARY_DIR}/assets
	COMMAND ${CMAKE_COMMAND} -E make_directory ${LUATH_APP_BINARY_DIR}/assets/font
	COMMAND ${CMAKE_COMMAND} -E make_directory ${LUATH_APP_BINARY_DIR}/assets/sample_midi

	COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_LIST_DIR}/../third_party/UmeFont/ume-tgo4.ttf ${LUATH_APP_BINARY_DIR}/assets/font
	COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_LIST_DIR}/../third_party/UmeFont/license.html ${LUATH_APP_BINARY_DIR}/assets/font

	COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_LIST_DIR}/assets ${LUATH_APP_BINARY_DIR}/assets
)