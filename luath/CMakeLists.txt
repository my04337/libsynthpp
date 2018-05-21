cmake_minimum_required(VERSION 3.2)

add_executable(
	luath WIN32 
	
	src/main.cpp
	
	src/Base/Base.cpp

	src/App/Application.cpp
	src/App/FontCache.cpp

	src/Window/MainWindow.cpp

	src/Widget/OscilloScope.cpp
	)
	
	
target_include_directories(
             luath
             PUBLIC  ./include
            )

target_link_libraries(luath libsynthpp SDL2main SDL2_ttf.lib)
add_dependencies(luath SDL2_ttf UmeFont)

get_property(LUATH_BINARY_DIR TARGET luath PROPERTY BINARY_DIR )
get_property(LUATH_SOURCEY_DIR TARGET luath PROPERTY SOURCE_DIR )
add_custom_command(TARGET luath POST_BUILD
    COMMAND robocopy /NFL /NDL /NJH /NJS /nc /ns /np  "${SDL2_LIB_DIRS}" "${LUATH_BINARY_DIR}" SDL2.dll 2> NUL & %COMSPEC% /C exit 0
    COMMAND robocopy /NFL /NDL /NJH /NJS /nc /ns /np  "${SDL2_ttf_LIB_DIRS}" "${LUATH_BINARY_DIR}" SDL2_ttf.dll libfreetype-6.dll zlib1.dll 2> NUL & %COMSPEC% /C exit 0
	COMMAND robocopy /NFL /NDL /NJH /NJS /nc /ns /np  "${UmeFont_ROOT_PATH}" "${LUATH_BINARY_DIR}/assets/font" license.html ume-tgo4.ttf 2> NUL & %COMSPEC% /C exit 0
	COMMAND robocopy /NFL /NDL /NJH /NJS /nc /ns /np  "${LSP_SAMPLE_MIDI_PATH}" "${LUATH_BINARY_DIR}/assets/midi" 2> NUL & %COMSPEC% /C exit 0
)
 