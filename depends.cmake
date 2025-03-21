﻿cmake_minimum_required(VERSION 3.26)


set(THIRD_PARTY_SOURCE_DIR  "${CMAKE_CURRENT_LIST_DIR}/third_party")
set(THIRD_PARTY_BINARY_DIR  "${CMAKE_CURRENT_BINARY_DIR}/third_party")

###########################################################
### JUCE 
# 
# 注意 : libsynth++では ISC Licenseで提供されるモジュールのみ利用可能。
#          * juce_core
#          * juce_audio_basics
#          * juce_audio_devices
#          * juce_events
#        上記以外のライブラリは GPLv3 または商用ライセンスであることに注意。

set(JUCE_MODULES_ONLY YES)
set(JUCE_BUILD_EXTRAS NO)
set(JUCE_BUILD_EXAMPLES NO)

set(JUCE_SOURCE_DIR  "${THIRD_PARTY_SOURCE_DIR}/JUCE")
set(JUCE_MODULES_DIR "${JUCE_SOURCE_DIR}/modules")
set(JUCE_GIT_REPOSITORY https://github.com/juce-framework/JUCE)
set(JUCE_GIT_TAG        "8.0.6")
set(JUCE_GIT_COMMIT     51a8a6d7aeae7326956d747737ccf1575e61e209)

find_package(Git REQUIRED)

if(NOT EXISTS ${JUCE_SOURCE_DIR})
    # 指定のタグを shallow clone する ※ただし今後他のタグをcheckoutする可能性があるため、メタ情報だけは取得しておく
    execute_process(
        COMMAND ${GIT_EXECUTABLE} clone ${JUCE_GIT_REPOSITORY} -b ${JUCE_GIT_TAG} ${JUCE_SOURCE_DIR} --depth=1 --no-single-branch --progress
    )
else()
    # 衝突を回避するため、全ての差分をリセット
    execute_process(
        COMMAND ${GIT_EXECUTABLE} reset --hard --quiet 
        WORKING_DIRECTORY ${JUCE_SOURCE_DIR}        
        COMMAND_ERROR_IS_FATAL ANY
        OUTPUT_QUIET
    )
    # 指定のタグのコミットがローカルにあるかを確認し、無ければfetchする
    execute_process(
        COMMAND ${GIT_EXECUTABLE} branch --contains ${JUCE_GIT_COMMIT}
        WORKING_DIRECTORY ${JUCE_SOURCE_DIR}
        RESULT_VARIABLE ret
        OUTPUT_QUIET
    )
    if(ret AND NOT ret EQUAL 0)
        execute_process(
            COMMAND ${GIT_EXECUTABLE} fetch
            WORKING_DIRECTORY ${JUCE_SOURCE_DIR}
            COMMAND_ERROR_IS_FATAL ANY
        )
    endif()
    # 指定のタグへ切替
    execute_process(
        COMMAND ${GIT_EXECUTABLE} checkout tags/${JUCE_GIT_TAG} --quiet
        WORKING_DIRECTORY ${JUCE_SOURCE_DIR}
        COMMAND_ERROR_IS_FATAL ANY
    )
endif()

include("${JUCE_SOURCE_DIR}/extras/Build/CMake/JUCEModuleSupport.cmake")


# ISC License
juce_add_modules(
    ALIAS_NAMESPACE juce
    "${JUCE_MODULES_DIR}/juce_core"
    "${JUCE_MODULES_DIR}/juce_audio_basics"
    "${JUCE_MODULES_DIR}/juce_audio_devices"
    "${JUCE_MODULES_DIR}/juce_events"
)

# GPL License or Commercial
juce_add_modules(
    ALIAS_NAMESPACE juce
    "${JUCE_MODULES_DIR}/juce_data_structures"
    "${JUCE_MODULES_DIR}/juce_graphics"
    "${JUCE_MODULES_DIR}/juce_gui_basics"
    "${JUCE_MODULES_DIR}/juce_gui_extra"
    "${JUCE_MODULES_DIR}/juce_opengl"
)

###########################################################
### UmeFont 

# 下記を有効にして.cppファイルに埋め込むための文字列表現を生成します
if(0)
    file(READ "${THIRD_PARTY_SOURCE_DIR}/UmeFont/ume-tgo4.ttf" UME_FONT_HEX HEX)
    string(REGEX REPLACE "(................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................)" "\\1\n" UME_FONT_HEX ${UME_FONT_HEX})
    string(REGEX REPLACE "([0-9a-f][0-9a-f])" "0x\\1, " UME_FONT_HEX ${UME_FONT_HEX})
    message(STATUS ${UME_FONT_HEX})
endif()
 