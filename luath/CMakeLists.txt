cmake_minimum_required(VERSION 3.2)

add_executable(
	luath WIN32 
	
	src/main.cpp
	src/App/Application.cpp
	src/Window/MainWindow.cpp
	)
	
	
target_include_directories(
             luath
             PUBLIC  ./include
            )

target_link_libraries(luath libsynthpp SDL2main)

get_property(LUATH_BINARY_DIR TARGET luath PROPERTY BINARY_DIR )
add_custom_command(TARGET luath POST_BUILD
    COMMAND robocopy /NFL /NDL /NJH /NJS /nc /ns /np  "${SDL2_LIB_DIRS}" "${LUATH_BINARY_DIR}" SDL2.dll 2> NUL & %COMSPEC% /C exit 0
)
 