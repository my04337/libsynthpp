cmake_minimum_required(VERSION 3.26)


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

set(JUCE_GIT_REPOSITORY https://github.com/juce-framework/JUCE)
set(JUCE_GIT_TAG        "7.0.12")
set(JUCE_GIT_COMMIT     4f43011b96eb0636104cb3e433894cda98243626)

set(JUCE_SOURCE_DIR  "${THIRD_PARTY_SOURCE_DIR}/JUCE/${JUCE_GIT_TAG}")
set(JUCE_MODULES_DIR "${JUCE_SOURCE_DIR}/modules")

find_package(Git REQUIRED)

if(NOT EXISTS ${JUCE_SOURCE_DIR})
    # 指定のタグを shallow clone する
    execute_process(
        COMMAND ${GIT_EXECUTABLE} clone ${JUCE_GIT_REPOSITORY} -b ${JUCE_GIT_TAG} ${JUCE_SOURCE_DIR} --depth=1 --no-single-branch --progress
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
 