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

target_link_libraries(luath libsynthpp SDL2-static SDL2main)
