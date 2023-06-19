cmake_minimum_required(VERSION 3.24)

include(FetchContent)

###########################################################
### SDL2
unset(SDL2_FIND_PACKAGE_EXTRA_ARGS)
if(MSVC)
	#[[
		set WORK_PATH=C:\build
		set LIBRARY_NAME=SDL2
		set LIBRARY_URL=https://github.com/libsdl-org/SDL
		set LIBRARY_TAG=release-2.26.5

		cd %WORK_PATH%
		mkdir %LIBRARY_NAME%-build
		git clone %LIBRARY_URL% -b %LIBRARY_TAG% %LIBRARY_NAME%-src

		cmake -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=%WORK_PATH%\%LIBRARY_NAME%  -S %LIBRARY_NAME%-src -B%LIBRARY_NAME%-build
		cmake --build %LIBRARY_NAME%-build
		cmake --install %LIBRARY_NAME%-build
	]]
	set(SDL2_PREBUILT_SUFFIX "2.26.5_win32_msvc2022_x64")
	set(SDL2_PREBUILT_DIR ${CMAKE_BINARY_DIR}/_deps/SDL2-${SDL2_PREBUILT_SUFFIX})
	if(NOT EXISTS ${SDL2_PREBUILT_DIR})
		message(STATUS "Extracting prebuilt binary : SDL-${SDL2_PREBUILT_SUFFIX}")
		file(ARCHIVE_EXTRACT 
			INPUT ${CMAKE_CURRENT_LIST_DIR}/third_party/SDL2/SDL2-${SDL2_PREBUILT_SUFFIX}.7z
			DESTINATION ${SDL2_PREBUILT_DIR}
		)
	endif()
	set(SDL2_FIND_PACKAGE_EXTRA_ARGS HINTS ${SDL2_PREBUILT_DIR})
endif()
find_package(SDL2 REQUIRED ${SDL2_FIND_PACKAGE_EXTRA_ARGS})


###########################################################
### SDL2_ttf
unset(SDL2_ttf_FIND_PACKAGE_EXTRA_ARGS)
if(MSVC)
	#[[
		set WORK_PATH=C:\build
		set LIBRARY_NAME=SDL2_ttf
		set LIBRARY_URL=https://github.com/libsdl-org/SDL_ttf
		set LIBRARY_TAG=release-2.20.2

		cd %WORK_PATH%
		mkdir %LIBRARY_NAME%-build
		git clone %LIBRARY_URL% --recursive -b %LIBRARY_TAG% %LIBRARY_NAME%-src

		cmake -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH=%WORK_PATH%\SDL2 -DCMAKE_INSTALL_PREFIX=%WORK_PATH%\%LIBRARY_NAME%  -S %LIBRARY_NAME%-src -B%LIBRARY_NAME%-build
		cmake --build %LIBRARY_NAME%-build
		cmake --install %LIBRARY_NAME%-build
	]]
	set(SDL2_ttf_PREBUILT_SUFFIX "2.20.2_win32_msvc2022_x64")
	set(SDL2_ttf_PREBUILT_DIR ${CMAKE_BINARY_DIR}/_deps/SDL2_ttf-${SDL2_ttf_PREBUILT_SUFFIX})
	if(NOT EXISTS ${SDL2_ttf_PREBUILT_DIR})
		message(STATUS "Extracting prebuilt binary : SDL_ttf-${SDL2_ttf_PREBUILT_SUFFIX}")
		file(ARCHIVE_EXTRACT 
			INPUT ${CMAKE_CURRENT_LIST_DIR}/third_party/SDL2_ttf/SDL2_ttf-${SDL2_ttf_PREBUILT_SUFFIX}.7z
			DESTINATION ${SDL2_ttf_PREBUILT_DIR}
		)
	endif()
	set(SDL2_ttf_FIND_PACKAGE_EXTRA_ARGS HINTS ${SDL2_ttf_PREBUILT_DIR})
endif()
find_package(SDL2_ttf REQUIRED ${SDL2_ttf_FIND_PACKAGE_EXTRA_ARGS})
