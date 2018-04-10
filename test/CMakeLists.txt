cmake_minimum_required(VERSION 3.2)

add_executable(
	testsuite 
	
	main.cpp
	case/test_base.cpp
	case/test_wasapi.cpp
	)
	
target_link_libraries(testsuite libsynthpp)
