cmake_minimum_required(VERSION 3.24)

file(GLOB_RECURSE srcs 
	src/*.h
	src/*.hpp
	src/*.c
	src/*.cpp
	)

add_library(
	libsynth++ STATIC 
	${srcs}
)

target_compile_options(
	libsynth++ 
	PRIVATE	$<$<CXX_COMPILER_ID:MSVC>: /W3>
)

target_include_directories(
    libsynth++
    PUBLIC src
)

unset(juce_modules)
list(APPEND juce_modules juce_core)
list(APPEND juce_modules juce_events)
list(APPEND juce_modules juce_audio_basics)
list(APPEND juce_modules juce_audio_devices)

# JUCEのライブラリを中間ライブラリ側でtarget_link_libraryする事は危険。
# そのためインクルードパスのみ通している。
# 
# どうもtarget_link_libraryする対象毎にライブラリのobjをビルドしてリンクしているようなので、
# 些細なオプション違いでODR違反が発生しリンク時の挙動が不確定になる可能性がある。

foreach(juce_module IN LISTS juce_modules)
	get_target_property(juce_module_include_dirs ${juce_module} INTERFACE_INCLUDE_DIRECTORIES)
	target_include_directories(libsynth++ PUBLIC ${juce_module_include_dirs})
endforeach()