cmake_minimum_required(VERSION 3.26)

include(FetchContent)

###########################################################
### C++ Modules for MSVC(experimental)
#if(MSVC) 
#	# 少なくともVS2022 17.6時代では std, std.compat ｗo自前でビルドする必要あり
#	#   see https://learn.microsoft.com/ja-jp/cpp/cpp/tutorial-import-stl-named-module?view=msvc-170
#
#	# 各モジュール置き場を解決
#	set(MSVC_CXX_STD_MODULE_DIR ${CMAKE_BINARY_DIR}/_deps/cxx_std)
#	cmake_path(SET VCToolsInstallDir NORMALIZE "$ENV{VCToolsInstallDir}" )
#	if(NOT EXISTS "${MSVC_CXX_STD_MODULE_DIR}") 
#		file(MAKE_DIRECTORY "${MSVC_CXX_STD_MODULE_DIR}")
#	endif()
#
#	# モジュールのビルド
#	if(NOT EXISTS "${MSVC_CXX_STD_MODULE_DIR}/std.ixx" OR NOT EXISTS "${MSVC_CXX_STD_MODULE_DIR}/std.compat.ixx")
#		file(COPY_FILE "${VCToolsInstallDir}modules/std.ixx" "${MSVC_CXX_STD_MODULE_DIR}/std.ixx")
#		file(COPY_FILE "${VCToolsInstallDir}modules/std.compat.ixx" "${MSVC_CXX_STD_MODULE_DIR}/std.compat.ixx")
#		execute_process(
#			COMMAND cl /std:c++latest /EHsc /nologo /W4 /MTd /c "${MSVC_CXX_STD_MODULE_DIR}/std.ixx" "${MSVC_CXX_STD_MODULE_DIR}/std.compat.ixx"
#			WORKING_DIRECTORY  "${MSVC_CXX_STD_MODULE_DIR}"
#		)
#	endif()
#
#	# モジュールを簡単にリンク出来るようにプロジェクトを用意
#	set(MODULE_NAME experimental_std_modules)
#	add_library(
#		${MODULE_NAME} STATIC
#	)
#	target_sources(
#		${MODULE_NAME}
#		PUBLIC
#			FILE_SET cxx_modules TYPE CXX_MODULES FILES
#				"${MSVC_CXX_STD_MODULE_DIR}/std.ixx"
#				"${MSVC_CXX_STD_MODULE_DIR}/std.compat.ixx"
#	)
#endif()

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
