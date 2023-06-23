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
