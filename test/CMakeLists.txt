cmake_minimum_required(VERSION 3.2)

add_executable(
	unit_test 
	
	main.cpp
	test_base.cpp
	test_util.cpp
	)
	
target_link_libraries(unit_test libsynthpp)
