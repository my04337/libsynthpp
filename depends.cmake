cmake_minimum_required(VERSION 3.26)

include(ExternalProject)
include(FetchContent)

###########################################################
### JUCE 
# 
# 注意 : ISC Licenseで提供されるモジュールのみ利用可能
#      * juce_core
#      * juce_audio_basics
#      * juce_audio_devices
#      * juce_events
#        

set(JUCE_MODULES_ONLY YES)
set(JUCE_BUILD_EXTRAS NO)
set(JUCE_BUILD_EXAMPLES NO)

set(JUCE_SOURCE_DIR  "${CMAKE_CURRENT_LIST_DIR}/third_party/JUCE")
set(JUCE_MODULES_DIR "${JUCE_SOURCE_DIR}/modules")

include("${JUCE_SOURCE_DIR}/extras/Build/CMake/JUCEModuleSupport.cmake")

juce_add_modules(
    ALIAS_NAMESPACE juce
    "${JUCE_MODULES_DIR}/juce_core"
    "${JUCE_MODULES_DIR}/juce_audio_basics"
    "${JUCE_MODULES_DIR}/juce_audio_devices"
    "${JUCE_MODULES_DIR}/juce_events"
)