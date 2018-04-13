cmake_minimum_required(VERSION 3.2)

add_executable(
	testsuite 
	
	main.cpp
	case/test_base.cpp
	case/test_wasapi.cpp
	case/test_smf.cpp
	)

# file(INSTALL ${CMAKE_CURRENT_SOURCE_DIR}/files DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

	
target_link_libraries(testsuite libsynthpp)
