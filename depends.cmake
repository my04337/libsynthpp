cmake_minimum_required(VERSION 3.26)


set(THIRD_PARTY_DIR  "${CMAKE_CURRENT_LIST_DIR}/third_party")

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

set(JUCE_SOURCE_DIR  "${THIRD_PARTY_DIR}/JUCE")
set(JUCE_MODULES_DIR "${JUCE_SOURCE_DIR}/modules")
set(JUCE_GIT_REPOSITORY https://github.com/juce-framework/JUCE)
set(JUCE_GIT_TAG        "7.0.5")
set(JUCE_GIT_COMMIT     69795dc8e589a9eb5df251b6dd994859bf7b3fab)

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
        COMMAND ${GIT_EXECUTABLE} branch --contains ${69795dc8e589a9eb5df251b6dd994859bf7b3fab}
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
)