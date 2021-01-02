cmake_minimum_required(VERSION 3.18)

add_executable(
	testsuite 
	
	main.cpp
	case/test_base.cpp
	case/test_sdl.cpp
	case/test_wasapi.cpp
	case/test_smf.cpp
	)

# file(INSTALL ${CMAKE_CURRENT_SOURCE_DIR}/files DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
	
target_link_libraries(testsuite libsynthpp SDL2main)

get_property(TESTSUITE_BINARY_DIR TARGET testsuite PROPERTY BINARY_DIR )
add_custom_command(TARGET testsuite POST_BUILD
    COMMAND robocopy /NFL /NDL /NJH /NJS /nc /ns /np  "${SDL2_LIB_DIRS}" "${TESTSUITE_BINARY_DIR}" SDL2.dll SDL2.dll 2> NUL & %COMSPEC% /C exit 0
)
